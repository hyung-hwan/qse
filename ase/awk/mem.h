/*
 * $Id: mem.h,v 1.1 2005-09-30 09:40:15 bacon Exp $
 */

#ifndef _XP_SCE_MEM_H_
#define _XP_SCE_MEM_H_

#include <xp/sce/sce.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_sce_mem_t* xp_sce_mem_open (
	xp_sce_mem_t* mem, xp_word_t capacity);
void xp_sce_mem_close (xp_sce_mem_t* mem);

void xp_sce_mem_gc (xp_sce_mem_t* mem);
xp_word_t xp_sce_mem_alloc (xp_sce_mem_t* mem, xp_word_t size);
void xp_sce_mem_dealloc (xp_sce_mem_t* mem, xp_word_t object_index);

#ifdef __cplusplus
}
#endif

#endif
