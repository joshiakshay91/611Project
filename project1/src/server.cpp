//Socket server
#ifndef SERVER_CPP
#define SERVER_CPP
#include "goldchase.h"
#include "Map.h"
#include <sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h> //for read/write
#include<string> //for memset
#include<stdio.h> //for fprintf, stderr, etc.
#include<stdlib.h> //for exit
#include "fancyRW.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <random>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include<signal.h>
#include <sys/types.h>
#include <mqueue.h>
#include <sstream>
bool Refresh;
using namespace std;
int new_sockfd;
sem_t *mysemaphore; //semaphore

struct GameBoard
{
	int rows;
	int coloumns;
	int array[5];
	int DaemonID;
	unsigned char mapya[0];
};

void Sother_interrupt(int SigNo)//handling interr
{
//	cerr<<"Fired up"<<endl;
  if(SigNo==2 ||SigNo ==-2)
  {
		close(new_sockfd);
    exit (0);
  }
  else
  {
    int fd;
//    char * myfifo = "/tmp/myfifo";

    /* create the FIFO (named pipe) */
    //mkfifo(myfifo, 0666);

    /* write "Hi" to the FIFO */
  //  fd = open(myfifo, O_WRONLY);
    if(SigNo==15||SigNo==-15)
    {
  //    WRITE(fd, "Got SIGTERM\n", sizeof("Got SIGTERM\n"));
      //close(fd);
    }
    else if(SigNo==1||SigNo==-1)
    {
			sem_close(mysemaphore);
			shm_unlink("/APJMEMORY");
			sem_unlink("APJgoldchase");
			exit(0);
  //    WRITE(fd, "Got SIGHUP\n", sizeof("Got SIGHUP\n"));
      //close(fd);
    }
    else if(SigNo==10||SigNo==-10)
    {

			Refresh=true;
		//	WRITE(fd, "Got SIGUSR1 refresh\n", sizeof("Got SIGUSR1 refresh\n"));
    //  close(fd);
    }

    /* remove the FIFO */
  //  unlink(myfifo);
  }
}
void ServerDaemon_function()
{
// printf("Daemon Started");

int rPid=fork();
if(rPid<0)
{
	cerr<<"Creation problem"<<endl;
}
if(rPid>0)
{
	wait(NULL);
	return;	//I'm the parent, leave the function
}
if(fork()>0)
{
    exit(0);//game child killed/exited
}
if(setsid()==-1)//game grand child got divorced from main game
	{
		perror("Couldnt create demon");
    exit(1);
	}
  for(int i=0; i< sysconf(_SC_OPEN_MAX); ++i)
	{
		(close(i)<0);	//perror("Close error");
	}
  open("/dev/null", O_RDWR); //fd 0 :: stdin
  open("/dev/null", O_RDWR); //fd 1 :: stdout
  open("/dev/null", O_RDWR); //fd 2::stderr
  umask(0);
	chdir("/");
  int fd;
  fd = shm_open("/APJMEMORY", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd<0)
		{
			perror("Shm not open");
		}
  int playerRows;
  int playerCol;
  read(fd,&playerRows,sizeof(int));
  read(fd,&playerCol,sizeof(int));
  GameBoard* GoldBoard= (GameBoard*)mmap(NULL,
      playerRows*playerCol+sizeof(GameBoard),
      PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	unsigned char* myLocalCopy;
	unsigned char* orig;
	orig=GoldBoard->mapya;
	int area=playerRows*playerCol;
	myLocalCopy=(unsigned char*)malloc(sizeof (char)*playerCol*playerRows);

	GoldBoard->DaemonID=getpid();
	for(int i=0;i<area;i++)
	{
		myLocalCopy[i]=orig[i];
	}

  //demon created

  //server.cpp
//  printf("In daemon");
  struct sigaction OtherAction;//handle the signals
	OtherAction.sa_handler=Sother_interrupt;
	sigemptyset(&OtherAction.sa_mask);
	OtherAction.sa_flags=0;
	OtherAction.sa_restorer=NULL;
	sigaction(SIGINT, &OtherAction, NULL);// sig usr1 - map refresh
	sigaction(SIGHUP, &OtherAction, NULL);// mqueue
	sigaction(SIGTERM, &OtherAction, NULL);
  sigaction(SIGUSR1, &OtherAction, NULL);
	int sockfd; //file descriptor for the socket
	int status; //for error checking


	//change this # between 2000-65k before using
	const char* portno="4500";
	struct addrinfo hints;
//	memset(&hints, 0, sizeof(hints)); //zero out everything in structure
	hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
	hints.ai_socktype=SOCK_STREAM; // TCP stream sockets
	hints.ai_flags=AI_PASSIVE; //file in the IP of the server for me

	struct addrinfo *servinfo;
	if((status=getaddrinfo(NULL, portno, &hints, &servinfo))==-1)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	//	exit(1);
	}
	sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	/*avoid "Address already in use" error*/
	int yes=1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1)
	{
		perror("setsockopt");
//		exit(1);
	}

	//We need to "bind" the socket to the port number so that the kernel
	//can match an incoming packet on a port to the proper process
	if((status=bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
	{
		perror("bind");
//		exit(1);
	}
	//when done, release dynamically allocated memory
	freeaddrinfo(servinfo);

	if(listen(sockfd,1)==-1)
	{
		perror("listen");
	//	exit(1);
	}

//	printf("Blocking, waiting for client to connect\n");


struct sockaddr_in client_addr;
socklen_t clientSize=sizeof(client_addr);
here: if((new_sockfd=accept(sockfd, (struct sockaddr*) &client_addr, &clientSize))==-1)
	{
		perror("accept");
		goto here;
//	exit(1);
	}
	int InitNum=0;
	WRITE(new_sockfd,&InitNum,sizeof(int));
	WRITE(new_sockfd,&playerRows,sizeof(int));
  WRITE(new_sockfd,&playerCol,sizeof(int));
	unsigned char *senderCopy=myLocalCopy;
	for(int J=0;J<(playerCol*playerRows);++J)
	{
		WRITE(new_sockfd,&senderCopy[J],sizeof(senderCopy[J]));
	}
	string message;
	//char message[1000];
	while(1){

	//read & write to the socket
//	char buffer[100];
//	memset(buffer,0,100);
	int n;
	if(Refresh)
	{
		Refresh=false;
	//	message="refresh from server";//
		int SendSize=message.size();
//		WRITE(new_sockfd, message.c_str(), SendSize);
		unsigned char* shared_memory_map=GoldBoard->mapya;

		vector< pair<short,unsigned char> > pvec;
  for(short i=0; i<area; ++i)
  {
    if(shared_memory_map[i]!=myLocalCopy[i])
    {
      pair<short,unsigned char> aPair;
      aPair.first=i;
      aPair.second=shared_memory_map[i];
      pvec.push_back(aPair);
      myLocalCopy[i]=shared_memory_map[i];
    }

  }
  //here iterate through pvec, writing out to socket

  //testing we will print it:
	int numSend=0;
  for(short i=0; i<pvec.size(); ++i)
  {
		WRITE(new_sockfd,&numSend,sizeof(int));//send 0
		WRITE(new_sockfd,&(pvec[i].first),sizeof(short));//send the offset
		WRITE(new_sockfd,&(pvec[i].second),sizeof(char));//send the bit
  }

	}
//	printf("say something to client\n");
//	scanf ("%s",message);

}
	close(new_sockfd);
//  return(0);
}















void ClientDaemon_function()
{
	char* pipefifo="/dev/tmp/waiter"
	int pipefd;
	//pipe(pipefd);
	mkfifo(pipefifo, 0666);
	  pipefd = open(pipefifo, O_WRONLY);
  int rPid=fork();
  if(rPid<0)
  {
  	cerr<<"Creation problem"<<endl;
  }
  if(rPid>0)
  {

//			close(pipefd[1]); //close write, parent only needs read
	    int val=99;
			while(1){
	    read(pipefd, &val, sizeof(val));
	    if(val==0)
		}
		{
  	wait(NULL);
  	return;	//I'm the parent, leave the function
	}else{ exit (0);}
  }
  if(fork()>0)
  {
      exit(0);//game child killed/exited
  }
  if(setsid()==-1)//game grand child got divorced from main game
  	{
  		perror("Couldnt create demon");
      exit(1);
  	}
    for(int i=0; i< sysconf(_SC_OPEN_MAX); ++i)
  	{
  		(close(i)<0);	//perror("Close error");
  	}
    open("/dev/null", O_RDWR); //fd 0 :: stdin
    open("/dev/null", O_RDWR); //fd 1 :: stdout
    open("/dev/null", O_RDWR); //fd 2::stderr
    umask(0);
  	chdir("/");
		int sockfd; //file descriptor for the socket
		int status; //for error checking

		//change this # between 2000-65k before using
		const char* portno="4500";

		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints)); //zero out everything in structure
		hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
		hints.ai_socktype=SOCK_STREAM; // TCP stream sockets

		struct addrinfo *servinfo;
		//instead of "localhost", it could by any domain name
		if((status=getaddrinfo("172.16.57.134", portno, &hints, &servinfo))==-1)
		{
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	//		exit(1);
		}
		sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

