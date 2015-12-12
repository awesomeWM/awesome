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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
