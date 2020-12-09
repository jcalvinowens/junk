#include "board.c"

/*
 * Validate the starting board is self-consistent
 */
static void test_starting_consistency(void)
{
	struct chessboard *c = get_new_board();
	int i;

	for (i = 0; i < 32; i++) {
		struct position p = *__pos(c, i);

		/*
		 * Validate piece @i in the position map does in fact exist at
		 * the coordinates it is supposed to on the starting board.
		 */
		BUG_ON(p_id(*__piece(c, p.x, p.y)) != i);
	}

	free(c);
}

static void (*const tests[])(void) = {
	test_starting_consistency,
};

int main(void)
{
	unsigned i;

	for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
		tests[i]();

	return 0;
}
