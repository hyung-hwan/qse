/*
 * $Id: object.c,v 1.4 2005-05-08 11:16:07 bacon Exp $
 */

#include <xp/stx/object.h>
#include <xp/stx/memory.h>

/* n: number of instance variables */
xp_stx_word_t xp_stx_alloc_object (xp_stx_t* stx, xp_stx_word_t n)
{
	xp_stx_word_t idx;

	/* bytes to allocidxed = 
	 *     number of instance variables * word_size 
	 */
	idx = xp_stx_memory_alloc (&stx->memory,
		n * xp_sizeof(xp_stx_word_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	XP_STX_OBJECT_CLASS(&stx->memory,idx) = stx->nil;
	XP_STX_OBJECT_ACCESS(&stx->memory,idx) = ((n << 2) | 0x00);
	while (n--) XP_STX_OBJECT_AT(&stx->memory,idx,n) = stx->nil;

	return idx;
}

/* n: number of bytes */
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_stx_word_t n)
{
	xp_stx_word_t idx;

	idx = xp_stx_memory_alloc (
		&stx->memory, n + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	XP_STX_OBJECT_CLASS(&stx->memory,idx) = stx->nil;
	XP_STX_OBJECT_ACCESS(&stx->memory,idx) = ((n << 2) | 0x01);
	while (n--) XP_STX_OBJECT_BYTEAT(&stx->memory,idx,n) = 0;

	return idx;
}

/* n: length of the string */
xp_stx_word_t xp_stx_alloc_string_object (
	xp_stx_t* stx, xp_stx_char_t* str, xp_stx_word_t n)
{
	xp_stx_word_t idx;

	idx = xp_stx_memory_alloc (&stx->memory, 
		(n + 1) * xp_sizeof(xp_stx_char_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	XP_STX_OBJECT_CLASS(&stx->memory,idx) = stx->nil;
	XP_STX_OBJECT_ACCESS(&stx->memory,idx) = ((n << 2) | 0x02);
	XP_STX_OBJECT_CHARAT(&stx->memory,idx,n) = XP_STX_CHAR('\0');
	while (n--) XP_STX_OBJECT_CHARAT(&stx->memory,idx,n) = str[n];

	return idx;
}
