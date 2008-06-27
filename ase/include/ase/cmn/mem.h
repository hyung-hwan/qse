/*
 * $Id: mem.h 223 2008-06-26 06:44:41Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_MEM_H_
#define _ASE_CMN_MEM_H_

#include <ase/types.h>
#include <ase/macros.h>

#ifdef __cplusplus
extern "C" {
#endif

void* ase_memcpy (void* dst, const void* src, ase_size_t n);
void* ase_memmove (void* dst, const void* src, ase_size_t n);
void* ase_memset (void* dst, int val, ase_size_t n);
int   ase_memcmp (const void* s1, const void* s2, ase_size_t n);
void* ase_memchr (const void* s, int val, ase_size_t n);
void* ase_memrchr (const void* s, int val, ase_size_t n);
void* ase_memmem (const void* hs, ase_size_t hl, const void* nd, ase_size_t nl);
void* ase_memrmem (const void* hs, ase_size_t hl, const void* nd, ase_size_t nl);

#ifdef __cplusplus
}
#endif

#endif