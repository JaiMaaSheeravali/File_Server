#include <iostream>
#include <cstring>
#include <string>
#include <dirent.h>
#include <sys/socket.h>

using namespace std;

#include "../include/Request.hpp"


int Request::get_file_list(){

    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    string list;
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            int filename_size = sizeof(dir->d_name);

            list.append(dir->d_name);
            list.push_back('\n');
        }
        closedir(d);
    }
    
    send_data(list.c_str(), list.size());

    return 0;
}