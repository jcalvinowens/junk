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

#define fatal(...) do { printf(__VA_ARGS__); abort(); } while (0)

#ifdef DEBUG
#define debug(...) do { fprintf(stderr, __VA_ARGS__); } while (0)
#else
#define debug(...) do {} while (0)
#endif

#define __fall_through __attribute__((fallthrough))
