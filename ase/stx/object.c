/*
 * $Id: object.c,v 1.1 2005-05-06 17:17:59 bacon Exp $
 */

#include <xp/stx/object.h>

/* n: number of instance variables */
xp_stx_word_t xp_stx_instantiate (xp_stx_memory_t* mem, xp_size_t n)
{
	xp_stx_word_t at;

	/* bytes to allocated = 
	 *     number of instance variables * word_size 
	 */
	at = xp_stx_alloc_object (mem, n * xp_sizeof(xp_stx_word_t));
	if (at >= mem->capacity) return at; /* failed */

	XP_STX_OBJECT_CLASS(mem,at) = XP_STX_NIL;
	XP_STX_OBJECT_ACCESS(mem,at) = ((n << 2) | 0x00);
	while (n--) XP_STX_OBJECT(mem,at)->data[n] = XP_STX_NIL;
	return at;
}

xp_stx_word_t xp_stx_instantiate_byte (xp_stx_memory_t* mem, xp_size_t n)
{
	xp_stx_word_t at;

	at = xp_stx_alloc_object (mem, n);
	if (at >= mem->capacity) return at; /* failed */

	XP_STX_OBJECT_CLASS(mem,at) = XP_STX_NIL;
	XP_STX_OBJECT_ACCESS(mem,at) = ((n << 2) | 0x01);
	while (n--) XP_STX_BYTE_OBJECT(mem,at)->data[n] = 0;
	return at;
}

xp_stx_word_t xp_stx_instantiate_string (
	xp_stx_memory_t* mem, xp_stx_char_t* str, xp_stx_word_t n)
{
	xp_stx_word_t at;

	at = xp_stx_alloc_object (mem, n * xp_sizeof(xp_stx_char_t));
	if (at >= mem->capacity) return at; /* failed */

	XP_STX_OBJECT_CLASS(mem,at) = XP_STX_NIL;
	XP_STX_OBJECT_ACCESS(mem,at) = ((n << 2) | 0x02);
	while (n--) XP_STX_BYTE_OBJECT(mem,at)->data[n] = str[n];

	return at;
}
