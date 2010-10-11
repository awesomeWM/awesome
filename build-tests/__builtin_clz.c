/*
 * build-tests/__builtin_clz.c
 *
 * test if the compiler has __builtin_clz().
 */

int
main(void)
{

	return (__builtin_clz(42));
}
