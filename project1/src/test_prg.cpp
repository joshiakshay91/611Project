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
	GameBoard* GoldBoard;
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
		GoldBoard= (GameBoard*) mmap(NULL, area+sizeof(GameBoard), PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);
		GoldBoard->rows=num_lines;
		GoldBoard->coloumns=line_length;
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
		char myplayer=G_PLR0;
		GoldBoard->players=myplayer;
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
			movement(GoldBoard,player1Placement,goldMine,myplayer,mysemaphore);
		}catch(std::runtime_error& e){
			sem_post(mysemaphore);
			GoldBoard->players &= ~myplayer;
			if(lastManStatus(GoldBoard))
			{
				sem_close(mysemaphore);
				shm_unlink("/APJMEMORY");
				sem_unlink("APJgoldchase");
			}
		}
		GoldBoard->players &= ~myplayer;
		if(lastManStatus(GoldBoard))
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
		GameBoard* GoldBoard= (GameBoard*)mmap(NULL, player2rows*player2col+sizeof(GameBoard),
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		GoldBoard->rows=player2rows;
		GoldBoard->coloumns=player2col;
		if(!(GoldBoard->players & G_PLR0))
		{
			currentPlayer=G_PLR0;
			GoldBoard->players|=currentPlayer;
		}
		else if(!(GoldBoard->players & G_PLR1))
		{
			currentPlayer=G_PLR1;
			GoldBoard->players|=currentPlayer;
		}
		else if(!(GoldBoard->players & G_PLR2))
		{
			currentPlayer=G_PLR2;
			GoldBoard->players|=currentPlayer;
		}
		else if(!(GoldBoard->players & G_PLR3))
		{
			currentPlayer=G_PLR3;
			GoldBoard->players|=currentPlayer;
		}
		else if(!(GoldBoard->players & G_PLR4))
		{
			currentPlayer=G_PLR4;
			GoldBoard->players|=currentPlayer;
		}
		else
		{
			sem_post(mysemaphore);
			cout<<"We are currently 5 player game Get out"<<endl;
			exit (0);
		}
		try{
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
					goldMine.drawMap();
				}
			}
			sem_post(mysemaphore);
			movement(GoldBoard,player2Placement,goldMine,currentPlayer,mysemaphore);
		}catch(std::runtime_error& e){
			sem_post(mysemaphore);
			GoldBoard->players &= ~currentPlayer;
			if(lastManStatus(GoldBoard))
			{
				sem_close(mysemaphore);
				shm_unlink("/APJMEMORY");
				sem_unlink("APJgoldchase");
			}
		}
		GoldBoard->players &= ~currentPlayer;
		if(lastManStatus(GoldBoard))
		{
			sem_close(mysemaphore);
			shm_unlink("/APJMEMORY");
			sem_unlink("APJgoldchase");
		}
	}
}

bool lastManStatus(GameBoard* GoldBoard)
{

	if((!(GoldBoard->players & G_PLR0)) && (!(GoldBoard->players & G_PLR1)) && (!(GoldBoard->players & G_PLR2)) && (!(GoldBoard->players & G_PLR3)) && (!(GoldBoard->players & G_PLR4)))
	{
		return true;
	}
	return false;
}

void movement(GameBoard* GoldBoard,int playerPlacement,Map goldMine,char myplayer, sem_t* mysemaphore)
{
	bool GoldFlag=false;
	int MapCol=GoldBoard->coloumns;
	int MapRow=GoldBoard->rows;
	char button='m'; //just a garbage
	goldMine.postNotice("This is a notice");
	while(button!='Q')
	{
		button=goldMine.getKey();
		if(button=='h')
		{
			if(((playerPlacement)%(MapCol))!=0)
			{
				if(GoldBoard->mapya[playerPlacement-1]!=G_WALL)
				{
					sem_wait(mysemaphore);
					GoldBoard->mapya[playerPlacement]&=~myplayer;
					playerPlacement--;
					if((GoldBoard->mapya[playerPlacement]!=G_FOOL)&&((GoldBoard->mapya[playerPlacement]!=G_GOLD)))
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
							GoldBoard->mapya[playerPlacement]|=myplayer;
							sem_post(mysemaphore);
							goldMine.postNotice("You got REAL GOLD make your escape!!!");
							GoldFlag=true;
						}
					}
					goldMine.drawMap();
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
					sem_wait(mysemaphore);
					GoldBoard->mapya[playerPlacement]&=~myplayer;
					playerPlacement++;
					if((GoldBoard->mapya[playerPlacement]!=G_FOOL)&&((GoldBoard->mapya[playerPlacement]!=G_GOLD)))
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
							GoldFlag=true;
							GoldBoard->mapya[playerPlacement]=myplayer;
							sem_post(mysemaphore);
							goldMine.postNotice("You got REAL GOLD make your escape!!!");
						}
					}
					goldMine.drawMap();
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
					sem_wait(mysemaphore);
					GoldBoard->mapya[playerPlacement]&=~myplayer;
					playerPlacement-=MapCol;
					if((GoldBoard->mapya[playerPlacement]!=G_FOOL)&&((GoldBoard->mapya[playerPlacement]!=G_GOLD)))
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
					goldMine.drawMap();
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
					sem_wait(mysemaphore);
					GoldBoard->mapya[playerPlacement]&=~myplayer;
					playerPlacement+=MapCol;
					if((GoldBoard->mapya[playerPlacement]!=G_FOOL)&&((GoldBoard->mapya[playerPlacement]!=G_GOLD)))
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
					goldMine.drawMap();
				}
			}
			else if(GoldFlag)
			{
				button='Q';
				goldMine.postNotice("You Won");
			}
		}
	}//while ends here
	sem_wait(mysemaphore);
	GoldBoard->mapya[playerPlacement]&=~myplayer;
	sem_post(mysemaphore);
}
