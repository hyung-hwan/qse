/*
 * $Id: stx.c,v 1.4 2005-05-08 11:16:07 bacon Exp $
 */

#include <xp/stx/stx.h>
#include <xp/stx/memory.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_stx_word_t capacity)
{
	if (stx == XP_NULL) {
		stx = (xp_stx_t*) xp_malloc (xp_sizeof(stx));
		if (stx == XP_NULL) return XP_NULL;
		stx->__malloced = xp_true;
	}
	else stx->__malloced = xp_false;

	if (xp_stx_memory_open (&stx->memory, capacity) == XP_NULL) {
		if (stx->__malloced) xp_free (stx);
		return XP_NULL;
	}

	stx->nil = XP_STX_NIL;
	stx->true = XP_STX_TRUE;
	stx->false = XP_STX_FALSE;

	return stx;
}

void xp_stx_close (xp_stx_t* stx)
{
	xp_stx_memory_close (&stx->memory);
	if (stx->__malloced) xp_free (stx);
}

int xp_stx_bootstrap (xp_stx_t* stx)
{
	xp_stx_word_t symbols;

	stx->nil = xp_stx_memory_alloc (&stx->memory, 0);
	stx->true = xp_stx_memory_alloc (&stx->memory, 0);
	stx->false = xp_stx_memory_alloc (&stx->memory, 0);

	xp_assert (stx->nil == XP_STX_NIL);
	xp_assert (stx->true == XP_STX_TRUE);
	xp_assert (stx->false == XP_STX_FALSE);

	symbol_table = xp_stx_memory_alloc (&stx->memory, 1);
	XP_STX_OBJECT_AT(&stx->memory, symbol_table, 0) = hash_table;

	XP_STX_OBJECT_CLASS(&stx->memory, sbs) = symbol_class;	
	return 0;
}

