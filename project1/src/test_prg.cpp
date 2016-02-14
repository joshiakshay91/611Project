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
struct GameBoard{
	int rows;
	int coloumns;
	unsigned char players;
	unsigned char map[0];
}

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

		int fd= shm_open("/AJGoldmem", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if(fd=-1)
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
		std::uniform_int_distribution<int> rand(1,area); //here
		engi.seed(aj());

		//char map[]; //something arbitrarily large for this test
		const char* ptr=data;
		char* mp=map;
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
				if(counter==1){
					*mp=G_GOLD;
				}else{
					*mp=G_FOOL;}
				counter--;
			}
		}
		Map goldMine(map,num_lines,line_length);
		int a=0;
		goldMine.postNotice("This is a notice");
		while(a=goldMine.getKey()!='Q'){}
		sem_post(mysemaphore);
		sem_close(mysemaphore);
		sem_unlink("TAGgoldchase");
	}//here semaphore not failed if ends

}
