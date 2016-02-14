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
	int num_lines=0;
	int line_length=0;
	std::default_random_engine engi;
	std::random_device aj;
	string line,text;

	sem_t *mysemaphore;
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

		int fd = shm_open("/APJMEMORY", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
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
		char map[area];

		if((ftruncate(fd,(area + sizeof(GameBoard))))==-1)
		{
			perror("FTRUNCATE FAILURE :");
			exit(1);
		}
		GameBoard* Goldberg= (GameBoard*) mmap(NULL, area, PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);
  //  Goldberg->mapya=text.c_str();



		std::uniform_int_distribution<int> rand(1,area); //here
		engi.seed(aj());
		counter++;
		//char map[]; //something arbitrarily large for this test
		const char* ptr=data;
		char* mp=map;
		char* PL1=map;
		//Convert the ASCII bytes into bit fields drawn from goldchase.h
		while(*ptr!='\0')
		{
			if(*ptr==' ')      *mp=0;
			else if(*ptr=='*') *mp=G_WALL; //A wall
			++ptr;
			++mp;
		}
		while(counter){
			int placement=rand(engi);
			int placement2=placement;
			ptr=data;
			mp=map;
			while(placement2){
				ptr++;
				mp++;
				placement2--;
			}
			if(*ptr==' ')
			{
				if(counter<=2){
					if(counter==1){
					*mp=G_GOLD;}
					else{
							*mp=G_PLR0;
							PL1=mp;
					}
				}else{
					*mp=G_FOOL;}
				counter--;
			}
		}
		Map goldMine(map,num_lines,line_length);
		int a=0;
		goldMine.postNotice("This is a notice");
		while(a=goldMine.getKey()!='Q'){
     if(goldMine.getKey()=='H'||goldMine.getKey()=='h')
		 {
			 mp=map;
			 mp=PL1;
			 mp--;
			 if(*mp!=G_WALL){
				 mp++;
				 *mp=0;
				 mp--;
				 PL1=mp;
				 *mp=G_PLR0;
			 }
			 
			 goldMine.drawMap();
		 }
		 else if(goldMine.getKey()=='K'|| goldMine.getKey()=='k')
		 {
			 mp=map;
			 mp=PL1;
			 mp++;
			 if(*mp!=G_WALL){
				 mp--;
				 *mp=0;
				 mp++;
				 PL1=mp;
				 *mp=G_PLR0;
			 }
			 goldMine.drawMap();
		 }
		 else if(goldMine.getKey()=='J'|| goldMine.getKey()=='j')
		 {
			 mp=map;
			 mp=PL1;
			 mp-=line_length;
			 if(*mp!=G_WALL){
				 mp+=line_length;
				 *mp=0;
				 mp-=line_length;
				 PL1=mp;
				 *mp=G_PLR0;
			 }
			 goldMine.drawMap();
		 }
		 else if(goldMine.getKey()=='L'|| goldMine.getKey()=='l')
		 {
			 mp=map;
			 mp=PL1;
			 mp+=line_length;
			 if(*mp!=G_WALL){
				 mp=mp-line_length;
				 *mp=0;
				 mp+=line_length;
				 PL1=mp;
				 *mp=G_PLR0;
			 }
			 goldMine.drawMap();
		 }

		}
		sem_post(mysemaphore);
		sem_close(mysemaphore);
		shm_unlink("/APJMEMORY");
		sem_unlink("APJgoldchase");
	}//here semaphore not failed if ends

}
