#include "../include/thread.hpp"

#include <poll.h>
#include <iostream>
#include <unistd.h>
#include <unordered_map>

#include "../include/Queue.hpp"
#include "../include/Request.hpp"

// number of file descriptors is same as number of sockets
// which in turn is same as number of requests

// maximum number of requests a single thread can handle
const nfds_t MAX_FDS = 50;

const int INFINITE = -1;
const int TIMEOUT = 5000;

void *thread_function(void *arg)
{
    // pointer to one of the queue
    Queue *q_requests = (Queue *)arg;

    // pollfd array for event programming
    struct pollfd pollFds[MAX_FDS];
    for (int i = 0; i < MAX_FDS; i++)
    {
        pollFds[i].fd = -1; // initially not interested in any sockfd
    }

    // initially no socket connection is being handled by the thread
    nfds_t nfds = 0;

    // hash map which takes socket fd and returns pointer to request object
    std::unordered_map<int, Request *> sockfdToReq;
    Request *req;
    while (true)
    {
        // thread(from thread pool) serves the request(a client)
        // that is dequeued from the queue
        if (nfds < MAX_FDS)
        {
            if (nfds == 0)
                // thread has no one to serve to so just
                // wait indefinelty until a client connects
                req = q_requests->dequeue();
            else
                // thread already has some clients to serve so just
                // try to dequeue from the queue not wait indefinitely
                req = q_requests->tryDequeue();

            if (req != nullptr)
            {
                nfds++;
                // a new connection is accepted so put it in the struct pollfd pollFds array
                // look for empty slot in the pollFds array
                int i = 0;
                while (i < MAX_FDS && pollFds[i].fd >= 0)
                {
                    i++;
                }
                // when the new connection is accepted, the server's thead from thread pool
                // will 'read' the client's request from the socket (that's why POLLIN is used)
                pollFds[i].fd = req->sockfd;
                pollFds[i].events = POLLIN; // initially read from the socket

                // tldr; hash map is used after poll system call
                // long explanation: poll system call just tells us the socket(socket fd) at
                // which the data can be read(POLLIN) or written(POLLOUT) but won't
                // tell us which client that socket is connected to; for that we need a hash map
                // that takes the socket id from poll system call and then returns the request object
                // (which contains all the information about that client)
                sockfdToReq[req->sockfd] = req;

                // store the address of corresponding pollFd in the request obj
                // this req->pollFd is required in the handle request funciton
                // because it could happen the client which was sending the data
                // now wants to receive the data from server
                // so we will need to change the event from POLLIN to POLLOUT
                req->pollFd = (pollFds + i);
            }
        }

        // wait for INFINITE time if the connections are full
        // else wait for TIMEOUT miliseconds
        int timeout = nfds == MAX_FDS ? INFINITE : TIMEOUT;
        int ready = poll(pollFds, MAX_FDS, timeout);

        if (ready <= 0)
        {
            // std::cout << "not yet available\n";
            continue; // either timeout or error occured then continue
        }

        for (int i = 0; i < MAX_FDS; i++)
        {
            auto [fd, events, revents] = pollFds[i];
            if (fd != -1 && (revents & events))
            {
                std::cout << fd << std::endl;
                req = sockfdToReq[fd];

                if (req->handle_request())
                {
                    // request completed
                    pollFds[i].fd = -1;
                    nfds--;
                    sockfdToReq.erase(fd);
                    delete req;
                }
            }
        }
    }
    return nullptr;
}