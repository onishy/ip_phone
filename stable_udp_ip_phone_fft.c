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
#define SPECTRUM_WIDTH 2
#define SPECTRUM_N 10
#define THRESHOLD_A 50
#define MINIMUM_F 100
#define MAXIMUM_F 500

#define DATASIZE SPECTRUM_WIDTH * SPECTRUM_N*2 * 2

typedef struct {
	uint16_t base_freq;
	complex double data[DATASIZE];
} voice_data_t;

// int to_freq(int index, int sample_n, int sampling_freq)
// {
// 	return index*n/sampling_freq;
// }

int find_base_freq(complex double *data, int n, int min_f, int max_f, double threshold)
{
	int max = (max_f < n) ? max_f : n;
	int i;
	int current_max_a = 0;
	int current_max_f = 0;

	int result = 0;
	int peak_cnt = 0;
	if(min_f >= n) return -1;
	printf("peak: {");
    for(i = min_f; i < max+1; i++){
     	if(cabs(data[i]) > threshold) {
     		if(result == 0) result = i * 44100 / n;
     		printf("%d, ", i * 44100 / n);
     		peak_cnt++;
//    		return i * 44100 / n;
			// current_max_f = i * 44100 / n;
			// current_max_a = cabs(data[i]);
    	}
    }
    printf("}\n");
    return result;
    // return -1;
    // return current_max_f;
}

void cut(complex double *y, complex double *compressed, int compressed_len, long n, long pitch, long width) {
	long i;
	long band_id = 1;
	long cnt = 0;
	
//	printf("cut begin\n");
	printf("n=%ld\n", n);
	for(band_id = 1; band_id <= SPECTRUM_N; band_id++) {
		for(i = 0; i < width*2; i++) {
			compressed[cnt++] = y[(band_id*pitch - width + i)*n/44100];
			compressed[cnt++] = y[n - 1 - (band_id*pitch - width + i)*n/44100];

			if(cnt >= compressed_len) {
				printf("cut: cnt over band=%ld, i=%ld\n", band_id, i);
				return;
			}
			if((band_id * pitch - width + i)*n/44100 >= n/2) return;
		}

	}


// 		for(i=0+(long)(pitch*j)*n/44100; i<(long)(pitch*(1+j))*n/44100; i++){
// 	    	if(i <= (long)(pitch*j)*n/44100+band || i >= (long)(pitch*(1+j))*n/44100-band){
// 	    		compressed[cnt++] = y[i];
// 				if(cnt >= compressed_len) return;

// 	    		compressed[cnt++] = y[n-i-1];
// 		      	// y[i]=0;
// 		      	// y[n-i]=0;
// //		      	printf("i=%ld\n", i);
// 				if(cnt >= compressed_len) {
// 					printf("cut: cnt over band=%ld, i=%ld\n", j, i);
// 					return;
// 				}
// 				if(i >= PACKET_SIZE) break;
// 		    }
// 	    }
// //      	printf("j=%ld\n", j);
// 		j++;
//   	}
//	printf("cut end\n");
}

void parse(complex double *compressed, complex double *recovered, int compressed_len, long n, uint16_t pitch, long width) {
	long i;
	long band_id = 0;
	long cnt = 0;

	for(band_id = 1; ; band_id++) {
		for(i = 0; i < width*2; i++) {
			recovered[(band_id * pitch - width+i)*n/44100] = compressed[cnt++];
			recovered[n - 1 -((band_id * pitch - width + i)*n/44100)] = compressed[cnt++];

			if(cnt >= compressed_len) {
				printf("parse: cnt over band=%ld, i=%ld\n", band_id, i);
				return;
			}
			if((band_id * pitch - width + i)*n/44100 >= n/2) return;
		}
	}
	return;
}


int main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("Error: too few arguments.\n");
		exit(1);
	}

	int addrlen;
	int s;
	struct sockaddr_in addr;
	int la_flag = atoi(argv[1]);
	if(la_flag) {
		printf("la flag on\n");
	}
	if(argc == 3) {
		// Server mode
		printf("Server Mode\n");

		struct sockaddr_in tmp_addr;

		s = socket(PF_INET, SOCK_DGRAM, 0);

		tmp_addr.sin_family = AF_INET;
		tmp_addr.sin_port = htons(atoi(argv[2]));
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
	} else if(argc == 4) {
		// Client mode
		printf("Client Mode\n");
		s = socket(PF_INET, SOCK_DGRAM, 0);

		addr.sin_family = AF_INET;
		if(inet_aton(argv[3], &addr.sin_addr) == 0) {
			perror("inet_aton\n");
			exit(1);
		}
		addr.sin_port = htons(atoi(argv[2]));

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

	voice_data_t *voice;

	FILE* send_fp = fopen("send_log.txt", "w");
	FILE* recv_fp = fopen("receive_log.txt", "w");
	while(1) {
		Y = calloc(sizeof(complex double), PACKET_SIZE);
		voice = calloc(sizeof(voice_data_t), 1);

		n = fread(data, 1, PACKET_SIZE * sizeof(sample_t), snd_in);
		printf("read_n %d\n", n);

	    /* 複素数の配列に変換 */
	    sample_to_complex(data, X, PACKET_SIZE);
	    /* FFT -> Y */
	    fft(X, Y, PACKET_SIZE);

	    int base = find_base_freq(Y, PACKET_SIZE, (PACKET_SIZE*MINIMUM_F)/44100, (PACKET_SIZE*MAXIMUM_F)/44100, THRESHOLD_A);
	    fprintf(stderr, "base = %d\n", base);


	    if(base <= 0) continue;
	    voice->base_freq = base;
	    cut(Y, voice->data, DATASIZE, PACKET_SIZE, base, SPECTRUM_WIDTH);

		printf("writing %ld bytes\n", sizeof(voice_data_t));
		if(n > 0) {
			sendto(s, (unsigned char*)voice, sizeof(voice_data_t), 0, (struct sockaddr *)&addr, addrlen);
			fwrite((unsigned char*)voice, 1, sizeof(voice_data_t), send_fp);
		}
		// else break;
		free(Y);

		Y = calloc(sizeof(complex double), PACKET_SIZE);
		n = recvfrom(s, (unsigned char*)voice, sizeof(voice_data_t), 0, (struct sockaddr *)&addr, &addrlen);
		printf("received %d bytes\n", n);
		fwrite(voice, 1, sizeof(voice_data_t), recv_fp);

		if(n > 0) {
			printf("received base = %d\n", voice->base_freq);

			uint16_t voice_base;
			if(la_flag) {
				voice_base = 220;
			} else {
				voice_base = voice->base_freq;
			}
			parse(voice->data, Y, DATASIZE, PACKET_SIZE, voice_base, SPECTRUM_WIDTH);
			ifft(Y, X, PACKET_SIZE);
			complex_to_sample(X, data, PACKET_SIZE);
			fwrite(data, 1, PACKET_SIZE * sizeof(sample_t), snd_out);
		}
		//else break;
		free(Y);
		free(voice);
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