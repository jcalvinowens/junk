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

#include "board.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "common.h"
#include "list.h"

/*
 * Squares on the chess board are represented as an 8x8 matxix of 8-bit
 * integers, each of which is divided into: a 3-bit type, 4-bit ID, and 1-bit
 * color flad. Two pieces of each ID occur on the board (ID is unique including
 * color). Empty squares are zeros.
 *
 * The current (x,y) position of each piece on the board is also maintained in
 * an array of pairs of 4-bit integers indexed by (color << 4 | id). The (x,y)
 * value (15,15) is used to represent a captured piece.
 */

struct piece {
	unsigned char type:3;
	unsigned char color:1;
	unsigned char id:4;
};

struct position {
	unsigned char x:4;
	unsigned char y:4;
};

struct chessboard {
	struct piece b[8][8]; /* [y][x] */
	struct position p[32];
};

/*
 * Viewed here as though you are sitting as black looking at the board, so the
 * rows at the top are white's pieces.
 */

#define P(t, c, n) (struct piece){.type = t, .color = c, .id = n, }
#define B(_x, _y) (struct position){.x = _x, .y = _y, }

static const struct chessboard starting_board = {
	.b = {	{ P(2,0,0x0),P(3,0,0x1),P(4,0,0x2),P(5,0,0x3),P(6,0,0x4),P(4,0,0x5),P(3,0,0x6),P(2,0,0x7) },
		{ P(1,0,0x8),P(1,0,0x9),P(1,0,0xa),P(1,0,0xb),P(1,0,0xc),P(1,0,0xd),P(1,0,0xe),P(1,0,0xf) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(1,1,0x8),P(1,1,0x9),P(1,1,0xa),P(1,1,0xb),P(1,1,0xc),P(1,1,0xd),P(1,1,0xe),P(1,1,0xf) },
		{ P(2,1,0x0),P(3,1,0x1),P(4,1,0x2),P(5,1,0x3),P(6,1,0x4),P(4,1,0x5),P(3,1,0x6),P(2,1,0x7) }},
	.p = {	B(0,0),B(1,0),B(2,0),B(3,0),B(4,0),B(5,0),B(6,0),B(7,0),B(0,1),B(1,1),B(2,1),B(3,1),B(4,1),B(5,1),B(6,1),B(7,1),
		B(0,7),B(1,7),B(2,7),B(3,7),B(4,7),B(5,7),B(6,7),B(7,7),B(0,6),B(1,6),B(2,6),B(3,6),B(4,6),B(5,6),B(6,6),B(7,6)}
};

static const struct chessboard zero_board = {
	.b = {{0}},
	.p = {
		B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),
		B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),
		B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),
		B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),B(15,15),
		B(15,15),B(15,15),B(15,15),B(15,15),
	},
};

static int __p_id(enum piece_color color, enum piece_id id)
{
	return color << 4 | id;
}

static int p_id(struct piece piece)
{
	return __p_id(piece.color, piece.id);
}

static struct piece *__piece(struct chessboard *c, int x, int y)
{
	BUG_ON(x < 0 || y < 0 || x > 7 || y > 7);
	return &c->b[y][x];
}

static struct position *__pos(struct chessboard *c, int nr)
{
	BUG_ON(nr < 0 || nr > 31);
	return &c->p[nr];
}

static struct piece get_piece(struct chessboard *c, int x, int y)
{
	return *__piece(c, x, y);
}

static struct position get_pos(struct chessboard *c, struct piece piece)
{
	return *__pos(c, p_id(piece));
}

static int pos_empty(struct chessboard *c, int x, int y)
{
	return __piece(c, x, y)->type == EMPTY;
}

#define for_each_position(_c, _p) \
	for (_p = _c->p; _p != _c->p + 32; _p++)

struct piece_iterator {
	int off;
};

const struct piece *iterate_color(struct chessboard *c,
				  struct piece_iterator **i,
				  enum piece_color color)
{
	if (!*i) {
		*i = calloc(1, sizeof(**i));
		if (!*i)
			fatal("Can't allocate iterator?\n");

		(*i)->off = color << 4;
	}

	while (1) {
		struct position p;

		if ((*i)->off < 0 || (*i)->off > ((signed)(color << 4) + 15))
			break;

		p = *__pos(c, (*i)->off++);
		if (p.x == 15)
			continue;

		return __piece(c, p.x, p.y);

	}

	free(*i);
	return NULL;
}

