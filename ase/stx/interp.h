/*
 * $Id: interp.h 118 2008-03-03 11:21:33Z baconevi $
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
