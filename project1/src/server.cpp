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

GameBoard* GoldBoard;
unsigned char* myLocalCopy;
int area;
int new_sockfd=0;
sem_t *mysemaphore1;

mqd_t readqueue_fdS[5];//file descriptor
mqd_t writequeue_fdS;//file descriptor
string mq_nameS="/APJqueue";

void QueueSetupS(int player);
void QueueCleanerS(int player);

void Sother_interrupt(int SigNo)
{
	if(SigNo==SIGUSR1)
	{
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
		unsigned char numSend=0;
		for(short i=0; i<(short(pvec.size())); ++i)
		{
			WRITE(new_sockfd,&numSend,sizeof(unsigned char));//send 0
			WRITE(new_sockfd,&(pvec[i].first),sizeof(short));//send the offset
			WRITE(new_sockfd,&(pvec[i].second),sizeof(char));//send the bit
		}
	}
	if(SigNo==SIGHUP)
	{
		unsigned char SockPlayer=G_SOCKPLR;
		unsigned char player_bit[5]={G_PLR0, G_PLR1, G_PLR2, G_PLR3, G_PLR4};
		for(int i=0; i<5; ++i) //loop through the player bits
		{
			if( GoldBoard->array[i]!=0)	SockPlayer|=player_bit[i];
		}
		if(new_sockfd!=0)	WRITE(new_sockfd,&SockPlayer,sizeof(unsigned char));//send sock
		bool tookLast=false;
		for (int n=0;n<5;n++)
		{
			if((GoldBoard->array[n]!=0)) //&& (GoldBoard->array[n]!=GoldBoard->DaemonID))
			{tookLast=true;}
		}
		if(tookLast==false)
		{
			SockPlayer=G_SOCKPLR;
			if(new_sockfd!=0)	WRITE(new_sockfd,&SockPlayer,sizeof(unsigned char));
			close(new_sockfd);
			sem_close(mysemaphore1);
			shm_unlink("/APJMEMORY");
			sem_unlink("APJgoldchase");
			exit(0);

		}

	}

}



void server_function()
{
	mysemaphore1=sem_open("/APJgoldchase",O_RDWR);
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
	GoldBoard= (GameBoard*)mmap(NULL,
			playerRows*playerCol+sizeof(GameBoard),
			PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	//unsigned char* myLocalCopy;
	unsigned char* orig = GoldBoard->mapya;
	area=playerRows*playerCol;
	myLocalCopy=(unsigned char*)malloc(sizeof (unsigned char)*playerCol*playerRows);

	GoldBoard->DaemonID=getpid();
	for(int i=0;i<area;i++)
	{
		myLocalCopy[i]=orig[i];
	}
	struct sigaction SotherAction;//handle the signals
	SotherAction.sa_handler=Sother_interrupt;
	sigemptyset(&SotherAction.sa_mask);
	SotherAction.sa_flags=0;
	SotherAction.sa_restorer=NULL;
	sigaction(SIGINT, &SotherAction, NULL);
	sigaction(SIGHUP, &SotherAction, NULL);
	sigaction(SIGTERM, &SotherAction, NULL);
	sigaction(SIGUSR1, &SotherAction,NULL);

	int sockfd;
	int status;
	const char* portno="4500";
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints)); //zero out everything in structure
	hints.ai_family = AF_UNSPEC; //don't care. Either IPv4 or IPv6
	hints.ai_socktype=SOCK_STREAM; // TCP stream sockets
	hints.ai_flags=AI_PASSIVE; //file in the IP of the server for me

	struct addrinfo *servinfo;
	if((status=getaddrinfo(NULL, portno, &hints, &servinfo))==-1)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	}
	sockfd=socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	/*avoid "Address already in use" error*/
	int yes=1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1)
	{
		perror("setsockopt");
	}

	//We need to "bind" the socket to the port number so that the kernel
	//can match an incoming packet on a port to the proper process
	if((status=bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen))==-1)
	{
		perror("bind");
	}
	//when done, release dynamically allocated memory
	freeaddrinfo(servinfo);

	if(listen(sockfd,1)==-1)
	{
		perror("listen");
	}

	//	printf("Blocking, waiting for client to connect\n");

	//int new_sockfd;
	struct sockaddr_in client_addr;
	socklen_t clientSize=sizeof(client_addr);