void execute_raw_move(struct chessboard *c, struct move m)
{
	struct piece dst, src;

	dst = get_piece(c, m.dx, m.dy);
	if (dst.type != EMPTY)
		*__pos(c, p_id(dst)) = B(15, 15);

	src = get_piece(c, m.sx, m.sy);
	*__pos(c, p_id(src)) = B(m.dx, m.dy);

	*__piece(c, m.dx, m.dy) = src;
	*__piece(c, m.sx, m.sy) = P(0, 0, 0x0);
}

/*
 * VALIDATION FUNCTIONS
 *
 * Check if a particular move is legal for that type of piece. Returns 0 if
 * valid, a negative error code otherwise. If applicable, they check for pieces
 * in the path of movement and in the case of pawns, validate captures.
 *
 * These are all relatively straightforward except for pawns, which are
 * annoying.
 */

static int validate_pawn_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int dist, starting_rank, direction;

	/*
	 * Black pawn, or white pawn?
	 */
	if (get_piece(c, sx, sy).color == WHITE) {
		dist = dy - sy;
		starting_rank = 1;
		direction = 1;
	} else {
		dist = sy - dy;
		starting_rank = 6;
		direction = -1;
	}

	if (dx == sx) {
		/*
		 * Pawns can only move forward one space, unless in their
		 * starting position, in which case they can move forward one
		 * or two spaces
		 */

		if (sy == starting_rank) {
			if (dist != 1 && dist != 2) {
				return -EINVAL;
			}

			/* Make sure we're not jumping over a piece */
			if (dist == 2) {
				if (!pos_empty(c, sx, sy + direction))
					return -EEXIST;
			}
		} else {
			if (dist != 1) {
				return -EINVAL;
			}
		}

		/* Pawns cannot capture going forward, only diagonally */
		if (!pos_empty(c, dx, dy))
			return -EEXIST;

		return 0;
	} else {
		/* Pawns can only move diagonally one space forward */
		if (dist != 1)
			return -EINVAL;
		if (dx != (sx + 1) && dx != (sx - 1))
			return -EINVAL;

		/* Pawns can only move diagonally to capture */
		if (pos_empty(c, dx, dy))
			return -EINVAL;

		return 0;
	}
}

static int validate_rook_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int adjx, adjy;

	if (sx != dx && sy != dy)
		return -EINVAL;

	adjx = (sx == dx) ? 0 : ((dx > sx) ? 1 : -1);
	adjy = (sy == dy) ? 0 : ((dy > sy) ? 1 : -1);

	/* Check for pieces in the path of movement */
	while (((sx += adjx) != dx) + ((sy += adjy) != dy)) {
		if (!pos_empty(c, sx, sy))
			return -EEXIST;
	}

	return 0;
}

static int validate_knight_move(struct chessboard *c __unused, int sx, int sy, int dx, int dy)
{
	int mvx, mvy;

	mvx = abs(dx - sx);
	mvy = abs(dy - sy);

	if (!((mvx == 2 && mvy == 1) || (mvx == 1 && mvy == 2)))
		return -EINVAL;

	return 0;
}

static int validate_bishop_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int mvx, mvy, adjx, adjy;

	mvx = dx - sx;
	mvy = dy - sy;
	if (abs(mvx) != abs(mvy))
		return -EINVAL;

	adjx = (mvx > 0) ? 1 : -1;
	adjy = (mvy > 0) ? 1 : -1;

	/* Check for pieces in the path of movement */
	while (((sx += adjx) != dx) + ((sy += adjy) != dy)) {
		if (!pos_empty(c, sx, sy))
			return -EEXIST;
	}

	return 0;
}

static int validate_queen_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int r, b;

	r = validate_rook_move(c, sx, sy, dx, dy);
	if (!r)
		return 0;

	b = validate_bishop_move(c, sx, sy, dx, dy);
	if (!b)
		return 0;

	if (r == b)
		return -EINVAL;
	return -EEXIST;
}

static int validate_king_move(struct chessboard *c __unused, int sx, int sy, int dx, int dy)
{
	int mvx, mvy;

	mvx = dx - sx;
	mvy = dy - sy;

	if (!(abs(mvx) <= 1 && abs(mvy) <= 1))
		return -EINVAL;

	return 0;
}

static int (*const val_funcs[8])(struct chessboard *, int, int, int, int) = {
	NULL,
	validate_pawn_move,
	validate_rook_move,
	validate_knight_move,
	validate_bishop_move,
	validate_queen_move,
	validate_king_move,
	NULL,
};

/*
 * Execute a chess move. Returns 0 if successful, a negative error code if not.
 */

