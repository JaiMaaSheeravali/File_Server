#include "../include/thread.hpp"

#include "../include/Queue.hpp"
#include "../include/Request.hpp"

void *thread_function(void *arg)
{
    // pointer to one of the queue
    Queue *q_requests = (Queue *)arg;
    while (true)
    {
        // thread(from thread pool) serves the request(a client)
        // dequeued from the queue
        Request *req = q_requests->dequeue();
        req->handle_request();
        delete req;
    }
}