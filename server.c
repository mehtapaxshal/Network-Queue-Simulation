#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <limits.h>
#include <sys/time.h>

#define MAX_CLIENTS 5
#define ERROR -1
#define nThreads 5

static const int BUFFER_SIZE=2*1024;
int i;
static int handle = 0;
pthread_mutex_t handleLock;
static int queueServed = 0;			//queue served
static int availableWorkerThreads = 0;		
pthread_mutex_t mutx; 
pthread_mutex_t mutx1; 
pthread_cond_t conditionVar = PTHREAD_COND_INITIALIZER;
pthread_t workerThread[nThreads];				//global worker threads
enum progress {IN_QUEUE,IN_PROGRESS,COMPLETE};

struct List{

	int handleId;
	int min;
	int max;
	int avg;
	char website[100];
	enum progress status;
	struct List *next;

};

struct List *current = NULL;
struct List* node = NULL;
struct List *head = NULL;
struct List *prev = NULL;

/* Send Handles to the client */

void showHandle(int newsockfd)
{
	int j=0;
	char buffer[BUFFER_SIZE];
	char result[BUFFER_SIZE];
	memset(result,'\0',strlen(result));
	memset(buffer,'\0',strlen(buffer));

	for(j=1;j<=handle;j++)
	{
		snprintf(buffer,sizeof buffer,"%d\n",j);
		strcat(result,buffer);

	}

	if(strlen(result)==0)
		strcpy(result,"No Handles in database");	
	send(newsockfd,result,strlen(result),0);
}

/* Handler Status Function for all handlers or specific Handler ID*/

void handleStatus(int newsockfd,char *input){

	struct List *traverseNode = head;
	char buffer[BUFFER_SIZE];
	char result[BUFFER_SIZE];
	int flag = 0;
	memset(result,'\0',strlen(result));
	memset(buffer,'\0',strlen(buffer));
			
	if(strcmp(input,"all")==0)
	{
		while(traverseNode!=NULL)
		{
			
			char* status = (traverseNode->status == 0 ? "IN_QUEUE" : (traverseNode->status == 1 ? "IN_PROGRESS" : "COMPLETED"));		
			snprintf(buffer,sizeof buffer,"%d %s %d %d %d %s\n",traverseNode->handleId,traverseNode->website,traverseNode->avg,traverseNode->min,traverseNode->max,status);
			strcat(result,buffer);
			traverseNode=traverseNode->next;

		}
			
			if(strlen(result)==0)
				strcpy(result,"No request in queue\n");
			send(newsockfd,result,strlen(result),0);
			
	}else
	{
		int handle = atoi(input);
		while(traverseNode!=NULL)
		{
			if(traverseNode->handleId == handle)
			{
				
				flag = 1;
				char* status = (traverseNode->status == 0 ? "IN_QUEUE" : (traverseNode->status == 1 ? "IN_PROGRESS" : "COMPLETED"));
										
				snprintf(buffer,sizeof buffer,"%d %s %d %d %d %s\n",traverseNode->handleId,traverseNode->website,traverseNode->avg,traverseNode->min,traverseNode->max,status);
				strcat(result,buffer);
				
			}	
			
			traverseNode=traverseNode->next;
		}

		if(strlen(result)==0)
			strcpy(result,"No request in queue\n");

		send(newsockfd,result,strlen(result),0);		
		if(flag == 0)
		{
			strcpy(buffer,"Handle Id not present in database");
			send(newsockfd,buffer,strlen(buffer),0);
		}
		
	}
	
}

/* Calculate Timing statistics for pingsites */

void calculateStats(struct List* node){
	
	char *site = node->website;
	int i=0;
	int time=0,min=INT_MAX,max=0,avg=0,total=0;
	char stats[500];

	struct timeval start,stop;	
	for(i=0;i<10;i++)
	{
		
		int port=80;
		int clientSocket;
		struct sockaddr_in serverAddr;
		socklen_t addr_size;
		struct hostent *host;

		gettimeofday(&start,NULL);
	
		clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  	
		if(clientSocket == -1)
			printf("Socket not created for website\n");
		if((host = gethostbyname(site)) == NULL)
		{
        	        perror("Host to IP Resolution Error\n");
        	        return;     
		}	

		memcpy(&serverAddr.sin_addr,host->h_addr_list[0],host->h_length);
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
	
		memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  
	
        	addr_size = sizeof(serverAddr);
	
  	
  	      	if(connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) < 0)
		{
  	              perror("Connection error\n");
  	              return;     
		}
		gettimeofday(&stop,NULL);
	
		time = ((stop.tv_sec*1000000 + stop.tv_usec)-(start.tv_sec*1000000 + start.tv_usec))/1000;
		if(time<min) min = time;
		if(time>max) max = time;
		total+=time;
		close(clientSocket);	
	}
	
	avg = total/10;	
	node->min = min;						//update minimum value in queue
	node->max = max;						//update maximum value in queue
	node->avg = avg;						//update average value in queue
				
}

/* Worker threads to handle websites in queue */

void handleFunction()
{
				
	pthread_mutex_lock(&mutx1);
	pthread_t self;
	self=pthread_self();
	
	while(node!=NULL)
	{								
		if(node->status == 0)
		{
			node->status = 1;
			calculateStats(node);
			node->status = 2;
			break;
		}else if(node->next!=NULL) node=node->next;
	}
	availableWorkerThreads-=1;
	pthread_mutex_unlock(&mutx1);		

}

/* Dispatcher function dispatching jobs to worker threads*/

