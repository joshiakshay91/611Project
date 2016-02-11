#include "goldchase.h"
#include "Map.h"
#include <fstream>

int main()
{
/*  const char *theMine=
    "** ** *****  *****" //18 characters wide
    "** ** ****** *****"
    "**  F *****  *****"
    "** *******  F   **"
    "** *******   *  **"
    " 2  ****  G  F  **"
    "* *****  *  * ****"
    "*  * *  ** 1  ****"
    "*      ***    ****"
    "**********   *****";
*/
  int count=1;
  int num_lines=0;
  int game_length=0;
  int line_length=0;
   std::string line,text;
   std::ifstream in("mymap.txt");
   while(std::getline(in, line))
   {  if(count==0){
       text += line + "\n";
       ++num_lines;
       game_length+=line.length();
       line_length=line.length();
     }
       count=0;
   }
   const char* data = text.c_str();
  char map[3080]; //something arbitrarily large for this test
  const char* ptr=data;
  char* mp=map;
  //Convert the ASCII bytes into bit fields drawn from goldchase.h
  while(*ptr!='\0')
  {
    if(*ptr==' ')      *mp=0;
    else if(*ptr=='*') *mp=G_WALL; //A wall
    else if(*ptr=='1') *mp=G_PLR0; //The first player
    else if(*ptr=='2') *mp=G_PLR1; //The second player
    else if(*ptr=='G') *mp=G_GOLD; //Real gold!
    else if(*ptr=='F') *mp=G_FOOL; //Fool's gold
    ++ptr;
    ++mp;
  }
  Map goldMine(map,num_lines,line_length);
  int a=0;
  goldMine.postNotice("This is a notice");
  while(a=goldMine.getKey()!='Q')
    ;//empty loop
}
