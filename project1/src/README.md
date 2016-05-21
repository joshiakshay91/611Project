#Goldchase- Two Tin Cans & a string
### This page has all the details regarding the Rubric.
* Compiling- The code compiles with std=c++11 and c++14 I have used c++14 in Makefile

* Running- The code runs as just the game on one machine as well as on two machine with the first machine as server and second machine as client. Usage the client side needs to put the IP address as an argument to connect to the server side, each time the client side wants to make a connection it has to carry the argument. The player has to type in his/her name for the player spot. The game works exactly as project 2, things covered- players on both sides can move and grab fools gold or real gold. The players on both server side and client side can communicate with each other by pressing m for message and b for broadcasting a message. The shared memory, semaphore and mq_ are deleted once player from both sides quit and the connection is no more.

* daemonID init: Both server and client (whichever is running) initialize the DaemonID inside GameBoard struct on line: 157(server.cpp) and on line 205 (client.cpp)

* send SIGHUP: the game test_prg.cpp sends SIGHUP while joining and leaving the game. on line 415 of (test_prg.cpp) the game sends SIGHUP signal when player joins and gets the PLR number, while leaving the game either as player one or other player the game sends SIGHUP while leaving on respective lines 377 and 386 of test_prg.cpp

* socket init: Socket initialization happens inside the daemons right after the daemon gets divorced from its grandparent process, in server.cpp the socket initialization starts right at line 171. For client daemon the initialization starts at line 148. On line number 219-221 in server.cpp 3 WRITEs are made for sending rows, coloumns and map. On line 176-178 of client.cpp 3 READs are done respective to the writes.

* Trap SIGHUP: The interrupt handler in server.cpp is called Sother_interrupt(int SigNo), and the one in client.cpp is called Clientother_interrupt(int SigNo), these functions trap multiple signals for handling Signal number 1 SIGHUP the respective function's code is in server.cpp at line 78 and for client.cpp at line 81.

* Process "Socket Player": In server.cpp I mark the daemonID in the array for adding new player and method called for setup of mq_ for the player joined on the remote machine the codes are on line 260 and 261 of server.cpp and the for loop starts at line 254-271 of server.cpp . Similarly in client these things are done at line number 254-270 in client.cpp the same for loop has the code for removing player from array and method call for removing the mq_

* Trap SIGUSR2: Like project 2 the mq_ SIGUSR2 is trapped by the function ReadMessageS(int) it is setup by the method QueueSetupS in server.cpp both the functions are present on lines 327 and 370 of server.cpp respectively. In similar fashion ReadMessageR(int) traps SIGUSR2 for client and QueueSetupR are sets the sigaction on lines 325 and 366 respectively in client.cpp

* Process "Socket Message": In server.cpp on line 286-321 I get the G_SOCKMSG look for whom it is intended and write the received string in that player's mq_ so it automatically triggers in that player's game. Similarly in client.cpp same thing happens from line 284-319

* Trap SIGUSR1: In server.cpp SIGUSR1 is trapped in from line 53-77 inside the Sother_interrupt, WRITES on socket are made in the same signal handler function, similarly in client.cpp 54-80 has the same functionality. The while(1) in both files have the code that reads the changes and updates the shm

* Process "Socket Map": Inside the while(1) in server.cpp lines 237-250 reads the changes for map refresh and update shm, in client.cpp line 236-249 do the same thing.

* Daemon integration: The whole code runs on Daemon functions in server.cpp line 108 and in client.cpp line 106 start the daemon. The daemon methods are called from test_prg.cpp from line 127-140 at the start of int main().


#####install library for using ncurses sudo apt-get install libncurses5-dev ncurses-doc