Lagain:if((status=connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
			{
				goto Lagain;
//			perror("connect");
//			exit(1);
		}
		//release the information allocated by getaddrinfo()
		freeaddrinfo(servinfo);
		int playerRows=0;
		int playerCol=0;
		int beginner=0;
		READ(sockfd,&beginner,sizeof(int));
		READ(sockfd,&playerRows,sizeof(int));
		READ(sockfd,&playerCol,sizeof(int));
		int mapSize=playerRows*playerCol;
		unsigned char tempData;
		GameBoard *GoldBoard;
		mysemaphore= sem_open("/APJgoldchase", O_CREAT|O_EXCL,
				S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);
		//if(mysemaphore!=SEM_FAILED) //you are the first palyer
		//{
			sem_wait(mysemaphore);
			int fd = shm_open("/APJMEMORY", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
			if(fd==-1)
			{
				perror("Shared memory creation failed");
				exit(1);
			}
			ftruncate(fd,sizeof(GameBoard)+(mapSize));
			GoldBoard=(GameBoard*) mmap(NULL,
					playerRows*playerCol+sizeof(GameBoard),
					PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

		unsigned char *dataMap=GoldBoard->mapya;
		unsigned char *clientLocalCopy=(unsigned char*)malloc(sizeof (char)*playerCol*playerRows);
		GoldBoard->rows=playerRows;
		GoldBoard->coloumns=playerCol;
		GoldBoard->array[0]=1;
		for(int i=0;i<mapSize;i++)
		{
			READ(sockfd,&tempData,sizeof(char));
			dataMap[i]=tempData;
			clientLocalCopy[i]=tempData;
		}
		sem_post(mysemaphore);
//	}
	int vala=0;
	write(pipefd, &vala, sizeof(vala));
	int readByteN;
	int CondiX;
	short positionC;
	unsigned char changed;
	while(1){
		readByteN=READ(sockfd,&CondiX,sizeof(int));
		if(CondiX==0)
		{
			READ(sockfd,&positionC,sizeof(short));
			READ(sockfd,&changed,sizeof(char));
			dataMap[positionC]=changed;
			clientLocalCopy[positionC]=changed;
			for(int i=0;i<5;i++)
			{
				if(GoldBoard->array[i]!=0)	kill(GoldBoard->array[i],SIGUSR1);
			}
		}
	}
}

#endif
