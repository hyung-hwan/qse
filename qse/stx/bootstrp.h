/*
 * $Id: bootstrp.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _ASE_STX_BOOTSTRP_H_
#define _ASE_STX_BOOTSTRP_H_

#include <ase/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

ase_word_t ase_stx_new_array (ase_stx_t* stx, ase_word_t size);
int ase_stx_bootstrap (ase_stx_t* stx);

#ifdef __cplusplus
}
#endif

#endif
