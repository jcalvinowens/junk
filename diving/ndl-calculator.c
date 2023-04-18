/*
 * NDL Calculator: Calculate NDLs after arbitrary dive profiles
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

#include "common.h"
#include "args.h"
#include "haldanian.h"

static const char *usage = "Calculate NDLs after arbitrary dive profiles\n";

int main(int argc, char **argv)
{
	const struct compartment *compartment;
	double *ndls, *ptr;
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

	printf("Computing NDLs at %.3f for surface pressure %.3f...\n",
	       (params.surface + params.depth) * params.ppn2,
	       params.surface * 0.79);

	ndls = alloca(params.tissues->model->count * sizeof(double));
	compute_ndls(params.tissues, (params.surface + params.depth) * params.ppn2,
		     params.surface * 0.79, params.gf, ndls);

	ptr = ndls;
	for_each_tissue(params.tissues, compartment, load) {
		double tmp = *ptr++;
		printf("Load: %.3f NDL: %0.fs\n", *load,
		       params.negvals ? tmp : (tmp >= 0.0 ? tmp : 0.0));
	}

	return 0;
}
