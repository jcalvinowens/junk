#pragma once

#include "list.h"

enum piece_type {
	EMPTY		= 0,
	PAWN		= 1,
	ROOK		= 2,
	KNIGHT		= 3,
	BISHOP		= 4,
	QUEEN		= 5,
	KING		= 6,
};

enum piece_color {
	WHITE		= 0,
	BLACK		= 1,
};

enum piece_id {
	Q_ROOK		= 0,
	Q_KNIGHT	= 1,
	Q_BISHOP	= 2,
	Q_QUEEN		= 3,
	K_KING		= 4,
	K_BISHOP	= 5,
	K_KNIGHT	= 6,
	K_ROOK		= 7,
	Q_ROOK_PAWN	= 8,
	Q_KNIGHT_PAWN	= 9,
	Q_BISHOP_PAWN	= 10,
	Q_QUEEN_PAWN	= 11,
	K_KING_PAWN	= 12,
	K_BISHOP_PAWN	= 13,
	K_KNIGHT_PAWN	= 14,
	K_ROOK_PAWN	= 15,
};

struct piece;
struct position;
struct chessboard;

extern struct chessboard *get_new_board(void);
extern struct chessboard *get_zero_board(void);
extern struct chessboard *copy_board(const struct chessboard *c);
extern void print_chessboard(const struct chessboard *c);

struct piece_iterator;
const struct piece *iterate_color(struct chessboard *c,
				  struct piece_iterator **i,
				  enum piece_color color);

extern void execute_raw_move(struct chessboard *c, struct move m);
extern int execute_move(struct chessboard *c, int sx, int sy, int dx, int dy);
extern int enumerate_moves(struct chessboard *c, const struct piece *p,
			   struct move_list *l);

extern int calculate_board_heuristic(struct chessboard *c);
