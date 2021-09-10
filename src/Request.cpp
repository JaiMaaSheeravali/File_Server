#include "../include/Request.hpp"
#include "../include/color.hpp"

#include <iostream>
#include <unistd.h>
#include <cstring>

using std::cout;
using std::cin;

int Request::accept_request(int server_socket)
{
    // create a socket for the newly connected client and get its ip address
    client.sockfd = accept(server_socket, (struct sockaddr *)&(client.ipaddr),
                           &(client.ipaddr_len));

    // store the presentational ip address in host and port number in serv
    getnameinfo((struct sockaddr *)&client.ipaddr, client.ipaddr_len,
                client.host, sizeof(client.host), client.serv,
                sizeof(client.serv), NI_NUMERICHOST | NI_NUMERICSERV);

    // print which client got connected to the server
    std::cout << CYAN << "Client ";
    client.printIpAddress();
    std::cout << " connected\n" << RESET;
    return client.sockfd;
}

void Request::handle_request()
{

    while(true){
        char action[200];
        memset(action, '\0', sizeof(action));

        if (recv(client.sockfd, &action, sizeof(action), 0) <= 0){
            cout << RED << "Couldn't receive Command\n" << RESET;
            break;
        }

        if(strcmp(action, "upload") == 0){
            
            get_file();
        
        } else if(strcmp(action, "download") == 0){
            
            send_file();

        } else if(strcmp(action, "list") == 0){

            get_file_list();
        } else if(strcmp(action, "delete") == 0){
            
            delete_file();
        } else if(strcmp(action, "rename") == 0){
            
            rename_file();
        } else if(strcmp(action, "exit") == 0){
            break;

        } else {
            cout << RED << "Invalid action" << RESET << std::endl;

        }
    }

    std::cout << MAGENTA << "Client ";
    client.printIpAddress();
    std::cout << " disconnected\n" << RESET;
    close(client.sockfd);
}