#include "../include/Request.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

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

int Request::handle_request()
{

    /*  expecting
        login/register username password\n
        operation filename \r\n
    */
    if (state == State::FETCHING)
    {
        int count = 0;
        count = recv(sockfd, buffer + strlen(buffer), 4096, 0);
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
    return 1;

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
                state = State::LISTING;
                return 0;
            }
            else if (req_kind == "upload")
            {
                std::string filename = request_line.back();
                std::string pathname;
                if (isGlobal)
                    pathname = "./storage/shared/" + username + "_" + filename;
                else
                    pathname = "./storage/private/" + username + "/" + filename;

                localfd = open(pathname.c_str(), O_WRONLY | O_CREAT | O_EXCL);
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
                    pathname = "./storage/shared/" + username + "_" + filename;
                else
                    pathname = "./storage/private/" + username + "/" + filename;

                localfd = open(pathname.c_str(), O_RDONLY);
                if (errno == ENOENT)
                {
                    // file does not exist so you cannot download
                    // send(NACK)
                    return 1;
                }

                // send(ACK)
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
                    old_pathname = "./storage/shared/" + username + "_" + old_filename;
                    new_filename = "./storage/shared/" + username + "_" + new_filename;
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
                    pathname = "./storage/shared/" + username + "_" + filename;
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