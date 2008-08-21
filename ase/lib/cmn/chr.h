/*
 * $Id$
 * 
 * {License}
 */

#ifndef _ASE_LIB_CMN_CHR_H_
#define _ASE_LIB_CMN_CHR_H_

#include <ase/cmn/chr.h>

#ifdef USE_STDC

	#if defined(ASE_CHAR_IS_MCHAR)

		#include <ctype.h>
		#define ASE_ISUPPER(c) isupper(c)
		#define ASE_ISLOWER(c) islower(c)
		#define ASE_ISALPHA(c) isalpha(c)
		#define ASE_ISDIGIT(c) isdigit(c)
		#define ASE_ISXDIGIT(c) isxdigit(c)
		#define ASE_ISALNUM(c) isalnum(c)
		#define ASE_ISSPACE(c) isspace(c)
		#define ASE_ISPRINT(c) isprint(c)
		#define ASE_ISGRAPH(c) isgraph(c)
		#define ASE_ISCNTRL(c) iscntrl(c)
		#define ASE_ISPUNCT(c) ispunct(c)
		#define ASE_TOUPPER(c) toupper(c)
		#define ASE_TOLOWER(c) tolower(c)
	
	#elif defined(ASE_CHAR_IS_WCHAR)
	
		#include <ctype.h>
		#include <wctype.h>
		#define ASE_ISUPPER(c) iswupper(c)
		#define ASE_ISLOWER(c) iswlower(c)
		#define ASE_ISALPHA(c) iswalpha(c)
		#define ASE_ISDIGIT(c) iswdigit(c)
		#define ASE_ISXDIGIT(c) iswxdigit(c)
		#define ASE_ISALNUM(c) iswalnum(c)
		#define ASE_ISSPACE(c) iswspace(c)
		#define ASE_ISPRINT(c) iswprint(c)
		#define ASE_ISGRAPH(c) iswgraph(c)
		#define ASE_ISCNTRL(c) iswcntrl(c)
		#define ASE_ISPUNCT(c) iswpunct(c)
		#define ASE_TOUPPER(c) towupper(c)
		#define ASE_TOLOWER(c) towlower(c)

	#else
		#error Unsupported character type
	#endif

#else

	#define ASE_ISUPPER(c) (ase_ccls_is(c,ASE_CCLS_UPPER))
	#define ASE_ISLOWER(c) (ase_ccls_is(c,ASE_CCLS_LOWER))
	#define ASE_ISALPHA(c) (ase_ccls_is(c,ASE_CCLS_ALPHA))
	#define ASE_ISDIGIT(c) (ase_ccls_is(c,ASE_CCLS_DIGIT))
	#define ASE_ISXDIGIT(c) (ase_ccls_is(c,ASE_CCLS_XDIGIT))
	#define ASE_ISALNUM(c) (ase_ccls_is(c,ASE_CCLS_ALNUM))
	#define ASE_ISSPACE(c) (ase_ccls_is(c,ASE_CCLS_SPACE))
	#define ASE_ISPRINT(c) (ase_ccls_is(c,ASE_CCLS_PRINT))
	#define ASE_ISGRAPH(c) (ase_ccls_is(c,ASE_CCLS_GRAPH))
	#define ASE_ISCNTRL(c) (ase_ccls_is(c,ASE_CCLS_CNTRL))
	#define ASE_ISPUNCT(c) (ase_ccls_is(c,ASE_CCLS_PUNCT))
	#define ASE_TOUPPER(c) (ase_ccls_to(c,ASE_CCLS_UPPER))
	#define ASE_TOLOWER(c) (ase_ccls_to(c,ASE_CCLS_LOWER))

#endif

#endif
