#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 128
typedef short int QUANT;

int main(int argc, char* argv[])
{
	int i = 0;
	int line_cnt = 0;
	QUANT buf[BUF_SIZE];

	int fd = open(argv[1], O_RDONLY);
	int size;

	if(fd == -1) {
		perror("open");
		exit(1);
	}

	while(size = read(fd, buf, BUF_SIZE*sizeof(QUANT))) {
		for(i = 0; i < size / sizeof(QUANT); i++) {
			printf("%d %d\n", line_cnt++, buf[i]);
		}
		if(size < BUF_SIZE) break;
	}

	return 0;
}