#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 128
typedef float QUANT;

int main(int argc, char* argv[])
{
	int i = 0;
	int line_cnt = 0;
	QUANT buf[BUF_SIZE];

	int fd = open(argv[1], O_WRONLY);
	int size;

	if(fd == -1) {
		perror("open");
		exit(1);
	}

	float n;
	n = 44100 / 659.25;
	for(i = 0; i < 100; i++) {
		write(fd, &n, sizeof(float));
	}
	n = 44100 / 739.98;
	for(i = 0; i < 100; i++) {
		write(fd, &n, sizeof(float));
	}
	n = 44100 / 830.60;
	for(i = 0; i < 300; i++) {
		write(fd, &n, sizeof(float));
	}


	return 0;
}