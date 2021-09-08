#pragma once
#include "../include/Client.hpp"

class Request
{
private:
    Client client;
    // enum class RequestType : unsigned char
    // {
    //     LOGIN,
    //     REGISTER,
    //     DOWNLOAD,
    //     UPLOAD,
    //     SHARE,
    //     RECEIVE,
    //     RENAME,
    //     DELETE,
    //     LIST,
    //     LIST_SHARED,
    // };

    // RequestType req_type;
    bool is_authorised = false;

public:
    int accept_request(int server_socket);
    void handle_request();
    int send_file();
    int delete_file();
    int get_file();
    int rename_file();
    int get_file_list();
    int send_data(const char *buffer, const int size);
    int recv_data(char* filename);
    char* recv_string();
};