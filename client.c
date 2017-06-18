#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
	
int clientSocket;
struct sockaddr_in serverAddr;
socklen_t addr_size;
char statusResult[2*1024];

/* Usage Function */

void usage(){

	printf("*****Commands Usage*****\n");
	printf("1. Ping site -> pingsites [website1,website2,...] (e.g. pingsites www.abc.com,www.xyz.com)\n");
	printf("2. Show handles -> showhandles (e.g. showhandles)\n");
	printf("3. Show status of specific handle -> showhandlestatus handle1 (e.g. showhandlestatus 2)\n");
	printf("4. Show status of all handle  -> showhandlestatus (e.g. showhandlestatus)\n");
	printf("5. For Usage Help -> help\n");
	printf("6. For Exit -> exit\n");

}

/* Main Function */

int main(int argc, char *argv[])
{
	char buffer[1024];
	int i=0;
	int port=atoi(argv[2]);								//port from argument		
	
	if(argc !=3)
	{
		printf("missing parameters\n");
		printf("Usage: ./client serverIP portNumber\n");
		return 1;
	}	
	
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);					//create Socket
  
	if(clientSocket == -1)
	{
		printf("Socket not created\n");
		exit(-1);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

	memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  

        addr_size = sizeof(serverAddr);
	
  
        if(connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) < 0)	//connect to the server
        {
                perror("Connection error\n");
                exit(-1);     
	}
	
	char cmd[1000];
	while(1)
	{
		
		memset(statusResult,0,sizeof statusResult);
		memset(cmd,0,sizeof cmd);
	
		printf("Input New Command:\n");						//insert command
		fgets(cmd,sizeof(cmd), stdin);
		
		cmd[strcspn(cmd,"\n")] = '\0';
		if(strcasecmp(cmd,"help")==0)
		{
			usage();
			continue;
		}

		if(strcasecmp(cmd,"exit")==0)
			exit(1);

		send(clientSocket,cmd,strlen(cmd),0);					//send command to the server	

		int ret = recv(clientSocket,statusResult,sizeof(statusResult),0);
		if(ret < 0)
		{
			continue;
		}
		
		printf("%s\n",statusResult);				//print Reply fromm server

	}

	close(clientSocket);
	return 0;
}
