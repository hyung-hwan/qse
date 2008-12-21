/* 
 * $Id: array.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/array.h>
#include <qse/stx/object.h>
#include <qse/bas/assert.h>

qse_word_t qse_stx_new_array (qse_stx_t* stx, qse_word_t size)
{
	qse_word_t x;

	qse_assert (stx->class_array != stx->nil);
	x = qse_stx_alloc_word_object (stx, QSE_NULL, 0, QSE_NULL, size);
	QSE_STX_CLASS(stx,x) = stx->class_array;

	return x;	
}