static int do_execute_move(struct chessboard *c, struct move m)
{
	struct piece sp, tmp;
	int v;

	/* Cannot move a piece off the board */
	if (m.sx > 7 || m.sy > 7 || m.dx > 7 || m.dy > 7)
		return -ERANGE;

	/* Cannot move piece to it's current location */
	if ((m.sx == m.dx) && (m.sy == m.dy))
		return -EFAULT;

	/* The piece we are moving must exist */
	sp = get_piece(c, m.sx, m.sy);
	if (sp.type == EMPTY)
		return -ENOENT;

	/* Make sure the move is legal for this particular piece */
	v = (*val_funcs[sp.type])(c, m.sx, m.sy, m.dx, m.dy);
	if (v)
		return v;

	/* Cannot capture pieces of the same color */
	tmp = get_piece(c, m.dx, m.dy);
	if (tmp.type != EMPTY && !(tmp.color ^ sp.color))
		return -EACCES;

	/* Move is valid, do it */
	execute_raw_move(c, m);
	return 0;
}

int execute_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	struct move m = {
		.sx = sx,
		.sy = sy,
		.dx = dx,
		.dy = dy,
	};

	return do_execute_move(c, m);
}

/*
 * ENUMERATION FUNCTIONS
 *
 * These functions enumerate all the legal moves for the piece located at
 * (sx,sy), appending them to the move_list provided.
 *
 * Note that these functions do not verify that a move does not place your king
 * in check or fail to remove your king from check: that is much easier to do
 * later.
 *
 * The functions return the number of moves that were appended to the list.
 *
 * These are much less elegant than the validation functions. Oh well...
 */

static void enumerate_pawn_moves(struct chessboard *c, int sx, int sy, struct move_list *l)
{
	int color, ending_rank, starting_rank, direction;

	if (get_piece(c, sx, sy).color == WHITE) {
		ending_rank = 7;
		starting_rank = 1;
		direction = 1;
		color = 0;
	} else {
		ending_rank = 0;
		starting_rank = 6;
		direction = -1;
		color = 1;
	}

	if (sy != ending_rank && pos_empty(c, sx, sy + direction))
		push_move(l, sx, sy, sx, sy + direction);

	if (sy == starting_rank && pos_empty(c, sx, sy + direction + direction))
		push_move(l, sx, sy, sx, sy + direction + direction);

	if (sx != 0 && sy != ending_rank)
		if (!pos_empty(c, sx - 1, sy + direction) && color ^ get_piece(c, sx - 1, sy + direction).color)
			push_move(l, sx, sy, sx - 1, sy + direction);

	if (sx != 7 && sy != ending_rank)
		if (!pos_empty(c, sx + 1, sy + direction) && color ^ get_piece(c, sx + 1, sy + direction).color)
			push_move(l, sx, sy, sx + 1, sy + direction);
}

static void enumerate_rook_moves(struct chessboard *c, int sx, int sy, struct move_list *l)
{
	int tdx, tdy, color;

	color = get_piece(c, sx, sy).color;

	/* Walk North */
	tdy = sy;
	while ((tdy += 1) <= 7) {
		if (pos_empty(c, sx, tdy)) {
			push_move(l, sx, sy, sx, tdy);
		} else {
			if (get_piece(c, sx, tdy).color ^ color) {
				push_move(l, sx, sy, sx, tdy);
			}

			break;
		}
	}

	/* Walk South */
	tdy = sy;
	while ((tdy -= 1) >= 0) {
		if (pos_empty(c, sx, tdy)) {
			push_move(l, sx, sy, sx, tdy);
		} else {
			if (get_piece(c, sx, tdy).color ^ color) {
				push_move(l, sx, sy, sx, tdy);
			}

			break;
		}
	}

	/* Walk East */
	tdx = sx;
	while ((tdx += 1) <= 7) {
		if (pos_empty(c, tdx, sy)) {
			push_move(l, sx, sy, tdx, sy);
		} else {
			if (get_piece(c, tdx, sy).color ^ color) {
				push_move(l, sx, sy, tdx, sy);
			}

			break;
		}
	}

	/* Walk West */
	tdx = sx;
	while ((tdx -= 1) >= 0) {
		if (pos_empty(c, tdx, sy)) {
			push_move(l, sx, sy, tdx, sy);
		} else {
			if (get_piece(c, tdx, sy).color ^ color) {
				push_move(l, sx, sy, tdx, sy);
			}

			break;
		}
	}
}

