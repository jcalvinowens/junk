/*
 * Copyright (C) 2013 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "negamax.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "common.h"
#include "board.h"
#include "list.h"

static unsigned long expanded_moves;
static unsigned long evaluated_moves;

static inline int max(int a, int b)
{
	return a > b ? a : b;
}

/* Enumeration of moves is batched by piece. Since we have the oppurtunity to
 * prune after the evaluation of each potential move, it's possible that we
 * end up not examining moves that we wasted time enumerating.
 *
 * Typically this overhead appears to be < 1%, which is probably less than the
 * overhead of the extra logic that would be required to evaluate between the
 * enumeration of each move. However, it can be as great as 10% with certain
 * board setups, so I need to think about this more...
 *
 * (I originally batched them by board, which resulted in waste overheads of
 * as high as 75%... so this is still a gigantic improvement.)
 */

static int negamax_algo(struct chessboard *c, int color, int depth, int alpha, int beta)
{
	struct chessboard *cb;
	const struct piece *p;
	struct piece_iterator *i = NULL;
	struct move_list *l;
	struct move m;
	int n, j, val, best_val = INT_MIN;

	if (!depth)
		return !color ? calculate_board_heuristic(c) : -calculate_board_heuristic(c);

	while ((p = iterate_color(c, &i, color))) {
		l = allocate_move_list();
		n = enumerate_moves(c, p, l);
		expanded_moves += n;

		for (j = 0; j < n; j++) {
			m = pop_move(l);
			cb = copy_board(c);

			execute_raw_move(cb, m);
			evaluated_moves++;

			val = -negamax_algo(cb, !color, depth - 1, -beta, -alpha);

			free(cb);

			best_val = max(best_val, val);
			alpha = max(alpha, val);
			if (alpha >= beta)
				break;
		}

		free_move_list(l);
	}

	return best_val;
}

/* Returns sx|sy|dx|dy in an integer byte-by-byte from least to most
 * significant, indicating which move should be made next.
 *
 * We seperate the initial iteration of negamax out like this to track the
 * actual move associated with the best score. Doing so during the
 * deeper iterations is a waste of time. */
unsigned int calculate_move(struct chessboard *c, int color, int depth)
{
	struct chessboard *cb;
	const struct piece *p;
	struct piece_iterator *i = NULL;
	struct move_list *l;
	int j, n = 0, fbsx = -1, fbsy = -1, fbdx = -1, fbdy = -1;
	int val, best_val = INT_MIN, alpha = INT_MIN, beta = INT_MAX;
	struct move m;

	expanded_moves = 0;
	evaluated_moves = 0;

	while ((p = iterate_color(c, &i, color))) {
		l = allocate_move_list();
		n = enumerate_moves(c, p, l);
		expanded_moves += n;

		for (j = 0; j < n; j++) {
			m = pop_move(l);
			cb = copy_board(c);

			execute_raw_move(cb, m);
			evaluated_moves++;

			val = -negamax_algo(cb, !color, depth - 1, -beta, -alpha);

			alpha = max(alpha, val);
			if (val > best_val) {
				best_val = val;
				fbsx = m.sx;
				fbsy = m.sy;
				fbdx = m.dx;
				fbdy = m.dy;
			}

			printf("Move %d/%d for piece %016lx (%d,%d) => (%d,%d) has heuristic value %d\n", j + 1, n, (unsigned long)p, m.sx, m.sy, m.dx, m.dy, val);
			free(cb);
		}

		free_move_list(l);
	}
	expanded_moves += n;
	printf("Evaluated %luM/%luM expanded moves\n", evaluated_moves / 1000000, expanded_moves / 1000000);

	return (fbsx) | (fbsy << 8) | (fbdx << 16) | (fbdy << 24);
}
