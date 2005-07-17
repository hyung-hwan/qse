/*
 * $Id: symtab.h,v 1.1 2005-07-17 16:06:28 bacon Exp $
 */

#ifndef _XP_STX_SYMTAB_H_
#define _XP_STX_SYMTAB_H_

#include <xp/stx/stx.h>

struct xp_stx_symtab_t
{
	xp_word_t* data;
	xp_word_t capacity;
	xp_word_t size;
	xp_bool_t __malloced;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_symtab_t* xp_stx_symtab_open (xp_stx_symtab_t* symtab);
void xp_stx_symtab_close (xp_stx_symtab_t* symtab);

#ifdef __cplusplus
}
#endif

#endif
