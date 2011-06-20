/*
 * $Id$
 */

#ifndef _QSE_LIB_STX_BOOT_H_
#define _QSE_LIB_STX_BOOT_H_

#ifdef __cplusplus
extern "C" {
#endif

int qse_stx_boot (
	qse_stx_t* stx
);

qse_word_t qse_stx_findclass (qse_stx_t* stx, const qse_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