here: if((new_sockfd=accept(sockfd, (struct sockaddr*) &client_addr, &clientSize))==-1)
      {
	      perror("accept");
	      goto here;
	      //	exit(1);
      }
      int InitNum=0;
      //	int SockPlrz=0;
      WRITE(new_sockfd,&InitNum,sizeof(int));
      WRITE(new_sockfd,&playerRows,sizeof(int));
      WRITE(new_sockfd,&playerCol,sizeof(int));

      unsigned char *senderCopy=myLocalCopy;
      for(int J=0;J<(playerCol*playerRows);++J)
      {
	      WRITE(new_sockfd,&senderCopy[J],sizeof(senderCopy[J]));
      }

      //int readByteN;
      unsigned char CondiX=-1;
      short positionC;
      unsigned char changed;
      int DamID=getpid();
      Sother_interrupt(SIGHUP);
      while(1)
      {
	      GoldBoard->DaemonID=getpid();
	      READ(new_sockfd,&CondiX,sizeof(unsigned char));
	      if(CondiX==0)
	      {
		      CondiX=-99;
		      READ(new_sockfd,&positionC,sizeof(short));
		      READ(new_sockfd,&changed,sizeof(char));
		      myLocalCopy[positionC]=changed;
		      GoldBoard->mapya[positionC]=changed;
		      //	orig=myLocalCopy;
		      for(int i=0;i<5;i++)
		      {
			      if(GoldBoard->array[i]!=0)	kill(GoldBoard->array[i],SIGUSR1);
		      }
		      //					handle_interrupt(0);
	      }
	      else if(CondiX & G_SOCKPLR)
	      {
		      unsigned char player_bit[5]={G_PLR0, G_PLR1, G_PLR2, G_PLR3, G_PLR4};
		      for(int i=0; i<5; ++i) //loop through the player bits
		      {
			      // If player bit is on and shared memory ID is zero,
			      // a player (from other computer) has joined:
			      if(CondiX & player_bit[i] && GoldBoard->array[i]==0)
							{
								GoldBoard->array[i]=DamID;
								QueueSetupS(player_bit[i]);
							}
			      //If player bit is off and shared memory ID is not zero,
			      //remote player has quit:
			      else if(!(CondiX & player_bit[i]) && GoldBoard->array[i]!=0)
						{
							GoldBoard->array[i]=0;
							QueueCleanerS(player_bit[i]);
						}

		      }
		      if(CondiX==G_SOCKPLR)
		      {
			      unsigned char SockPlayer=G_SOCKPLR;
			      int ret=400;
			      ret=	WRITE(new_sockfd,&SockPlayer,sizeof(unsigned char));
			      close(new_sockfd);
			      sem_close(mysemaphore1);
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
					READ(new_sockfd, buffer,121);
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
							if((writequeue_fdS=mq_open(reciver.c_str(), O_WRONLY|O_NONBLOCK))==-1)
							{
								perror("msgq open error");
								//	exit(1);
							}
							char message_text[121];
							memset(message_text, 0, 121);
							strncpy(message_text, ptr, 120);
							if(mq_send(writequeue_fdS, message_text, strlen(message_text), 0)==-1)
							{
								perror("msgq send error");
								//	exit(1);
							}
							mq_close(writequeue_fdS);
						}
					}
				}



      }
}




//Read Message means I send it to the opposite side

void ReadMessageS(int)
{
	struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	for(int mend=0;mend<5;++mend)
	{
			int ret_mq=mq_notify(readqueue_fdS[mend], &mq_notification_event);
			if(ret_mq==0)
			{
				int err;
				char msg[121];
				memset(msg, 0, 121);
				if((err=mq_receive(readqueue_fdS[mend], msg, 120, NULL))!=-1)
				{
					unsigned char player_bit[5]={G_PLR0, G_PLR1, G_PLR2, G_PLR3, G_PLR4};
					unsigned char SendMo=G_SOCKMSG;
					//update
					SendMo|=player_bit[mend];
					WRITE(new_sockfd,&SendMo,sizeof(unsigned char));
					WRITE(new_sockfd,&msg,strlen(msg));
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






//write message means I give it to plr on my side by reading from sock




void QueueSetupS(int player)
{
	int FdNum;
	if(player == G_PLR0)
	{
		mq_nameS="/APJplayer0_mq";
		FdNum=0;
	}
	else if(player == G_PLR1)
		{
			mq_nameS="/APJplayer1_mq";
			FdNum=1;
		}
	else if(player == G_PLR2)
	{
		mq_nameS="/APJplayer2_mq";
		FdNum=2;
	}
	else if(player == G_PLR3)
	{
		mq_nameS="/APJplayer3_mq";
		FdNum=3;
	}
	else if(player == G_PLR4)
	{
		mq_nameS="/APJplayer4_mq";
		FdNum=4;
	}

	struct sigaction action_to_take;
	action_to_take.sa_handler=ReadMessageS;
	sigemptyset(&action_to_take.sa_mask);
	action_to_take.sa_flags=0;
	sigaction(SIGUSR2, &action_to_take, NULL);
	struct mq_attr mq_attributes;
	mq_attributes.mq_flags=0;
	mq_attributes.mq_maxmsg=10;
	mq_attributes.mq_msgsize=120;
	if((readqueue_fdS[FdNum]=mq_open(mq_nameS.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
					S_IRUSR|S_IWUSR, &mq_attributes))==-1)
	{
		perror("mq_open");
		exit(1);
	}
	struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	mq_notify(readqueue_fdS[FdNum], &mq_notification_event);
}



void QueueCleanerS(int player)
{

int FdNum;
if(player == G_PLR0)
{
	mq_nameS="/APJplayer0_mq";
	FdNum=0;
}
else if(player == G_PLR1)
	{
		mq_nameS="/APJplayer1_mq";
		FdNum=1;
	}
else if(player == G_PLR2)
{
	mq_nameS="/APJplayer2_mq";
	FdNum=2;
}
else if(player == G_PLR3)
{
	mq_nameS="/APJplayer3_mq";
	FdNum=3;
}
else if(player == G_PLR4)
{
	mq_nameS="/APJplayer4_mq";
	FdNum=4;
}

	mq_close(readqueue_fdS[FdNum]);
	if(mq_unlink(mq_nameS.c_str())==-1)
	{
		if(errno==EACCES)	perror("Access eror");
		else if(errno==ENAMETOOLONG)	perror("Name to long");
		else if(ENOENT==errno)	perror("Queue with no name");
		//exit(1);
	}
}
