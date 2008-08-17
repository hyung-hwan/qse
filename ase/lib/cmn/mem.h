/*
 * $Id$
 * 
 * {License}
 */

#ifndef _ASE_LIB_CMN_MEM_H_
#define _ASE_LIB_CMN_MEM_H_

#include <ase/cmn/mem.h>

#ifdef USE_STDC

#include <string.h>
#define ASE_MEMCPY(dst,src,len) memcpy(dst,src,len)
#define ASE_MEMCMP(p1,p2,len) memcmp(p1,p2,len)
#define ASE_MEMSET(dst,val,len) memset(dst,val,len)
#define ASE_MEMBYTE(s,val,len) memchr(s,val,len)
#define ASE_MEMRBYTE(s,val,len) memrchr(s,val,len)
#define ASE_MEMMEM(hs,hl,nd,nl) memmem(hs,hl,nd,nl)
#define ASE_MEMRMEM(hs,hl,nd,nl) memrmem(hs,hl,nd,nl)

#else

#define ASE_MEMCPY(dst,src,len) ase_memcpy(dst,src,len)
#define ASE_MEMCMP(p1,p2,len) ase_memcmp(p1,p2,len)
#define ASE_MEMSET(dst,val,len) ase_memset(dst,val,len)
#define ASE_MEMBYTE(s,val,len) ase_membyte(s,val,len)
#define ASE_MEMRBYTE(s,val,len) ase_memrbyte(s,val,len)
#define ASE_MEMMEM(hs,hl,nd,nl) ase_memmem(hs,hl,nd,nl)
#define ASE_MEMRMEM(hs,hl,nd,nl) ase_memrmem(hs,hl,nd,nl)

#endif

#endif
