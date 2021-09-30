#include "../include/Request.hpp"
#include "../include/color.hpp"

#include <iostream>
#include <unistd.h>
#include <cstring>

// using std::cin;
// using std::cout;

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
            std::cout << MAGENTA << "Client ";
            client.printIpAddress();
            std::cout << " disconnected\n"
                      << RESET;
            close(sockfd);
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

int Request::parse_request()
{
    // extract username password, operation filename from buffer and then
    // perform authorisation
    // and then open the corresponding file if required
    // if (!check_username_password(client.username, client.password))
    // {
    //     return -1;
    // }
    state = State::RECEIVING;
    std::cout << buffer;
    memset(buffer, '\0', sizeof(buffer));
    close(sockfd);
    return 1;
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