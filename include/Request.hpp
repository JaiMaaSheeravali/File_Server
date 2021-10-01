#pragma once
#include "../include/Client.hpp"
#include <poll.h>

class Request
{
private:
    Client client;
    static const int BUF_SIZE = 4096;
    char buffer[BUF_SIZE] = "\0";

    enum class State : unsigned char
    {
        FETCHING,
        LISTING,
        SENDING,
        RECEIVING
    };

    State state = State::FETCHING;

public:
    ~Request();
    int sockfd, diskfilefd;
    char fileSize[64];
    int bytes_sent = 0, bytes_recvd = 0, bytes_left = 0;
    struct pollfd *pollFd;
    int accept_request(int server_socket);
    int handle_request();
    int perform_operation();
    int parse_request();
    int send_file();
    int delete_file();
    int get_file();
    int rename_file();
    int get_file_list();
    int send_data(const char *buffer, const int size);
    int recv_data(char *filename);
    char *recv_string();
};