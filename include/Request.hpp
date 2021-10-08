#pragma once
#include "../include/Client.hpp"
#include <poll.h>

#define COMPLETED 1
#define ONGOING 0

class Request
{
private:
    Client client;                   // for storing the address of the cleint
    char buffer[BUFSIZ] = "\0";      // for receiving ftp request and file data
    const char *bigBuffer;           // for sending the list of files
    char *varBuffer;
    char *result;
    std::string list;                // same as bigBuffer but in c++ string
    std::string pathname;
    enum class State : unsigned char // for maintaining the state of the request
    {
        FETCHING,
        LISTING,
        SENDING,
        RECEIVING
    };
    State state = State::FETCHING; // initially retrieve ftp request from the client
    int bytes_sent = 0, bytes_recvd = 0, bytes_left = 0;
    char fileSize[64];
    bool isGlobal = false;

    int fetchFtpRequest();
    int recvFileFromClient();
    int sendListToClient();
    int sendFileToClient();

    // functions for parsing the ftp request after fetching it
    int parseFtpRequest();
    int parseListRequest();
    int parseUploadRequest(const std::string &filename);
    int parseDownloadRequest(const std::string &filename);
    int parseAndExecuteRenameRequest(const std::string &new_filename, const std::string &old_filename);
    int parseAndExecuteDeleteRequest(const std::string &filename);

public:
    ~Request();
    int sockfd, diskfilefd;
    struct pollfd *pollFd;
    int accept_request(int server_socket);
    int handle_request();
    int send_file();
    int delete_file();
    int get_file();
    int rename_file();
    int get_file_list();
    void send_ack(const char c);
    // int send_data(const char *buffer, const int size);
    // int recv_data(char *filename);
    char *recv_string();
};
