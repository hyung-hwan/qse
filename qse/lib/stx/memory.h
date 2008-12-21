/*
 * $Id: memory.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_MEMORY_H_
#define _QSE_STX_MEMORY_H_

#include <qse/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

qse_stx_memory_t* qse_stx_memory_open (
	qse_stx_memory_t* mem, qse_word_t capacity);
void qse_stx_memory_close (qse_stx_memory_t* mem);

void qse_stx_memory_gc (qse_stx_memory_t* mem);
qse_word_t qse_stx_memory_alloc (qse_stx_memory_t* mem, qse_word_t size);
void qse_stx_memory_dealloc (qse_stx_memory_t* mem, qse_word_t object_index);

#ifdef __cplusplus
}
#endif

#endif
