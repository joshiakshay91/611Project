/*
Author: Akshay Joshi
Date: 13 March 2016
 */
#include "goldchase.h"
#include "Map.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <random>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
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
//the GameBoard struct
	//sem_t *mysemaphore; //semaphore
int pid;
int DAM_ID=0;
bool Somewhere=true;//for handling interrupt
bool ColdFlag=true;//for handling interrupt when getting input
Map* pointer=NULL;//Global Map Pointer
bool lastManStatus(GameBoard*); //func to check last player ? y or n
void movement(GameBoard*,int,Map&,char,sem_t*); //for moving the players
char playerSpot(GameBoard*, int); //to check which spot is available
void QueueSetup(int player);//function to setup mqueue
void QueueCleaner();
void broadcaster(string msg,GameBoard* GoldBoard);//broadcasts message
string senderI;//username
void SignalKiller(int PlayerArray[], int);
void handle_interrupt(int)
{
	if(pointer)
	{
		pointer->drawMap();
	}
}
void other_interrupt(int)
{
	if(ColdFlag)
	{
		cout<<"Sorry, I have been signaled that,It is too cold!!!"<<endl;
		exit(0);
	}
	Somewhere=false;
}

mqd_t readqueue_fd;//file descriptor
mqd_t writequeue_fd;//file descriptor
string mq_name="/APJqueue";
void ReadMessage(int)
{
	struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	mq_notify(readqueue_fd, &mq_notification_event);
	int err;
	char msg[121];
	memset(msg, 0, 121);
	while((err=mq_receive(readqueue_fd, msg, 120, NULL))!=-1)
	{
		pointer->postNotice(msg);
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
		exit(1);
	}
}

void writeMessage(string message,int player)
{
	string reciver;
	if(player == G_PLR0)	reciver="/APJplayer0_mq";
	else if(player == G_PLR1)	reciver="/APJplayer1_mq";
	else if(player == G_PLR2)	reciver="/APJplayer2_mq";
	else if(player == G_PLR3)	reciver="/APJplayer3_mq";
	else if(player == G_PLR4)	reciver="/APJplayer4_mq";
	const char *ptr=message.c_str();
	if((writequeue_fd=mq_open(reciver.c_str(), O_WRONLY|O_NONBLOCK))==-1)
	{
		perror("msgq open error");
		exit(1);
	}
	char message_text[121];
	memset(message_text, 0, 121);
	strncpy(message_text, ptr, 120);
	if(mq_send(writequeue_fd, message_text, strlen(message_text), 0)==-1)
	{
		perror("msgq send error");
		exit(1);
	}
	mq_close(writequeue_fd);
}

