/*
 * $Id: bootstrp.h,v 1.1.1.1 2007/03/28 14:05:25 bacon Exp $
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
