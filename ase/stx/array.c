/* 
 * $Id: array.c,v 1.1 2005-08-15 16:03:57 bacon Exp $
 */

#include <xp/stx/array.h>
#include <xp/stx/object.h>
#include <xp/bas/assert.h>

xp_word_t xp_stx_new_array (xp_stx_t* stx, xp_word_t size)
{
	xp_word_t x;

	xp_assert (stx->class_array != stx->nil);
	x = xp_stx_alloc_word_object (stx, XP_NULL, 0, XP_NULL, size);
	XP_STX_CLASS(stx,x) = stx->class_array;

	return x;	
}