int main(int argc, char* argv[])
{

	int Turn=0;
	if(argc>1){
	try{
		Turn=stoi(argv[1]);
	}catch(...)
	{}
	if(Turn==999){
		sem_t *mysemaphore;
		mysemaphore=sem_open("/APJgoldchase",O_RDWR);
		if(mysemaphore==SEM_FAILED)
				{
					client_function();
				}
		else
			{
				sem_close(mysemaphore);
			}
		//		sleep(10);
	}
}
	//////////////////////////////////////////
	struct sigaction OtherAction;//handle the signals
	OtherAction.sa_handler=other_interrupt;
	sigemptyset(&OtherAction.sa_mask);
	OtherAction.sa_flags=0;
	OtherAction.sa_restorer=NULL;
	sigaction(SIGINT, &OtherAction, NULL);
	sigaction(SIGHUP, &OtherAction, NULL);
	sigaction(SIGTERM, &OtherAction, NULL);
	/////////////////////////////////////////
	cout<<"What is you name?"<<endl;
	getline(cin,senderI);
	ColdFlag=false;// if sig int,hup,or termed coldly while getting i/p
	int counter,fd;
	char byte=0;
	int num_lines=0;
	int line_length=0;
	GameBoard* GoldBoard;
	bool lastPos= false; //checking the last player status;
	////////////////////////////////Sigaction declaration chunk
	struct sigaction ActionJackson;
	ActionJackson.sa_handler=handle_interrupt;
	sigemptyset(&ActionJackson.sa_mask);
	ActionJackson.sa_flags=0;
	ActionJackson.sa_restorer=NULL;
	sigaction(SIGUSR1, &ActionJackson, NULL);
	pid=getpid();
	//////////////////////////////////////////
	std::default_random_engine engi;
	std::random_device aj;//random and
	string line,text;
	sem_t *mysemaphore; //semaphore
	mysemaphore= sem_open("/APJgoldchase", O_CREAT|O_EXCL,
			S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);
	if(mysemaphore!=SEM_FAILED) //you are the first palyer
	{
		sem_wait(mysemaphore);
		fd = shm_open("/APJMEMORY", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if(fd==-1)
		{
			perror("Shared memory creation failed");
			exit(1);
		}
//loading the map
		ifstream in("mymap.txt");
		getline(in,line);
		counter=std::stoi(line.c_str());//convert using stoi ..inclass
		while(getline(in, line))
		{
			text += line;
			++num_lines;
			line_length=line.length();
		}
		const char* data = text.c_str();
		int area = line_length*num_lines;
		if((ftruncate(fd,(area + sizeof(GameBoard))))==-1)
		{
			perror("FTRUNCATE FAILURE :");
			exit(1);
		}
		GoldBoard= (GameBoard*) mmap(NULL, area+sizeof(GameBoard),
				PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);
		GoldBoard->rows=num_lines;
		GoldBoard->coloumns=line_length;
		unsigned char myplayer=G_PLR0;
		GoldBoard->array[0]=pid;
		QueueSetup(myplayer);
		for(int itr=1;itr<5;itr++)
		{
			GoldBoard->array[itr]=0;
		}
		byte=0;
		int index=0;


		std::uniform_int_distribution<int> rand(1,area); //here
		engi.seed(aj());
		const char* ptr=data;//Convert the ASCII bytes into bit
		//fields drawn from goldchase.h
		while(*ptr!='\0')
		{
			if(*ptr==' ')
			{
				byte=0;
				GoldBoard->mapya[index]=byte;

			}
			else if(*ptr=='*')
			{
				byte|=G_WALL;
				GoldBoard->mapya[index]=byte;
			}
			++ptr;
			++index;
		}

		while(counter)
		{
			int placement=rand(engi);
			int placement2=placement;
			index=0;
			while(placement2)
			{
				index++;
				placement2--;
			}
			if(GoldBoard->mapya[index]==0)
			{
				if(counter==1)
				{
					byte=0;
					byte|=G_GOLD;
					GoldBoard->mapya[index]=byte;
				}else
				{
					byte=0;
					byte|=G_FOOL;
					GoldBoard->mapya[index]=byte;
				}
				counter--;
			}
		}
		try{
			Map goldMine((GoldBoard->mapya),num_lines,line_length);
			bool loopFlag=true;
			int player1Placement;
			while(loopFlag)
			{
				player1Placement=rand(engi);
				byte=0;
				if(GoldBoard->mapya[player1Placement]==0)
				{
					byte|=G_PLR0;
					GoldBoard->mapya[player1Placement]|=byte;
					loopFlag=false;
					goldMine.drawMap();
				}
			}
			sem_post(mysemaphore);
			pointer=&goldMine;
//			if(GoldBoard->DaemonID=0)
				//{
					server_function();
					//sighup
//					if(GoldBoard->DaemonID!=0)	kill(GoldBoard->DaemonID,SIGHUP);
				//}
			movement(GoldBoard,player1Placement,goldMine,myplayer,mysemaphore);
		}catch(std::runtime_error& e){
			sem_post(mysemaphore);
			GoldBoard->array[0]=0;
			if(lastManStatus(GoldBoard))
			{
				sem_close(mysemaphore);
				shm_unlink("/APJMEMORY");
				sem_unlink("APJgoldchase");
			}
		}
		GoldBoard->array[0]=0;
		SignalKiller((GoldBoard->array), GoldBoard->DaemonID);
		lastPos=lastManStatus(GoldBoard); //player1 ends here
	}// if ends on this line
	else
	{//all subsequent players
		unsigned char currentPlayer;
		mysemaphore=sem_open("/APJgoldchase",O_RDWR);
		if(mysemaphore==SEM_FAILED)
		{
			perror("Something went wrong!!!");
			exit(1);
		}
		sem_wait(mysemaphore);
		fd = shm_open("/APJMEMORY", O_RDWR, S_IRUSR | S_IWUSR);
		int player2rows;
		int player2col;
		read(fd,&player2rows,sizeof(int));
		read(fd,&player2col,sizeof(int));
		std::uniform_int_distribution<int> rand(1,(player2rows*player2col)); //here
		engi.seed(aj());
		GameBoard* GoldBoard= (GameBoard*)mmap(NULL,
				player2rows*player2col+sizeof(GameBoard),
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		GoldBoard->rows=player2rows;
		GoldBoard->coloumns=player2col;
		currentPlayer=playerSpot(GoldBoard,pid);
		//while deciding player spot pid is provided
		if(currentPlayer=='F') //if F is returned it means 5 players
		{ 										//are already playing
			sem_post(mysemaphore);
			cout<<"We are currently Full, Get out...!!!"<<endl;
			exit(0);
		}
		try{
			QueueSetup(currentPlayer);
			Map goldMine((GoldBoard->mapya),player2rows,player2col);
			bool loopFlag=true;
			int player2Placement;
			while(loopFlag)
			{
				player2Placement=rand(engi);
				byte=0;
				if(GoldBoard->mapya[player2Placement]==0)
				{
					byte|=currentPlayer;
					GoldBoard->mapya[player2Placement]|=byte;
					loopFlag=false;
					SignalKiller((GoldBoard->array),GoldBoard->DaemonID);
				}
			}
			sem_post(mysemaphore);

			pointer=&goldMine;
			goldMine.drawMap();
			//sighup
//			if(GoldBoard->DaemonID!=0)
			movement(GoldBoard,player2Placement,goldMine,currentPlayer,mysemaphore);
		}catch(std::runtime_error& e){
			sem_post(mysemaphore);
			for(int i=0;i<5;i++)
			{
				if((GoldBoard->array[i])==pid)
				{
					GoldBoard->array[i]=0;
				}
			}
			if(lastManStatus(GoldBoard))
			{
				sem_close(mysemaphore);
				shm_unlink("/APJMEMORY");
				sem_unlink("APJgoldchase");
			}
		}
		for(int i=0;i<5;i++)
		{
			if((GoldBoard->array[i])==pid)
			{
				GoldBoard->array[i]=0;
			}
		}
		SignalKiller((GoldBoard->array), GoldBoard->DaemonID);
		lastPos=lastManStatus(GoldBoard);
		if(GoldBoard->DaemonID!=0)	kill(GoldBoard->DaemonID,SIGHUP);
		QueueCleaner();
		return 0;
	}
	QueueCleaner();
	//if(lastPos)
	if(GoldBoard->DaemonID!=0)	kill(GoldBoard->DaemonID,SIGHUP);
	if(lastPos){
			sem_close(mysemaphore);
			shm_unlink("/APJMEMORY");
			sem_unlink("APJgoldchase");
		}
	return 0;
}
//checks the last man status in the game
bool lastManStatus(GameBoard* GoldBoard)
{
	for(int i=0;i<5;i++)
	{
		if((GoldBoard->array[i])!=0)
		{
			return false;//that means someone is there
		}
	}
	return true;
}

/*-------------------------------------------------------------------*/
//movement function for h,j,k,l for all players
/*-------------------------------------------------------------------*/

void movement(GameBoard* GoldBoard,int playerPlacement,Map& goldMine,
		char myplayer, sem_t* mysemaphore)
{
	kill(GoldBoard->DaemonID,SIGHUP);
	DAM_ID=GoldBoard->DaemonID;
	bool GoldFlag=false,Flag=false;
	int MapCol=GoldBoard->coloumns;
	int MapRow=GoldBoard->rows;
	int OldLocation;
	int ToPerson;
	string msg;
	char button='m'; //just a garbage
	goldMine.postNotice("Welcome To The Gold Chase Game This Box is a notice Box");
	cout<<"DaemonID: "<<GoldBoard->DaemonID<<endl;
	while(button!='Q'&& (Somewhere))
	{
		button=goldMine.getKey();
		if(button=='h')
		{
			if(((playerPlacement)%(MapCol))!=0)
			{
				if(GoldBoard->mapya[playerPlacement-1]!=G_WALL)
				{
					Flag=true;
					OldLocation=playerPlacement;
					--playerPlacement;
				}
			}
			else if(GoldFlag)
			{
				button='Q';
				goldMine.postNotice("You Won");
			}
		}
		else if(button=='l')
		{
			if(((playerPlacement+1)%(MapCol))!=0)
			{
				if(GoldBoard->mapya[playerPlacement+1]!=G_WALL)
				{
					Flag=true;
					OldLocation=playerPlacement;
					++playerPlacement;
				}
			}
			else if(GoldFlag)
			{
				button='Q';
				goldMine.postNotice("You Won");
			}
		}
		else if(button=='k')
		{
			if((playerPlacement-MapCol)>=0)
			{
				if(GoldBoard->mapya[(playerPlacement-MapCol)]!=G_WALL)
				{
					Flag=true;
					OldLocation=playerPlacement;
					playerPlacement-=MapCol;
				}
			}
			else if(GoldFlag)
			{
				button='Q';
				goldMine.postNotice("You Won");
			}
		}
		else if(button=='j')
		{
			if((playerPlacement+MapCol)<(MapRow*MapCol))
			{
				if(GoldBoard->mapya[playerPlacement+MapCol]!=G_WALL)
				{
					Flag=true;
					OldLocation=playerPlacement;
					playerPlacement+=MapCol;
				}
			}
			else if(GoldFlag)
			{
				button='Q';
				goldMine.postNotice("You Won");
			}
		}
		else if(button=='m')
		{
			ToPerson=0;
			if(GoldBoard->array[0]!=0)	ToPerson|=G_PLR0;
			if(GoldBoard->array[1]!=0)	ToPerson|=G_PLR1;
			if(GoldBoard->array[2]!=0)	ToPerson|=G_PLR2;
			if(GoldBoard->array[3]!=0)	ToPerson|=G_PLR3;
			if(GoldBoard->array[4]!=0)	ToPerson|=G_PLR4;
			ToPerson&=~myplayer; //i dont want to send myself anything
			int toAddress=goldMine.getPlayer(ToPerson);
			if(toAddress!=0)
			{
				msg="Player->"+senderI+" says:";
				msg=msg+goldMine.getMessage();
				writeMessage(msg,toAddress);
			}
		}
		else if(button=='b')
		{
			int counter=0;
			for(int i=0;i<5;i++)
			{
				if(GoldBoard->array[i]!=0)
				{
					++counter;
				}
			}
			if(counter>1)
			{
				msg="Player->"+senderI+" says:";
				msg=msg+goldMine.getMessage();
				broadcaster(msg,GoldBoard);
			}
			else
			{
				goldMine.postNotice("Dude you are alone in this game.");
			}
		}
		if(Flag)
		{
			sem_wait(mysemaphore);
			GoldBoard->mapya[OldLocation]&=~myplayer;
			if((GoldBoard->mapya[playerPlacement]!=G_FOOL)&&
					((GoldBoard->mapya[playerPlacement]!=G_GOLD)))
			{
				GoldBoard->mapya[playerPlacement]|=myplayer;
				sem_post(mysemaphore);
			}
			else
			{
				if((GoldBoard->mapya[playerPlacement]==G_FOOL))
				{
					GoldBoard->mapya[playerPlacement]&=~G_FOOL;
					GoldBoard->mapya[playerPlacement]=myplayer;
					sem_post(mysemaphore);
					goldMine.postNotice("Tricked You!!! 'FOOLS GOLD'");
				}
				else if((GoldBoard->mapya[playerPlacement]==G_GOLD))
				{
					GoldBoard->mapya[playerPlacement]&=~G_GOLD;
					GoldBoard->mapya[playerPlacement]=myplayer;
					sem_post(mysemaphore);
					GoldFlag=true;
					goldMine.postNotice("You got REAL GOLD make your escape!!!");
				}
			}
			Flag=false;
			SignalKiller((GoldBoard->array), GoldBoard->DaemonID);
		}
	}//while ends here
	if(GoldFlag)
	{
		int counter=0;
		for(int i=0;i<5;i++)
		{
			if(GoldBoard->array[i]!=0)
			{
				++counter;
			}
		}
		if(counter>1)
		{
			msg="Player->"+senderI+" Is winner and he has escaped!!!";
			broadcaster(msg,GoldBoard);
		}
	}
	sem_wait(mysemaphore);
	GoldBoard->mapya[playerPlacement]&=~myplayer;
	SignalKiller((GoldBoard->array),GoldBoard->DaemonID);
	cout<<"Quiting"<<endl;
	cout<<"DaemonID: "<<GoldBoard->DaemonID<<endl;
	sem_post(mysemaphore);
}


/*-------------------------------------------------------------------*/
//checks for player spot available
/*-------------------------------------------------------------------*/


char playerSpot(GameBoard* GoldBoard, int pid)
{
	char currentPlayer;
	if(GoldBoard->array[0]==0)
	{
		currentPlayer=G_PLR0;
		GoldBoard->array[0]=pid;
	}
	else if(GoldBoard->array[1]==0)
	{
		currentPlayer=G_PLR1;
		GoldBoard->array[1]=pid;
	}
	else if(GoldBoard->array[2]==0)
	{
		currentPlayer=G_PLR2;
		GoldBoard->array[2]=pid;
	}
	else if(GoldBoard->array[3]==0)
	{
		currentPlayer=G_PLR3;
		GoldBoard->array[3]=pid;
	}
	else if(GoldBoard->array[4]==0)
	{
		currentPlayer=G_PLR4;
		GoldBoard->array[4]=pid;
	}
	else
	{
		char F='F';           //return F indicating full status
		currentPlayer=F;
	}
	return currentPlayer;
}


/*--------------------------------------------------------------------*/
/*Send refresh signal to all available players*/
void SignalKiller(int PlayerArray[],int DID)
{
	for(int i=0;i<5;i++)
	{
		if(PlayerArray[i])
		{
			kill(PlayerArray[i],SIGUSR1);
		}
	}
	if(DID!=0)	kill(DID,SIGUSR1);
}
/*-----------------------------------------------------------------*/

/*Setting up the queue with sig action*/
//given part
void QueueSetup(int player)
{
	if(player == G_PLR0)	mq_name="/APJplayer0_mq";
	else if(player == G_PLR1)	mq_name="/APJplayer1_mq";
	else if(player == G_PLR2)	mq_name="/APJplayer2_mq";
	else if(player == G_PLR3)	mq_name="/APJplayer3_mq";
	else if(player == G_PLR4)	mq_name="/APJplayer4_mq";
	struct sigaction action_to_take;
	action_to_take.sa_handler=ReadMessage;
	sigemptyset(&action_to_take.sa_mask);
	action_to_take.sa_flags=0;
	sigaction(SIGUSR2, &action_to_take, NULL);
	struct mq_attr mq_attributes;
	mq_attributes.mq_flags=0;
	mq_attributes.mq_maxmsg=10;
	mq_attributes.mq_msgsize=120;
	if((readqueue_fd=mq_open(mq_name.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
					S_IRUSR|S_IWUSR, &mq_attributes))==-1)
	{
		perror("mq_open");
		exit(1);
	}
	struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	mq_notify(readqueue_fd, &mq_notification_event);
}

//magic done
////////////////////////////////
/*We are responsible for destroying our own queue*/
void QueueCleaner()
{
	mq_close(readqueue_fd);
	if(mq_unlink(mq_name.c_str())==-1)
	{
		if(errno==EACCES)	perror("Access eror");
		else if(errno==ENAMETOOLONG)	perror("Name to long");
		else if(ENOENT==errno)	perror("Queue with no name");
		exit(1);
	}
}
////////////////////////////////
/*Function to broadcast message*/

void broadcaster(string msg,GameBoard* GoldBoard)
{
	for (int i = 0; i < 5; i++)
	{
		if((GoldBoard->array[i]!=pid)&&(GoldBoard->array[i]!=0))
		{
			if(i==0)		writeMessage(msg,G_PLR0);
			if(i==1)		writeMessage(msg,G_PLR1);
			if(i==2)		writeMessage(msg,G_PLR2);
			if(i==3)		writeMessage(msg,G_PLR3);
			if(i==4)		writeMessage(msg,G_PLR4);
		}
	}
}
////////////////////////////////
