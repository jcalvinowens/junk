/*
 * Altitude calculator: This tool uses a set of dives and surface intervals you
 * provide to calculate minimum ambiant pressures tolerable without exceeding
 * the M-values (+GF) for various Haldanian decompression models.
 *
 * Copyright (C) 2017 Calvin Owens <jcalvinowens@gmail.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>

#include "common.h"
#include "args.h"
#include "haldanian.h"

struct pressure_to_asl {
	double pressure;
	double fasl;
	double masl;
};

/*
 * FIXME: Actually do this with math
 */
static const struct pressure_to_asl asl_conv[] = {
	{-INFINITY, INFINITY, INFINITY},
	{0.000, INFINITY, INFINITY},
	{0.118, 40000, 12192},
	{0.301, 30000, 9144},
	{0.446, 20000, 6096},
	{0.572, 15000, 4572},
	{0.697, 10000, 3048},
	{0.724, 9000, 2743},
	{0.753, 8000, 2134},
	{0.782, 7000, 1829},
	{0.812, 6000, 1524},
	{0.843, 5000, 1372},
	{0.875, 4000, 1219},
	{0.891, 3500, 1067},
	{0.908, 3000, 914},
	{0.925, 2500, 762},
	{0.942, 2000, 610},
	{0.960, 1500, 457},
	{0.977, 1000, 305},
	{0.995, 500, 152},
	{1.013, 0, 0},
	{INFINITY, -INFINITY, -INFINITY},
};

static const struct pressure_to_asl *get_asl(double pressure)
{
	const struct pressure_to_asl *c;

	for (c = asl_conv; pressure > c->pressure; c++)
		;

	return c;
}

static const char *usage =
"This tool uses a set of dives and surface intervals you provide to calculate\n"
"minimum ambiant pressures tolerable without exceeding the M-values (+GF) for\n"
"various Haldanian decompression models.\n";

int main(int argc, char **argv)
{
	const struct compartment *compartment;
	double *ceils, *ptr;
	const double *load;
	struct params params = {
		.usage = usage,

		.surface = 1.013,
		.ppn2 = 0.79,
		.gf = 1.0,
		.negvals = 0,
	};

	parse_arguments(argc, argv, &params);

	puts("Computing loads...");
	compute_loading(params.tissues, params.vectors, params.nr_vectors);

	puts("Computing ceilings...");
	ceils = calloc(params.tissues->model->count, sizeof(double));
	compute_ceilings(params.tissues, params.gf, ceils);

	ptr = ceils;
	for_each_tissue(params.tissues, compartment, load) {
		double tmp = *ptr++ * (1.0 / 0.79);
		tmp = params.negvals ? tmp : (tmp > 0.0 ? tmp : 0.0);

		printf("Load: %.3f Ceil: %.3f %5.fft %5.fm\n", *load, tmp,
		       get_asl(tmp)->fasl, get_asl(tmp)->masl);
	}

	return 0;
}
