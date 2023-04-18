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

#ifndef __HALDANIAN_H__
#define __HALDANIAN_H__

/*
 * Structures to describe a Haldenian model
 */

struct compartment {
	double halftime;
	double mvalue;
	double mslope;
};

struct model {
	const char *name;
	int count;
	struct compartment compartments[];
};

/*
 * Tracks tissue loading for a hypothetical diver, with some model
 */
struct tissues {
	const struct model *model;
	double loads[];
};

/*
 * Give a pointer to each compartment in the model
 */
#define for_each_compartment(model, compartment) \
	for (compartment = (model)->compartments; \
	     compartment < (model)->compartments + (model)->count; \
	     compartment++)
/*
 * Give a pointer to each load in the tissues, and to the associated definition
 * for the compartment in the model
 */
#define for_each_tissue(tissues, compartment, ptr) \
	for (compartment = (tissues)->model->compartments, (ptr) = (tissues)->loads; \
	     compartment < (tissues)->model->compartments + (tissues)->model->count; \
	     compartment++, (ptr)++)

/*
 * Give a pointer to each load in the tissues
 */
#define for_each_load(tissues, ptr) \
	for ((ptr) = (tissues)->loads; \
	     (ptr) < (tissues)->loads + (tissues)->model->count; \
	     (ptr)++)

/*
 * Describes spending some duration at some pressure
 */
struct vector {
	double time;
	double pressure;
};

/*
 * Give a pointer to each (time, pressure) vector in the provided array
 */
#define for_each_vector(vectors, len, ptr) \
	for (ptr = (vectors); ptr < (vectors) + (len); (ptr)++)

extern const struct model *models[];

void compute_loading(struct tissues *tissues, const struct vector *vectors,
		     int nr_vectors);
void compute_ceilings(const struct tissues *tissues, double gf,
		      double *ceilings);
void compute_ndls(const struct tissues *tissues, double depth, double surface,
		  double gf, double *ndls);

#endif
