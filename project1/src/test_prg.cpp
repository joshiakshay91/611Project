#include "goldchase.h"
#include "Map.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <random>
#include <ctime>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
struct GameBoard{
	int rows;
	int coloumns;
	unsigned char players;
	unsigned char mapya[0];
};
bool lastManStatus(GameBoard*);
void movement(GameBoard*,int,Map,char,sem_t*);

using namespace std;
int main()
{
	int counter;
	int fd;
	char byte=0;
	int num_lines=0;
	int line_length=0;
	GameBoard* Goldberg;
	std::default_random_engine engi;
	std::random_device aj;
	string line,text;
	sem_t *mysemaphore;
	mysemaphore= sem_open("/APJgoldchase", O_CREAT|O_EXCL,
			S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);
	if(mysemaphore!=SEM_FAILED) //you are the first palyer
	{
		int mysemaphoreVal;
		sem_getvalue(mysemaphore,&mysemaphoreVal);
		sem_wait(mysemaphore);

		fd = shm_open("/APJMEMORY", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if(fd==-1)
		{
			perror("Shared memory creation failed");
			exit(1);
		}

		ifstream in("mymap.txt");
		getline(in,line);
		counter=std::atoi(line.c_str());
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
		Goldberg= (GameBoard*) mmap(NULL, area+sizeof(GameBoard), PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);
		Goldberg->rows=num_lines;
		Goldberg->coloumns=line_length;
		byte=0;
		int index=0;


		std::uniform_int_distribution<int> rand(1,area); //here
		engi.seed(aj());
		const char* ptr=data;
		//Convert the ASCII bytes into bit fields drawn from goldchase.h
		while(*ptr!='\0')
		{
			if(*ptr==' ')
			{
				byte=0;
				Goldberg->mapya[index]=byte;

			}
			else if(*ptr=='*')
			{
				byte|=G_WALL;
				Goldberg->mapya[index]=byte;
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
			if(Goldberg->mapya[index]==0)
			{
				if(counter==1)
				{
					byte=0;
					byte|=G_GOLD;
					Goldberg->mapya[index]=byte;
				}else
				{
					byte=0;
					byte|=G_FOOL;
					Goldberg->mapya[index]=byte;
				}
				counter--;
			}
		}
		char myplayer=G_PLR0;
		Goldberg->players=myplayer;
		try{
			Map goldMine((Goldberg->mapya),num_lines,line_length);
			bool loopFlag=true;
			int player1Placement;
			while(loopFlag)
			{
				player1Placement=rand(engi);
				byte=0;
				if(Goldberg->mapya[player1Placement]==0)
				{
					byte|=G_PLR0;
					Goldberg->mapya[player1Placement]=byte;
					loopFlag=false;
					goldMine.drawMap();
				}
			}
			sem_post(mysemaphore);
			movement(Goldberg,player1Placement,goldMine,myplayer,mysemaphore);
		}catch(std::runtime_error& e){
			sem_post(mysemaphore);
			Goldberg->players &= ~myplayer;
			if(lastManStatus(Goldberg))
			{
				sem_close(mysemaphore);
				shm_unlink("/APJMEMORY");
				sem_unlink("APJgoldchase");
			}
		}
		Goldberg->players &= ~myplayer;
		if(lastManStatus(Goldberg))
		{
			sem_close(mysemaphore);
			shm_unlink("/APJMEMORY");
			sem_unlink("APJgoldchase");
		}
	}//here semaphore not failed if ends on this line
	else
	{
		char currentPlayer;
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
		GameBoard* Goldberg= (GameBoard*)mmap(NULL, player2rows*player2col+sizeof(GameBoard),
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		Goldberg->rows=player2rows;
		Goldberg->coloumns=player2col;
		if(!(Goldberg->players & G_PLR0))
		{
			currentPlayer=G_PLR0;
			Goldberg->players|=currentPlayer;
		}
		else if(!(Goldberg->players & G_PLR1))
		{
			currentPlayer=G_PLR1;
			Goldberg->players|=currentPlayer;
		}
		else if(!(Goldberg->players & G_PLR2))
		{
			currentPlayer=G_PLR2;
			Goldberg->players|=currentPlayer;
		}
		else if(!(Goldberg->players & G_PLR3))
		{
			currentPlayer=G_PLR3;
			Goldberg->players|=currentPlayer;
		}
		else if(!(Goldberg->players & G_PLR4))
		{
			currentPlayer=G_PLR4;
			Goldberg->players|=currentPlayer;
		}
		else
		{
			sem_post(mysemaphore);
			cout<<"We are currently 5 player game Get out"<<endl;
			exit (0);
		}
		try{
			Map goldMine((Goldberg->mapya),player2rows,player2col);
			bool loopFlag=true;
			int player2Placement;
			while(loopFlag)
			{
				player2Placement=rand(engi);
				byte=0;
				if(Goldberg->mapya[player2Placement]==0)
				{
					byte|=currentPlayer;
					Goldberg->mapya[player2Placement]=byte;
					loopFlag=false;
					goldMine.drawMap();
				}
			}
			sem_post(mysemaphore);
			movement(Goldberg,player2Placement,goldMine,currentPlayer,mysemaphore);
		}catch(std::runtime_error& e){
			sem_post(mysemaphore);
			Goldberg->players &= ~currentPlayer;
			if(lastManStatus(Goldberg))
			{
				sem_close(mysemaphore);
				shm_unlink("/APJMEMORY");
				sem_unlink("APJgoldchase");
			}
		}
		Goldberg->players &= ~currentPlayer;
		if(lastManStatus(Goldberg))
		{
			sem_close(mysemaphore);
			shm_unlink("/APJMEMORY");
			sem_unlink("APJgoldchase");
		}
	}
}

bool lastManStatus(GameBoard* Goldberg)
{

	if((!(Goldberg->players & G_PLR0)) && (!(Goldberg->players & G_PLR1)) && (!(Goldberg->players & G_PLR2)) && (!(Goldberg->players & G_PLR3)) && (!(Goldberg->players & G_PLR4)))
	{
		return true;
	}
	return false;
}

void movement(GameBoard* Goldberg,int playerPlacement,Map goldMine,char myplayer, sem_t* mysemaphore)
{
	int MapCol=Goldberg->coloumns;
  int MapRow=Goldberg->rows;
	char button='m'; //just a garbage
	goldMine.postNotice("This is a notice");
	while(button!='Q')
	{
		button=goldMine.getKey();
		if(button=='h')
		{
			if(((playerPlacement)%(MapCol))!=0)
			{
				sem_wait(mysemaphore);
				if(Goldberg->mapya[playerPlacement-1]!=G_WALL)
				{
					Goldberg->mapya[playerPlacement]=0;
					playerPlacement--;
					if((Goldberg->mapya[playerPlacement]!=G_FOOL)&&((Goldberg->mapya[playerPlacement]!=G_GOLD)))
					{
						Goldberg->mapya[playerPlacement]=myplayer;
					}
					else
					{
						if((Goldberg->mapya[playerPlacement]==G_FOOL))
						{
							goldMine.postNotice("You been tricked its fools gold");
						}
						if((Goldberg->mapya[playerPlacement]==G_GOLD))
						{
							goldMine.postNotice("Run barry you got the real gold");
							Goldberg->mapya[playerPlacement]=myplayer;
						}
					}
					goldMine.drawMap();
				}
				sem_post(mysemaphore);
			}
		}
		else if(button=='l')
		{
			if(((playerPlacement+1)%(MapCol))!=0)
			{
				sem_wait(mysemaphore);
				if(Goldberg->mapya[playerPlacement+1]!=G_WALL)
				{
					Goldberg->mapya[playerPlacement]=0;
					playerPlacement++;
					if((Goldberg->mapya[playerPlacement]!=G_FOOL)&&((Goldberg->mapya[playerPlacement]!=G_GOLD)))
					{
						Goldberg->mapya[playerPlacement]=myplayer;
					}
					else
					{
						if((Goldberg->mapya[playerPlacement]==G_FOOL))
						{
							goldMine.postNotice("You been tricked its fools gold");
						}
						if((Goldberg->mapya[playerPlacement]==G_GOLD))
						{
							goldMine.postNotice("Run barry you got the real gold");
							Goldberg->mapya[playerPlacement]=myplayer;
						}
					}
					goldMine.drawMap();
				}
				sem_post(mysemaphore);
			}

		}
		if(button=='k')
		{
			if((playerPlacement-MapCol)>=0)
			{
				sem_wait(mysemaphore);
				if(Goldberg->mapya[(playerPlacement-MapCol)]!=G_WALL)
				{
					Goldberg->mapya[playerPlacement]=0;
					playerPlacement-=MapCol;
					if((Goldberg->mapya[playerPlacement]!=G_FOOL)&&((Goldberg->mapya[playerPlacement]!=G_GOLD)))
					{
						Goldberg->mapya[playerPlacement]=myplayer;
					}
					else
					{
						if((Goldberg->mapya[playerPlacement]==G_FOOL))
						{
							goldMine.postNotice("You been tricked its fools gold");
						}
						if((Goldberg->mapya[playerPlacement]==G_GOLD))
						{
							goldMine.postNotice("Run barry you got the real gold");
							Goldberg->mapya[playerPlacement]=myplayer;
						}
					}
					goldMine.drawMap();
				}
				sem_post(mysemaphore);
			}
		}
		else if(button=='j')
		{
			if((playerPlacement+MapCol)<(MapRow*MapCol))
			{
			sem_wait(mysemaphore);
			if(Goldberg->mapya[playerPlacement+MapCol]!=G_WALL)
			{
				Goldberg->mapya[playerPlacement]=0;
				playerPlacement+=MapCol;
				if((Goldberg->mapya[playerPlacement]!=G_FOOL)&&((Goldberg->mapya[playerPlacement]!=G_GOLD)))
				{
					Goldberg->mapya[playerPlacement]=myplayer;
				}
				else
				{
					if((Goldberg->mapya[playerPlacement]==G_FOOL))
					{
						goldMine.postNotice("You been tricked its fools gold");
					}
					if((Goldberg->mapya[playerPlacement]==G_GOLD))
					{
						goldMine.postNotice("Run barry you got the real gold");
						Goldberg->mapya[playerPlacement]=myplayer;
					}
				}
				goldMine.drawMap();
			}
			sem_post(mysemaphore);
		}
		}
	}//while ends here
	sem_wait(mysemaphore);
	Goldberg->mapya[playerPlacement]=0;
	sem_post(mysemaphore);
}
