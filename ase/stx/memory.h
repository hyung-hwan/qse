/*
 * $Id: memory.h,v 1.1 2007/03/28 14:05:28 bacon Exp $
 */

#ifndef _ASE_STX_MEMORY_H_
#define _ASE_STX_MEMORY_H_

#include <ase/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

ase_stx_memory_t* ase_stx_memory_open (
	ase_stx_memory_t* mem, ase_word_t capacity);
void ase_stx_memory_close (ase_stx_memory_t* mem);

void ase_stx_memory_gc (ase_stx_memory_t* mem);
ase_word_t ase_stx_memory_alloc (ase_stx_memory_t* mem, ase_word_t size);
void ase_stx_memory_dealloc (ase_stx_memory_t* mem, ase_word_t object_index);

#ifdef __cplusplus
}
#endif

#endif
