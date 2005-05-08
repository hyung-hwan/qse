/*
 * $Id: stx.c,v 1.1 2005-05-08 07:39:51 bacon Exp $
 */

#include <xp/stx/stx.h>
#include <xp/stx/memory.h>
#include <xp/bas/memory.h>

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

	return stx;
}

void xp_stx_close (xp_stx_t* stx)
{
	xp_stx_memory_close (&stx->memory);
	if (stx->__malloced) xp_free (stx);
}

