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
#include "../include/helper.hpp"
#include "../include/parse.hpp"

#define COMPLETED 1
#define ONGOING 0
// expecting
// login/register username password\n
// operation filename

Request::~Request()
{
    std::cout << MAGENTA << "Client ";
    client.printIpAddress();
    std::cout << " disconnected" << RESET << std::endl;
    close(sockfd);
    close(diskfilefd);
}

int Request::accept_request(int server_socket)
{
    // create a nonblocking socket for the newly connected client and get its ip address
    sockfd = accept4(server_socket, (struct sockaddr *)&(client.ipaddr),
                     &(client.ipaddr_len), SOCK_NONBLOCK);

    // store the presentational ip address in host and port number in serv
    getnameinfo((struct sockaddr *)&client.ipaddr, client.ipaddr_len,
                client.host, sizeof(client.host), client.serv,
                sizeof(client.serv), NI_NUMERICHOST | NI_NUMERICSERV);

    // print which client got connected to the server
    std::cout << CYAN << "Client ";
    client.printIpAddress();
    std::cout << " connected" << RESET << std::endl;
    return sockfd;
}

int Request::handle_request()
{
    int status = ONGOING;
    switch (state)
    {
    case State::FETCHING:
        status = fetchFtpRequest();
        break;
    case State::LISTING:
        status = sendListToClient();
        break;
    case State::RECEIVING:
        status = recvFileFromClient();
        break;
    case State::SENDING:
        status = sendFileToClient();
        break;
    default:
        // send(NACK)
        break;
    }
    return status;
}

// ignore
// is a return ACK required after fetching and processing the ftp header?
// list send number of bytes as ACk
// upload neccesity
// download send file size in bytes as ACK
// rename neccessity and sufficient
// delete neccesesty and sufficeint

