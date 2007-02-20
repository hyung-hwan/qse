/*
 * $id$
 */

#ifndef _ASE_CTYPE_H_
#define _ASE_CTYPE_H_

#include <ase/types.h>
#include <ase/macros.h>

#if defined(ASE_CHAR_IS_MCHAR) 
	#include <ctype.h>

	#define ase_isupper  isupper
	#define ase_islower  islower
	#define ase_isalpha  isalpha
	#define ase_isdigit  isdigit
	#define ase_isxdigit isxdigit
	#define ase_isalnum  isalnum
	#define ase_isspace  isspace
	#define ase_isprint  isprint
	#define ase_isgraph  isgraph
	#define ase_iscntrl  iscntrl
	#define ase_ispunct  ispunct

	#define ase_toupper  tolower
	#define ase_tolower  tolower
#else
	#include <wctype.h>

	#define ase_isupper  iswupper
	#define ase_islower  iswlower
	#define ase_isalpha  iswalpha
	#define ase_isdigit  iswdigit
	#define ase_isxdigit iswxdigit
	#define ase_isalnum  iswalnum
	#define ase_isspace  iswspace
	#define ase_isprint  iswprint
	#define ase_isgraph  iswgraph
	#define ase_iscntrl  iswcntrl
	#define ase_ispunct  iswpunct

	#define ase_toupper  towlower
	#define ase_tolower  towlower
#endif

#endif
