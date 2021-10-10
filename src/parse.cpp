#include "../include/parse.hpp"
#include "../include/Request.hpp"
#include "../include/color.hpp"
#include "../include/helper.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <utility>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include <sys/sendfile.h>

using namespace std;

static inline std::string &ltrim(std::string &s)
{
    s.erase(
        s.begin(),
        std::find_if(
            s.begin(),
            s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(
                s.rbegin(),
                s.rend(),
                std::not1(std::ptr_fun<int, int>(std::isspace)))
                .base(),
            s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
    return ltrim(rtrim(s));
}

// split the input string into list of words depending on delimiter
std::vector<std::string> ftp_tokenizer(const std::string &input, const char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;

    // input stream used to split string around delimiter
    std::istringstream iss(input);

    // Tokenizing w.r.t. specified delimiter
    while (getline(iss, token, delimiter))
    {
        token = trim(token);
        if (!token.empty())
        {
            tokens.push_back(token);
        }
    }

    return tokens;
}

int Request::parseFtpRequest()
{
    // extract username password, operation filename from buffer and then
    // perform authorisation
    // and then open the corresponding file if required
    std::string header = buffer;

    auto list_lines = ftp_tokenizer(header, '\n');
    auto auth_line = ftp_tokenizer(list_lines[0], ' ');

    if (auth_line.size() != 3)
    {
        std::cout << "error 400 malformed request syntax \n";
        return 1;
    }

    std::string auth_kind = auth_line[0];
    client.username = auth_line[1];
    client.password = auth_line[2];

    if (auth_kind == "login")
    {
        std::cout << "client wants to login" << std::endl;
        if (login_user(client.username, client.password))
        // if (/*loginWithUsernamePassword(client.username, client.password)  */ true)
        {
            // "done" means client has sent the fpt header completely
            if (list_lines[1] == "done")
            {
                cout << "Completed login on server" << endl;
                send_ack('0');
                // client just wanted to login so we are done
                return COMPLETED;
            }

            // client authoried and now wants to access a service
            isGlobal = list_lines[1].find("-g") != std::string::npos;

            auto request_line = ftp_tokenizer(list_lines[1], ' ');
            std::string req_kind = request_line.front();

            if (req_kind == "list")
            {
                return parseListRequest();
            }
            else if (req_kind == "upload")
            {
                bytes_left = stoi(request_line.rbegin()[1]);
                return parseUploadRequest(request_line.back());
            }
            else if (req_kind == "download")
            {
                return parseDownloadRequest(request_line.back());
            }
            else if (req_kind == "rename")
            {
                auto rev_itr = request_line.rbegin();
                return parseAndExecuteRenameRequest(rev_itr[0], rev_itr[1]);
            }
            else if (req_kind == "delete")
            {
                return parseAndExecuteDeleteRequest(request_line.back());
            }
            else
            {
                // send(NACK);
                std::cout << "error 404 request not found" << std::endl;
                return COMPLETED;
            }
        }
        else
        {
            send_ack('1');
            std::cout << "error 401 unauthorized client" << std::endl;
            return COMPLETED;
        }
    }
    else if (auth_kind == "register")
    {
        std::cout << "client wants to register" << std::endl;
        string path = "storage/private/"+client.username;

        if(login_user(client.username, client.password)){
            send_ack('2');

        } else if(checkifExisting((char*)(path.c_str()))){
            send_ack('3');
            
        } else if(register_user(client.username, client.password)){
            
            makeDirectory((char*)(path.c_str()));
            send_ack('0');
        } else {
            send_ack('1');
        }
        return COMPLETED;
    }
    else
    {
        std::cout << "error 400 wrong request syntax" << std::endl;
        return COMPLETED;
    }
}
int Request::parseListRequest()
{
    std::string pathname;
    DIR *d;
    struct dirent *dir;
    if (isGlobal)
        pathname = "./storage/shared/";
    else
        pathname = "./storage/private/" + client.username;

    d = opendir(pathname.c_str());
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_type == DT_REG)
            {
                list.append(dir->d_name);
                list.push_back('\n');
            }
        }
        closedir(d);
    }
    std::cout << BOLDBLUE << list
              << RESET << std::endl;

    bigBuffer = list.c_str();
    bytes_left = list.size()+1;

    pollFd->events = POLLOUT;
    state = State::LISTING;

    int size = htonl(bytes_left);
    if(send(sockfd, &size, sizeof(int), 0) < 0){
        std::cerr << "List Failed\n";
    }

    return ONGOING;
}

