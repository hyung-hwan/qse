/*
 * $Id: mem.h,v 1.3 2007-03-06 14:16:52 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_CMN_MEM_H_
#define _ASE_CMN_MEM_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

#ifdef __cplusplus
extern "C" {
#endif

void* ase_memcpy (void* dst, const void* src, ase_size_t n);
void* ase_memset (void* dst, int val, ase_size_t n);
int   ase_memcmp (const void* s1, const void* s2, ase_size_t n);

#ifdef __cplusplus
}
#endif

#endif
