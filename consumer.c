/*
 * producer.c
 *
 *  Created on: Dec 1, 2013
 *      Author: abhijeet and Yash
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {

	int ret,r=1,i=0;
	char buf[512];

	ret = open("/dev/scullbuffer0", O_RDONLY);
	printf("\nCONSUMER: Consumer Sleep 10s called in user prog, will make producer wait in driver.\n");
	sleep(10);
	if (-1 == ret) {
		perror("CONSUMER: OPEN failed.\n");
		return -1;
	}

	printf("CONSUMER: Read all items, will unblock producer which is blocked due to buffer full.\n");

	r =  read(ret, &buf, 512);
	while (r) {
        	buf[r] = '\0';

	        if (-1 == r) {
        	        perror("CONSUMER: READ Failed.\n");
			return -1;
        	}
 
		printf("CONSUMER : Buffer item consumed - %s\n", buf);
		if (i == 2) {
			sleep(1);
		}
		r =  read(ret, &buf, 512);
		i++;
		//sleep(1);
	}//

	//r = read(ret, &buf, 512);
	if (-1 == r) {
		perror("CONSUMER: READ Failed.\n");
	}
	//buf[r] = '\0';

	//printf("CONSUMER: Buffer item consumed - %s\n", buf);
	printf("CONSUMER: Not waiting any more on empty buffer as there are no open producers.\n");
	printf("DONE!! Hit enter!\n");

	close(ret);
}

