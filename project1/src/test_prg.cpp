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
	fd = shm_open("/APJMEMORY",O_RDWR, S_IRUSR | S_IWUSR);
	if(fd==-1){

	mysemaphore= sem_open("/APJgoldchase", O_CREAT|O_EXCL,
			S_IROTH| S_IWOTH| S_IRGRP| S_IWGRP| S_IRUSR| S_IWUSR,1);
	if(mysemaphore==SEM_FAILED)
	{
		if(errno!=EEXIST)
		{
			perror("semaphore error");
			exit(1);
		}
	}
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
		Goldberg= (GameBoard*) mmap(NULL, area, PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);
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
				byte|=G_GOLD;
				Goldberg->mapya[index]=byte;
				}else
				{
					byte|=G_FOOL;
					Goldberg->mapya[index]=byte;
				}
				counter--;
			}
		}
		Map goldMine((char*)(Goldberg->mapya),num_lines,line_length);
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
		sem_unlink("APJgoldchase");

		int a=0;
		goldMine.postNotice("This is a notice");
		while(a=goldMine.getKey()!='Q')
		{
			if(goldMine.getKey()=='h'||goldMine.getKey()=='H')
			{
				sem_wait(mysemaphore);
				if(Goldberg->mapya[player1Placement-1]!=G_WALL)
				{
					Goldberg->mapya[player1Placement]=0;
					player1Placement--;
					Goldberg->mapya[player1Placement]=G_PLR0;
					goldMine.drawMap();
				}
				sem_post(mysemaphore);
			}

		}

	}//here semaphore not failed if ends
}
else{//player 2
	GameBoard* Goldberg2= (GameBoard*) mmap(NULL, sizeof (GameBoard), PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);
		std::uniform_int_distribution<int> rand(1,((Goldberg2->rows)*(Goldberg2->coloumns))); //here
		engi.seed(aj());
		sem_wait(mysemaphore);
		Map goldMine((char*)(Goldberg2->mapya),Goldberg2->rows,Goldberg->coloumns);
		bool loopFlag=true;
		int player2Placement;
		while(loopFlag)
		{
		  player2Placement=rand(engi);
			byte=0;
			if(Goldberg2->mapya[player2Placement]==0)
			{
				byte|=G_PLR1;
				Goldberg2->mapya[player2Placement]=byte;
				loopFlag=false;
				goldMine.drawMap();
			}
		}
		sem_post(mysemaphore);

cerr<<"CoDE for player 2"<<endl;
	}
	sem_close(mysemaphore);
	shm_unlink("/APJMEMORY");
	sem_unlink("APJgoldchase");
}