void dispatcher()
{
	while((current!=NULL))
	{
		if (availableWorkerThreads>=nThreads)
			continue;

		if(current->status != IN_QUEUE)
		{
			current=current->next;
			continue;
		}
		pthread_cond_signal(&conditionVar);				//site available in queue and so signal thread
		availableWorkerThreads+=1;		
		current=current->next;
	}
}

/* Worker Threads in blocking state */

void* waitBlock(void* unused)
{
	
	while(1) 
	{
		pthread_mutex_lock(&mutx);					
		pthread_cond_wait(&conditionVar,&mutx);				//wait for signal from dispatcher
		pthread_mutex_unlock(&mutx);
		handleFunction();
	}					
		
}


/* Build Queue of each sites */

void buildQueue(int id,char* site)
{
	struct List *newNode = (struct List *)malloc(sizeof(struct List));
	
	newNode->handleId = id;
	strcpy(newNode->website,site);	

	newNode->status = 0;
	newNode->next = NULL;
	
	if(head == NULL)
	{
		head = newNode;
		prev = head;
		node = head;
	}else
	{
		prev->next = newNode;
		prev = newNode;
	}
	current = head;
}

/* Function for pingSites command */

void pingSites(int newsockfd,char *input)
{

	
	char *sites[100];
	char *token;
	int i=0;
	char del[2]=",\n";
	token = strtok(input,del);
	
	while(token!=NULL)
	{
		sites[i] = (char *)malloc(sizeof(char)*1000);
		memcpy(sites[i],token,strlen(token));
		token = strtok(NULL,del);
		i++;
	
	}
	
	
	pthread_mutex_lock(&handleLock);
	int cnt = i;							//count of websites for each request
	handle++;							//increment static handle for unique id
	char handleClient[1000];
	struct List *temp = head;	
	snprintf(handleClient,sizeof handleClient,"%d",handle);
	
	for(i=0;i<cnt;i++)
	{
		buildQueue(handle,sites[i]);				//build Queue for each website
	}
	while(temp!=NULL)
	{
		temp=temp->next;
	}
	
	send(newsockfd,handleClient,sizeof handleClient,0);		//send handle to the client
	pthread_mutex_unlock(&handleLock);

	dispatcher();

}

/* Request Handler for clients */

void requestHandler(int newsockfd)
{

	char *sites[100];
	char *token = NULL;
	char command[6000];
	//char *input[10];
	int readSize;
	int i = 0;
	
	while((readSize = recv(newsockfd,command,6000,0))>0)
	{
		char *input[3];		
		token = NULL;
		i=0;
		char del[4]=" \n";
	
		
		token = strtok(command,del);
		while(token!=NULL)
		{
			input[i] = (char *)malloc(sizeof(char)*1000);
			memcpy(input[i],token,strlen(token));
			token = strtok(NULL,del);
			i++;
		}

		if(strcasecmp(input[0],"pingSites")==0)				//pingsites request
		{
			pingSites(newsockfd,input[1]);

		}else if(strcasecmp(input[0],"showHandleStatus")==0)		//showHandleStatus request
		{
			if(input[1]!=NULL){
				handleStatus(newsockfd,input[1]);
			} else {
				handleStatus(newsockfd,"all");
			}
		}else if(strcasecmp(input[0],"showHandles")==0)			//showHandles request
		{
			showHandle(newsockfd);
		}
		memset(command,'\0',sizeof(command));	
		memset(input,0,sizeof(input));
	}
}
	
/* Main Function */

int main(int argc, char* argv[])
{

	int sockfd, newsockfd, portno;
	socklen_t clilen;
	int n;
	struct sockaddr_in serv_addr, cli_addr;

	pthread_t thread1;
	pthread_attr_t attr1;
	pthread_attr_init(&attr1);
	pthread_attr_setdetachstate(&attr1,PTHREAD_CREATE_JOINABLE);    

	if (argc != 2) 
	{
         	printf("ERROR, You have entered wrong Input.Please try again...\n");
         	exit(-1);
	}

	struct List* node=head;
	
	for(i=0;i<nThreads;i++)
	{
		int ret = pthread_create(&workerThread[i],NULL,waitBlock,NULL);
		
		if(ret){

			perror("worker thread creation fail\n");
			exit(1);
		}
	}


	/********Socket*********/

     	sockfd = socket(AF_INET, SOCK_STREAM, 0);
     	if (sockfd < 0)
	{     		
		printf("ERROR in opening socket");
		exit(-1);
	}
     	bzero((char *) &serv_addr, sizeof(serv_addr));
     	portno = atoi(argv[1]);
     	serv_addr.sin_family = AF_INET;
     	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     	serv_addr.sin_port = htons(portno);

	/********Binding*********/

     	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        {
		printf("ERROR in binding Socket");
		exit(-1);
	}
	/********Listen*********/

     	if((listen(sockfd,MAX_CLIENTS))==ERROR)
     	{
             perror("Error in Listen:");
             exit(-1);

     	}

     	clilen = sizeof(cli_addr);
     	printf("Server is Listening...\n");

	while(1)
	{ 

    		/********Accept*********/

    		if((newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen))==ERROR)
		{
        	        perror("Error in Accept:");
        	        exit(-1);
        	}
 
		if(pthread_create(&thread1,&attr1,(void *(*)(void *))requestHandler,(int*)newsockfd)<0)
		{
				perror("pthread_create error\n");
		} 
	
	}

     close(newsockfd);
     close(sockfd);

     return 0;
}
