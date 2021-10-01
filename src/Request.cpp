#include "../include/Request.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/sendfile.h>

#include "../include/color.hpp"
#include "../include/parse.hpp"

// using std::cin;
// using std::cout;

Request::~Request()
{
    std::cout << MAGENTA << "Client ";
    client.printIpAddress();
    std::cout << " disconnected\n"
              << RESET;
    close(sockfd);
    close(diskfilefd);
}

int Request::accept_request(int server_socket)
{
    // create a socket for the newly connected client and get its ip address
    sockfd = accept(server_socket, (struct sockaddr *)&(client.ipaddr),
                    &(client.ipaddr_len));

    // store the presentational ip address in host and port number in serv
    getnameinfo((struct sockaddr *)&client.ipaddr, client.ipaddr_len,
                client.host, sizeof(client.host), client.serv,
                sizeof(client.serv), NI_NUMERICHOST | NI_NUMERICSERV);

    // print which client got connected to the server
    std::cout << CYAN << "Client ";
    client.printIpAddress();
    std::cout << " connected\n"
              << RESET;
    return sockfd;
}

// void Request::handle_request()
// {

//     while (true)
//     {
//         char action[200];
//         memset(action, '\0', sizeof(action));

//         if (recv(sockfd, &action, sizeof(action), 0) <= 0)
//         {
//             cout << RED << "Couldn't receive Command\n"
//                  << RESET;
//             break;
//         }

//         if (strcmp(action, "upload") == 0)
//         {

//             get_file();
//         }
//         else if (strcmp(action, "download") == 0)
//         {

//             send_file();
//         }
//         else if (strcmp(action, "list") == 0)
//         {

//             get_file_list();
//         }
//         else if (strcmp(action, "delete") == 0)
//         {

//             delete_file();
//         }
//         else if (strcmp(action, "rename") == 0)
//         {

//             rename_file();
//         }
//         else if (strcmp(action, "exit") == 0)
//         {
//             break;
//         }
//         else
//         {
//             cout << RED << "Invalid action" << RESET << std::endl;
//         }
//     }

//     std::cout << MAGENTA << "Client ";
//     client.printIpAddress();
//     std::cout << " disconnected\n"
//               << RESET;
//     close(sockfd);
// }

ssize_t
writen(int fd, const char *buffer, size_t n)
{
    ssize_t numWritten; /* # of bytes written by last write() */
    size_t totWritten;  /* Total # of bytes written so far */
    const char *buf = buffer;
    for (totWritten = 0; totWritten < n;)
    {
        numWritten = write(fd, buf, n - totWritten);

        if (numWritten <= 0)
        {
            if (numWritten == -1 && errno == EINTR)
                continue; /* Interrupted --> restart write() */
            else
                return -1; /* Some other error */
        }
        totWritten += numWritten;
    }
    return totWritten; /* Must be 'n' bytes if we get here */
}

int Request::handle_request()
{

    /*  expecting
        login/register username password\n
        operation filename \r\n
    */
    if (state == State::FETCHING)
    {
        int count = recv(sockfd, buffer + strlen(buffer), BUF_SIZE, 0);
        if (count == 0)
        {
            // disconnected
            return 1;
        }

        if ((strstr(buffer, "done")))
        {
            std::cout << "completion \n";
            return parse_request();
        }
        else
        {
            // still haven't received the ftp request header
            return 0;
        }
    }
    else if (state == State::LISTING)
    {
        return 1;
    }
    else if (state == State::RECEIVING)
    {
        // create a new buffer since we are going to write(in diskfd) whatever
        // data received from the client in one go (no funny behavior)
        char buffer[BUF_SIZE];
        // get the data from the client, store it in the buffer
        bytes_recvd = recv(sockfd, buffer, BUF_SIZE, NULL);

        if (bytes_recvd == 0)
        {
            std::cout << "client done uploading" << std::endl;
            return 1;
        }
        // copy the data from the buffer to the local disk file
        if (writen(diskfilefd, buffer, bytes_recvd) < 0)
        {
            perror("write failed\n");
            return 1;
        }
        return 0;
    }
    else if (state == State::SENDING)
    {
        std::cout << "entered sending section" << std::endl;
        /* Sending file data */
        while (bytes_left > 0)
        {
            // perform non blocking io on the socket file descriptor
            if (((bytes_sent = sendfile(sockfd, diskfilefd, nullptr, BUFSIZ)) == -1))
            {
                if (errno == EAGAIN)
                {
                    std::cout << "blocked" << std::endl;
                    // no data avalaible currently try again later (after poll)
                    return 0;
                }
                else
                {
                    perror("failed file transfer");
                    return 1;
                }
            }
            std::cout << "bytes sent yet: " << bytes_sent << std::endl;
            bytes_left -= bytes_sent;
        }

        return 1;
    }
    else
    {
        return 1;
    }

    // return perform_operation();
}

