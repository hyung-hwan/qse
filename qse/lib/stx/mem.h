/*
 * $Id$
 */

#ifndef _QSE_LIB_STX_MEM_H_
#define _QSE_LIB_STX_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

int qse_stx_initmem (
	qse_stx_t* stx,
	qse_size_t capa
);

void qse_stx_finimem (
	qse_stx_t* stx
);

void qse_stx_gcmem (
	qse_stx_t* stx
);

qse_stx_objidx_t qse_stx_allocmem (
	qse_stx_t* stx,
	qse_size_t nbytes
);

void qse_stx_freemem (
	qse_stx_t*       stx,
	qse_stx_objidx_t objidx
);

void qse_stx_swapmem (
	qse_stx_t*       stx,
	qse_stx_objidx_t idx1,
	qse_stx_objidx_t idx2
);

#ifdef __cplusplus
}
#endif

#endif
