/*
 * $Id: stx.h,v 1.1 2005-05-08 07:39:51 bacon Exp $
 */

#ifndef _XP_STX_STX_H_
#define _XP_STX_STX_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef xp_byte_t xp_stx_byte_t;
typedef xp_char_t xp_stx_char_t;
typedef xp_size_t xp_stx_word_t;
typedef xp_size_t xp_stx_size_t;
typedef xp_size_t xp_stx_index_t;

typedef struct xp_stx_object_t xp_stx_object_t;
typedef struct xp_stx_byte_object_t xp_stx_byte_object_t;
typedef struct xp_stx_string_object_t xp_stx_string_object_t;
typedef struct xp_stx_memory_t xp_stx_memory_t;
typedef struct xp_stx_t xp_stx_t;


#define XP_STX_CHAR(x) XP_CHAR(x)
#define XP_STX_TEXT(x) XP_TEXT(x)

/* common object header structure */
struct xp_stx_object_t
{
	/* access - is_byte_indexed: 1; size: rest */
	xp_stx_word_t access; 
	xp_stx_word_t class;
	xp_stx_word_t data[1];
};

struct xp_stx_byte_object_t
{
	xp_stx_word_t access; 
	xp_stx_word_t class;
	xp_stx_byte_t data[1];
};

struct xp_stx_string_object_t
{
	xp_stx_word_t access; 
	xp_stx_word_t class;
	xp_stx_char_t data[1];
};

struct xp_stx_memory_t
{
	xp_stx_word_t capacity;
	xp_stx_object_t** slots;
	xp_stx_object_t** free;
	xp_bool_t __malloced;
};

struct xp_stx_t
{
	xp_stx_memory_t memory;

	xp_stx_word_t nil;
	xp_stx_word_t true;
	xp_stx_word_t false;

	xp_bool_t __malloced;
};

#define XP_STX_OBJECT(mem,at) \
	((xp_stx_object_t*)((mem)->slots[at]))
#define XP_STX_BYTE_OBJECT(mem,at) \
	((xp_stx_byte_object_t*)((mem)->slots[at]))
#define XP_STX_STRING_OBJECT(mem,at) \
	((xp_stx_string_object_t*)((mem)->slots[at]))

#define XP_STX_OBJECT_ACCESS(mem,at) ((XP_STX_OBJECT(mem,at))->access)
#define XP_STX_OBJECT_CLASS(mem,at) ((XP_STX_OBJECT(mem,at))->class)

#define XP_STX_OBJECT_DATA(mem,at) \
	(((XP_STX_OBJECT_ACCESS(mem,at) & 0x04) == 0x00)? \
		(XP_STX_OBJECT(mem,at)).data): \
	 (((XP_STX_OBJECT_ACCESS(mem,at) & 0x04) == 0x01)? \
		(XP_STX_BYTE_OBJECT(mem,at)).data): \
		(XP_STX_STRING_OBJECT(mem,at)).data))
		

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_stx_word_t capacity);
void xp_stx_close (xp_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
