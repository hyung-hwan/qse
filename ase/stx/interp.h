/*
 * $Id: interp.h,v 1.3 2007/04/30 08:32:41 bacon Exp $
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
