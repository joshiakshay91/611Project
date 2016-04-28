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
	GameBoard* GoldBoard;
  unsigned char* myLocalCopy;
  int area;
	int new_sockfd=0;
	sem_t *mysemaphore1;
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
	bool tookLast=false;
	 for (int n=0;n<5;n++)
		 {
			 if((GoldBoard->array[n]!=0) && (GoldBoard->array[n]!=GoldBoard->DaemonID))
			 {tookLast=true;}
		 }
	 if(tookLast==false)
	 {
	 sem_close(mysemaphore1);
	 shm_unlink("/APJMEMORY");
	 sem_unlink("APJgoldchase");
	 exit(0);
	 }
	 unsigned char SockPlayer;
	 SockPlayer=G_SOCKPLR;
	 for(int i=0; i<5; ++i)
	 {
  		if(GoldBoard->array[i]!=0)
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
			if(new_sockfd!=0)	WRITE(new_sockfd,&SockPlayer,sizeof(unsigned char));//send sock
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
	myLocalCopy=(unsigned char*)malloc(sizeof (char)*playerCol*playerRows);

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
			int SockPlrz=0;
      WRITE(new_sockfd,&InitNum,sizeof(int));
      WRITE(new_sockfd,&playerRows,sizeof(int));
      WRITE(new_sockfd,&playerCol,sizeof(int));
			for(int z=0;z<5;z++)
			{
				if(GoldBoard->array[z]!=0)
				{
					int byter=0;
					switch (z) {
						case 0:	byter=G_PLR0;
										break;
						case 1: byter=G_PLR1;
										break;
						case 2:	byter=G_PLR2;
										break;
						case 3: byter=G_PLR3;
										break;
						case 4: byter=G_PLR4;
										break;
					}
					SockPlrz|=byter;
				}
			}
			WRITE(new_sockfd,&SockPlrz,sizeof(int));
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

while(1)
{
	READ(new_sockfd,&CondiX,sizeof(unsigned char));
	if(CondiX==0)
	{
		CondiX=-1;
		READ(new_sockfd,&positionC,sizeof(short));
		READ(new_sockfd,&changed,sizeof(char));
		myLocalCopy[positionC]=changed;
		GoldBoard->mapya[positionC]=changed;
		orig=myLocalCopy;
		for(int i=0;i<5;i++)
		{
			if(GoldBoard->array[i]!=0)	kill(GoldBoard->array[i],SIGUSR1);
		}
//					handle_interrupt(0);
	}
	unsigned char temp = CondiX;
	//temG_SOCKPLR;
	if(temp & G_SOCKPLR)
	{
		unsigned char temp1=CondiX;

				unsigned char temp2=temp1;
				//temp2&=G_PLR0;
				if(temp2 & G_PLR0)
				{
					if(GoldBoard->array[0]==0)
					{
						GoldBoard->array[0]=DamID;
					}

				}
				else
				{
					if(GoldBoard->array[0]!=0)
					{
						GoldBoard->array[0]=0;
					}
				}
				temp2=temp1;
				//temp2&=G_PLR1;
				if(temp2 & G_PLR1)
				{
					if(GoldBoard->array[1]==0)
					{
						GoldBoard->array[1]=DamID;
					}

				}
				else
				{
					if(GoldBoard->array[1]!=0)
					{
						GoldBoard->array[1]=0;
					}
				}

				temp2=temp1;
	//			temp2&=G_PLR2;
				if(temp2 & G_PLR2)
				{
					if(GoldBoard->array[2]==0)
					{
						GoldBoard->array[2]=DamID;
					}

				}
				else
				{
					if(GoldBoard->array[2]!=0)
					{
						GoldBoard->array[2]=0;
					}
				}
				temp2=temp1;
	//			temp2&=G_PLR3;
				if(temp2 & G_PLR3)
				{
					if(GoldBoard->array[3]==0)
					{
						GoldBoard->array[3]=DamID;
					}
				}
				else
				{
					if(GoldBoard->array[3]!=0)
					{
						GoldBoard->array[3]=0;
					}
				}
				temp2=temp1;
	//			temp2&=G_PLR4;
				if(temp2 & G_PLR4)
				{
					if(GoldBoard->array[4]==0)
					{
						GoldBoard->array[4]=DamID;
					}

				}
				else
				{
					if(GoldBoard->array[4]!=0)
					{
						GoldBoard->array[4]=0;
					}
				}
		}
	}



}
