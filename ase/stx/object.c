/*
 * $Id: object.c,v 1.7 2005-05-10 08:21:10 bacon Exp $
 */

#include <xp/stx/object.h>
#include <xp/stx/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>

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

	n = xp_strlen(str);
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

/*
xp_stx_word_t xp_stx_new_symbol (xp_stx_t* stx, xp_stx_char_t* name)
{
	xp_stx_word_t x;

	x = xp_stx_alloc_string_object (stx, name);
	XP_STX_CLASS(&stx,x) = stx->class_string;
	return x;
}
*/

/*
struct class_info_t
{
	const xp_stx_char_t* name;
	xp_size_word_t inst_vars;
};
typedef struct class_info_t class_info_t;

class_info_t class_info[] = 
{
	{ XP_STX_TEXT("Class"), 5 },
	{ XP_STX_TEXT("Metaclass"), 5 },
	{ XP_NULL, 0 }
};
*/


/*

xp_stx_word_t xp_stx_instantiate_class (xp_stx_t* stx, xp_stx_char_t* name)
{
	xp_stx_word_t x;

	x = xp_str_alloc_object (str, classSize);
	XP_STX_CLASS(stx,x) = globalValue("Metaclass");
	XP_STX_AT(stx,x,sizeInClass) = XP_STX_TO_SMALLINT(classSize);
	
	y = xp_str_alloc_object (str, classSize):
	XP_STX_CLASS(stx,y) = x;
		
	return x;
}

xp_stx_word_t xp_stx_instantiate_string (xp_stx_t* stx, xp_stx_char_t* str)
{
	xp_stx_word_t x;
	x = xp_stx_alloc_string_object (stx, str);
	XP_STX_CLASS(&stx->memory,x) = stx->class_string;
	return x;	
}

*/
