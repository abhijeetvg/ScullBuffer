/*
 * producer.c
 *
 *  Created on: Dec 1, 2013
 *      Author: abhijeet
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {


	int ret, ret1, ret2,ret3,ret4;
	//char buf[10] = ;
	ret = open("/dev/scullbuffer0", O_WRONLY);

	if (-1 == ret) {
		//TODO err mg
		return -1;
	}
	if (-1 == write(ret, "abhijeetg", 9)) {
		perror("fata");
	}
	
	ret1 = open("/dev/scullbuffer0", O_WRONLY);
	write(ret, "abhi", 4);


        ret2 = open("/dev/scullbuffer0", O_WRONLY);
	write(ret, "Yash", 4);
        
	ret3 = open("/dev/scullbuffer0", O_WRONLY);
	write(ret, "Operating", 9);
        
	ret4 = open("/dev/scullbuffer0", O_WRONLY);

        if(0==write(ret4, "Linux", 5))
	{
	printf("Retuned 0\n"); 
	}
	close(ret); close(ret1); close(ret2); close(ret3); close(ret4);
	return 0;
}

