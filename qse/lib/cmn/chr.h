/*
 * $Id: chr.h 76 2009-02-22 14:18:06Z hyunghwan.chung $
 * 
 * {License}
 */

#ifndef _QSE_LIB_CMN_CHR_H_
#define _QSE_LIB_CMN_CHR_H_

#include <qse/cmn/chr.h>

#ifdef USE_STDC

	#if defined(QSE_CHAR_IS_MCHAR)

		#include <ctype.h>
		#define QSE_ISUPPER(c) isupper(c)
		#define QSE_ISLOWER(c) islower(c)
		#define QSE_ISALPHA(c) isalpha(c)
		#define QSE_ISDIGIT(c) isdigit(c)
		#define QSE_ISXDIGIT(c) isxdigit(c)
		#define QSE_ISALNUM(c) isalnum(c)
		#define QSE_ISSPACE(c) isspace(c)
		#define QSE_ISPRINT(c) isprint(c)
		#define QSE_ISGRAPH(c) isgraph(c)
		#define QSE_ISCNTRL(c) iscntrl(c)
		#define QSE_ISPUNCT(c) ispunct(c)
		#define QSE_TOUPPER(c) toupper(c)
		#define QSE_TOLOWER(c) tolower(c)
	
	#elif defined(QSE_CHAR_IS_WCHAR)
	
		#include <wctype.h>
		#define QSE_ISUPPER(c) iswupper(c)
		#define QSE_ISLOWER(c) iswlower(c)
		#define QSE_ISALPHA(c) iswalpha(c)
		#define QSE_ISDIGIT(c) iswdigit(c)
		#define QSE_ISXDIGIT(c) iswxdigit(c)
		#define QSE_ISALNUM(c) iswalnum(c)
		#define QSE_ISSPACE(c) iswspace(c)
		#define QSE_ISPRINT(c) iswprint(c)
		#define QSE_ISGRAPH(c) iswgraph(c)
		#define QSE_ISCNTRL(c) iswcntrl(c)
		#define QSE_ISPUNCT(c) iswpunct(c)
		#define QSE_TOUPPER(c) towupper(c)
		#define QSE_TOLOWER(c) towlower(c)

	#else
		#error Unsupported character type
	#endif

#else

	#define QSE_ISUPPER(c) (qse_ccls_is(c,QSE_CCLS_UPPER))
	#define QSE_ISLOWER(c) (qse_ccls_is(c,QSE_CCLS_LOWER))
	#define QSE_ISALPHA(c) (qse_ccls_is(c,QSE_CCLS_ALPHA))
	#define QSE_ISDIGIT(c) (qse_ccls_is(c,QSE_CCLS_DIGIT))
	#define QSE_ISXDIGIT(c) (qse_ccls_is(c,QSE_CCLS_XDIGIT))
	#define QSE_ISALNUM(c) (qse_ccls_is(c,QSE_CCLS_ALNUM))
	#define QSE_ISSPACE(c) (qse_ccls_is(c,QSE_CCLS_SPACE))
	#define QSE_ISPRINT(c) (qse_ccls_is(c,QSE_CCLS_PRINT))
	#define QSE_ISGRAPH(c) (qse_ccls_is(c,QSE_CCLS_GRAPH))
	#define QSE_ISCNTRL(c) (qse_ccls_is(c,QSE_CCLS_CNTRL))
	#define QSE_ISPUNCT(c) (qse_ccls_is(c,QSE_CCLS_PUNCT))
	#define QSE_TOUPPER(c) (qse_ccls_to(c,QSE_CCLS_UPPER))
	#define QSE_TOLOWER(c) (qse_ccls_to(c,QSE_CCLS_LOWER))

#endif

#endif
