/*
 * $Id: object.c,v 1.2 2005-05-08 07:39:51 bacon Exp $
 */

#include <xp/stx/object.h>
#include <xp/stx/memory.h>

/* n: number of instance variables */
xp_stx_word_t xp_stx_alloc_object (xp_stx_t* stx, xp_size_t n)
{
	xp_stx_word_t at;

	/* bytes to allocated = 
	 *     number of instance variables * word_size 
	 */
	at = xp_stx_alloc_objmem (
		&stx->memory, n * xp_sizeof(xp_stx_word_t));
	if (at >= stx->memory.capacity) return at; /* failed */

	XP_STX_OBJECT_CLASS(&stx->memory,at) = stx->nil;
	XP_STX_OBJECT_ACCESS(&stx->memory,at) = ((n << 2) | 0x00);
	while (n--) XP_STX_OBJECT(&stx->memory,at)->data[n] = stx->nil;

	return at;
}

/* n: number of bytes */
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_size_t n)
{
	xp_stx_word_t at;

	at = xp_stx_alloc_objmem (&stx->memory, n);
	if (at >= stx->memory.capacity) return at; /* failed */

	XP_STX_OBJECT_CLASS(&stx->memory,at) = stx->nil;
	XP_STX_OBJECT_ACCESS(&stx->memory,at) = ((n << 2) | 0x01);
	while (n--) XP_STX_BYTE_OBJECT(&stx->memory,at)->data[n] = 0;

	return at;
}

/* n: length of the string */
xp_stx_word_t xp_stx_alloc_string_object (
	xp_stx_t* stx, xp_stx_char_t* str, xp_stx_word_t n)
{
	xp_stx_word_t at;

	at = xp_stx_alloc_objmem (
		&stx->memory, (n + 1) * xp_sizeof(xp_stx_char_t));
	if (at >= stx->memory.capacity) return at; /* failed */

	XP_STX_OBJECT_CLASS(&stx->memory,at) = stx->nil;
	XP_STX_OBJECT_ACCESS(&stx->memory,at) = ((n << 2) | 0x02);
	XP_STX_BYTE_OBJECT(&stx->memory,at)->data[n] = XP_STX_CHAR('\0');
	while (n--) XP_STX_BYTE_OBJECT(&stx->memory,at)->data[n] = str[n];

	return at;
}
