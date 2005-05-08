/*
 * $Id: memory.h,v 1.5 2005-05-08 10:31:24 bacon Exp $
 */

#ifndef _XP_STX_MEMORY_H_
#define _XP_STX_MEMORY_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_memory_t* xp_stx_memory_open (
	xp_stx_memory_t* mem, xp_stx_word_t capacity);
void xp_stx_memory_close (xp_stx_memory_t* mem);

void xp_stx_memory_gc (xp_stx_memory_t* mem);
xp_stx_word_t xp_stx_memory_alloc (xp_stx_memory_t* mem, xp_stx_word_t size);
void xp_stx_memory_dealloc (xp_stx_memory_t* mem, xp_stx_word_t object_index);

#ifdef __cplusplus
}
#endif

#endif
