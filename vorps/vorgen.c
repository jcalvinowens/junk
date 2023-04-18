/*
 * vorgen.c: Simple VOR signal generator.
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

#define SAMPLERATE (2400000L)
#define fatal(...) do { printf("FATAL: " __VA_ARGS__); abort(); } while (0)

static void generate_vor_signal(int rate, long nr, float f, float complex *out)
{
	float period = 1.0F / rate;
	long i;

	for (i = 0; i < nr; i++) {
		float complex sample = 0.0F;

		/*
		 * REF signal: 30Hz sine wave
		 */
		sample += sinf((float)i / rate * 2.0F*M_PI * (f + 30.0F));

		/*
		 * VAR signal: 30Hz
		 */

		*out++ = sample;
	}
}

static float complex data[SAMPLERATE * 10];

int main(int argc, char **argv)
{
	int fd;

	generate_vor_signal(SAMPLERATE, SAMPLERATE*10, 2009900.0F, data);

	fd = open(argv[1], O_WRONLY | O_CREAT, 0644);
	if (fd == -1)
		return -1;

	write(fd, data, sizeof(data) * sizeof(float complex));
	close(fd);
	return 0;
}
