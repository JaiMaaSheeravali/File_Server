#include "../include/Client.hpp"

#include <iostream>

Client::Client() : ipaddr_len(sizeof(ipaddr)) {}

void Client::printIpAddress()
{
    std::cout << "host: " << host << "\t port: " << serv;
}