/*
 * $Id$
 * 
 * {License}
 */

#ifndef _ASE_LIB_CMN_MEM_H_
#define _ASE_LIB_CMN_MEM_H_

#ifdef USE_STDC

#include <string.h>
#define ASE_MEMCPY(dst,src,len) memcpy(dst,src,len)
#define ASE_MEMCMP(p1,p2,len) memcmp(p1,p2,len)
#define ASE_MEMSET(dst,val,len) memset(dst,val,len)

#else

#include <ase/cmn/mem.h>
#define ASE_MEMCPY(dst,src,len) ase_memcpy(dst,src,len)
#define ASE_MEMCMP(p1,p2,len) ase_memcmp(p1,p2,len)
#define ASE_MEMSET(dst,val,len) ase_memset(dst,val,len)

#endif

#endif
