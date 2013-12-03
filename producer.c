/*
 * producer.c
 *
 *  Created on: Dec 1, 2013
 *      Author: abhijeet and yash
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
	
	int ret, ret1, ret2,ret3,ret4;
	//char buf[10] = ;
	static const char* item1 = "Linux";
	static const char* item2 = "Device";
	static const char* item3 = "Driver";
	static const char* item4 = "by Abhijeet and Yash";
	static const char* item5 = "works well!";

	ret = open("/dev/scullbuffer0", O_WRONLY);
	if (-1 == ret) {
		perror("PRODUCER: Open failed.\n");
		goto fail;
	}
	
	if (-1 == write(ret, item1, 5)) {
		goto fail;
	}
	printf("PRODUCER : Produced item1 : %s\n", item1);
	
	ret1 = open("/dev/scullbuffer0", O_WRONLY);
	if (-1 == ret1 || -1 == write(ret1, item2, 6)) {
		goto fail;
	}
	printf("PRODUCER : Produced item2 : %s\n", item2);

        ret2 = open("/dev/scullbuffer0", O_WRONLY);
	if (-1 == ret2 || -1 == write(ret2, item3, 6)) {
		goto fail;
	}
	printf("PRODUCER : Produced item3 : %s\n", item3);
        
	ret3 = open("/dev/scullbuffer0", O_WRONLY);
	if (-1 == ret3 || -1 == write(ret3, item4, 20)) {
		goto fail;
	}
	printf("PRODUCER : Produced item4 : %s\n", item4);
	sleep(1);
        
	printf("PRODUCER: Not waiting on buffer full any more, as there are no consumers open.\n");

	goto close;
fail:
	printf("PRODUCER: Open or Write failed.\n");

close:
	close(ret1); close(ret2); close(ret3);

	printf("PRODUCER: Producer Sleep 20s called in user prog, will make consumer wait in driver.\n");
	sleep(20);
	
	printf("PRODUCER: Write item, this will unblock consumer which is waiting due to empty buffer.\n");
	if (-1 == write(ret, item5, 11)) {
		printf("PRODUCER: Write failed!\n");
		return -1;
	}
	printf("PRODUCER: Produced item : %s\n", item5);

	close(ret);
	return 0;
}

