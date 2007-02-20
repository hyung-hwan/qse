/*
 * $id$
 */

#ifndef _ASE_CTYPE_H_
#define _ASE_CTYPE_H_

#include <ase/types.h>
#include <ase/macros.h>

#if defined(ASE_CHAR_IS_MCHAR) 
	#include <ctype.h>

	#define ase_isupper(x)  isupper(x)
	#define ase_islower(x)  islower(x)
	#define ase_isalpha(x)  isalpha(x)
	#define ase_isdigit(x)  isdigit(x)
	#define ase_isxdigit(x) isxdigit(x)
	#define ase_isalnum(x)  isalnum(x)
	#define ase_isspace(x)  isspace(x)
	#define ase_isprint(x)  isprint(x)
	#define ase_isgraph(x)  isgraph(x)
	#define ase_iscntrl(x)  iscntrl(x)
	#define ase_ispunct(x)  ispunct(x)

	#define ase_toupper(x)  tolower(x)
	#define ase_tolower(x)  tolower(x)
#else
	#include <wctype.h>

	#define ase_isupper(x)  iswupper(x)
	#define ase_islower(x)  iswlower(x)
	#define ase_isalpha(x)  iswalpha(x)
	#define ase_isdigit(x)  iswdigit(x)
	#define ase_isxdigit(x) iswxdigit(x)
	#define ase_isalnum(x)  iswalnum(x)
	#define ase_isspace(x)  iswspace(x)
	#define ase_isprint(x)  iswprint(x)
	#define ase_isgraph(x)  iswgraph(x)
	#define ase_iscntrl(x)  iswcntrl(x)
	#define ase_ispunct(x)  iswpunct(x)

	#define ase_toupper(x)  towlower(x)
	#define ase_tolower(x)  towlower(x)
#endif

#endif
