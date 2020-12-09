#pragma once

#include <stdio.h>

#define S(x) #x
#define S_(x) S(x)
#define SLINE S_(__LINE__)
#define __log(pfx, ...)							\
do {									\
	fprintf(stderr, __FILE__ ":" SLINE ": " pfx __VA_ARGS__);	\
	fflush(stderr);							\
} while (0)

#define fatal(...)							\
do {									\
	__log("FATAL: ", __VA_ARGS__);					\
	abort();							\
} while (0)

#define BUG_ON(cond)							\
do {									\
	if (cond)							\
		fatal("BUG: '" #cond "' in %s()\n", __func__);		\
} while (0)								\

#define __unused __attribute__((unused))
