#include <xp/stx/memory.h>
#include <xp/bas/stdio.h>

int xp_main ()
{
	xp_stx_memory_t mem;
	xp_stx_word_t i;

	if (xp_stx_memory_open (&mem, 10) == XP_NULL) {
		xp_printf (XP_TEXT("cannot open memory\n"));
		return -1;
	}

	for (i = 0; i < 20; i++) {
		xp_printf (XP_TEXT("%d, %d\n"), 
			i, xp_stx_alloc_object(&mem, 100));
	}

	for (i = 0; i < 5; i++) {
		xp_stx_dealloc_object (&mem, i);
	}

	for (i = 0; i < 20; i++) {
		xp_printf (XP_TEXT("%d, %d\n"), 
			i, xp_stx_alloc_object(&mem, 100));
	}

	xp_printf (XP_TEXT("End of program\n"));
	return 0;
}