static void enumerate_knight_moves(struct chessboard *c, int sx, int sy, struct move_list *l)
{
	struct piece tmp;
	int color;

	color = get_piece(c, sx, sy).color;

	if (sx <= 6 && sy <= 5) {
		tmp = get_piece(c, sx + 1, sy + 2);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx + 1, sy + 2);
	}

	if (sx <= 5 && sy <= 6) {
		tmp = get_piece(c, sx + 2, sy + 1);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx + 2, sy + 1);
	}

	if (sx <= 5 && sy >= 1) {
		tmp = get_piece(c, sx + 2, sy - 1);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx + 2, sy - 1);
	}

	if (sx <= 6 && sy >= 2) {
		tmp = get_piece(c, sx + 1, sy - 2);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx + 1, sy - 2);
	}

	if (sx >= 1 && sy >= 2) {
		tmp = get_piece(c, sx - 1, sy - 2);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx - 1, sy - 2);
	}

	if (sx >= 2 && sy >= 1) {
		tmp = get_piece(c, sx - 2, sy - 1);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx - 2, sy - 1);
	}

	if (sx >= 2 && sy <= 6) {
		tmp = get_piece(c, sx - 2, sy + 1);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx - 2, sy + 1);
	}

	if (sx >= 1 && sy <= 5) {
		tmp = get_piece(c, sx - 1, sy + 2);
		if (tmp.type == EMPTY || tmp.color ^ color)
			push_move(l, sx, sy, sx - 1, sy + 2);
	}
}

static void enumerate_bishop_moves(struct chessboard *c, int sx, int sy, struct move_list *l)
{
	int tdx, tdy, color;

	color = get_piece(c, sx, sy).color;

	/* Walk Northeast */
	tdx = sx;
	tdy = sy;
	while (((tdx += 1) <= 7) && ((tdy += 1) <= 7)) {
		if (pos_empty(c, tdx, tdy)) {
			push_move(l, sx, sy, tdx, tdy);
		} else {
			if (get_piece(c, tdx, tdy).color ^ color) {
				push_move(l, sx, sy, tdx, tdy);
			}

			break;
		}
	}

	/* Walk Southeast */
	tdx = sx;
	tdy = sy;
	while (((tdx += 1) <= 7) && ((tdy -= 1) >= 0)) {
		if (pos_empty(c, tdx, tdy)) {
			push_move(l, sx, sy, tdx, tdy);
		} else {
			if (get_piece(c, tdx, tdy).color ^ color) {
				push_move(l, sx, sy, tdx, tdy);
			}

			break;
		}
	}

	/* Walk Northwest */
	tdx = sx;
	tdy = sy;
	while (((tdx -= 1) >= 0) && ((tdy += 1) <= 7)) {
		if (pos_empty(c, tdx, tdy)) {
			push_move(l, sx, sy, tdx, tdy);
		} else {
			if (get_piece(c, tdx, tdy).color ^ color) {
				push_move(l, sx, sy, tdx, tdy);
			}

			break;
		}
	}

	/* Walk Southwest */
	tdx = sx;
	tdy = sy;
	while (((tdx -= 1) >= 0) && ((tdy -= 1) >= 0)) {
		if (pos_empty(c, tdx, tdy)) {
			push_move(l, sx, sy, tdx, tdy);
		} else {
			if (get_piece(c, tdx, tdy).color ^ color) {
				push_move(l, sx, sy, tdx, tdy);
			}

			break;
		}
	}
}

static void enumerate_queen_moves(struct chessboard *c, int sx, int sy, struct move_list *l)
{
	enumerate_rook_moves(c, sx, sy, l);
	enumerate_bishop_moves(c, sx, sy, l);
}

/* This one is extra shitty */
static void enumerate_king_moves(struct chessboard *c, int sx, int sy, struct move_list *l)
{
	struct piece tmp;
	int color;

	color = get_piece(c, sx, sy).color;

	if (sx != 0) {
		tmp = get_piece(c, sx - 1, sy);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx - 1, sy);
			}
		} else {
			push_move(l, sx, sy, sx - 1, sy);
		}
	}

	if (sx != 7) {
		tmp = get_piece(c, sx + 1, sy);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx + 1, sy);
			}
		} else {
			push_move(l, sx, sy, sx + 1, sy);
		}
	}

	if (sy != 0) {
		tmp = get_piece(c, sx, sy - 1);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx, sy - 1);
			}
		} else {
			push_move(l, sx, sy, sx, sy - 1);
		}
	}

	if (sy != 7) {
		tmp = get_piece(c, sx, sy + 1);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx, sy + 1);
			}
		} else {
			push_move(l, sx, sy, sx, sy + 1);
		}
	}

	if (sx != 7 && sy != 7) {
		tmp = get_piece(c, sx + 1, sy + 1);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx + 1, sy + 1);
			}
		} else {
			push_move(l, sx, sy, sx + 1, sy + 1);
		}
	}

	if (sx != 7 && sy != 0) {
		tmp = get_piece(c, sx + 1, sy - 1);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx + 1, sy - 1);
			}
		} else {
			push_move(l, sx, sy, sx + 1, sy - 1);
		}
	}

	if (sx != 0 && sy != 7) {
		tmp = get_piece(c, sx - 1, sy + 1);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx - 1, sy + 1);
			}
		} else {
			push_move(l, sx, sy, sx - 1, sy + 1);
		}
	}

	if (sx != 0 && sy != 0) {
		tmp = get_piece(c, sx - 1, sy - 1);
		if (tmp.type != EMPTY) {
			if (tmp.color ^ color) {
				push_move(l, sx, sy, sx - 1, sy - 1);
			}
		} else {
			push_move(l, sx, sy, sx - 1, sy - 1);
		}
	}
}

