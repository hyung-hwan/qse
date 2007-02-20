/*
 * $id$
 */

#ifndef _ASE_CTYPE_H_
#define _ASE_CTYPE_H_

#include <ase/types.h>
#include <ase/macros.h>

#if defined(ASE_CHAR_IS_MCHAR) 
	#include <ctype.h>
	
	#if !defined(isupper) 
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
	#endif

	#if !defined(toupper)
		#define ase_toupper  toupper
		#define ase_tolower  tolower
	#endif

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

	#define ase_toupper  towupper
	#define ase_tolower  towlower
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ASE_CHAR_IS_MCHAR) && defined(isupper)
	int ase_isupper (int c);
	int ase_islower (int c);
	int ase_isalpha (int c);
	int ase_isdigit (int c);
	int ase_isxdigit (int c);
	int ase_isalnum (int c);
	int ase_isspace (int c);
	int ase_isprint (int c);
	int ase_isgraph (int c);
	int ase_iscntrl (int c);
	int ase_ispunct (int c);
#endif

#if defined(ASE_CHAR_IS_MCHAR) && defined(toupper)
	int ase_toupper (int c);
	int ase_tolower (int c);
#endif

#ifdef __cplusplus
}
#endif

#endif
