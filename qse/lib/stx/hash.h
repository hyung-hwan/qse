/*
 * $Id$
 */

#ifndef _QSE_LIB_STX_MISC_H_
#define _QSE_LIB_STX_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_hashbytes (
	qse_stx_t*        stx,
	const void*       data,
	qse_word_t        len
);

qse_word_t qse_stx_hashstr (
	qse_stx_t*        stx,
	const qse_char_t* str
);

qse_word_t qse_stx_hashstrx (
	qse_stx_t*        stx,
	const qse_char_t* str,
	qse_word_t        len
);

#ifdef __cplusplus
}
#endif

#endif
