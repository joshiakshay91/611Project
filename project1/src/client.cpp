/*
Author: Akshay Joshi
Date: 10 May 2016
 */
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

mqd_t readqueue_fdR[5];//file descriptor
mqd_t writequeue_fdR;//file descriptor
string mq_nameR="/APJqueue";

void QueueSetupR(int player);
void QueueCleanerR(int player);



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
		unsigned char SockPlayer=G_SOCKPLR;
		unsigned char player_bit[5]={G_PLR0, G_PLR1, G_PLR2, G_PLR3, G_PLR4};
		for(int i=0; i<5; ++i) //loop through the player bits
		{
			if( GoldBoardR->array[i]!=0)	SockPlayer|=player_bit[i];

		}
		if(sockfd!=0)	WRITE(sockfd,&SockPlayer,sizeof(unsigned char));//send sock
		bool tookLast=false;
		for (int n=0;n<5;n++)
		{
			if((GoldBoardR->array[n]!=0) )
			{tookLast=true;}
		}
		if(tookLast==false)
		{
			unsigned char SockPlayer=G_SOCKPLR;
			if(sockfd!=0)	WRITE(sockfd,&SockPlayer,sizeof(unsigned char));
		}
	}
}


void client_function(string addrto)
{
	int pipefd;
	const char* pipefifo="/tmp/waiter";
	mkfifo(pipefifo,0666);
	int rPid=fork();
	if(rPid<0)
	{
		cerr<<"Creation problem"<<endl;
	}
	if(rPid>0)
	{
		//			close(pipefd[1]); //close write, parent only needs read
		int val=99;
		pipefd = open(pipefifo, O_RDONLY);
		while(1){
			read(pipefd, &val, sizeof(val));
			if(val==0);
			{
				wait(NULL);//zombie
				return;	//I'm the parent, leave the function
			}
		}
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
	//client socket initialization
	int status; //for error checking
	//change this # between 2000-65k before using
	const char* portno="4525";

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints)); //zero out everything in structure
	hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
	hints.ai_socktype=SOCK_STREAM; // TCP stream sockets

	struct addrinfo *servinfo;
	//instead of "localhost", it could by any domain name
	if((status=getaddrinfo(addrto.c_str(), portno, &hints, &servinfo))==-1)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	}
	sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

