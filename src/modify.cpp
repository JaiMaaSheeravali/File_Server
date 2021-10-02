// #include <iostream>
// #include <cstring>
// #include <string>
// #include <sys/socket.h>

// #include "../include/Request.hpp"
// #include "../include/color.hpp"

// using namespace std;

// int Request::delete_file(){
//     char* filename = recv_string();

//     if(remove(filename) != 0){

//         cerr << RED << "Error Deleting File: '" << filename << "'!\n" << RESET;
//     } else {
        
//         cout << GREEN << "Successfully deleted '" << filename << "' from Server.\n" << RESET;
//     }
//     return 0;
// }

// int Request::rename_file(){

//     char *original_filename = recv_string();
//     char *new_filename = recv_string();

//     if(rename(original_filename, new_filename) != 0){

//         cerr << RED << "Error Renaming '" << original_filename << "' to '" << new_filename << "'!\n" << RESET;
//     } else {

//         cout << GREEN << "Successfully renamed '" << original_filename << "' to '" << new_filename << "'.\n" << RESET;
//     }
//     return 0;
// }