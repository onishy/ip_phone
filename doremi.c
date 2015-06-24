#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DATA_PER_SOUND 13230

typedef short int QUANT;

int main(int argc, char *argv[])
{
	if(argc != 3) {
		printf("Invalid number of inputs!\n");
		exit(1);
	}

	const double SAMPLING_F = 44100;
	const double SF = pow(2.0, 1.0/6.0);
	const double HALF_SF = pow(2.0, 1.0/12.0);
	const double DO = 440 * SF * HALF_SF;

	int A = atoi(argv[1]);
	int n = atoi(argv[2]);

	double f = DO;

	int i = 0;
	int j = 0;
	QUANT data;

	double sec = 0;

	for(j = 0; j < n; j++) {
		int k = 0;
		for(k = 0 ;k < DATA_PER_SOUND; k++) {
			data = (QUANT)(A*sin(2*M_PI*f*k/SAMPLING_F));
			write(1, &data, sizeof(QUANT));
		}
		f *= SF;
		if(j % 7 == 2) f /= HALF_SF;
		if(j % 7 == 6) f /= HALF_SF;
	}

}