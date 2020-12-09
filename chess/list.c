#include "list.h"

#include <stdlib.h>

#include "common.h"
#include "board.h"

/*
 * MOVE LISTS
 *
 * These hold the moves enumerated for a given piece as packed 4-bit integers
 * signifying (sx,sy) and (dx,dy). A single move_list can hold 32 moves, which
 * is overkill since the maximum number of moves a given piece can have is 27.
 */

struct move_list {
	int w_off;
	int r_off;
	char m[64];
};

struct move_list *allocate_move_list(void)
{
	struct move_list *ret = malloc(sizeof(struct move_list));

	if (!ret)
		return NULL;

	ret->r_off = 0;
	ret->w_off = 0;
	return ret;
}

void free_move_list(struct move_list *l)
{
	free(l);
}

int move_list_length(struct move_list *l)
{
	return l->w_off - l->r_off;
}

extern struct move pop_move(struct move_list *l)
{
	int n = l->r_off++ << 1;

	if (n > 64)
		fatal("whoops1\n");

	return (struct move){
		.sx = l->m[n] & 0x0f,
		.sy = l->m[n] >> 4,
		.dx = l->m[n + 1] & 0x0f,
		.dy = l->m[n + 1] >> 4,
	};
}

#define B(a,b) (((b << 4) | a))
extern void push_move(struct move_list *l, int sx, int sy, int dx, int dy)
{
	int n = l->w_off++ << 1;

	if (n > 64)
		fatal("whoops2\n");

	l->m[n] = B(sx, sy);
	l->m[n + 1] = B(dx, dy);
}
