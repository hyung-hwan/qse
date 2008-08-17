/*
 * $Id: helper.h 329 2008-08-16 14:08:53Z baconevi $
 */

#ifndef _ASE_UTL_HELPER_H_
#define _ASE_UTL_HELPER_H_

#include <ase/types.h>
#include <ase/macros.h>

#define ASE_GETCCLS()  (ase_ccls)
#define ASE_SETCCLS(c) ((ase_ccls) = (c))

#ifdef __cplusplus
extern "C" {
#endif

extern ase_ccls_t* ase_ccls;

#ifdef __cplusplus
}
#endif

#endif
