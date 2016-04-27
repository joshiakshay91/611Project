#ifndef fancyRW_h
#define fancyRW_h
#include<unistd.h>
#include <errno.h>
template<typename T>
int READ(int fd, T* obj_ptr, int count)
{
	int err=0;
	char* addr=(char*)obj_ptr;
again:
		err=read(fd,addr,count);
		if(err<0 && errno==EINTR)
		{
			goto again;
		}
		else if(err==-1)
		{
			return err;
		}
	return err;//success on reading
	//loop. Read repeatedly until count bytes are read in
}

template<typename T>
int WRITE(int fd, T* obj_ptr, int count)
{
	int originalC=count;
	int err=0;
	char* addr=(char*)obj_ptr;
	while(count)
	{
		err=write(fd,addr,count);
		if(err==-1 && errno==EINTR)
		{
			count=originalC;
			continue;
		}
		 if(err== -1)
		{
			return err;
		}
		count-=err;
	}
	return err;
}





void server_function();
void client_function();
#endif
