/*
 * $Id: bootstrp.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_BOOTSTRP_H_
#define _QSE_STX_BOOTSTRP_H_

#include <qse/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_new_array (qse_stx_t* stx, qse_word_t size);
int qse_stx_bootstrap (qse_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
