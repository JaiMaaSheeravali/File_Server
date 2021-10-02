#pragma once

#include <netdb.h>
#include <string>

class Client
{
public:
    struct sockaddr_storage ipaddr;
    socklen_t ipaddr_len;
    char host[100];
    char serv[100];
    // std::string username;
    // std::string password;

public:
    Client();
    void printIpAddress();
};