// is a return ACK required after fetching and processing the ftp header
// list send number of bytes as ACk
// upload neccesity
// download send file size in bytes as ACK
// rename neccessity and sufficient
// delete neccesesty and sufficeint
int Request::parse_request()
{
    // extract username password, operation filename from buffer and then
    // perform authorisation
    // and then open the corresponding file if required
    // if (!check_username_password(client.username, client.password))
    // {
    //     return -1;
    // }
    // state = State::RECEIVING;
    std::cout << buffer;
    // memset(buffer, '\0', sizeof(buffer));
    std::string header = buffer;

    auto list_lines = ftp_tokenizer(header, '\n');
    auto auth_line = ftp_tokenizer(list_lines[0], ' ');

    if (auth_line.size() != 3)
    {
        std::cout << "error 400 malformed request syntax \n";
        return 1;
    }

    std::string auth_kind = auth_line[0];
    std::string username = auth_line[1];
    std::string password = auth_line[2];

    if (auth_kind == "login")
    {
        std::cout << "client wants to login" << std::endl;
        if (/*loginWithUsernamePassword(username, password)  */ true)
        {
            if (list_lines[1] == "done")
            {
                // send(ACK);
                return 1;
            }

            bool isGlobal = list_lines[1].find("-g") != std::string::npos;

            auto request_line = ftp_tokenizer(list_lines[1], ' ');
            std::string req_kind = request_line.front();

            if (req_kind == "list")
            {
                std::string pathname;
                DIR *d;
                struct dirent *dir;
                if (isGlobal)
                    pathname = "./storage/shared/";
                else
                    pathname = "./storage/private/" + username;
                d = opendir(pathname.c_str());

                std::string list;
                if (d)
                {
                    while ((dir = readdir(d)) != NULL)
                    {
                        list.append(dir->d_name);
                        list.push_back('\n');
                    }
                    closedir(d);
                }
                pollFd->events = POLLOUT;
                state = State::LISTING;
                return 0;
            }
            else if (req_kind == "upload")
            {
                std::string filename = request_line.back();
                std::string pathname;
                if (isGlobal)
                    pathname = "./storage/shared/" + filename;
                else
                    pathname = "./storage/private/" + username + "/" + filename;

                diskfilefd = open(pathname.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
                if (errno == EEXIST)
                {
                    // file already exists so you cant upload
                    // send(NACK)
                    return 1;
                }

                // send(ACK)
                state = State::RECEIVING;
                return 0;
            }
            else if (req_kind == "download")
            {
                std::string filename = request_line.back();
                std::string pathname;
                if (isGlobal)
                    pathname = "./storage/shared/" + filename;
                else
                    pathname = "./storage/private/" + username + "/" + filename;

                diskfilefd = open(pathname.c_str(), O_RDONLY | O_NONBLOCK);
                if (errno == ENOENT)
                {
                    // file does not exist so you cannot download
                    // send(NACK)
                    return 1;
                }

                // send(ACK)
                /* Get file stats */
                struct stat file_stat;
                if (fstat(diskfilefd, &file_stat) < 0)
                {
                    perror("fstat failed");
                    exit(EXIT_FAILURE);
                }

                bytes_left = file_stat.st_size;

                std::cout << "File Size: " << bytes_left << " bytes\n";
                sprintf(fileSize, "%d", bytes_left);

                /* Sending file size */
                bytes_sent = send(sockfd, fileSize, sizeof(fileSize), NULL);
                if (bytes_sent < 0)
                {
                    perror("send failed");
                    exit(EXIT_FAILURE);
                }

                fprintf(stdout, "Server sent %d bytes for the size\n", bytes_sent);
                pollFd->events = POLLOUT;
                state = State::SENDING;

                return 0;
            }
            else if (req_kind == "rename")
            {
                std::string new_filename = request_line.rbegin()[0];
                std::string old_filename = request_line.rbegin()[1];

                std::string old_pathname;
                std::string new_pathname;

                if (isGlobal)
                {
                    old_pathname = "./storage/shared/" + old_filename;
                    new_pathname = "./storage/shared/" + new_filename;
                }
                else
                {
                    old_pathname = "./storage/private/" + username + "/" + old_filename;
                    new_pathname = "./storage/private/" + username + "/" + new_filename;
                }

                if (rename(old_pathname.c_str(), new_pathname.c_str()) != 0)
                {

                    std::cerr << RED << "Error Renaming '"
                              << old_pathname
                              << "' to '"
                              << new_pathname << "'!\n"
                              << RESET;
                    // send(NACK)
                    return 1;
                }
                else
                {
                    std::cout << GREEN
                              << "Successfully renamed '" << old_pathname
                              << "' to '"
                              << new_pathname << "'.\n"
                              << RESET;
                    //send(ACK)
                    return 0;
                }
            }
            else if (req_kind == "delete")
            {
                std::string filename = request_line.back();
                std::string pathname;
                if (isGlobal)
                    pathname = "./storage/shared/" + filename;
                else
                    pathname = "./storage/private/" + username + "/" + filename;

                if (remove(pathname.c_str()) != 0)
                {
                    std::cerr << RED
                              << "Error Deleting File: '"
                              << pathname << "'!\n"
                              << RESET;
                    // send(NACK)
                    return 1;
                }
                else
                {
                    std::cout << GREEN << "Successfully deleted '"
                              << pathname
                              << "' from Server.\n"
                              << RESET;
                    //send(ACK)
                    return 0;
                }
            }
            else
            {
                // send(NACK);
                std::cout << "error 404 request not found" << std::endl;
                return 1;
            }
        }
        // else
        // {
        //     send(NACK);
        // }
    }
    else if (auth_kind == "register")
    {
        std::cout << "client wants to register" << std::endl;
        // if (registerWithUsernamePassword(username, password))
        // {
        //   send(ACK);

        // }
        // else
        // {
        //   send(NACK);

        // }
        return 1;
    }
    else
    {
        std::cout << "error 404 request not found" << std::endl;
        return 1;
    }

    return 0;
}

int Request::perform_operation()
{
    switch (state)
    {
    case State::RECEIVING:
        /* code */
        break;

    default:
        break;
    }
}