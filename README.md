# Network-Queue-Simulation
Implemented Network Queue Simulation using POSIX threads.

Client(s) will send request with specific command to the server.

Command:
1. pingsites [site-name separated with commas]    --> open and close sockets with website and calculate min, max, avg time for ping functionality simulation
    e.g. pingsites www.xyz.com,www.abc.com
2. showhandlestatus [handleId]                    --> show status of sites (IN QUEUE, COMPLETED or IN PROCESS)
    e.g. showhandlestatus 2
3. showhandles                                    --> display all unique request id
4. help                                           --> Usage
5. exit                                           --> Exit

Note: In command 2, handleId is optional. If handleId mentioned then it will give status of that specific request else will give status of all requests.



-	Server initially create socket and keeps accepting connections from the clients.
-	Once, request is received from some client(s), request handler calls corresponding sub routine for the command.
-	For ping site command, queue (Linked List) is created with each website.
-	In parallel, worker threads are in wait state.
-	Dispatcher continuously monitors for new request in queue and dispatch job to worker threads.
-	Once, there is any new website request in the queue, dispatcher signals thread to start serving that request.
-	The process keeps continuing till server closes its socket.

Instructions on how to compile and run the code:

There are 2 source code files (server.c and client.c). To compile and run both files, command is:
For Server:
-	Compile: gcc server.c -o server -lpthread -w
-	Run: ./server Port
o	E.g. ./server 9999
For Client:
-	Compile: gcc client.c -o client
-	Run: ./client IP_Adress Port
o	E.g. ./client  127.0.0.1 9999
