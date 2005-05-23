/*
 * $Id: object.c,v 1.22 2005-05-23 14:43:03 bacon Exp $
 */

#include <xp/stx/object.h>
#include <xp/stx/memory.h>
#include <xp/stx/symbol.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

/* n: number of instance variables */
xp_stx_word_t xp_stx_alloc_word_object (xp_stx_t* stx, xp_stx_word_t n)
{
	xp_stx_word_t idx;
	xp_stx_word_object_t* obj;

	/* bytes to allocidxed = 
	 *     number of instance variables * word_size 
	 */
	idx = xp_stx_memory_alloc (&stx->memory,
		n * xp_sizeof(xp_stx_word_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	xp_stx_assert (stx->nil == XP_STX_NIL);

	/*
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_WORD_INDEXED;
	while (n-- > 0) XP_STX_AT(stx,idx,n) = stx->nil;
	*/
	obj = XP_STX_WORD_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_WORD_INDEXED;
	while (n-- > 0) obj->data[n] = stx->nil;

	return idx;
}

/* n: number of bytes */
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_stx_word_t n)
{
	xp_stx_word_t idx;
	xp_stx_byte_object_t* obj;

	idx = xp_stx_memory_alloc (
		&stx->memory, n + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	xp_stx_assert (stx->nil == XP_STX_NIL);

	/*
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_BYTE_INDEXED;
	while (n-- > 0) XP_STX_BYTEAT(stx,idx,n) = 0;
	*/
	obj = XP_STX_BYTE_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_BYTE_INDEXED;
	while (n-- > 0) obj->data[n] = 0;

	return idx;
}

xp_stx_word_t xp_stx_alloc_char_object (
	xp_stx_t* stx, const xp_stx_char_t* str)
{
	xp_stx_word_t idx, n;
	xp_stx_char_object_t* obj;

	n = xp_stx_strlen(str);
	idx = xp_stx_memory_alloc (&stx->memory, 
		(n + 1) * xp_sizeof(xp_stx_char_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	xp_stx_assert (stx->nil == XP_STX_NIL);

	/*
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_CHAR_INDEXED;
	XP_STX_CHARAT(stx,idx,n) = XP_STX_CHAR('\0');
	while (n-- > 0) XP_STX_CHARAT(stx,idx,n) = str[n];
	*/
	obj = XP_STX_CHAR_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_CHAR_INDEXED;
	obj->data[n] = XP_STX_CHAR('\0');
	while (n-- > 0) obj->data[n] = str[n];

	return idx;
}

xp_stx_word_t xp_stx_allocn_char_object (xp_stx_t* stx, ...)
{
	xp_stx_word_t idx, n = 0;
	const xp_stx_char_t* p;
	xp_stx_va_list ap;
	xp_stx_char_object_t* obj;

	xp_stx_va_start (ap, stx);
	while ((p = xp_stx_va_arg(ap, const xp_stx_char_t*)) != XP_NULL) {
		n += xp_stx_strlen(p);
	}
	xp_stx_va_end (ap);

	idx = xp_stx_memory_alloc (&stx->memory, 
		(n + 1) * xp_sizeof(xp_stx_char_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	xp_stx_assert (stx->nil == XP_STX_NIL);

	/*
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_CHAR_INDEXED;
	XP_STX_CHARAT(stx,idx,n) = XP_STX_CHAR('\0');
	*/
	obj = XP_STX_CHAR_OBJECT(stx,idx);
	obj->header.class = stx->nil;
	obj->header.access = (n << 2) | XP_STX_CHAR_INDEXED;
	obj->data[n] = XP_STX_CHAR('\0');

	xp_stx_va_start (ap, stx);
	n = 0;
	while ((p = xp_stx_va_arg(ap, const xp_stx_char_t*)) != XP_NULL) {
		while (*p != XP_STX_CHAR('\0')) {
			/*XP_STX_CHARAT(stx,idx,n++) = *p++;*/
			obj->data[n++] = *p++;
		}
	}
	xp_stx_va_end (ap);

	return idx;
}

xp_stx_word_t xp_stx_hash_char_object (xp_stx_t* stx, xp_stx_word_t idx)
{
	xp_stx_assert (XP_STX_TYPE(stx,idx) == XP_STX_CHAR_INDEXED);
	return xp_stx_strxhash (
		XP_STX_DATA(stx,idx), XP_STX_SIZE(stx,idx));
}

