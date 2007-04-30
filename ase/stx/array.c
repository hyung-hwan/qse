/* 
 * $Id: array.c,v 1.1.1.1 2007/03/28 14:05:25 bacon Exp $
 */

#include <ase/stx/array.h>
#include <ase/stx/object.h>
#include <ase/bas/assert.h>

ase_word_t ase_stx_new_array (ase_stx_t* stx, ase_word_t size)
{
	ase_word_t x;

	ase_assert (stx->class_array != stx->nil);
	x = ase_stx_alloc_word_object (stx, ASE_NULL, 0, ASE_NULL, size);
	ASE_STX_CLASS(stx,x) = stx->class_array;

	return x;	
}
