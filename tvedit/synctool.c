/*
 * synctool.c: Simple tool to automatically synchronize audio tracks.
 *
 * Copyright (C) 2020 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <fftw3.h>

#define err(...) do { fprintf(stderr, __VA_ARGS__); } while (0)
#define fatal(...) do { err("FATAL: " __VA_ARGS__); abort(); } while (0)

static float power(float complex v)
{
	return sqrt(creal(v) * creal(v) + cimag(v) * cimag(v));
}

static void fftwf_r2c_1d(int n, float *in, float complex *out)
{
	fftwf_plan p = fftwf_plan_dft_r2c_1d(n, in, out,
		       FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);

	fftwf_execute(p);
	fftwf_destroy_plan(p);
}

static float *s16_to_float32(const int16_t *in, long samples)
{
	float *ret = calloc(samples, sizeof(float));
	int i = 0;

	for (i = 0; i < samples; i++)
		ret[i] = (float)in[i] / INT16_MAX;

	return ret;
}

struct spectrogram {
	int len;
	int N;

	float complex *dfts[0];
};

static const char gnuplot_cmd_fmt[] = "\
set ylabel 'Time (U=%dms)';\
set title 'Spectrogram (N=%d, len=%d)';\
unset key;\
set palette rgbformula 34,34,34;\
set cbrange [0:0.1];\
set cblabel 'power';\
set xlabel 'Frequency (Hz)';\
unset cbtics;\
set view map;\
splot '-' matrix with image;\
";

static void gnuplot_spectrogram(struct spectrogram *s)
{
	char cmd[4096] = {0};
	int pipefd[2];
	pid_t pid;
	long i, j;

	if (pipe(pipefd))
		fatal("Can't make pipes: %m\n");

	snprintf(cmd, sizeof(cmd), gnuplot_cmd_fmt,
		 1000 / (44100 / s->N),
		 s->N,
		 s->len);

	pid = vfork();
	if (!pid) {
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		execlp("gnuplot", "gnuplot", "--persist", "-e", cmd, NULL);

		_exit(errno);
	}

	close(pipefd[0]);

	for (i = 0; i < s->len; i++) {
		for (j = 0; j < s->N / 2; j++)
			dprintf(pipefd[1], "%f ", power(s->dfts[i][j]));

		dprintf(pipefd[1], "\n");
	}

	close(pipefd[1]);
	waitpid(pid, NULL, 0);
}

struct file {
	int fd;
	int len;

	int rate;
	int samples;
	float *data;

	/*
	 * Read-only raw data from the file
	 */
	union {
		void *map;
		int8_t *s8;
		uint8_t *u8;
		int16_t *s16;
		uint16_t *u16;
		int32_t *s32;
		uint32_t *u32;
	};
};

static void open_file(struct file *f, const char *path)
{
	struct stat s;

	f->fd = open(path, O_RDONLY);
	if (f->fd == -1)
		fatal("Can't open %s: %m\n", path);

	if (fstat(f->fd, &s))
		fatal("Can't stat %s: %m\n", path);

	f->len = s.st_blocks * 512;
	f->map = mmap(NULL, f->len, PROT_READ, MAP_SHARED, f->fd, 0);
	if (f->map == MAP_FAILED)
		fatal("Can't mmap %s: %m\n", path);

	/*
	 * FIXME: For now, assume 44.1KHz S16_LE
	 */
	f->rate = 44100;
	f->samples = f->len / sizeof(int16_t);
	f->data = s16_to_float32(f->s16, f->samples);
}

static void close_file(struct file *f)
{
	if (close(f->fd))
		err("Bad close (fd=%d): %m\n", f->fd);

	if (munmap(f->map, f->len))
		err("Bad unmap (fd=%d, p=%016lx)\n", f->fd, (long)f->map);

	free(f->data);

	f->fd = -1;
	f->data = NULL;
	f->map = NULL;
}

static struct spectrogram *compute_spectrogram(struct file *f, int N)
{
	struct spectrogram *ret;
	int len, i, j, p;
	char *base;

	len = f->samples / N;
	ret = calloc(1, sizeof(*ret) + sizeof(float complex *) * len +
			sizeof(float complex) * len * N / 2);
	if (!ret)
		fatal("No memory for spectrogram\n");

	base = (char *)ret + sizeof(*ret) + sizeof(float complex *) * len;
	for (i = 0, p = 0; i < len; i++, p += N) {
		ret->dfts[i] = (void*)base + sizeof(float complex) * N / 2 * i;
		fftwf_r2c_1d(N, f->data + p, ret->dfts[i]);

		for (j = 0; j < N / 2; j++)
			ret->dfts[i][j] /= (float)N;
	}

	ret->N = N;
	ret->len = len;
	return ret;
}

static int find_offset_ms(char *p1, char *p2)
{
	struct spectrogram *s1_100, *s2_100;
	struct file f1, f2;

	open_file(&f1, p1);
	open_file(&f2, p2);

	s1_100 = compute_spectrogram(&f1, 8820);
	s2_100 = compute_spectrogram(&f2, 8820);

	gnuplot_spectrogram(s1_100);
	gnuplot_spectrogram(s2_100);

	free(s1_100);
	free(s2_100);
	close_file(&f1);
	close_file(&f2);

	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 3)
		fatal("Usage: %s <file1> <file2>\n", argv[0] ?: "synctool");

	find_offset_ms(argv[1], argv[2]);
	return 0;
}