int Request::parseUploadRequest(const std::string &filename)
{
    std::string pathname;
    if (isGlobal)
        pathname = "./storage/shared/" + filename;
    else
        pathname = "./storage/private/" + client.username + "/" + filename;

    diskfilefd = open(pathname.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (errno == EEXIST)
    {
        // file already exists so you cant upload
        send_ack('2');
        return COMPLETED;
    }

    send_ack('0');
    state = State::RECEIVING;
    return ONGOING;
}

int Request::parseDownloadRequest(const std::string &filename)
{
    std::string pathname;
    if (isGlobal)
        pathname = "./storage/shared/" + filename;
    else
        pathname = "./storage/private/" + client.username + "/" + filename;

    diskfilefd = open(pathname.c_str(), O_RDONLY);
    if (errno == ENOENT)
    {
        // file does not exist so you cannot download
        send_ack('2');
        return COMPLETED;
    }

    /* Get file stats */
    struct stat file_stat;
    if (fstat(diskfilefd, &file_stat) < 0)
    {
        perror("fstat failed");
        send_ack('1');
        exit(EXIT_FAILURE);
    }

    send_ack('0');

    bytes_left = file_stat.st_size;

    std::cout << "File Size: " << bytes_left << " bytes\n";
    sprintf(fileSize, "%d", bytes_left);

    
    pollFd->events = POLLOUT;
    state = State::SENDING;

    cout << "Sending bytes " << bytes_left << endl;
    int size = htonl(bytes_left);
    if(send(sockfd, &size, sizeof(int), 0) < 0){
        std::cerr << "Download Failed\n";
    }

    return ONGOING;
}

int Request::parseAndExecuteRenameRequest(const std::string &new_filename, const std::string &old_filename)
{
    std::string old_pathname;
    std::string new_pathname;

    if (isGlobal)
    {
        old_pathname = "./storage/shared/" + old_filename;
        new_pathname = "./storage/shared/" + new_filename;
    }
    else
    {
        old_pathname = "./storage/private/" + client.username + "/" + old_filename;
        new_pathname = "./storage/private/" + client.username + "/" + new_filename;
    }

    if(access(old_pathname.c_str(), F_OK ) == -1)
    {

        std::cout << RED << "Rename Error: file at '" << old_pathname << "' doesn't exist!\n" << RESET;
        send_ack('2');
    } else if(access(new_pathname.c_str(), F_OK) != -1){

        std::cout << RED << "Rename Error: file with '" << new_pathname << "' already exists can't overwrite!\n" << RESET;
        send_ack('3');

    } else if (rename(old_pathname.c_str(), new_pathname.c_str()) != 0)
    {

        std::cerr << RED << "Error Renaming '"
                  << old_pathname
                  << "' to '"
                  << new_pathname << "'!\n"
                  << RESET;
        send_ack('1');
    }
    else
    {
        std::cout << GREEN
                  << "Successfully renamed '" << old_pathname
                  << "' to '"
                  << new_pathname << "'.\n"
                  << RESET;
        send_ack('0');
    }
    return COMPLETED;
}
int Request::parseAndExecuteDeleteRequest(const std::string &filename)
{
    
    if (isGlobal)
        pathname = "./storage/shared/" + filename;
    else
        pathname = "./storage/private/" + client.username + "/" + filename;

    if(access( pathname.c_str(), F_OK ) == -1)
    {

        std::cout << RED << "Delete Error: file at '" << pathname << "' doesn't exist!\n" << RESET;
        send_ack('2');

    } else if (remove(pathname.c_str()) != 0)
    {
        std::cerr << RED
                  << "Error Deleting File: '"
                  << pathname << "'!\n"
                  << RESET;
        send_ack('1');
    }
    else
    {
        std::cout << GREEN << "Successfully deleted '"
                  << pathname
                  << "' from Server.\n"
                  << RESET;
        send_ack('0');
    }
    return COMPLETED;
}