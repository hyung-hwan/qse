/*
 * $Id: interp.h,v 1.1 2007/03/28 14:05:28 bacon Exp $
 */

#ifndef _ASE_STX_INTERP_H_
#define _ASE_STX_INTERP_H_

#include <ase/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

int ase_stx_interp (ase_stx_t* stx, ase_word_t receiver, ase_word_t method);

#ifdef __cplusplus
}
#endif

#endif
