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

#include "haldanian.h"

struct params {
	const char *usage;

	struct tissues *tissues;
	double surface;
	double depth;
	double gf;
	double ppn2;
	int negvals;

	int nr_vectors;
	const struct vector *vectors;
};

/*
 * Converts string input with various unit suffixes to Bar
 */
double parse_pressure(const char *str);

/*
 * Parses H:M notation into seconds
 */
double parse_time(const char *str);

/*
 * Parse generic arguments and populate @params
 */
void parse_arguments(int argc, char **argv, struct params *params);
