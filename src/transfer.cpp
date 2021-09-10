#include <iostream>
#include <cstring>
#include <fstream>
#include <sys/socket.h>

#include "../include/Request.hpp"

using namespace std;

int Request::send_data(const char *buffer, const int size){

    if(send(client.sockfd, &size, sizeof(int), 0) < 0){
        cout << "Unable to send message size\n";
        return -1;
    }
    if(send(client.sockfd, buffer, size, 0) < 0){
        cout << "Unable to send message content\n";
        return -1;
    }
    return 0;
}

int Request::recv_data(char* filename){
    int file_size = 0;

    if (recv(client.sockfd, &file_size, sizeof(int), 0) < 0){
        cout << "Couldn't receive\n";
        return -1;
    }
    cout << "Size of file: " << file_size << "\n";

    char buffer[file_size];

    if(recv(client.sockfd, buffer, file_size, 0) < 0){
        cout << "Couldn't receive\n";
        return -1;
    }

    ofstream file(filename, ios::out|ios::binary);
    file.write(buffer, file_size);
    return 0;
}

char* Request::recv_string(){
    int file_size = 0;

    if (recv(client.sockfd, &file_size, sizeof(int), 0) < 0){
        cout << "Couldn't receive\n";
    }
    cout << "Size of file: " << file_size << "\n";

    char *buffer = new char[file_size+1];
    buffer[file_size] = '\0';

    if(recv(client.sockfd, buffer, file_size, 0) < 0){
        cout << "Couldn't receive\n";
        return buffer;
    }
    return buffer;
}

int Request::send_file(){
    
    char *filename = recv_string();

    cout << filename << endl;

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

    return 0;
}

int Request::get_file(){

    char *filename = recv_string();

    cout << filename << endl;

    recv_data(filename);
    
    return 0;
}