/*
 * $Id: helper.h 231 2008-06-28 08:37:09Z baconevi $
 */

#ifndef _ASE_UTL_HELPER_H_
#define _ASE_UTL_HELPER_H_

#include <ase/types.h>
#include <ase/macros.h>

#define ASE_GETMMGR()  (ase_mmgr)
#define ASE_SETMMGR(m) ((ase_mmgr) = (m))

#define ASE_GETCCLS()  (ase_ccls)
#define ASE_SETCCLS(c) ((ase_ccls) = (c))

#ifdef __cplusplus
extern "C" {
#endif

extern ase_mmgr_t* ase_mmgr;
extern ase_ccls_t* ase_ccls;

#ifdef __cplusplus
}
#endif

#endif
