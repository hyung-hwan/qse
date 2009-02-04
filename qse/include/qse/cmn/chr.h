/*
 * $Id: ctype.h 223 2008-06-26 06:44:41Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_CMN_CHR_H_
#define _QSE_CMN_CHR_H_

#include <qse/types.h>
#include <qse/macros.h>

/* gets a pointer to the default memory manager */
#define QSE_CCLS_GETDFL()  (qse_ccls)

/* sets a pointer to the default memory manager */
#define QSE_CCLS_SETDFL(m) ((qse_ccls)=(m))

#define QSE_CCLS_IS(ccls,c,type) ((ccls)->is((ccls)->data,c,type))
#define QSE_CCLS_TO(ccls,c,type) ((ccls)->to((ccls)->data,c,type))

#define QSE_CCLS_ISUPPER(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_UPPER)
#define QSE_CCLS_ISLOWER(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_LOWER)
#define QSE_CCLS_ISALPHA(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_ALPHA)
#define QSE_CCLS_ISDIGIT(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_DIGIT)
#define QSE_CCLS_ISXDIGIT(ccls,c) QSE_CCLS_IS(ccls,c,QSE_CCLS_XDIGIT)
#define QSE_CCLS_ISALNUM(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_ALNUM)
#define QSE_CCLS_ISSPACE(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_SPACE)
#define QSE_CCLS_ISPRINT(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_PRINT)
#define QSE_CCLS_ISGRAPH(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_GRAPH)
#define QSE_CCLS_ISCNTRL(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_CNTRL)
#define QSE_CCLS_ISPUNCT(ccls,c)  QSE_CCLS_IS(ccls,c,QSE_CCLS_PUNCT)
#define QSE_CCLS_TOUPPER(ccls,c)  QSE_CCLS_TO(ccls,c,QSE_CCLS_UPPER)
#define QSE_CCLS_TOLOWER(ccls,c)  QSE_CCLS_TO(ccls,c,QSE_CCLS_LOWER)

#ifdef __cplusplus
extern "C" {
#endif

extern qse_ccls_t* qse_ccls;

qse_bool_t qse_ccls_is (
	qse_cint_t      c,
	qse_ccls_type_t type
);

qse_cint_t qse_ccls_to (
	qse_cint_t      c,
	qse_ccls_type_t type
);



qse_size_t qse_mblen (
	const qse_mchar_t* mb,
	qse_size_t mblen
);

/****f* Common/qse_mbtowc
 * NAME
 *  qse_mbtowc - convert a multibyte sequence to a wide character.
 * RETURN
 *  The qse_mbtowc() function returns 0 if an invalid multibyte sequence is
 *  detected, mblen + 1 if the sequence is incomplete. It returns the number
 *  of bytes processed to form a wide character.
 * SYNOPSIS
 */
qse_size_t qse_mbtowc (
	const qse_mchar_t* mb,
	qse_size_t         mblen,
	qse_wchar_t*       wc
);
/******/

/****f* Common/qse_wctomb
 * NAME
 *  qse_wctomb - convert a wide character to a multibyte sequence
 * RETURN
 *  The qse_wctomb() functions returns 0 if the wide character is illegal, 
 *  mblen + 1 if mblen is not large enough to hold the multibyte sequence.
 *  On successful conversion, it returns the number of bytes in the sequence.
 * SYNOPSIS
 */
qse_size_t qse_wctomb (
	qse_wchar_t        wc,
	qse_mchar_t*       mb,
	qse_size_t         mblen
);
/******/

#ifdef __cplusplus
}
#endif

#endif
