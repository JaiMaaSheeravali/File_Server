#include <iostream>
#include <cstring>
#include <fstream>
#include <sys/socket.h>

#include "../include/Request.hpp"
#include "../include/color.hpp"

using namespace std;

int Request::send_file(){
    
    char *filename = recv_string();

    //open file in binary mode, get pointer at the end of the file (ios::ate)
    ifstream file (filename, ios::in|ios::binary|ios::ate); 

    //retrieve get pointer position
    int file_size = file.tellg();

    //position get pointer at the begining of the file                                   
    file.seekg (0, ios::beg);
    char buffer[file_size];
    file.read (buffer, file_size);
    file.close(); 
    
    send_data(buffer, file_size);

    cout << GREEN << "Successfully Sent '" << filename << "' to Client ";
    client.printIpAddress();
    cout << ".\n" << RESET;

    return 0;
}

int Request::get_file(){

    char *filename = recv_string();

    recv_data(filename);

    cout << GREEN << "Successfully Received '" << filename << "' from Client ";
    client.printIpAddress();
    cout << ".\n" << RESET;
    
    return 0;
}