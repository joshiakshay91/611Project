#ifndef SERVER_H
#define SERVER_H

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
#endif
