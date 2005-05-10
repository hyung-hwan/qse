/*
 * $Id: object.c,v 1.10 2005-05-10 15:15:57 bacon Exp $
 */

#include <xp/stx/object.h>
#include <xp/stx/memory.h>
#include <xp/stx/hash.h>
#include <xp/bas/assert.h>
#include <xp/bas/stdarg.h>

xp_stx_word_t xp_stx_strlen (const xp_stx_char_t* str)
{
	const xp_stx_char_t* p = str;
	while (*p != XP_STX_CHAR('\0')) p++;
	return p - str;
}

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

	xp_assert (stx->nil == XP_STX_NIL);
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_INDEXED;
	while (n--) XP_STX_AT(stx,idx,n) = stx->nil;

	return idx;
}

/* n: number of bytes */
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_stx_word_t n)
{
	xp_stx_word_t idx;

	idx = xp_stx_memory_alloc (
		&stx->memory, n + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	xp_assert (stx->nil == XP_STX_NIL);
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_BYTE_INDEXED;
	while (n--) XP_STX_BYTEAT(stx,idx,n) = 0;

	return idx;
}

xp_stx_word_t xp_stx_alloc_string_object (
	xp_stx_t* stx, const xp_stx_char_t* str)
{
	xp_stx_word_t idx, n;

	n = xp_stx_strlen(str);
	idx = xp_stx_memory_alloc (&stx->memory, 
		(n + 1) * xp_sizeof(xp_stx_char_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	xp_assert (stx->nil == XP_STX_NIL);
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_CHAR_INDEXED;
	XP_STX_CHARAT(stx,idx,n) = XP_STX_CHAR('\0');
	while (n--) XP_STX_CHARAT(stx,idx,n) = str[n];

	return idx;
}

xp_stx_word_t xp_stx_allocn_string_object (xp_stx_t* stx, ...)
{
	xp_stx_word_t idx, n = 0;
	const xp_stx_char_t* p;
	xp_va_list ap;

	xp_va_start (ap, stx);
	while ((p = xp_va_arg(ap, const xp_stx_char_t*)) != XP_NULL) {
		n += xp_stx_strlen(p);
	}
	xp_va_end (ap);

	idx = xp_stx_memory_alloc (&stx->memory, 
		(n + 1) * xp_sizeof(xp_stx_char_t) + xp_sizeof(xp_stx_object_t));
	if (idx >= stx->memory.capacity) return idx; /* failed */

	xp_assert (stx->nil == XP_STX_NIL);
	XP_STX_CLASS(stx,idx) = stx->nil;
	XP_STX_ACCESS(stx,idx) = (n << 2) | XP_STX_CHAR_INDEXED;
	XP_STX_CHARAT(stx,idx,n) = XP_STX_CHAR('\0');

	xp_va_start (ap, stx);
	n = 0;
	while ((p = xp_va_arg(ap, const xp_stx_char_t*)) != XP_NULL) {
		while (*p != XP_STX_CHAR('\0')) 
			XP_STX_CHARAT(stx,idx,n) = *p++;
	}
	xp_va_end (ap);

	return idx;
}

xp_stx_word_t xp_stx_hash_string_object (xp_stx_t* stx, xp_stx_word_t idx)
{
	xp_stx_word_t nb, h = 0;
	xp_byte_t* p, * end;

	xp_assert (XP_STX_TYPE(stx,idx) == XP_STX_CHAR_INDEXED);
	nb = XP_STX_SIZE(stx,idx) * xp_sizeof(xp_stx_char_t);
	p = (xp_byte_t*)&XP_STX_AT(stx,idx,0); end = p + nb;

	while (p < end) h = h * 31 + *p++;
	return h;
}

xp_stx_word_t xp_stx_new_string_object (
	xp_stx_t* stx, const xp_stx_char_t* name, xp_stx_word_t class)
{
	xp_stx_word_t x;
	x = xp_stx_alloc_string_object (stx, name);
	XP_STX_CLASS(stx,x) = class;
	return x;
}

xp_stx_word_t xp_stx_new_class (xp_stx_t* stx, xp_stx_char_t* name)
{
	xp_stx_word_t meta, class;
	xp_stx_word_t meta_name, class_name;

	meta = xp_stx_alloc_object (stx, XP_STX_CLASS_DIMENSION);
	XP_STX_CLASS(stx,meta) = stx->class_metaclass;
	XP_STX_AT(stx,meta,XP_STX_CLASS_SIZE) = 
		XP_STX_TO_SMALLINT(XP_STX_CLASS_DIMENSION);
	
	class = xp_stx_alloc_object (stx, XP_STX_CLASS_DIMENSION);
	XP_STX_CLASS(stx,class) = meta;

	meta_name = xp_stx_new_string_object (stx, name, stx->class_symbol);
	XP_STX_AT(stx,meta,XP_STX_CLASS_NAME) = meta_name;
	class_name = xp_stx_new_string_object (stx, name, stx->class_symbol);
	XP_STX_AT(stx,class,XP_STX_CLASS_NAME) = class_name;

	xp_stx_hash_insert (stx, stx->symbol_table, 
		xp_stx_hash_string_object(stx, meta_name),
		meta_name, meta);
	xp_stx_hash_insert (stx, stx->symbol_table, 
		xp_stx_hash_string_object(stx, class_name),
		meta_name, class);

	return class;
}

