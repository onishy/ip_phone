#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <signal.h>

#define PACKET_SIZE 4096

int my_mlsadf(int argc, char **argv, FILE *in_sound, FILE *in_mcep, FILE *out);
int my_sopr(int argc, char *argv[], FILE *in, FILE *out);


pid_t dopen(char *cmd, char **args, FILE **readpipe, FILE **writepipe)
{
	pid_t childpid;
	int pipe1[2], pipe2[2];

	if((pipe(pipe1) < 0) || (pipe(pipe2) < 0)) {
		perror("pipe"); exit(-1);
	}

	if((childpid = vfork()) < 0) {
		perror("fork"); exit(-1);
	} else if(childpid > 0) {
		close(pipe1[0]); close(pipe2[1]);
		*readpipe = fdopen(pipe2[0], "rb");
		*writepipe = fdopen(pipe1[1], "w");
		setlinebuf(*writepipe);
		return childpid;
	} else {
		close(pipe1[1]); close(pipe2[0]);
		dup2(pipe1[0], 0);
		dup2(pipe2[1], 1);
		close(pipe1[0]); close(pipe2[1]);

		if(execvp(cmd, args) < 0) perror("execlp");
	}
}

int main()
{
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

	int n, m;
	short *data;
	float *float_data;
	while(1) {
		pid_t pitch_pid;
		FILE *pitch_in;
		FILE *pitch_out;
		pid_t excite_pid;
		FILE *excite_in;
		FILE *excite_out;
		pid_t mcep_pid;
		FILE *mcep_in;
		FILE *mcep_out;
		pid_t frame_pid;
		FILE *frame_in;
		FILE *frame_out;
		pid_t window_pid;
		FILE *window_in;
		FILE *window_out;
		pid_t sopr_pid;
		FILE *sopr_in;
		FILE *sopr_out;
		pid_t mlsadf_pid;
		FILE *mlsadf_in;
		FILE *mlsadf_out;
		pid_t clip_pid;
		FILE *clip_in;
		FILE *clip_out;

		void* buf = malloc(1000000);
		data = calloc(sizeof(short), PACKET_SIZE);
		float_data = calloc(sizeof(float), PACKET_SIZE);

		n = fread(data, 1, PACKET_SIZE, snd_in);

		// x2x +sf
		int i;
		for(i = 0; i < n; i++) {
			float_data[i] = (float)data[i];
        }

        // xtract pitch
        char *pitch_op[] = {"pitch", "-a", "1", "-s", "16", "-p", "80", NULL};
		pitch_pid = dopen("pitch", pitch_op, &pitch_out, &pitch_in);
		setlinebuf(pitch_out);
		setlinebuf(pitch_in);
		fwritef(float_data, 1, n, pitch_in);
		printf("pitch data: %d bytes", (int)fread(float_data, 1, 100000, pitch_out));
		printf("pitch started.\n");

		// xtract mcep
		char *frame_op[] = {"frame", "-p", "80", NULL};
		frame_pid = dopen("frame", frame_op, &frame_out, &frame_in);
		setlinebuf(frame_out);
		setlinebuf(frame_in);
		fwrite(float_data, 1, n, frame_in);
		printf("frame started.\n");
		
		char *window_op[] = {"window", NULL};
		window_pid = dopen("window", window_op, &window_out, &window_in);
		setlinebuf(window_out);
		setlinebuf(window_in);

		int frame_to_window[2];
		pipe(frame_to_window);
		dup2(frame_to_window[0], fileno(frame_out));
		dup2(frame_to_window[1], fileno(window_in));
		// while(n > 0) {
		// 	n = fread(buf, 1, 1000, frame_out);
		// 	printf("%d bytes read\n", n);
		// 	if(n < 0) break;
		// 	fwrite(buf, 1, n, window_in);
		// }
		kill(frame_pid, SIGTERM);
		pclose(frame_in);
		pclose(frame_out);
		printf("window started.\n");

		char *mcep_op[] = {"mcep", "-m", "25", "-a", "0.42", NULL};
		mcep_pid = dopen("mcep", mcep_op, &mcep_out, &mcep_in);
		setlinebuf(mcep_out);
		setlinebuf(mcep_in);

		int window_to_mcep[2];
		pipe(window_to_mcep);
		dup2(window_to_mcep[0], fileno(window_out));
		dup2(window_to_mcep[1], fileno(mcep_in));

		// while(n > 0) {
		// 	n = fread(buf, 1, 1000000, window_out);
		// 	if(n < 0) break;
		// 	fwrite(buf, 1, n, mcep_in);
		// }
		kill(window_pid, SIGTERM);
		pclose(window_in);
		pclose(window_out);
		printf("mcep started.\n");

//		printf("test: %d\n", (int)fread(buf, 1, sizeof(int), pitch_out));
		char* sopr_op[] = {"sopr", "-m", "0.3"};
		int sopr_pipe[2];
		pipe(sopr_pipe);
		sopr_in = fdopen(sopr_pipe[0], "w");
		sopr_out = fdopen(sopr_pipe[1], "rb");
		my_sopr(3, sopr_op, pitch_out, sopr_in);
		printf("sopr started.\n");

		char *excite_op[] = {"excite", "-p", "80", NULL};
		excite_pid = dopen("excite", excite_op, &excite_out, &excite_in);

		int sopr_to_excite[2];
		pipe(sopr_to_excite);
		dup2(sopr_to_excite[0], sopr_pipe[1]);
		dup2(sopr_to_excite[1], fileno(excite_in));
		// while(n > 0) {
		// 	n = fread(buf, 1, 1000000, sopr_out);
		// 	if(n < 0) break;
		// 	fwrite(buf, 1, n, excite_in);
		// }
//		pclose(sopr_in);
//		pclose(sopr_out);
		printf("excite started.\n");

		int mlsadf_pipe[2];
		if(vfork() == 0) { 
			char* mlsadf_op[] = {"mlsadf", "-m", "25", "-a", "0.42", "-p", "80"};
			pipe(mlsadf_pipe);
			mlsadf_in = fdopen(mlsadf_pipe[0], "wb");
			mlsadf_out = fdopen(mlsadf_pipe[1], "rb");
			my_mlsadf(7, mlsadf_op, excite_out, mcep_out, mlsadf_in);
			kill(excite_pid, SIGTERM);
			pclose(excite_in);
			pclose(excite_out);
			kill(mcep_pid, SIGTERM);
			pclose(mcep_in);
			pclose(mcep_out);
		}
		printf("mlsadf started.\n");

		char *clip_op[] = {"clip", "-y", "32000", "32000", NULL};
		clip_pid = dopen("clip", clip_op, &clip_out, &clip_in);

		int mlsadf_to_clip[2];
		pipe(mlsadf_to_clip);
		dup2(mlsadf_to_clip[0], mlsadf_pipe[1]);
		dup2(mlsadf_to_clip[1], fileno(clip_in));
		// while(n > 0) {
		// 	n = fread(buf, 1, 1000000, mlsadf_out);
		// 	if(n < 0) break;
		// 	fwrite(buf, 1, n, clip_in);
		// }
		// pclose(mlsadf_in);
		// pclose(mlsadf_out);
		printf("clip started.\n");

		while(n > 0) {
			n = fread(float_data, sizeof(float), PACKET_SIZE, clip_out);
			if(n < 0) break;
			for(i = 0; i < n; i++) {
				data[i] = (short)float_data[i];
	        }
			fwrite(data, 1, n, snd_out);
		}
		kill(clip_pid, SIGTERM);
		pclose(clip_in);
		pclose(clip_out);

		free(buf);
		free(data);
		free(float_data);
	}
	printf("done.\n");

	// extract pitch	


}