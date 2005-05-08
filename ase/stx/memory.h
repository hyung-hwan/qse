/*
 * $Id: memory.h,v 1.4 2005-05-08 07:39:51 bacon Exp $
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

void xp_stx_gaxpage_collect (xp_stx_memory_t* mem);
xp_stx_word_t xp_stx_alloc_objmem (xp_stx_memory_t* mem, xp_stx_word_t size);
void xp_stx_dealloc_objmem (xp_stx_memory_t* mem, xp_stx_word_t object_index);

#ifdef __cplusplus
}
#endif

#endif
