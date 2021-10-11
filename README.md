# CSN 510: File Server

**Table of Contents**

- [Installation](#installation)
- [FTP Protocol](#ftp-protocol)
  * [FTP Header](#ftp-header)
  * [Functioning of Protocol](#functioning-of-protocol)
- [Multithreaded Server](#multithreaded-server)
  * [Dynamic Thread Approach](#dynamic-thread-approach)
  * [Thread Pool Approach](#thread-pool-approach)
  * [Thread Pool with Event Programming Approach](#thread-pool-with-event-programming-approach)
    + [Detailed Explanation](#detailed-explanation)
    + [Thread Function](#thread-function)
    + [State Management](#state-management)
    + [Networking Concepts](#networking-concepts)
      - [Receiving](#receiving)
      - [Sending](#sending)
      - [Listing](#listing)

## Installation

1. Install the required packages.

```bas
sudo apt update
# install c++ essentials
sudo apt install build-essential
# install database
sudo apt-get install libmysqlcppconn-dev`
```

2. Clone the repository and move inside File_Server directory

```bash
# clone the github repository
git clone https://github.com/JaiMaaSheeravali/File_Server.git
cd File_Server
```

3. Setup Database. 

```bash
# Type the MySql password to create the database along with the table for authentication
cat instruct.txt | mysql -u root -p

# Open the `File_Server/src/authentication.cpp` and then
# Set the empty string pass1 to the MySql password you chose above.
vim ./src/authentication.cpp

# Finally go to parent directory of the project.
cd ..
```

4. Build the file server project

```bash
# link
cmake .
# build
make
# run the project
./output/server
```




## FTP Protocol

### FTP Header

* Client will always send login data in the first line.
* Client will send the request type and the parameters in the second line in the same order as mentioned in the table.

Client Sends FTP header depending on the type of request from the following table.

| Request Type | Header                                                       |
| :----------: | ------------------------------------------------------------ |
|    login     | login username password<br>done                              |
|   register   | register username password<br>done                           |
|   download   | login username password<br>download filename.txt<br>done<br>or<br>login username password<br>download -g filename.txt<br>done |
|    upload    | login username password<br>upload filesize filename.txt<br>done<br>or<br>login username password<br>upload -g filesize filename.txt<br>done |
|    rename    | login username password<br>rename hello.txt hola.txt<br>done<br>or<br>login username password<br>rename -g hello.txt hola.txt<br>done |
|    delete    | login username password<br>delete filename.txt<br>done<br>or<br>login username password<br>delete -g filename.txt<br>done |
|     list     | login username password<br>list<br>done<br>or<br>login username password<br>list -g<br>done |

### Functioning of Protocol

<ol style="line-height:2em">
  	<li>Client creates a FTP header depending on the request and sends it to the server</li>
  	<li>Server keeps receiving the header until the <em>done</em> is received.</li>
  	<li>Server parses the ftp request in several steps.
  		<ol type="i">
            <li>In the first line of header it checks for authentication data which could either be register or login.</li>
            <li> In case of register it checks if the username already exists, registers the user, creates a new directory <em>storage/private/username</em> and send the ACK.</li>
            <li> And in case of login it first performs authentication and then checks for a second line in the header which could contain request type as mentioned in the table. If there is no request line server just sends the ACK to the client.</li>
            <li>If there is a second line containing request information server parses it and makes preparations to handle that request.</li>
          </ol>
    </li>
</ol>

> In case of requests like rename and delete, server will immediately complete the whole job just on the basis of the header, since we don't need to transfer data here. But if the request is something else then we are not yet done. 

Table illustrating the Server's parser job:

| Request Type | Parser's Job                                                 |
| :----------: | ------------------------------------------------------------ |
|    login     | perform authentication with the<br>help of MySql database.   |
|   register   | check if username is not taken,<br>and create the directory<br>"./storage/private/username" |
|    :bulb:    | in all the below requests<br>perform login authentication<br>and check if global flag is<br>passed or not |
|   download   | get the filename from the<br>request and open the file in<br>read only mode. |
|    upload    | get the filename and the file<br>size from the request and open<br>the file in write only mode<br>with some appropriate<br>flags(check the code). |
|    rename    | get the old and new filename<br>from the request and make the<br>rename system call and be done<br>with it. |
|    delete    | get the filename and make the<br>remove system call and be done<br>with it. |
|     list     | get the filenames from the<br>appropriate directory and store<br>it in the bigBuffer since we<br>could have lots of files. |

<ol style="line-height:2em" start="4">
  <li>After parsing is done send appropriate acknowledgements. So that client is ready to do the next job which could be transferring data or just console logging error messages to the user.</li>
  <li>Client accepts the acknowledgement and does the needful.</li>
    <li>After the data is transferred, do a final acknowledgement just in case anything goes wrong. e.g in case of uploading checking whether the file size sent in the FTP header is the same as the number of bytes received.</li>
</ol>
Table summarizes what the client and server will do after the acknowledgements are done.

| Request Type | Ftp Protocol                                                 |
| :----------: | ------------------------------------------------------------ |
|    login     | nothing significant other than<br>the fact that login credentials<br>have to be cached in the client<br>machine for further requests. |
|   register   | same as above                                                |
|   download   | Client receives the file size<br>as a separate packet. Client <br>opens the file and the buffer,<br>while the server sends the data<br>(non blocking style) through<br>the network. |
|    upload    | Client opens the file and sends<br>the file through the network,<br>while the server which has<br>already opened the file reads<br>chunks of data whenever<br>available (in non blocking way). |
|    rename    | Client receives the ack and done.                            |
|    delete    | Client receives the ack and done.                            |
|     list     | Client opens the buffer and<br>receives the data in chunks of course,<br>while the server bigBuffer sends the<br>data (non blocking style). |

## Multithreaded Server

Multithreading is needed because server can take lots of time if there are multiple clients and to handle each of the clients we can create multiple threads so that each thread can serve a unique client and fasten the process. 

### Dynamic Thread Approach

```cpp
int accept_connection(int server_socket);
int handle_request();

// main
int server_socket;
while (true)
{
	printf("waiting for connection\n");

	// 1. main thread accepts the connection
	// 2. creates a new thread
	// 3. hands over the job of request handling to the new thread
	int client_socket = accept_connection(server_socket);

	// now thread will be used to handle the connection
	pthread_t t;
	pthread_create(&t, NULL, (void*)(handle_request), (void*)(&client_socket));
}
```

### Thread Pool Approach

In the above implementation, there is a major issue that if thousands of clients connect to the server there will be thousands of threads created which of course is not a good idea. Because we won't have that much memory and cores on the processor.

So instead, we will create some constant number of threads `THEAD_POOL_SIZE` in the beginning and use those threads to handle the connection for the clients. Whenever a client `connects` to the server, the main thread will `accept` the connection and put it in the shared queue for thread pool to handle it.

`THEAD_POOL_SIZE` will determine the performance of the server and could be determined by running some test cases or benchmarks.

1. Server creates `THEAD_POOL_SIZE` number of threads and `THEAD_POOL_SIZE`  number of queues (of requests). Request is a class which contains all the necessary information required to serve the client e.g. socket id, IP address, username, password etc.
2. Client `connect()` to the server, the server's main thread accepts the connection and put the socket id into one of the  queue (of requests). Threads are repeatedly checking (conditional waiting actually) for a connection inside the queue. If there is some connection, then the threads from thread pool handles it and works upon it.
3. Else the thread keeps sleeping and whenever new connection comes to the corresponding queue it wakes up and handles it.

Since the queue is a shared data structure, we will need some kind of synchronisation primitive. This is *Producer Consumer Problem*.

In the approach below a single client is handled by single thread at a time. For simplicity assume that its a queue of socket file descriptors.

```cpp
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
queue<int> q[THREAD_POOL_SIZE];
void* thread_function(void* arg)
{
    while (true) {
        // acquire lock for mutual exclusion to queue
        pthread_mutex_lock(&mutex[i]);

        // sleep if no connection is inside queue
        while (q[i].empty()) {
            pthread_cond_wait(&condition_var, &mutex[i]);
        }
        // fetch the first client in the queue
        int client_socket = q[i].front();
        q[i].pop();

        /* release the lock so that either main thread could accept a new
           connection or the worker threads can handle a connection inside the queue*/
        pthread_mutex_unlock(&mutex[i]);

        // current thread serves the client just popped from the queue
        handle_connection(client_socket);
        /* the above function could be really slow if the client is using slow
           connection. So we could use select system call to allow a single thread
           to server multiple clients */
    }
}

```

```cpp
// main thread
int server_socket;
// create, bind, listen
while (true) {
    // which queue(i) to pick out of THREAD_POOL_SIZE queues
    // i could be chosen randomly
    int client_socket = accept_connection(server_socket);
    pthread_mutex_lock(&mutex[i]);
    q[i].push(client_socket);
    pthread_cond_signal(&condition_var[i]);
    pthread_mutex_unlock(&mutex[i]);
}
```

> This approach is better than the previous approach in a way that we have limited constant amount of threads which means we would not have to spend lots of time on creation, deletion and memory management of the threads. But this approach has its own issue that we cannot handle more than `THREAD_POOL_SIZE` clients because each thread can at most handle one client at a time. If there are more clients than `THREAD_POOL_SIZE`, then those clients have to wait until one of the thread is available.

### Thread Pool with Event Programming Approach

In the above approach, one thread from thread pool was handling only one client at a time, which of course is a wastage of computer resources (more like not using the full potential of the computer). Why?

Because first we are handling only at most `THREAD_POOL_SIZE` clients at a time. Second reason is what if we connect to a client who is really slow (slow internet speed), he will monopolise the thread and wont let the thread handle other clients which sadly would are waiting for a long time.

This is exactly the problem event programming is going to handle. Now what does event programming means in simple terms.

```cpp
while (true) {
    // getEvents tell us what has happened
    // maybe data has arrived to the socket or
    // maybe we can write data to the socket (buffer is free)
    events = getEvents();
    for (e in events) {
        // the crux of the event programming is that
        // processEvent(e) should be a non blocking function
        // which means that it should not wait and must return immediately
        processEvent(e);
    }
}
```

Now how do we convert the above pseudo code to a code in C. For that we have several options like `select()`, `poll()` & `epoll()`.  In this project we are going with `poll()` system call, although the more modern and efficient one is the `epoll()`.

#### Detailed Explanation

In this approach as well we will have a thread pool of  `THREAD_POOL_SIZE` with as many number of queues. Server's main thread will be responsible to `accept4()` the connection and put it in one of the queue out of  `THREAD_POOL_SIZE` number of queues. On the other side Server's thread pool will be waiting for a connection to come up in the corresponding queue. If some request is indeed there in the queue, then the thread will wake up and handle it. 

But the interesting part is that when the thread is handling the request it will do non blocking I/O only. That is the thread will never make a call which will put it to sleeping mode. To achieve that we would need two things:

1. We will have to make a `poll()` system call and pass in the *socket file descriptors* along with the *kind of event* which we are interested on that socket. Poll provides us with several kinds of events but the one which we really need are `POLLIN` and `POLLOUT`. 

   * `POLLIN` is used when we want to read data from the socket. `POLLIN` is important because it could happen that there is no data available on the socket and when we make  a `recv()` system call on the socket, the thread will go to sleep.

   * `POLLOUT` is used when we want to write data to the socket. `POLLOUT` is important because it could happen that the kernel inbound buffer is full and when we make a `send()` system call, the thread will go to sleep because we can't write the data to the socket currently.

     If we don't want the thread to block, why not `send()` and `recv()` the data only when it is possible. Now that is exactly what the `poll()` system call will help us to achieve which is it will tell us on which sockets the data can be written and on which sockets the data can be transferred *without blocking*.
   
2. We would make the socket accepted (`accept4()`) to be non blocking, which means that the `send()` and `recv()` system call would not block even if the data transfer is not possible. If data transfer is not possible it would give us an error and we will know that we have to retry the operation again later. 

   - This part is not compulsory if we are using `poll()` system call which gives us level triggered notifications. Because we are already transferring the data only when it is possible anyways (we are already careful, so no need). 

   - But this step is helpful because it could allow us to transfer more data even if we have not yet made the `poll()` system call. For example let us assume that `poll()` indicated that from a socket `sockfd` we can read the data, and then the thread made a `recv()` system call for reading the data. But what if we make more than one `recv()` system call on `sockfd` e.g in a loop. Now that may or may not block. But if we have a non blocking socket and run a loop and make `recv()` system calls, even if something goes wrong we will know and `recv() `the data later without blocking. 
   
   
    This allows us to transfer more data and speed up the process. Although this step make the data transfer fast but unfair to other clients.

#### Thread Function

The pseudo code below shows how event programming is done by a thread from thread pool using `poll()` system call. The actual code is in `src/thread.cpp` file.

```cpp
void* thread_function(void* arg)
{
    while (true) {
        /* ------------ getEvents ------------ */
        int ready = poll(pollFds, MAX_FDS, timeout);

        /* ------------ processEvents ------------ */
        for (int i = 0; i < MAX_FDS; i++) {
            auto [fd, events, revents] = pollFds[i];
            if (fd != -1 && (revents & events)) {
                req->handle_request();
            }
        }
    }
}
```

#### State Management

The main crux of the event programming is state management. In dynamic thread approach or simple thread pool approach where a single thread handled a single client, we did not explicitly need to store the state. Why? Because the thread control block which consists of thread stack stored all the information there till the request is fulfilled. However in event programming, since one thread can handle multiple clients at once(in a loop) we would have to store the state somewhere explicitly. Now that's where the Request Class would be helpful, not only we would store the client's IP address and socket file descriptor, we would also store extra data inside the class like whether data has to be sent or received or FTP request has to be fetched, what is the disk file descriptor etc.

The state would be stored in the enum given below: 

```cpp
// for maintaining the state of the request
enum class State : unsigned char
{
    FETCHING,	// initially when we are receiving FTP Header
    LISTING,	// request type is list or list -g
    SENDING,	// request type is download or download -g
    RECEIVING	// request type is upload or upload -g
};
```

In the beginning any request will start from fetching the FTP Header without which we cannot know whether the client is authorised and the type of request which is asked. This is very similar to HTTP requests in which the clients send what they want along with authorisation token.

How will we know when the client has completely sent the FTP Header? Well we would know it after *done* is received by the server. After the FTP header is received (which could be send in chunks or all together), we will parse it.

> It is very important to understand that the thread (from the thread pool) won't be waiting for the client to send the whole FTP header completely. Instead it would simultaneously (in event loop) be accepting new incoming clients if there are any (in the queue).

After the FTP Header is retrieved by the thread, it will start parsing it and perform authentication and then get the request data from it. Now the question is Why isn't there a separate state for *Parsing* in the `enum State` ? Because parsing is not a blocking operation, it does not involve the network it just involves the local operations.  We could immediately parse the data without going to sleep. 

> Actually parsing could also be a blocking operation as well, since we are going to perform database queries, open files if required, rename, delete etc.  But in this project we are not worrying about local blocking operations for which we would need to perform `Posix Asynchronous I/O (AIO)`.

What exactly happens in the parsing request information step? First the server checks what kind of operation it is upload, rename, delete etc. then it checks for a `-g flag` and finally it checks for the filename if there is any. For example list operation does not require filename, while rename requires both `old_filename` and `new_filename`. The table discussed in FTP Protocol section summarizes what kind of request line is expected by the server.

> Rename and delete operations are done completely in the parsing step, since we don't need to transfer any data here. Only an ACK is sufficient for the client to know whether the operation was successful or not.

The server performs parsing depending upon what kind of request the client sent in the header. To summarise after the parsing is done, the thread will have complete information in the Request object about the operation to be performed. For example in the upload operation, Request object would contain an open file descriptor in write only mode so that when we move to the next state(State::RECEIVING), the thread does not have to open the file descriptor again.

After the parsing is completed by the thread, the Request would now switch from Fetching State to one of the other states depending on the request type. As mentioned above if the request type is rename or delete, we would simply indicate to the thread that we are done with this request so you can delete it.

| Request Type | Request's New State |
| :----------: | :-----------------: |
|   download   |       SENDING       |
|    upload    |      RECEIVING      |
|     list     |       LISTING       |

#### Networking Concepts

In the below section, we will discuss in brief about the core networking concepts on the transferring of data.

##### Receiving

Once the thread is in receiving state, it will `poll()` on the socket file descriptor and wait for data to arrive in it. Once the data arrives we do a non blocking `recv()` system calls on it until either we get a return value of -1 or we have received the whole file. If we have received the whole file we will again indicate to the thread that we are done and you can delete the request. But if we get a return value of -1, it doesn't necessarily means an error occurred (in case of non-blocking I/O), for that we will need to check the `errno`. The below code snippet tells us what happens on a return value of -1.

```cpp
if (((bytes_recvd = recv(sockfd, buffer, BUFSIZ, 0)) == -1))
{
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        // no data avalaible currently try again later (after poll)
        return ONGOING;
    }
    else
    {
        perror("failed file transfer");
        // send(NACK);
        return COMPLETED;
    }
}
```

##### Sending

If the client wants to download some file, the parser would not only set the request to Sending state but also update the `struct pollfd.events ` to `POLLOUT` to indicate the fact that now we are interested to write to the socket and please let us know if the kernel inbound buffer is full or free. In the sending state we are using a special system call `ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count)` for which we don't need to create our own user space buffer. This helps to increase the speed of data transfer.

> sendfile()  copies  data  between  one  file  descriptor  and  another. Because this copying is done within  the  kernel, sendfile()  is  more efficient  than  the  combination  of read(2) and write(2), which would require transferring data to and from user space.
>
> Source: Manual Pages

**Note: `in_fd` can only be a disk file, that is the reason we didn't use it in the upload.**

The following code snippet illustrates how non-blocking I/O is performed on the socket `sockfd`.

```cpp
while (bytes_left > 0)
{
    // perform non blocking io on the socket file descriptor
    if (((bytes_sent = sendfile(sockfd, diskfilefd, nullptr, bytes_left)) == -1))
    {
        if (errno == EAGAIN)
        {
            // kernel buffer is full currently try again later (after poll)
            return ONGOING;
        }
        else
        {
            perror("failed file transfer");
            // send(NACK);
            return COMPLETED;
        }
    }
    std::cout << "bytes sent yet: " << bytes_sent << std::endl;
    bytes_left -= bytes_sent;
}
```

##### Listing

Listing is very similar to sending, at protocol level but the implementation for the sending is special as explained above. In the listing, we would store the list of files in a buffer `char *bigBuffer` and send the contents of the `bigBuffer` in a non blocking fashion. We would need to increment the `bigBuffer` by the amount of bytes sent in the `send()` system call, so that when we come back again after `poll()` we don't send the same data again.

> Please note that we did not increment any pointer in the Sending function because the sendfile() system call automatically updates the file table after some bytes are sent.

The following code snippet shows how the data is sent and the `bigBuffer` is updated. 

```cpp
while (bytes_left > 0)
{
    // perform non blocking io on the socket file descriptor
    if (((bytes_sent = send(sockfd, bigBuffer, bytes_left, 0)) == -1))
    {
        if (errno == EAGAIN)
        {
            // kernel buffer is full currently, try again later (after poll)
            return ONGOING;
        }
        else
        {
            perror("failed data transfer");
            // send(NACK);
            return COMPLETED;
        }
    }
    std::cout << "bytes sent yet: " << bytes_sent << std::endl;
    bigBuffer += bytes_sent;
    bytes_left -= bytes_sent;
}
```

> `bigBuffer` contains the list of files which are retrieved with the help of `opendir()` and `readdir()` system calls.
