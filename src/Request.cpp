#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <sys/sendfile.h>

#include "../include/color.hpp"
#include "../include/helper.hpp"
#include "../include/parse.hpp"
#include "../include/Request.hpp"

using namespace std;

// expecting
// login/register username password\n
// operation filename

Request::~Request()
{
    std::cout << MAGENTA << "Client ";
    client.printIpAddress();
    std::cout << " disconnected" << RESET << std::endl;
    close(sockfd);
    close(diskfilefd);
}

int Request::accept_request(int server_socket)
{
    // create a nonblocking socket for the newly connected client and get its ip address
    sockfd = accept4(server_socket, (struct sockaddr *)&(client.ipaddr),
                     &(client.ipaddr_len), SOCK_NONBLOCK);

    // store the presentational ip address in host and port number in serv
    getnameinfo((struct sockaddr *)&client.ipaddr, client.ipaddr_len,
                client.host, sizeof(client.host), client.serv,
                sizeof(client.serv), NI_NUMERICHOST | NI_NUMERICSERV);

    // print which client got connected to the server
    std::cout << CYAN << "Client ";
    client.printIpAddress();
    std::cout << " connected" << RESET << std::endl;
    return sockfd;
}

int Request::handle_request()
{
    int status = ONGOING;
    switch (state)
    {
    case State::FETCHING:
        status = fetchFtpRequest();
        break;
    case State::LISTING:
        status = sendListToClient();
        break;
    case State::RECEIVING:
        status = recvFileFromClient();
        break;
    case State::SENDING:
        status = sendFileToClient();
        break;
    default:
        // send(NACK)
        break;
    }
    return status;
}

// ignore
// is a return ACK required after fetching and processing the ftp header?
// list send number of bytes as ACk
// upload neccesity
// download send file size in bytes as ACK
// rename neccessity and sufficient
// delete neccesesty and sufficeint

void Request::send_ack(const char c) {
    if(send(sockfd, &c, sizeof(c), 0) < 0){
        cout << RED << "Error sending ack!" << RESET;
    }
}

int Request::fetchFtpRequest()
{
    int count = recv(sockfd, buffer + strlen(buffer), BUFSIZ, 0);
    if (count == 0)
    {
        // disconnected
        // send(NACK)
        return COMPLETED;
    }

    if ((strstr(buffer, "done")))
    {
        std::cout << "completion \n";
        return parseFtpRequest();
    }
    else
    {
        // still haven't received the ftp request header
        return ONGOING;
    }
}

int Request::recvFileFromClient()
{
    std::cout << "entered receiving section" << std::endl;
    /* Sending file data */
    while (bytes_left > 0)
    {
        // create a new buffer since we are going to write(in diskfd), whatever
        // data received from the client in one go (no funny behavior)
        // although its not necessary that the whole file will be receivied in one recv
        char buffer[BUFSIZ];
        memset(buffer, '\0', sizeof(buffer));

        // perform non blocking io on the socket file descriptor
        // get the data from the client, store it in the buffer
        if (((bytes_recvd = recv(sockfd, buffer, BUFSIZ, 0)) == -1))
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no data avalaible currently try again later (after poll)
                std::cout << "blocked" << std::endl;
                return ONGOING;
            }
            else
            {
                perror("failed file transfer");
                send_ack('1');
                return COMPLETED;
            }
        }
        else if (bytes_recvd == 0)
        {
            send_ack('2');
            perror("upload failed");
            std::cout << "client closed the connection abruptly" << std::endl;
            // maybe delete the file since the uploading was disrupted
            return COMPLETED;
        }
        else
        {
            std::cout << "bytes recvd yet: " << bytes_recvd << std::endl;
            bytes_left -= bytes_recvd;

            // copy the data from the buffer to the local disk file
            if (writen(diskfilefd, buffer, bytes_recvd) < 0)
            {
                send_ack('3');
                perror("disk write failed\n");
                return COMPLETED;
            }
        }
    }

    send_ack('0');
    return COMPLETED;
}


int Request::sendListToClient()
{
    std::cout << "entered listing section" << std::endl;
    /* Sending content of bigFuffer that contains list of files */
    while (bytes_left > 0)
    {
        // perform non blocking io on the socket file descriptor
        if (((bytes_sent = send(sockfd, bigBuffer, bytes_left, 0)) == -1))
        {
            if (errno == EAGAIN)
            {
                std::cout << "blocked" << std::endl;
                // kernel buffer is full currently, try again later (after poll)
                return ONGOING;
            }
            else
            {
                perror("failed data transfer");
                // send(NACK)
                return COMPLETED;
            }
        }
        std::cout << "bytes sent yet: " << bytes_sent << std::endl;
        bigBuffer += bytes_sent;
        bytes_left -= bytes_sent;
    }

    std::cout << "completed list\n";
    // send(ACK)
    return COMPLETED;
}

int Request::sendFileToClient()
{
    std::cout << "entered sending section" << std::endl;
    /* Sending file data */
    while (bytes_left > 0)
    {
        // perform non blocking io on the socket file descriptor
        if (((bytes_sent = sendfile(sockfd, diskfilefd, nullptr, bytes_left)) == -1))
        {
            if (errno == EAGAIN)
            {
                std::cout << "blocked" << std::endl;
                // kernel buffer is full currently try again later (after poll)
                return ONGOING;
            }
            else
            {
                perror("failed file transfer");
                // send(NACK)
                return COMPLETED;
            }
        }
        std::cout << "bytes sent yet: " << bytes_sent << std::endl;
        bytes_left -= bytes_sent;
    }

    // send(ACK)
    return COMPLETED;
}