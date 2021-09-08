#include "../include/socket.hpp"

#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

int create_server_socket()
{
    int server_socket;
    struct addrinfo hints, *server_addr, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // use any ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; // tcp socket
    hints.ai_flags = AI_PASSIVE;     // use my IP

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &server_addr)) != 0)
    {
        std::cerr << "gettaddrinfo: " << gai_strerror(status) << '\n';
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = server_addr; p != nullptr; p = p->ai_next)
    {
        if ((server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(server_socket);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(server_addr); // all done with this structure

    // if no address was available to bind to then exit
    if (p == nullptr)
    {
        std::cerr << "server: failed to bind\n";
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, BACKLOG) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return server_socket;
}