#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>

#define PACKET_SIZE 1000

/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt)
{
const enum AVSampleFormat *p = codec->sample_fmts;
while (*p != AV_SAMPLE_FMT_NONE) {
if (*p == sample_fmt)
return 1;
p++;
}
return 0;
}
/* just pick the highest supported samplerate */
static int select_sample_rate(AVCodec *codec)
{
const int *p;
int best_samplerate = 0;
if (!codec->supported_samplerates)
return 44100;
p = codec->supported_samplerates;
while (*p) {
best_samplerate = FFMAX(*p, best_samplerate);
p++;
}
return best_samplerate;
}
/* select layout with the highest channel count */
static int select_channel_layout(AVCodec *codec)
{
const uint64_t *p;
uint64_t best_ch_layout = 0;
int best_nb_channels = 0;
if (!codec->channel_layouts)
return AV_CH_LAYOUT_STEREO;
p = codec->channel_layouts;
while (*p) {
int nb_channels = av_get_channel_layout_nb_channels(*p);
if (nb_channels > best_nb_channels) {
best_ch_layout = *p;
best_nb_channels = nb_channels;
}
p++;
}
return best_ch_layout;
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

	AVCodec *codec;
	AVCodecContext *c = NULL;
	AVFrame *frame;
	AVPacket pkt;
	uint16_t *samples;
	int got_output;

	codec = avcodec_find_encoder(AV_CODEC_ID_MP2);
	fprintf(stderr, "codec=%p\n", codec);
	if(!codec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	c->bit_rate = 48000;
	c->sample_fmt = AV_SAMPLE_FMT_S16;
	 if (!check_sample_fmt(codec, c->sample_fmt)) {
		fprintf(stderr, "Encoder does not support sample format %s",
		av_get_sample_fmt_name(c->sample_fmt));
		exit(1);
	}
	c->sample_rate = select_sample_rate(codec);
	c->channel_layout = select_channel_layout(codec);
	c->channels = av_get_channel_layout_nb_channels(c->channel_layout);

	if(avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate audio frame\n");
		exit(1);
	}
	frame->nb_samples = c->frame_size;
	frame->format = c->sample_fmt;
	frame->channel_layout = c->channel_layout;


	int buffer_size = av_samples_get_buffer_size(NULL, c->channels, c->frame_size, c->sample_fmt, 0);

	if (buffer_size < 0) {
		fprintf(stderr, "Could not get sample buffer size\n");
		exit(1);
	}
	
	samples = av_malloc(buffer_size);
	if (!samples) {
		fprintf(stderr, "Could not allocate %d bytes for samples buffer\n",
		buffer_size);
		exit(1);
	}
	int ret = avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt, (const uint8_t*)samples, buffer_size, 0);

	if (ret < 0) {
		fprintf(stderr, "Could not setup audio frame\n");
		exit(1);
	}

	FILE * fp = fopen ("myout.ogg","w");

	while(1) {
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
		n = fread(samples, sizeof(uint16_t), c->frame_size * 2, snd_in);
		printf("writing %d bytes\n", n);
		if(n > 0) {
			int opus_n = avcodec_encode_audio2(c, &pkt, frame, &got_output);

			sendto(s, pkt.data, pkt.size, 0, (struct sockaddr *)&addr, addrlen);
			fwrite(pkt.data, 1, pkt.size, fp);
		}
//		else break;

		n = recvfrom(s, data, PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
		printf("received %d bytes\n", n);
		if(n > 0) {
			fwrite(data, 1, n, snd_out);
		}
//		else break;
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