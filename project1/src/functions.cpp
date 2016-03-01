//functions.h
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
//the GameBoard struct
struct GameBoard
{
	int rows;
	int coloumns;
	int array[5];
	unsigned char players;
	unsigned char mapya[0];
};


Map* pointer=NULL;
using namespace std;
void handle_interrupt(int)
{
 //  std::cerr << "interrupt!\n";
 if(pointer!=NULL)
 {
	 pointer->drawMap();
 }
}


/*--------------------------------------------------------------------*/
/*Send refresh signal to all available players*/
void SignalKiller(int PlayerArray[])
{
	for(int i=0;i<5;i++)
	{
		if(PlayerArray[i])
		{
			kill(PlayerArray[i],SIGINT);
		}
	}
}
/*-----------------------------------------------------------------*/

//checks the last man status in the game
bool lastManStatus(GameBoard* GoldBoard)
{

	if((!(GoldBoard->players & G_PLR0)) && (!(GoldBoard->players & G_PLR1))
			&& (!(GoldBoard->players & G_PLR2)) && (!(GoldBoard->players & G_PLR3))
			&& (!(GoldBoard->players & G_PLR4)))
	{
		return true;
	}
	return false;
}

/*-------------------------------------------------------------------*/
//movement function for h,j,k,l for all players
/*-------------------------------------------------------------------*/

void movement(GameBoard* GoldBoard,int playerPlacement,Map goldMine,
		char myplayer, sem_t* mysemaphore)
{
	bool GoldFlag=false,Flag=false;
	int MapCol=GoldBoard->coloumns;
	int MapRow=GoldBoard->rows;
	int OldLocation;
	char button='m'; //just a garbage
	goldMine.postNotice("Welcome To The Gold Chase Game This Box is a notice Box");
	while(button!='Q')
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
		if(button=='k')
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
		  SignalKiller((GoldBoard->array));
		}

	}//while ends here
	sem_wait(mysemaphore);
	GoldBoard->mapya[playerPlacement]&=~myplayer;
	sem_post(mysemaphore);
}


/*-------------------------------------------------------------------*/
//checks for player spot available
/*-------------------------------------------------------------------*/


char playerSpot(GameBoard* GoldBoard, int pid)
{
	char currentPlayer;
	if(!(GoldBoard->players & G_PLR0))
	{
		currentPlayer=G_PLR0;
		GoldBoard->players|=currentPlayer;
		GoldBoard->array[0]=pid;
	}
	else if(!(GoldBoard->players & G_PLR1))
	{
		currentPlayer=G_PLR1;
		GoldBoard->players|=currentPlayer;
		GoldBoard->array[1]=pid;
	}
	else if(!(GoldBoard->players & G_PLR2))
	{
		currentPlayer=G_PLR2;
		GoldBoard->players|=currentPlayer;
		GoldBoard->array[2]=pid;
	}
	else if(!(GoldBoard->players & G_PLR3))
	{
		currentPlayer=G_PLR3;
		GoldBoard->players|=currentPlayer;
		GoldBoard->array[3]=pid;
	}
	else if(!(GoldBoard->players & G_PLR4))
	{
		currentPlayer=G_PLR4;
		GoldBoard->players|=currentPlayer;
		GoldBoard->array[4]=pid;
	}
	else
	{
		char F='F';           //return F indicating full status
		currentPlayer=F;
	}
	return currentPlayer;
}
