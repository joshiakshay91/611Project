#include "goldchase.h"
#include "Map.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <random>
#include <ctime>

using namespace std;
int main()
{
	int counter;
	int num_lines=0;
	int game_length=0;
	int line_length=0;
	std::default_random_engine engi;
	std::random_device aj;


	int flipper;
	string line,text;
	ifstream in("mymap.txt");
	getline(in,line);
	counter=std::atoi(line.c_str());
	while(getline(in, line))
	{
		text += line;
		++num_lines;
		game_length+=line.length();
		line_length=line.length();
	}
	//counter--;
	cerr<<"this is the counter::"<<counter<<endl;
	const char* data = text.c_str();
	int area = line_length*num_lines;
	//  std::cerr<<"The Value of area is here :: "<<area<<std::endl;
	char map[area];
	std::uniform_int_distribution<int> rand(1,area); //here
	engi.seed(aj());

	//char map[]; //something arbitrarily large for this test
	const char* ptr=data;
	char* mp=map;
	//Convert the ASCII bytes into bit fields drawn from goldchase.h
	while(*ptr!='\0')
	{
		flipper=1;
		if(*ptr==' ')      *mp=0;
		else if(*ptr=='*') *mp=G_WALL; //A wall
		else if(*ptr=='1') *mp=G_PLR0; //The first player
		else if(*ptr=='2') *mp=G_PLR1; //The second player
		else if(*ptr=='G') *mp=G_GOLD; //Real gold!
		else if(*ptr=='F') *mp=G_FOOL; //Fool's gold

		++ptr;
		++mp;
	}
 while(counter){
	int placement=rand(engi);
	int placement2=placement;
	const char* ptr2=data;
	mp=map;
	while(placement2){
		ptr2++;
		mp++;
		placement2--;
	}
	if(*ptr2==' ')
	{
		if(counter==1){
			*mp=G_GOLD;
		}else{
		*mp=G_FOOL;}
		counter--;
	}
}

/*	while((counter) && (flipper==1))
	{
//		int Dptr=rand(engi);
	//	if(Dptr==' '){
		*mp=G_FOOL;
		counter--;
		flipper=0;//}
	}*/
	Map goldMine(map,num_lines,line_length);
	int a=0;
	goldMine.postNotice("This is a notice");
	while(a=goldMine.getKey()!='Q')
		;//empty loop
}
