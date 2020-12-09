/*
 * vor.c: Simple VOR signal decoder
 *
 * Copyright (C) 2020 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <math.h>
#include <float.h>
#include <complex.h>
#include <fftw3.h>

#define SAMPLERATE (2400000L)
#define fatal(...) do { printf("FATAL: " __VA_ARGS__); abort(); } while (0)

static void gnuplot_dft(const float complex *dft, long n, long d)
{
	static const char plotfmt[] = "set term x11; set title '%d'; plot '-';";
	static int ctr = 0;

	char *plotcmd = NULL;
	int pipefd[2];
	pid_t pid;
	long i;

	if (pipe(pipefd))
		fatal("Can't make pipes: %m\n");

	if (asprintf(&plotcmd, plotfmt, ctr++) == -1)
		fatal("Can't make plotcmd: %m\n");

	pid = vfork();
	if (!pid) {
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		execlp("gnuplot", "gnuplot", "--persist", "-e", plotcmd, NULL);

		_exit(errno);
	}

	close(pipefd[0]);
	free(plotcmd);

	for (i = 0; i < n; i += d) {
		float tmp = 0.0;
		long o;

		for (o = 0; o < d; o++)
			tmp += sqrt(creal(dft[i + o]) * creal(dft[i + o]) +
				    cimag(dft[i + o]) * cimag(dft[i + o]));

		tmp /= (float)d;
		dprintf(pipefd[1], "%ld %f\n", i, tmp);
	}

	close(pipefd[1]);
	waitpid(pid, NULL, 0);
}

/*
 * Return input on [0, 2pi)
 */
static float unwrap(float v)
{
	float ret = remainderf(v, 2.0*M_PI);
	return ret < 0.0 ? ret + 2.0*M_PI : ret;
}

static float rad2deg(float v)
{
	return 360.0 * (unwrap(v) / (2.0*M_PI));
}

static long fftidx(long rate, long count, float freq)
{
	return lrint(freq / rate * count);
}

static void fftwf_dft_1d(long n, float complex *buf, int d)
{
	fftwf_plan p;

	p = fftwf_plan_dft_1d(n, buf, buf, d, FFTW_ESTIMATE);
	fftwf_execute(p);
	fftwf_destroy_plan(p);
}

struct vor_decode_result {
	float var_phase;
	float ref_phase;
	float radial;
};

static void vor_decode(const float complex *iq_data, long rate, long count,
		       float freq, struct vor_decode_result *out)
{
	float complex *buf, *ptr;
	long width, i;


	buf = fftwf_alloc_complex(count);
	if (!buf)
		fatal("Can't allocate FFTW buffer\n");

	memset(out, 0, sizeof(*out));
	memcpy(buf, iq_data, count * sizeof(*iq_data));

	/*
	 * Transform iq_data to frequency domain.
	 */

	fftwf_dft_1d(count, buf, FFTW_FORWARD);

	/*
	 * Find the VOR signal at the supplied frequency.
	 */

	ptr = buf + fftidx(rate, count, freq);
	width = 20000;

	gnuplot_dft(ptr, 200, 1);

	/*
	 * Get the phase of the REF signal, at 30Hz.
	 */

	out->ref_phase = carg(ptr[29]);

	/*
	 * Isolate the VAR signal, and transform back to time domain.
	 */

	memset(ptr, 0, (9960 - 500) * sizeof(*ptr));
	memset(ptr + 9960 + 500, 0, (width - 9960 - 500) * sizeof(*ptr));
	fftwf_dft_1d(width, ptr, FFTW_BACKWARD);

	/*
	 * Demodulate the FM VAR signal in place.
	 */

	for (i = 0; i < width - 1; i++) {
		float complex cur, nxt;

		cur = (ptr[i] / (float)width) *
		      cexp(-1.0*I * 2.0*M_PI * 9960.0 * i);
		nxt = (ptr[i + 1] / (float)width) *
		      cexp(-1.0*I * 2.0*M_PI * 9960.0 * (i + 1));

		ptr[i] = carg(conj(cur) * nxt);
	}

	/*
	 * Transform the demodulated VAR signal to frequency domain, and get the
	 * phase at 30Hz.
	 */

	fftwf_dft_1d(width, ptr, FFTW_FORWARD);
	out->var_phase = carg(ptr[30]);

	/*
	 * The radial is the difference between VAR and REF phases.
	 */

	out->radial = unwrap(out->var_phase - out->ref_phase);
	free(buf);
}

static const float complex *load_raw_iq(const char *path, long *count)
{
	const float complex *new = NULL;
	struct stat s;
	int fd;

	*count = 0;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return NULL;

	if (fstat(fd, &s))
		goto out;

	new = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (new == MAP_FAILED) {
		new = NULL;
		goto out;
	}

	*count = s.st_size / sizeof(*new);
out:
	close(fd);
	return new;
}

int main(int argc, char **argv)
{
	struct vor_decode_result res;
	const float complex *data;
	long count, i;

	if (argc != 2)
		return printf("Usage: %s <rtlsdr_raw_iq_file>\n", argv[0]);

	data = load_raw_iq(argv[1], &count);
	if (!data)
		return printf("Failed to load data (%m?)\n");

	for (i = 10000; i < count - SAMPLERATE; i += 1) {
		vor_decode(data + i, SAMPLERATE, SAMPLERATE, 2009900.0F, &res);
		printf("%5.1f = %5.1f - %5.1f\n", rad2deg(res.radial),
		       rad2deg(res.var_phase), rad2deg(res.ref_phase));
	}

	return 0;
}
