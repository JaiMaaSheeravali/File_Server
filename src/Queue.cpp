#include "../include/Queue.hpp"

#include "../include/Request.hpp"

Queue::Queue()
{
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond_var, nullptr);
}

Queue::~Queue()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_var);
}

void Queue::enqueue(Request *req)
{
    // acquire lock for mutual exclusion to the queue
    pthread_mutex_lock(&mutex);
    myqueue.push(req);

    // wakeup a waiting thread if there is any
    pthread_cond_signal(&cond_var);
    pthread_mutex_unlock(&mutex);
}
Request *Queue::dequeue()
{
    // acquire lock for mutual exclusion to the queue
    pthread_mutex_lock(&mutex);

    // go to sleep if no connection is inside the queue
    while (myqueue.empty())
    {
        pthread_cond_wait(&cond_var, &mutex);
    }

    // fetch the first request in the queue
    Request *req = myqueue.front();
    myqueue.pop();

    // release the lock so that either main thread could accept a new connection
    // or the worker threads can handle a connection inside the queue
    pthread_mutex_unlock(&mutex);
    return req;
}