
#include <assert.h>
#include <complex.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef short sample_t;

/* fd から 必ず n バイト読み, bufへ書く.
   n バイト未満でEOFに達したら, 残りは0で埋める.
   fd から読み出されたバイト数を返す */
ssize_t read_n(int fd, ssize_t n, void * buf);

/* fdへ, bufからnバイト書く */
ssize_t write_n(int fd, ssize_t n, void * buf);

/* 標本(整数)を複素数へ変換 */
void sample_to_complex(sample_t * s, 
		       complex double * X, 
		       long n);
/* 複素数を標本(整数)へ変換. 虚数部分は無視 */
void complex_to_sample(complex double * X, 
		       sample_t * s, 
		       long n);

/* 高速(逆)フーリエ変換;
   w は1のn乗根.
   フーリエ変換の場合   偏角 -2 pi / n
   逆フーリエ変換の場合 偏角  2 pi / n
   xが入力でyが出力.
   xも破壊される
 */
void fft_r(complex double * x, 
	   complex double * y, 
	   long n, 
	   complex double w);

void fft(complex double * x, 
	 complex double * y, 
	 long n);

void ifft(complex double * y, 
	  complex double * x, 
	  long n);

int pow2check(long N);

void bpf(complex double *y,long min, long max,long n);