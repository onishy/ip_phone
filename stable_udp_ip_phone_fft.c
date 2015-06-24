#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include "fft.h"

#define PACKET_SIZE 2048

int main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("Error: too few arguments.\n");
		exit(1);
	}

	int addrlen;
	int s;
	struct sockaddr_in addr;
	if(argc == 2) {
		// Server mode
		printf("Server Mode\n");

		struct sockaddr_in tmp_addr;

		s = socket(PF_INET, SOCK_DGRAM, 0);

		tmp_addr.sin_family = AF_INET;
		tmp_addr.sin_port = htons(atoi(argv[1]));
		tmp_addr.sin_addr.s_addr = INADDR_ANY;
		
		if(bind(s, (struct sockaddr *)&tmp_addr, sizeof(tmp_addr)) < 0) {
			perror("bind failed\n");
			exit(1);
		}

		printf("Binded Port: %d\n", s);

		int data = 0;

		while(data != 2) {
			recvfrom(s, &data, 1, 0, (struct sockaddr *)&addr, &addrlen);
		}

//		sleep(1);
		data = 3;
		int cnt = 0;
		for(cnt = 0; cnt < 100; cnt++) {
			sendto(s, &data, 1, 0, (struct sockaddr *)&addr, addrlen);
		}
	} else if(argc == 3) {
		// Client mode
		printf("Client Mode\n");
		s = socket(PF_INET, SOCK_DGRAM, 0);

		addr.sin_family = AF_INET;
		if(inet_aton(argv[2], &addr.sin_addr) == 0) {
			perror("inet_aton\n");
			exit(1);
		}
		addr.sin_port = htons(atoi(argv[1]));

		addrlen = sizeof(addr);

		unsigned int pk = 2;
		sendto(s, &pk, 1, 0, (struct sockaddr *)&addr, addrlen);

		// while(pk != 3) {
		// 	read(s, &pk, 1);
		// }
	}
	printf("connection established.\n");

	FILE *snd_in;
	FILE *snd_out;
	if((snd_in = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r")) == NULL) {
		fprintf(stderr, "could not open rec\n");
		exit(1);
	}
	if((snd_out = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w")) == NULL) {
		fprintf(stderr, "could not open play\n");
		exit(1);
	}

	int n;
	sample_t data[PACKET_SIZE];
	
	complex double * X = calloc(sizeof(complex double), PACKET_SIZE);
	complex double * Y = calloc(sizeof(complex double), PACKET_SIZE);

	FILE* send_fp = fopen("send_log.txt", "w");
	FILE* recv_fp = fopen("receive_log.txt", "w");
	while(1) {
		n = fread(data, 1, PACKET_SIZE * sizeof(sample_t), snd_in);
		printf("read_n %d\n", n);

	    /* 複素数の配列に変換 */
	    sample_to_complex(data, X, PACKET_SIZE);
	    /* FFT -> Y */
	    fft(X, Y, PACKET_SIZE);

		printf("writing %d bytes\n", n);
		if(n > 0) {
			sendto(s, (unsigned char*)Y, PACKET_SIZE * sizeof(complex double), 0, (struct sockaddr *)&addr, addrlen);
			fwrite((unsigned char*)Y, 1, PACKET_SIZE * sizeof(complex double), send_fp);
		}
		//else break;

		n = recvfrom(s, (unsigned char*)Y, PACKET_SIZE * sizeof(complex double), 0, (struct sockaddr *)&addr, &addrlen);
		n /= sizeof(complex double);
		fwrite(Y, 1, PACKET_SIZE * sizeof(complex double), recv_fp);
		printf("received %d bytes\n", n);
		if(n > 0) {
			ifft(Y, X, PACKET_SIZE);
			complex_to_sample(X, data, PACKET_SIZE);
			fwrite(data, 1, n * sizeof(sample_t), snd_out);
		}
		//else break;
	}
	printf("done.\n");

	pclose(snd_in);
	pclose(snd_out);

	// close(snd_in);
	// close(snd_out);

	close(s);
//	shutdown(s, SHUT_WR);
//	shutdown(ss, SHUT_WR);

	return 0;
}