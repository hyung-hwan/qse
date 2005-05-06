/*
 * $Id: memory.h,v 1.1 2005-05-06 15:54:47 bacon Exp $
 */

#ifndef _XP_STX_MEMORY_H_
#define _XP_STX_MEMORY_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_stx_memory_t xp_stx_memory_t;
typedef struct xp_stx_object_t xp_stx_object_t;

typedef xp_byte   xp_stx_byte_t;
typedef xp_size_t xp_stx_word_t;
typedef xp_size_t xp_stx_size_t;
typedef xp_size_t xp_stx_index_t;
typedef xp_stx_object_t* xp_stx_pointer_t;

#define XP_STX_OBJECT(mem,index) (mem->slots[index>>1])

/* access -  is_byte_indexed: 1; size: rest */
#define XP_STX_OBJECT_HEADER \
	xp_stx_word_t access; \
	xp_stx_word_t class

/* common object header structure */
struct xp_stx_object_t
{
	XP_STX_OBJECT_HEADER;
};

struct xp_stx_memory_t
{
	xp_stx_word_t capacity;
	xp_stx_object_t** slots;
	xp_stx_object_t** free;

	xp_stx_word_t nil;
	//xp_stx_word_t smalltalk;
	//xp_stx_word_t classes[];
	xp_stx_word_t symbol_table;
};

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
