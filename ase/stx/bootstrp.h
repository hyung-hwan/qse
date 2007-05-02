/*
 * $Id: bootstrp.h,v 1.3 2007/04/30 08:32:40 bacon Exp $
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
