/*
 * $Id$
 * 
 * {License}
 */

#ifndef _ASE_LIB_CMN_CHR_H_
#define _ASE_LIB_CMN_CHR_H_

#include <ase/cmn/chr.h>

#ifdef USE_STDC

#include <ctype.h>

#else

#define ASE_ISALPHA(c) ase_isalpha(c)

#endif

#endif
