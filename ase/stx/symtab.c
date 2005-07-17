/*
 * $Id: symtab.c,v 1.1 2005-07-17 16:06:28 bacon Exp $
 */

#include <xp/stx/symtab.h>
#include <xp/bas/memory.h>

xp_stx_symtab_t* xp_stx_symtab_open (xp_stx_symtab_t* symtab)
{
	if (symtab == XP_NULL) {
		symtab = (xp_stx_symtab_t*)
			xp_malloc (xp_sizeof(xp_stx_symtab_t));
		if (symtab == XP_NULL) return -1;
		symtab->__malloced = xp_true;
	}
	else symtab->__malloced = xp_false;

	symtab->data = XP_NULL;
	symtab->capacity = 0;
	symtab->size = 0;

	return symtab;
}

void xp_stx_symtab_close (xp_stx_symtab_t* symtab)
{
	if (symtab->data != XP_NULL) xp_free (symtab->data);
	if (symtab->__malloced) xp_free (symtab);
}

int xp_stx_symtab_get (xp_stx_symtab_t* symtab, const xp_char_t* str)
{
}
