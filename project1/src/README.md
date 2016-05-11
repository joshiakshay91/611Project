#Goldchase- Two Tin Cans & a string
### This page has all the details regarding the Rubric.
* Compiling- The code compiles with std=c++11 and c++14 I have used c++14 in Makefile

* Running- The code runs as just the game on one machine as well as on two machine with the first machine as server and second machine as client. Usage the client side needs to put the IP address as an argument to connect to the server side, each time the client side wants to make a connection it has to carry the argument. The player has to type in his/her name for the player spot. The game works exactly as project 2, things covered- players on both sides can move and grab fools gold or real gold. The players on both server side and client side can communicate with each other by pressing m for message and b for broadcasting a message. The shared memory, semaphore and mq_ are deleted once player from both sides quit and the connection is no more.

* daemonID init: Both server and client (whichever is running) initialize the DaemonID inside GameBoard struct on line: 157(server.cpp) and on line 205 (client.cpp)

* send SIGHUP: the game test_prg.cpp sends SIGHUP while joining and leaving the game. on line 415 of (test_prg.cpp) the game sends SIGHUP signal when player joins and gets the PLR number, while leaving the game either as player one or other player the game sends SIGHUP while leaving on respective lines 377 and 386 of test_prg.cpp

* socket INIT:
