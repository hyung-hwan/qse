/*
 * $Id: interp.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_INTERP_H_
#define _QSE_STX_INTERP_H_

#include <qse/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

int qse_stx_interp (qse_stx_t* stx, qse_word_t receiver, qse_word_t method);

#ifdef __cplusplus
}
#endif

#endif
