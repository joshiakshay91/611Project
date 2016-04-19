#ifndef CLIENT_CPP
#define CLIENT_CPP
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
void ClientDaemon_function()
{
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
}


#endif
