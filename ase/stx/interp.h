/*
 * $Id: interp.h,v 1.7 2007-03-22 11:19:28 bacon Exp $
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
