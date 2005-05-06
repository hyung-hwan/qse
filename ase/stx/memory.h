/*
 * $Id: memory.h,v 1.3 2005-05-06 17:17:59 bacon Exp $
 */

#ifndef _XP_STX_MEMORY_H_
#define _XP_STX_MEMORY_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_stx_memory_t xp_stx_memory_t;
typedef struct xp_stx_object_t xp_stx_object_t;
typedef struct xp_stx_byte_object_t xp_stx_byte_object_t;
typedef struct xp_stx_string_object_t xp_stx_string_object_t;

typedef xp_byte_t xp_stx_byte_t;
typedef xp_char_t xp_stx_char_t;
typedef xp_size_t xp_stx_word_t;
typedef xp_size_t xp_stx_size_t;
typedef xp_size_t xp_stx_index_t;
typedef xp_stx_object_t* xp_stx_pointer_t;


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

#define XP_STX_NIL (0)

#define XP_STX_OBJECT(mem,at) \
	((xp_stx_object_t*)(mem->slots[at]))
#define XP_STX_BYTE_OBJECT(mem,at) \
	((xp_stx_byte_object_t*)(mem->slots[at]))
#define XP_STX_STRING_OBJECT(mem,at) \
	((xp_stx_string_object_t*)(mem->slots[at]))

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

xp_stx_memory_t* xp_stx_memory_open (
	xp_stx_memory_t* mem, xp_stx_word_t capacity);
void xp_stx_memory_close (xp_stx_memory_t* mem);

void xp_stx_gaxpage_collect (xp_stx_memory_t* mem);
xp_stx_word_t xp_stx_alloc_object (xp_stx_memory_t* mem, xp_stx_word_t size);
void xp_stx_dealloc_object (xp_stx_memory_t* mem, xp_stx_word_t object_index);

#ifdef __cplusplus
}
#endif

#endif
