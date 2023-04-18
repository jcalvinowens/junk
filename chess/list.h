#pragma once

struct move {
	unsigned char sx;
	unsigned char sy;
	unsigned char dx;
	unsigned char dy;
};

struct move_list;

extern struct move_list *allocate_move_list(void);
extern void free_move_list(struct move_list *l);
extern int move_list_length(struct move_list *l);

extern struct move pop_move(struct move_list *l);
extern void push_move(struct move_list *l, int sx, int sy, int dx, int dy);
