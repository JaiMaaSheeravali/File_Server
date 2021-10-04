#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#include "../include/helper.hpp"
#include "../include/socket.hpp"
#include "../include/thread.hpp"
#include "../include/Request.hpp"
#include "../include/Queue.hpp"

const int THREAD_POOL_SIZE = 20;

pthread_t threads[THREAD_POOL_SIZE];
Queue list_queues[THREAD_POOL_SIZE];


void makeDirectory(char* dirname){
	DIR* dir = opendir(dirname);

	if(dir){
		// do nothing
	}
	else if(ENOENT == errno){
		if(mkdir("storage", 0777) == -1){
			std::cerr << "Error Creating Storage Directory.\n";
		}
	} else {
		std::cerr << "Error opening Storage Directory\n";
	}
}

int main(int argc, char *argv[])
{

	makeDirectory((char*)"storage");
	makeDirectory((char*)"storage/shared");
	makeDirectory((char*)"storage/private");

	// create socket, bind to localip address and listen for incoming connections
	int server_socket = create_server_socket();

	// create a thread pool and pass the address of ith queue to ith thread
	// or in other words ith thread will handle requests in ith queue
	for (int i = 0; i < THREAD_POOL_SIZE; i++)
	{
		pthread_create(&threads[i], nullptr, thread_function, &list_queues[i]);
	}

	// main thread's job is to accept() the connections
	// and put in the queue so that thread pool could handle it
	std::cout << "server: waiting for connections...\n";
	while (true)
	{
		// accept the connection(create a new socket)
		// for serving the client's request
		Request *req = new Request();
		int rv = req->accept_request(server_socket);
		if (rv == -1)
		{
			perror("accept");
			delete req;
			continue;
		}

		// main thread randomly chooses the queue for pushing the request
		int queueID = rand(0, THREAD_POOL_SIZE - 1);
		// push the request to the ith queue(where i =queueID)
		list_queues[queueID].enqueue(req);
	}

	return 0;
}