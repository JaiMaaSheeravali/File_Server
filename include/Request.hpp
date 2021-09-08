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
};