static void enumerate_empty(struct chessboard *c __unused, int sx, int sy,
			    struct move_list *l __unused)
{
	fatal("Enumerate on EMPTY position at (%d,%d)\n", sx, sy);
}

static void enumerate_invalid(struct chessboard *c __unused, int sx, int sy,
			      struct move_list *l __unused)
{
	fatal("Enumerate with invalid type 7 at (%d,%d)\n", sx, sy);
}

static void (*const enum_funcs[8])(struct chessboard *, int, int, struct move_list *) = {
	enumerate_empty,
	enumerate_pawn_moves,
	enumerate_rook_moves,
	enumerate_knight_moves,
	enumerate_bishop_moves,
	enumerate_queen_moves,
	enumerate_king_moves,
	enumerate_invalid,
};

/*
 * Enumerate all possible moves for a given piece. The piece_number must includ
 * the color bit.
 */
int enumerate_moves(struct chessboard *c, const struct piece *piece,
		    struct move_list *l)
{
	struct position pos;

	pos = get_pos(c, *piece);
	(*enum_funcs[piece->type])(c, pos.x, pos.y, l);
	return move_list_length(l);
}

struct chessboard *copy_board(const struct chessboard *c)
{
	void *ret = malloc(sizeof(struct chessboard));

	if (!ret)
		fatal("-ENOMEM allocating chessboard struct\n");

	memcpy(ret, c, sizeof(struct chessboard));
	return ret;
}

struct chessboard *get_new_board(void)
{
	return copy_board(&starting_board);
}

struct chessboard *get_zero_board(void)
{
	return copy_board(&zero_board);
}

/*
 * Always returns a heuristic such that higher is better for white and lower is
 * better for black.
 */
static const int piece_values[8] = {0, 12, 60, 36, 36, 108, 240, 0};

int calculate_board_heuristic(struct chessboard *c)
{
	struct position *pos;
	struct piece piece;
	int white_h = 0;
	int black_h = 0;

	for_each_position(c, pos) {
		if (pos->x == 15)
			continue;

		piece = get_piece(c, pos->x, pos->y);
		if (piece.color == WHITE)
			white_h += piece_values[piece.type];
		else
			black_h += piece_values[piece.type];
	}

	return white_h - black_h;
}

static const char *asciiart_board_skel = "\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n";

static const char *ansi_chess_colors[2] = {
	"\x1b[36m!\x1b[0m",
	"\x1b[32m!\x1b[0m",
};

static char get_character(unsigned char p)
{
	switch (p & 0x7) {
	case 0:	return ' ';
	case 1:	return 'P';
	case 2:	return 'R';
	case 3:	return 'N';
	case 4:	return 'B';
	case 5:	return 'Q';
	case 6:	return 'K';
	case 7: return '?';
	}

	__builtin_unreachable();
}

void print_chessboard(const struct chessboard *c)
{
	int r;
	char *tmp, *board = strdup(asciiart_board_skel);
	struct piece p;

	for (int i = 0, j = 63; i < 64; i++, j = 63 - i) {
		p = c->b[j >> 3][j & 0x7];
		r = (p.color << 7) | (p.id << 3) | p.type;

		r = (r & 0x7) | ((r & 0x80) >> 4);
		tmp = &board[18 + ((i >> 3) * 18) + (90 * (i >> 3)) + ((7 - (i & 0x7)) * 11) + 1];
		memcpy(tmp, ansi_chess_colors[(r >> 3) & 1], 10);
		tmp[5] = get_character(r);
	}
	printf("%s", board);
	free(board);
}
