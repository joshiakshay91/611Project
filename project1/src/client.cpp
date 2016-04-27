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


/*struct GameBoard
{
	int rows;
	int coloumns;
	int array[5];
	unsigned char mapya[0];
	int DaemonID;
};*/
GameBoard *GoldBoardR;
sem_t *mysemaphore;
unsigned char* clientLocalCopy;
int sockfd;
int areaC;
void Clientother_interrupt(int SigNo)
{
	if(SigNo==SIGUSR1)
	{
		unsigned char* shared_memory_map=GoldBoardR->mapya;

		vector< pair<short,unsigned char> > pvec;
		for(short i=0; i<areaC; ++i)
		{
			if(shared_memory_map[i]!=clientLocalCopy[i])
			{
				pair<short,unsigned char> aPair;
				aPair.first=i;
				aPair.second=shared_memory_map[i];
				pvec.push_back(aPair);
				clientLocalCopy[i]=shared_memory_map[i];
			}

		}
		//here iterate through pvec, writing out to socket

		//testing we will print it:
		unsigned char numSend=0;
		for(short i=0; i<(short(pvec.size())); ++i)
		{
			WRITE(sockfd,&numSend,sizeof(unsigned char));//send 0
			WRITE(sockfd,&(pvec[i].first),sizeof(short));//send the offset
			WRITE(sockfd,&(pvec[i].second),sizeof(char));//send the bit
		}
}
if(SigNo==SIGHUP)
{
	bool tookLast=false;
 for (int n=0;n<5;n++)
	 {
		if((GoldBoardR->array[n]!=0) &&(GoldBoardR->array[n]!=GoldBoardR->DaemonID))
		 {tookLast=true;}
	 }
 if(tookLast==false)
 {
 sem_close(mysemaphore);
 shm_unlink("/APJMEMORY");
 sem_unlink("APJgoldchase");
 exit(0);
 }
 unsigned char SockPlayer;
 SockPlayer=G_SOCKPLR;
 for(int i=0; i<5; ++i)
 {
		if(GoldBoardR->array[i]!=0)
		{
			switch(i)
			{
			case 0:
				SockPlayer|=G_PLR0;
				break;
			case 1:
				SockPlayer|=G_PLR1;
				break;
			case 2:
				SockPlayer|=G_PLR2;
				break;
			case 3:
				SockPlayer|=G_PLR3;
				break;
			case 4:
				SockPlayer|=G_PLR4;
				break;
			}
		}
	}
		if(sockfd!=0)	WRITE(sockfd,&SockPlayer,sizeof(unsigned char));//send sock
}

}


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
	//int sockfd; //file descriptor for the socket
	int status; //for error checking

	//change this # between 2000-65k before using
	const char* portno="4500";

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints)); //zero out everything in structure
	hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
	hints.ai_socktype=SOCK_STREAM; // TCP stream sockets

	struct addrinfo *servinfo;
	//instead of "localhost", it could by any domain name
	if((status=getaddrinfo("172.16.57.132", portno, &hints, &servinfo))==-1)
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
//       sem_t *mysemaphore;

       mysemaphore= sem_open("/APJgoldchase", O_CREAT|O_EXCL,
		       S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);
       //if(mysemaphore!=SEM_FAILED) //you are the first palyer
       //{
			 areaC=playerRows*playerCol;
       sem_wait(mysemaphore);
       int fd = shm_open("/APJMEMORY", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
       if(fd==-1)
       {
	       perror("Shared memory creation failed");
	       exit(1);
       }
       ftruncate(fd,sizeof(GameBoard)+(mapSize));
       GoldBoardR=(GameBoard*) mmap(NULL,
		       playerRows*playerCol+sizeof(GameBoard),
		       PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

       unsigned char *dataMap=GoldBoardR->mapya;
       clientLocalCopy=(unsigned char*)malloc(sizeof (char)*playerCol*playerRows);
       GoldBoardR->rows=playerRows;
       GoldBoardR->coloumns=playerCol;
       int SockPlrz;
       READ(sockfd,&SockPlrz,sizeof(int));
       GoldBoardR->DaemonID=getpid();
       int DamID=getpid();
       int OutByte=SockPlrz;

       for(int z=0;z<5;z++)
 			{
 				if(z==0)
        {
          OutByte&=G_PLR0;
          if(OutByte==G_PLR0) GoldBoardR->array[z]=DamID;
        }
        else if(z==1)
        {
          OutByte&=G_PLR1;
          if(OutByte==G_PLR1) GoldBoardR->array[z]=DamID;
        }
        else if(z==2)
        {
          OutByte&=G_PLR2;
          if(OutByte==G_PLR2) GoldBoardR->array[z]=DamID;
        }
        else if(z==3)
        {
          OutByte&=G_PLR3;
          if(OutByte==G_PLR3) GoldBoardR->array[z]=DamID;
        }
        else if(z==4)
        {
          OutByte&=G_PLR4;
          if(OutByte==G_PLR4) GoldBoardR->array[z]=DamID;
        }
        OutByte=SockPlrz;
 			}

       //GoldBoardR->array[0]=1;////////////////////////////////////////////////////
       //GoldBoardR->DaemonID=getpid();
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
	//	int readByteN;
		unsigned char CondiX=-1;
		short positionC;
		unsigned char changed;
		GoldBoardR->DaemonID=getpid();
    while(1){

			READ(sockfd,&CondiX,sizeof(unsigned char));
			if(CondiX==0)
			{
				CondiX=-1;
				READ(sockfd,&positionC,sizeof(short));
				READ(sockfd,&changed,sizeof(char));
				clientLocalCopy[positionC]=changed;
				sem_wait(mysemaphore);
				GoldBoardR->mapya[positionC]=changed;
				sem_post(mysemaphore);
				for(int i=0;i<5;i++)
				{
					if(GoldBoardR->array[i]!=0)	kill(GoldBoardR->array[i],SIGUSR1);
				}
			}
    }
		close(sockfd);

}
