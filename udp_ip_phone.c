#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sox.h>
#include <stdint.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define PACKET_SIZE 48000 * 2 * 3

int client_connect(char *str_addr, int port, struct sockaddr_in *addr, int *addrlen);
int server_connect(int port, struct sockaddr_in *addr, int *addrlen);

int main(int argc, char *argv[])
{
	printf("Welcome.\n");
	if(argc < 2) {
		printf("Error: too few arguments.\n");
		exit(1);
	}

	int addrlen;
	int s;
	struct sockaddr_in addr;

	if(argc == 3) {
	  // Client mode
	  printf("Client Mode\n");
	  client_connect(argv[2], atoi(argv[1]), &addr, &addrlen);
	} else if(argc == 2) {
	  // Server mode
	  printf("Server Mode\n");
	  server_connect(atoi(argv[1]), &addr, &addrlen);
	}
	printf("connection established.\n");

	sox_format_t *ft = sox_open_read("default", 0, 0, "pulseaudio");
	sox_init();

	int n;
	int32_t data[PACKET_SIZE];
	while(1) {
		n = sox_read(ft, data, PACKET_SIZE);
		printf("writing %d bytes\n", n);
		if(n > 0) {
			sendto(s, data, n, 0, (struct sockaddr *)&addr, addrlen);
		}
		else break;

		n = recvfrom(s, data, PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
		printf("received %d bytes\n", n);
		if(n > 0) {
			sox_write(ft, data, n);
		}
		else break;
	}
	printf("done.\n");

	sox_close(ft);
	sox_quit();
	// close(snd_in);
	// close(snd_out);

	close(s);
//	shutdown(s, SHUT_WR);
//	shutdown(ss, SHUT_WR);

	return 0;
}

int server_connect(int port, struct sockaddr_in *addr, int *addrlen)
{
    struct sockaddr_in tmp_addr;

    int s = socket(PF_INET, SOCK_DGRAM, 0);

    tmp_addr.sin_family = AF_INET;
    tmp_addr.sin_port = htons(port);
    tmp_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(s, (struct sockaddr *)&tmp_addr, sizeof(tmp_addr)) < 0){
        perror("bind failed\n");
	exit(1);
    }
    printf("Binded Port: %d\n", s);

    unsigned char data = 0;

    while(data != 2) {
        recvfrom(s, &data, 1, 0, (struct sockaddr *)addr, addrlen);
        printf("%d\n", data);
        sleep(1);
        printf("wait.\n");
    }

    data = 3;
    sendto(s, &data, 1, 0, (struct sockaddr *)addr, *addrlen);

    return s;
}

int client_connect(char *str_addr, int port, struct sockaddr_in *addr, int *addrlen)
{
    // Client mode
    int s = socket(PF_INET, SOCK_DGRAM, 0);

    addr->sin_family = AF_INET;
    if(inet_aton(str_addr, &addr->sin_addr) == 0) {
    perror("inet_aton\n");
    exit(1);
    }
    addr->sin_port = htons(port);

    *addrlen = sizeof(struct sockaddr_in);

    unsigned char pk = 2;
    sendto(s, &pk, 1, 0, (struct sockaddr *)addr, *addrlen);

    while(pk != 3) {
    	recvfrom(s, &pk, 1, 0, (struct sockaddr *)addr, addrlen);
        printf("%d\n", pk);
    }

    return s;
}
