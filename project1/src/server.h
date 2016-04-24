#ifndef SERVER_H
#define SERVER_H
#include "goldchase.h"
#include "Map.h"
int area;
struct GameBoard
{
	int rows;
	int coloumns;
	int array[5];
	int DaemonID;
	unsigned char mapya[0];
};
sem_t *mysemaphore; //semaphore
GameBoard* GoldBoard=NULL;
int pipefd;

Map* pointer=NULL;//Global Map Pointer
void handle_interrupt(int)
{
	if(pointer)
	{
		pointer->drawMap();
	}
}
#endif
