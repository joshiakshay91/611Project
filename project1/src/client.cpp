#include "goldchase.h"
#include "Map.h"

#include <sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h> //for read/write
#include<string> //for memset
#include<stdio.h> //for fprintf, stderr, etc.
#include<stdlib.h> //for exit


#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <random>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include<signal.h>
#include <sys/types.h>
#include <mqueue.h>
using namespace std;
#include "fancyRW.h"


struct GameBoard
{
	int rows;
	int coloumns;
	int array[5];
	unsigned char mapya[0];
	int DaemonID;
};



void Clientother_interrupt(int){}



void client_function()
{
  int rPid=fork();
	if(rPid<0)
	{
		cerr<<"Creation problem"<<endl;
	}
	if(rPid>0)
	{
/*
		//			close(pipefd[1]); //close write, parent only needs read
		int val=99;
		while(1){
			read(pipefd, &val, sizeof(val));
			if(val==0);
			{
				wait(NULL);//zombie
				return;	//I'm the parent, leave the function
			}
		}*/
    sleep(3);
    return;
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
       sem_t *mysemaphore;
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
       int SockPlrz;
       READ(sockfd,&SockPlrz,sizeof(int));
       GoldBoard->DaemonID=getpid();
       int DamID=getpid();
       int OutByte=SockPlrz;

       for(int z=0;z<5;z++)
 			{
 				if(z==0)
        {
          OutByte&=G_PLR0;
          if(OutByte==G_PLR0) GoldBoard->array[z]=DamID;
        }
        else if(z==1)
        {
          OutByte&=G_PLR1;
          if(OutByte==G_PLR1) GoldBoard->array[z]=DamID;
        }
        else if(z==2)
        {
          OutByte&=G_PLR2;
          if(OutByte==G_PLR2) GoldBoard->array[z]=DamID;
        }
        else if(z==3)
        {
          OutByte&=G_PLR3;
          if(OutByte==G_PLR3) GoldBoard->array[z]=DamID;
        }
        else if(z==4)
        {
          OutByte&=G_PLR4;
          if(OutByte==G_PLR4) GoldBoard->array[z]=DamID;
        }
        OutByte=SockPlrz;
 			}

       //GoldBoard->array[0]=1;////////////////////////////////////////////////////
       //GoldBoard->DaemonID=getpid();
       for(int i=0;i<mapSize;i++)
       {
	       READ(sockfd,&tempData,sizeof(char));
	       dataMap[i]=tempData;//shm
	       clientLocalCopy[i]=tempData;//loc copy
       }
       sem_post(mysemaphore);
       //	}
       //
       struct sigaction OtherAction;//handle the signals
       OtherAction.sa_handler=Clientother_interrupt;
       sigemptyset(&OtherAction.sa_mask);
       OtherAction.sa_flags=0;
       OtherAction.sa_restorer=NULL;
       sigaction(SIGINT, &OtherAction, NULL);// sig usr1 - map refresh
       sigaction(SIGHUP, &OtherAction, NULL);// mqueue
       sigaction(SIGTERM, &OtherAction, NULL);
       sigaction(SIGUSR1, &OtherAction, NULL);
       ////////////////
    //   int vala=0;
    //   write(pipefd, &vala, sizeof(vala));
    while(1){
      sleep(1);
    }

}
