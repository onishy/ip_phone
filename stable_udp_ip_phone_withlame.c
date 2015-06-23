#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <lame/lame.h>

#define PACKET_SIZE 1000

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
	if((snd_in = popen("rec -t raw -b 16 -c 2 -e s -r 44100 -", "r")) == NULL) {
		fprintf(stderr, "could not open rec\n");
		exit(1);
	}
	if((snd_out = popen("play -t mp3 -b 16 -c 2 -e s -r 44100 -", "w")) == NULL) {
		fprintf(stderr, "could not open play\n");
		exit(1);
	}

	int n;
	short int data[PACKET_SIZE * 2];
	unsigned char mp3_buffer[PACKET_SIZE];

	lame_t lame = lame_init();
	lame_set_in_samplerate(lame, 44100);
	lame_set_VBR(lame, vbr_default);
	lame_init_params(lame);

	while(1) {
		n = fread(data, 2* sizeof(short int), PACKET_SIZE, snd_in);
		printf("writing %d bytes\n", n);
		if(n > 0) {
			int mp3_n = lame_encode_buffer_interleaved(lame, data, n, mp3_buffer, PACKET_SIZE);

			sendto(s, mp3_buffer, mp3_n, 0, (struct sockaddr *)&addr, addrlen);
		}
		else break;

		n = recvfrom(s, data, PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
		printf("received %d bytes\n", n);
		if(n > 0) {
			fwrite(data, 1, n, snd_out);
		}
		else break;
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