#include <iostream>
#include <cstring>
#include <string>
#include <sys/socket.h>

#include "../include/Request.hpp"

using namespace std;

int Request::delete_file(){
    char* filename = recv_string();

    if(remove(filename) != 0){
        cerr << "Error Deleting File: " << filename << "!" << endl;
    }
    return 0;
}

int Request::rename_file(){

    char *original_filename = recv_string();
    char *new_filename = recv_string();

    if(rename(original_filename, new_filename) != 0){
        cerr << "Error Renaming " << original_filename << " to " << new_filename << "!" << endl;
    }

    return 0;
}