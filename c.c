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

	int ret,r=0,i=0;
	char buf[512];
	ret = open("/dev/scullbuffer0", O_RDONLY);

	if (-1 == ret) {
		//TODO err mg
		return -1;
	}

while (1) {
	r =  read(ret, &buf, 512);
        buf[r] = '\0';
	if (-1 == r) {
		perror("fata");
	}

	if (r) {
	//	i = 0;
	//	i++;
	//}

	printf("R %d : \n ", r);
	buf[r] = '\0';
	printf("Buffer: %s\n", buf);
}
}//
	//printf("Buffer is %s: \n ", buf);
	//write(fp, &buf, sizeof(buf));
	//write(fp, &buf, sizeof(buf));

	//printf("%d\n", ret);
	close(ret);
}

