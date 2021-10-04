#pragma once
#include <queue>
#include "./Request.hpp"

// Queue of pointers to Requests
class Queue
{
private:
    std::queue<Request *> myqueue;
    pthread_mutex_t mutex;
    pthread_cond_t cond_var;

public:
    Queue();
    ~Queue();
    void enqueue(Request *req);
    Request *dequeue();
    Request* tryDequeue();
};