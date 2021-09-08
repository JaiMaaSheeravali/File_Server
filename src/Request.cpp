#include "../include/Request.hpp"

#include <iostream>
#include <unistd.h>

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
    std::cout << "Client ";
    client.printIpAddress();
    std::cout << " connected\n";
    return client.sockfd;
}

void Request::handle_request()
{
    while (true)
    {
        std::cout << "Reading request...\n";
        char recieved[1024];
        int bytes_received = recv(client.sockfd, recieved, 1024, 0);
        if (bytes_received == 0)
            break;

        printf("recieved:  %.*s", bytes_received, recieved);

        if (send(client.sockfd, "Hello, world!", 13, 0) == -1)
            perror("send");
    }

    std::cout << "Client ";
    client.printIpAddress();
    std::cout << " disconnected\n";
    close(client.sockfd);

    // while (true)
    // {
    //     char action[200];
    //     memset(action, '\0', sizeof(action));
    //     if (recv(client.sockfd, &action, sizeof(action), 0) < 0)
    //     {
    //         std::cerr << "Couldn't receive Command\n";
    //         return;
    //     }
    //     if (strcmp(action, "upload") == 0)
}