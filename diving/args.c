/*
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
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "common.h"
#include "args.h"

/*
 * Convert various input pressure units to Bar, using the suffix
 */

struct conv {
	double (*fromfn)(double orig, double arg);
	double (*tofn)(double orig, double arg);
	const char **sfx;
	double arg;
};

static double __convert_factor(double orig, double factor)
{
	return orig * factor;
}

static double __convert_divisor(double orig, double divisor)
{
	return orig / divisor;
}

static double __convert_none(double orig, double arg)
{
	return orig;
}

static const struct conv convs[] = {
	{
		.sfx = (const char *[]){"fsw", "ft", "f", NULL},
		.fromfn = __convert_factor,
		.tofn = __convert_divisor,
		.arg = 0.030643,
	},
	{
		.sfx = (const char *[]){"msw", "m", NULL},
		.fromfn = __convert_factor,
		.tofn = __convert_divisor,
		.arg = 0.098064,
	},
	{
		.sfx = (const char *[]){"bar", NULL},
		.fromfn = __convert_none,
		.tofn = __convert_none,
	},

};

double parse_pressure(const char *str)
{
	int len;
	unsigned int i;
	const char **cur;

	len = strlen(str);
	for (i = 0; i < sizeof(convs) / sizeof(struct conv); i++) {
		for (cur = convs[i].sfx; *cur; cur++) {
			int slen = strlen(*cur);
			const char *tmp = str + len - slen;

			if (len > slen && !strcmp(tmp, *cur)) {
				return convs[i].fromfn(atof(str), convs[i].arg);
			}
		}
	}

	fatal("Cannot parse pressure '%s'!", str);
}

/*
 * Parses "H:M" notation, allowing H or M to be omitted. Bad inputs are handled
 * poorly and will result in garbage.
 */

double parse_time(const char *str)
{
	const char *tmp = index(str, ':');
	double seconds;

	/*
	 * If there is no ':', assume the user passed seconds
	 */
	if (!tmp)
		seconds = atoi(str);
	/*
	 * Parse minutes (':M')
	 */
	else if (tmp == str)
		seconds = atoi(str + 1) * 60;
	/*
	 * Parse hours ('H:')
	 */
	else if (tmp[1] == '\0')
		seconds = atoi(str) * 3600;
	/*
	 * Parse hours and minutes ('H:M')
	 */
	else
		seconds = atoi(str) * 3600 + atoi(tmp + 1) * 60;

	/*
	 * Nowhere in the program is zero a valid time
	 */
	if (seconds == 0.0)
		fatal("Bad time '%s': enter 'H:M', 'H:', ':M', or seconds\n", str);

	return seconds;
}

static const char *usage_fmt =
"Usage: ./%s [-n][-m model][-g N][-a N][-p N] T1 D1 [T2 D2 [...]]\n\n";

static const char *optstr = "a:d:g:hm:np:";
static const struct option optlong[] = {
	{
		.name = "surface-altitude",
		.has_arg = required_argument,
		.val = 'a',
	},
	{
		.name = "depth",
		.has_arg = required_argument,
		.val = 'd',
	},
	{
		.name = "gf",
		.has_arg = required_argument,
		.val = 'g',
	},
	{
		.name = "help",
		.has_arg = no_argument,
		.val = 'h',
	},
	{
		.name = "model",
		.has_arg = required_argument,
		.val = 'm',
	},
	{
		.name = "negative-pressures",
		.has_arg = no_argument,
		.val = 'n',
	},
	{
		.name = "ppn2",
		.has_arg = required_argument,
		.val = 'p',
	},
	{
		.name = NULL,
	},
};

void parse_arguments(int argc, char **argv, struct params *params)
{
	const struct model **cur, *model = models[0];
	struct vector *tmp;
	double *load;
	int i;

	while (1) {
		i = getopt_long(argc, argv, optstr, optlong, NULL);

		switch (i) {
		case 'g':
			params->gf = atoi(optarg) / 100.0;
			if (params->gf <= 0.0)
				fatal("GF-Hi cannot be negative or zero!\n");

			printf("Using GF-Hi %d%%\n", (int)(params->gf * 100.0));
			break;
		case 'a':
			params->surface = parse_pressure(optarg);
			printf("Using surface pressure of %.3f ('%s')\n",
			       params->surface, optarg);
			break;
		case 'd':
			params->depth = parse_pressure(optarg);
			printf("Using depth pressure of %.3f ('%s')\n",
			       params->depth, optarg);
			break;
		case 'n':
			params->negvals = 1;
			break;
		case 'm':
			for (cur = models; *cur; cur++) {
				if (strstr((*cur)->name, optarg)) {
					model = *cur;
					printf("Using model '%s'\n", model->name);
					break;
				}
			}

			if (!*cur)
				fatal("No match for model '%s'\n", optarg);
			break;
		case 'p':
			params->ppn2 = atof(optarg);
			if (params->ppn2 > 1.0)
				fatal("Can't have PPN2 more than 1.0\n");
			else if (params->ppn2 <= 0.0)
				fatal("Can't have negative (or zero) PPN2\n");
			break;
		case -1:
			goto done;
		case 'h':
			printf(usage_fmt, argv[0]);
			puts(params->usage);

			__fall_through;
		default:
			exit(1);
		}
	}
done:
	/*
	 * Initialize tissue loading. Assume we begin saturated at surface
	 * pressure breathing air.
	 */

	params->tissues = malloc(sizeof(*params->tissues) + sizeof(double) * model->count);
	memset(params->tissues, 0, sizeof(*params->tissues));
	params->tissues->model = model;

	for_each_load(params->tissues, load)
		*load = params->surface * 0.79;

	/*
	 * Pull TIME DEPTH pairs out of argv into a vector array
	 */

	if ((argc - optind) & 1)
		fatal("Bad input: missing a depth or time?\n");

	params->nr_vectors = (argc - optind) / 2;
	tmp = calloc(params->nr_vectors, sizeof(*params->vectors));
	for (i = 0; i < params->nr_vectors; i++) {
		const char *time = argv[optind + i * 2];
		const char *pressure = argv[optind + i * 2 + 1];

		tmp[i].time = parse_time(time);
		tmp[i].pressure = parse_pressure(pressure) + params->surface;

		/*
		 * We assume the diver breathes air during surface intervals
		 */
		tmp[i].pressure *= tmp[i].pressure <= params->surface
				? 0.79 : params->ppn2;

		printf("%02d> '%6s %6s' => % 7.0fs at % 6.3f bar\n", i,
		       time, pressure, tmp[i].time, tmp[i].pressure);
	}

	params->vectors = tmp;
}
