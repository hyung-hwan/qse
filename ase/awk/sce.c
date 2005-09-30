/*
 * $Id: sce.c,v 1.1 2005-09-30 09:40:15 bacon Exp $
 */

#include <xp/sce/sce.h>
#include <xp/sce/mem.h>
#include <xp/sce/misc.h>

xp_sce_t* xp_sce_open (xp_sce_t* sce, xp_word_t capacity)
{
	if (sce == XP_NULL) {
		sce = (xp_sce_t*)xp_malloc (xp_sizeof(sce));
		if (sce == XP_NULL) return XP_NULL;
		sce->__malloced = xp_true;
	}
	else sce->__malloced = xp_false;

	if (xp_sce_mem_open (&sce->mem, capacity) == XP_NULL) {
		if (sce->__malloced) xp_free (sce);
		return XP_NULL;
	}

	return sce;
}

void xp_sce_close (xp_sce_t* sce)
{
	xp_sce_mem_close (&sce->mem);
	if (sce->__malloced) xp_free (sce);
}