Lagain:if((status=connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
       {
	       goto Lagain;
	       //exit(1);
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
      
       mysemaphore= sem_open("/APJgoldchase", O_CREAT|O_EXCL,
		       S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);

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
       clientLocalCopy=(unsigned char*)malloc(sizeof (unsigned char)*playerCol*playerRows);
       GoldBoardR->rows=playerRows;
       GoldBoardR->coloumns=playerCol;
       int DamID=getpid();
       GoldBoardR->DaemonID=getpid();
       for(int i=0;i<mapSize;i++)
       {
	       READ(sockfd,&tempData,sizeof(char));
	       dataMap[i]=tempData;//shm
	       clientLocalCopy[i]=tempData;//loc copy
       }
       sem_post(mysemaphore);
       struct sigaction OtherAction;//handle the signals
       OtherAction.sa_handler=Clientother_interrupt;
       sigemptyset(&OtherAction.sa_mask);
       OtherAction.sa_flags=0;
       OtherAction.sa_restorer=NULL;
       sigaction(SIGINT, &OtherAction, NULL);// sig usr1 - map refresh
       sigaction(SIGHUP, &OtherAction, NULL);// mqueue
       sigaction(SIGTERM, &OtherAction, NULL);
       sigaction(SIGUSR1, &OtherAction, NULL);

       int vala=0;
       pipefd = open(pipefifo, O_WRONLY);
       write(pipefd, &vala, sizeof(vala));
       close(pipefd);
       unsigned char CondiX=-1;
       short positionC;
       unsigned char changed;
       GoldBoardR->DaemonID=getpid();
       DamID=getpid();
       while(1)
			 {
	       GoldBoardR->DaemonID=getpid();
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
	       //			Clientother_interrupt(SIGHUP);
	       else if(CondiX & G_SOCKPLR)
	       {
		       unsigned char player_bit[5]={G_PLR0, G_PLR1, G_PLR2, G_PLR3, G_PLR4};
		       for(int i=0; i<5; ++i) //loop through the player bits
		       {
			       // If player bit is on and shared memory ID is zero,
			       // a player (from other computer) has joined:
			       if(CondiX & player_bit[i] && GoldBoardR->array[i]==0)
			       {
				       GoldBoardR->array[i]=DamID;
				       QueueSetupR(player_bit[i]);
			       }
			       //If player bit is off and shared memory ID is not zero,
			       //remote player has quit:
			       else if(!(CondiX & player_bit[i]) && GoldBoardR->array[i]!=0)
			       {
				       GoldBoardR->array[i]=0;
				       QueueCleanerR(player_bit[i]);
			       }
		       }
		       if(CondiX==G_SOCKPLR)
		       {
			       unsigned char SockPlayer=G_SOCKPLR;
			       WRITE(sockfd,&SockPlayer,sizeof(unsigned char));
			       close(sockfd);
			       sem_close(mysemaphore);
			       shm_unlink("/APJMEMORY");
			       sem_unlink("APJgoldchase");
			       exit(0);
		       }
		       //no players are left in the game.  Close and unlink the shared memory.
		       //Close and unlink the semaphore.  Then exit the program.
	       }
	       else if(CondiX & G_SOCKMSG)
	       {
		       char buffer[121];
		       memset(buffer, 0, 121);
		       READ(sockfd, buffer,121);
		       string putputya(buffer);
		       unsigned char player_bit[5]={G_PLR0, G_PLR1, G_PLR2, G_PLR3, G_PLR4};
		       for(int i=0;i<5;++i)
		       {
			       if(CondiX & player_bit[i])
			       {
				       CondiX&~player_bit[i];
				       string reciver;
				       if(player_bit[i] == G_PLR0)	reciver="/APJplayer0_mq";
				       else if(player_bit[i] == G_PLR1)	reciver="/APJplayer1_mq";
				       else if(player_bit[i] == G_PLR2)	reciver="/APJplayer2_mq";
				       else if(player_bit[i] == G_PLR3)	reciver="/APJplayer3_mq";
				       else if(player_bit[i] == G_PLR4)	reciver="/APJplayer4_mq";
				       const char *ptr=putputya.c_str();
				       if((writequeue_fdR=mq_open(reciver.c_str(), O_WRONLY|O_NONBLOCK))==-1)
				       {
					       perror("msgq open error");
					       //	exit(1);
				       }
				       char message_text[121];
				       memset(message_text, 0, 121);
				       strncpy(message_text, ptr, 120);
				       if(mq_send(writequeue_fdR, message_text, strlen(message_text), 0)==-1)
				       {
					       perror("msgq send error");
					       //	exit(1);
				       }
				       mq_close(writequeue_fdR);
			       }
		       }
	       }
       }
       close(sockfd);
}

//Read Message means I send it to the opposite side
void ReadMessageR(int)
{
	struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	for(int mend=0;mend<5;++mend)
	{
		int ret_mq=mq_notify(readqueue_fdR[mend], &mq_notification_event);
		if(ret_mq==0)
		{
			int err;
			char msg[121];
			memset(msg, 0, 121);
			if((err=mq_receive(readqueue_fdR[mend], msg, 120, NULL))!=-1)
			{
				unsigned char player_bit[5]={G_PLR0, G_PLR1, G_PLR2, G_PLR3, G_PLR4};
				unsigned char SendMo=G_SOCKMSG;
				//update
				SendMo|=player_bit[mend];
				WRITE(sockfd,&SendMo,sizeof(unsigned char));
				WRITE(sockfd,&msg,strlen(msg));
				//		pointer->postNotice(msg);
				memset(msg, 0, 121);
			}
			if(errno!=EAGAIN)
			{
				if(errno==EBADF)
				{
					perror("bad file descriptor");
				}
				if(errno==EINTR)
				{
					perror("Signal interference");
				}
				perror("mq receive");
				//	exit(1);
			}
		}
	}
}

void QueueSetupR(int player)
{
	int FdNum;
	if(player == G_PLR0)
	{
		mq_nameR="/APJplayer0_mq";
		FdNum=0;
	}
	else if(player == G_PLR1)
	{
		mq_nameR="/APJplayer1_mq";
		FdNum=1;
	}
	else if(player == G_PLR2)
	{
		mq_nameR="/APJplayer2_mq";
		FdNum=2;
	}
	else if(player == G_PLR3)
	{
		mq_nameR="/APJplayer3_mq";
		FdNum=3;
	}
	else if(player == G_PLR4)
	{
		mq_nameR="/APJplayer4_mq";
		FdNum=4;
	}

	struct sigaction action_to_take;
	action_to_take.sa_handler=ReadMessageR;
	sigemptyset(&action_to_take.sa_mask);
	action_to_take.sa_flags=0;
	sigaction(SIGUSR2, &action_to_take, NULL);
	struct mq_attr mq_attributes;
	mq_attributes.mq_flags=0;
	mq_attributes.mq_maxmsg=10;
	mq_attributes.mq_msgsize=120;
	if((readqueue_fdR[FdNum]=mq_open(mq_nameR.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
					S_IRUSR|S_IWUSR, &mq_attributes))==-1)
	{
		perror("mq_open");
		exit(1);
	}
	struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	mq_notify(readqueue_fdR[FdNum], &mq_notification_event);
}



void QueueCleanerR(int player)
{

	int FdNum;
	if(player == G_PLR0)
	{
		mq_nameR="/APJplayer0_mq";
		FdNum=0;
	}
	else if(player == G_PLR1)
	{
		mq_nameR="/APJplayer1_mq";
		FdNum=1;
	}
	else if(player == G_PLR2)
	{
		mq_nameR="/APJplayer2_mq";
		FdNum=2;
	}
	else if(player == G_PLR3)
	{
		mq_nameR="/APJplayer3_mq";
		FdNum=3;
	}
	else if(player == G_PLR4)
	{
		mq_nameR="/APJplayer4_mq";
		FdNum=4;
	}

	mq_close(readqueue_fdR[FdNum]);
	if(mq_unlink(mq_nameR.c_str())==-1)
	{
		if(errno==EACCES)	perror("Access eror");
		else if(errno==ENAMETOOLONG)	perror("Name to long");
		else if(ENOENT==errno)	perror("Queue with no name");
		//exit(1);
	}
}
