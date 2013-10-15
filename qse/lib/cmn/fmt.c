/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
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
#include "fmt.h"

#undef T
#undef char_t
#undef fmt_uintmax
#undef strlen

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define fmt_uintmax fmt_unsigned_to_wcs
#define strlen(x) qse_wcslen(x)
#include "fmt.h"

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

