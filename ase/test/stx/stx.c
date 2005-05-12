#include <xp/stx/stx.h>
#include <xp/bas/stdio.h>

#include <xp/stx/hash.h>
void print_symbol_names (xp_stx_t* stx, xp_stx_word_t idx)
{
	xp_stx_word_t key = XP_STX_AT(stx,idx,1);
	xp_printf (XP_TEXT("%s\n"), &XP_STX_CHARAT(stx,key,0));
}

int xp_main ()
{
	xp_stx_t stx;
	xp_stx_word_t i;

	if (xp_stx_open (&stx, 10000) == XP_NULL) {
		xp_printf (XP_TEXT("cannot open stx\n"));
		return -1;
	}

	if (xp_stx_bootstrap(&stx) == -1) {
		xp_stx_close (&stx);
		xp_printf (XP_TEXT("cannot bootstrap\n"));
		return -1;
	}	

	xp_printf (XP_TEXT("stx.nil %d\n"), stx.nil);
	xp_printf (XP_TEXT("stx.true %d\n"), stx.true);
	xp_printf (XP_TEXT("stx.false %d\n"), stx.false);
	
	xp_stx_hash_traverse (&stx, stx.symbol_table, print_symbol_names);

/*
	for (i = 0; i < 20; i++) {
		xp_printf (XP_TEXT("%d, %d\n"), 
			i, xp_stx_memory_alloc(&stx.memory, 100));
	}

	for (i = 5; i < 10; i++) {
		xp_stx_memory_dealloc (&stx.memory, i);
	}

	for (i = 0; i < 20; i++) {
		xp_printf (XP_TEXT("%d, %d\n"), 
			i, xp_stx_memory_alloc(&stx.memory, 100));
	}
*/
	xp_stx_close (&stx);
	xp_printf (XP_TEXT("End of program\n"));
	return 0;
}