int Request::parseFtpRequest()
{
    // extract username password, operation filename from buffer and then
    // perform authorisation
    // and then open the corresponding file if required
    std::cout << buffer;
    std::string header = buffer;

    auto list_lines = ftp_tokenizer(header, '\n');
    auto auth_line = ftp_tokenizer(list_lines[0], ' ');

    if (auth_line.size() != 3)
    {
        std::cout << "error 400 malformed request syntax \n";
        return 1;
    }

    std::string auth_kind = auth_line[0];
    client.username = auth_line[1];
    client.password = auth_line[2];

    if (auth_kind == "login")
    {
        std::cout << "client wants to login" << std::endl;
        if (/*loginWithUsernamePassword(client.username, client.password)  */ true)
        {
            if (list_lines[1] == "done")
            {
                // send(ACK);
                // client just wanted to login so we are done
                return COMPLETED;
            }

            // client authoried and now wants to access a service
            isGlobal = list_lines[1].find("-g") != std::string::npos;

            auto request_line = ftp_tokenizer(list_lines[1], ' ');
            std::string req_kind = request_line.front();

            if (req_kind == "list")
            {
                return parseListRequest();
            }
            else if (req_kind == "upload")
            {
                bytes_left = stoi(request_line.rbegin()[1]);
                return parseUploadRequest(request_line.back());
            }
            else if (req_kind == "download")
            {
                return parseDownloadRequest(request_line.back());
            }
            else if (req_kind == "rename")
            {
                auto rev_itr = request_line.rbegin();
                return parseAndExecuteRenameRequest(rev_itr[0], rev_itr[1]);
            }
            else if (req_kind == "delete")
            {
                return parseAndExecuteDeleteRequest(request_line.back());
            }
            else
            {
                // send(NACK);
                std::cout << "error 404 request not found" << std::endl;
                return COMPLETED;
            }
        }
        else
        {
            // send(NACK);
            std::cout << "error 401 unauthorized client" << std::endl;
            return COMPLETED;
        }
    }
    else if (auth_kind == "register")
    {
        std::cout << "client wants to register" << std::endl;
        // if (registerWithUsernamePassword(client.username, client.password))
        // {
        //   send(ACK);
        // }
        // else
        // {
        //   send(NACK);
        // }
        return COMPLETED;
    }
    else
    {
        std::cout << "error 400 wrong request syntax" << std::endl;
        return COMPLETED;
    }
}
int Request::parseListRequest()
{
    std::string pathname;
    DIR *d;
    struct dirent *dir;
    if (isGlobal)
        pathname = "./storage/shared/";
    else
        pathname = "./storage/private/" + client.username;
    d = opendir(pathname.c_str());

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_type == DT_REG)
            {
                list.append(dir->d_name);
                list.push_back('\n');
            }
        }
        closedir(d);
    }
    std::cout << BOLDBLUE << list
              << RESET << std::endl;

    bigBuffer = list.c_str();
    bytes_left = list.size();

    pollFd->events = POLLOUT;
    state = State::LISTING;
    return ONGOING;
}
int Request::parseUploadRequest(const std::string &filename)
{
    std::string pathname;
    if (isGlobal)
        pathname = "./storage/shared/" + filename;
    else
        pathname = "./storage/private/" + client.username + "/" + filename;

    diskfilefd = open(pathname.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (errno == EEXIST)
    {
        // file already exists so you cant upload
        // send(NACK)
        return COMPLETED;
    }

    // send(ACK)
    state = State::RECEIVING;
    return ONGOING;
}
int Request::parseDownloadRequest(const std::string &filename)
{
    std::string pathname;
    if (isGlobal)
        pathname = "./storage/shared/" + filename;
    else
        pathname = "./storage/private/" + client.username + "/" + filename;

    diskfilefd = open(pathname.c_str(), O_RDONLY);
    if (errno == ENOENT)
    {
        // file does not exist so you cannot download
        // send(NACK)
        return COMPLETED;
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
    bytes_sent = send(sockfd, fileSize, sizeof(fileSize), 0);
    if (bytes_sent < 0)
    {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server sent %d bytes for the size\n", bytes_sent);
    pollFd->events = POLLOUT;
    state = State::SENDING;

    return ONGOING;
}
int Request::parseAndExecuteRenameRequest(const std::string &new_filename, const std::string &old_filename)
{
    std::string old_pathname;
    std::string new_pathname;

    if (isGlobal)
    {
        old_pathname = "./storage/shared/" + old_filename;
        new_pathname = "./storage/shared/" + new_filename;
    }
    else
    {
        old_pathname = "./storage/private/" + client.username + "/" + old_filename;
        new_pathname = "./storage/private/" + client.username + "/" + new_filename;
    }

    if (rename(old_pathname.c_str(), new_pathname.c_str()) != 0)
    {

        std::cerr << RED << "Error Renaming '"
                  << old_pathname
                  << "' to '"
                  << new_pathname << "'!\n"
                  << RESET;
        // send(NACK)
        return COMPLETED;
    }
    else
    {
        std::cout << GREEN
                  << "Successfully renamed '" << old_pathname
                  << "' to '"
                  << new_pathname << "'.\n"
                  << RESET;
        //send(ACK)
        return ONGOING;
    }
}
int Request::parseAndExecuteDeleteRequest(const std::string &filename)
{
    std::string pathname;
    if (isGlobal)
        pathname = "./storage/shared/" + filename;
    else
        pathname = "./storage/private/" + client.username + "/" + filename;

    if (remove(pathname.c_str()) != 0)
    {
        std::cerr << RED
                  << "Error Deleting File: '"
                  << pathname << "'!\n"
                  << RESET;
        // send(NACK)
        return COMPLETED;
    }
    else
    {
        std::cout << GREEN << "Successfully deleted '"
                  << pathname
                  << "' from Server.\n"
                  << RESET;
        //send(ACK)
        return ONGOING;
    }
}

int Request::fetchFtpRequest()
{
    int count = recv(sockfd, buffer + strlen(buffer), BUFSIZ, 0);
    if (count == 0)
    {
        // disconnected
        // send(NACK)
        return COMPLETED;
    }

    if ((strstr(buffer, "done")))
    {
        std::cout << "completion \n";
        return parseFtpRequest();
    }
    else
    {
        // still haven't received the ftp request header
        return ONGOING;
    }
}
int Request::recvFileFromClient()
{
    std::cout << "entered receiving section" << std::endl;
    /* Sending file data */
    while (bytes_left > 0)
    {
        // create a new buffer since we are going to write(in diskfd), whatever
        // data received from the client in one go (no funny behavior)
        // although its not necessary that the whole file will be receivied in one recv
        char buffer[BUFSIZ];
        memset(buffer, '\0', sizeof(buffer));

        // perform non blocking io on the socket file descriptor
        // get the data from the client, store it in the buffer
        if (((bytes_recvd = recv(sockfd, buffer, BUFSIZ, 0)) == -1))
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no data avalaible currently try again later (after poll)
                std::cout << "blocked" << std::endl;
                return ONGOING;
            }
            else
            {
                perror("failed file transfer");
                // send(NACK)
                return COMPLETED;
            }
        }
        else if (bytes_recvd == 0)
        {
            // send(NACK)
            perror("wtf");
            std::cout << "client closed the connection abruptly" << std::endl;
            // maybe delete the file since the uploading was disrupted
            return COMPLETED;
        }
        else
        {
            std::cout << "bytes recvd yet: " << bytes_recvd << std::endl;
            bytes_left -= bytes_recvd;

            // copy the data from the buffer to the local disk file
            if (writen(diskfilefd, buffer, bytes_recvd) < 0)
            {
                // send(NACK)
                perror("disk write failed\n");
                return COMPLETED;
            }
        }
    }

    // send(ACK)
    return COMPLETED;
}
int Request::sendListToClient()
{
    std::cout << "entered listing section" << std::endl;
    /* Sending content of bigFuffer that contains list of files */
    while (bytes_left > 0)
    {
        // perform non blocking io on the socket file descriptor
        if (((bytes_sent = send(sockfd, bigBuffer, bytes_left, 0)) == -1))
        {
            if (errno == EAGAIN)
            {
                std::cout << "blocked" << std::endl;
                // kernel buffer is full currently try again later (after poll)
                return ONGOING;
            }
            else
            {
                perror("failed data transfer");
                // send(NACK)
                return COMPLETED;
            }
        }
        std::cout << "bytes sent yet: " << bytes_sent << std::endl;
        bigBuffer += bytes_sent;
        bytes_left -= bytes_sent;
    }

    // send(ACK)
    return COMPLETED;
}
int Request::sendFileToClient()
{
    std::cout << "entered sending section" << std::endl;
    /* Sending file data */
    while (bytes_left > 0)
    {
        // perform non blocking io on the socket file descriptor
        if (((bytes_sent = sendfile(sockfd, diskfilefd, nullptr, bytes_left)) == -1))
        {
            if (errno == EAGAIN)
            {
                std::cout << "blocked" << std::endl;
                // kernel buffer is full currently try again later (after poll)
                return ONGOING;
            }
            else
            {
                perror("failed file transfer");
                // send(NACK)
                return COMPLETED;
            }
        }
        std::cout << "bytes sent yet: " << bytes_sent << std::endl;
        bytes_left -= bytes_sent;
    }

    // send(ACK)
    return COMPLETED;
}
