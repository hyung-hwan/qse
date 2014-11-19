/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/fmt.h>
#include <qse/cmn/str.h>

#undef T
#undef char_t
#undef fmt_uintmax
#undef strlen

#define T(x) QSE_MT(x)
#define char_t qse_mchar_t
#define fmt_uintmax fmt_unsigned_to_mbs
#define strlen(x) qse_mbslen(x)
#include "fmt-intmax.h"

#undef T
#undef char_t
#undef fmt_uintmax
#undef strlen

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define fmt_uintmax fmt_unsigned_to_wcs
#define strlen(x) qse_wcslen(x)
#include "fmt-intmax.h"

/* ==================== multibyte ===================================== */

int qse_fmtintmaxtombs (
	qse_mchar_t* buf, int size, 
	qse_intmax_t value, int base_and_flags, int prec,
	qse_mchar_t fillchar, const qse_mchar_t* prefix)
{
	qse_mchar_t signchar;
	qse_uintmax_t absvalue;

	if (value < 0)
	{
		signchar = QSE_MT('-');
		absvalue = -value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_PLUSSIGN)
	{
		signchar = QSE_MT('+');
		absvalue = value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_MT(' ');
		absvalue = value;
	}
	else
	{
		signchar = QSE_MT('\0');
		absvalue = value;
	}

	return fmt_unsigned_to_mbs (
		buf, size, absvalue, base_and_flags, prec, fillchar, signchar, prefix);
}

int qse_fmtuintmaxtombs (
	qse_mchar_t* buf, int size, 
	qse_uintmax_t value, int base_and_flags, int prec,
	qse_mchar_t fillchar, const qse_mchar_t* prefix)
{
	qse_mchar_t signchar;

	/* determine if a sign character is needed */
	if (base_and_flags & QSE_FMTINTMAXTOMBS_PLUSSIGN)
	{
		signchar = QSE_MT('+');
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_MT(' ');
	}
	else
	{
		signchar = QSE_MT('\0');
	}

	return fmt_unsigned_to_mbs (
		buf, size, value, base_and_flags, prec, fillchar, signchar, prefix);
}

/* ==================== wide-char ===================================== */

int qse_fmtintmaxtowcs (
	qse_wchar_t* buf, int size, 
	qse_intmax_t value, int base_and_flags, int prec,
	qse_wchar_t fillchar, const qse_wchar_t* prefix)
{
	qse_wchar_t signchar;
	qse_uintmax_t absvalue;

	if (value < 0)
	{
		signchar = QSE_WT('-');
		absvalue = -value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOWCS_PLUSSIGN)
	{
		signchar = QSE_WT('+');
		absvalue = value;
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_WT(' ');
		absvalue = value;
	}
	else
	{
		signchar = QSE_WT('\0');
		absvalue = value;
	}

	return fmt_unsigned_to_wcs (
		buf, size, absvalue, base_and_flags, prec, fillchar, signchar, prefix);
}

int qse_fmtuintmaxtowcs (
	qse_wchar_t* buf, int size, 
	qse_uintmax_t value, int base_and_flags, int prec,
	qse_wchar_t fillchar, const qse_wchar_t* prefix)
{
	qse_wchar_t signchar;

	/* determine if a sign character is needed */
	if (base_and_flags & QSE_FMTINTMAXTOWCS_PLUSSIGN)
	{
		signchar = QSE_WT('+');
	}
	else if (base_and_flags & QSE_FMTINTMAXTOMBS_EMPTYSIGN)
	{
		signchar = QSE_WT(' ');
	}
	else
	{
		signchar = QSE_WT('\0');
	}

	return fmt_unsigned_to_wcs (
		buf, size, value, base_and_flags, prec, fillchar, signchar, prefix);
}


/* ==================== floating-point number =========================== */

/* TODO: finish this function */
int qse_fmtfltmaxtombs (qse_mchar_t* buf, qse_size_t bufsize, qse_fltmax_t f, qse_mchar_t point, int digits)
{
	qse_size_t len;
	qse_uintmax_t v;

	/*if (bufsize <= 0) return -reqlen; TODO: */

	if (f < 0) 
	{
		f *= -1;
		v = (qse_uintmax_t)f;  
		len = qse_fmtuintmaxtombs (buf, bufsize, v, 10, 0, QSE_MT('\0'), QSE_MT("-"));
	}
	else
	{
		v = (qse_uintmax_t)f;  
		len = qse_fmtuintmaxtombs (buf, bufsize, v, 10, 0, QSE_MT('\0'), QSE_NULL);
	}
 
	if (len + 1 < bufsize)
	{
		buf[len++] = point;  // add decimal point to string
		buf[len] = QSE_MT('\0');
	}
        
	while (len + 1 < bufsize && digits-- > 0)
	{
		f = (f - (qse_fltmax_t)v) * 10;
		v = (qse_uintmax_t)f;
		len += qse_fmtuintmaxtombs (&buf[len], bufsize - len, v, 10, 0, QSE_MT('\0'), QSE_NULL);
	}

	return (int)len;
}

