#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>



#define BUFSIZE 512


int main(int argc, char *argv[])
{
	char *dev;			/* The device to sniff on */
	char buf[BUFSIZE]="";	/* The filter expression */

	int fd=3;
	int a=0;
	int n=0;
       if (argc < 3) {
           fprintf(stderr, "Usage: %s file int_val string\n", argv[0]);
           exit(EXIT_FAILURE);
       } 

	a=atoi(argv[2]);

       if( (fd=open(argv[1], O_RDWR | O_CREAT, 0666)) == -1 ){
		perror("open error");
		exit(-1);
	}

	if( (n=read(fd, buf, BUFSIZE)) == -1){
		perror("read #1 error");
		exit(-1);
	}
	buf[n]=0;
	printf("Read %d characters, string: %s \n", n, buf);
	fflush(stdout);
	if( write(fd, &a, sizeof(a)) ==  -1 ){
		perror("write #1 error");
		exit(-1);
	}
	if( write(fd, argv[3], strlen(argv[3])) == -1) {
		perror("write #2 error");
		exit(-1);
	}
	close(fd);
	
}
