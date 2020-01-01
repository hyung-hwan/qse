/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#include "awk-prv.h"

/*#define USE_REX */

#if defined(USE_REX)
#	include <qse/cmn/rex.h>
#else
#	include <qse/cmn/tre.h>
#endif

#include <qse/cmn/mbwc.h>

void* qse_awk_allocmem (qse_awk_t* awk, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC(awk, size);
	if (ptr == QSE_NULL) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

void* qse_awk_callocmem (qse_awk_t* awk, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC(awk, size);
	if (ptr) QSE_MEMSET (ptr, 0, size);
	else qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

void* qse_awk_reallocmem (qse_awk_t* awk, void* ptr, qse_size_t size)
{
	void* nptr = QSE_AWK_REALLOC(awk, ptr, size);
	if (!nptr) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return nptr;
}

qse_char_t* qse_awk_strdup (qse_awk_t* awk, const qse_char_t* s)
{
	qse_char_t* ptr = QSE_AWK_STRDUP(awk, s);
	if (!ptr) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

qse_char_t* qse_awk_strxdup (qse_awk_t* awk, const qse_char_t* s, qse_size_t l)
{
	qse_char_t* ptr = QSE_AWK_STRXDUP(awk, s, l);
	if (!ptr) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

qse_char_t* qse_awk_cstrdup (qse_awk_t* awk, const qse_cstr_t* s)
{
	qse_char_t* ptr = qse_cstrdup(s, qse_awk_getmmgr(awk));
	if (!ptr) qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}



qse_wchar_t* qse_awk_mbstowcsdup (qse_awk_t* awk, const qse_mchar_t* mbs, qse_size_t* _wcslen)
{
	qse_size_t mbslen, wcslen;
	qse_wchar_t* wcs;

	/* if i use qse_mbstowcsdupwithcmgr(), i cannot pinpoint the exact failure cause.
	 * let's do it differently. */
	if (qse_mbstowcswithcmgr(mbs, &mbslen, QSE_NULL, &wcslen, qse_awk_getcmgr(awk)) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	wcslen = wcslen + 1; /* for terminating null */

	wcs = qse_awk_allocmem(awk, QSE_SIZEOF(*wcs) * wcslen);
	if (!wcs) return QSE_NULL;

	qse_mbstowcswithcmgr (mbs, &mbslen, wcs, &wcslen, qse_awk_getcmgr(awk));
	if (_wcslen) *_wcslen = wcslen;
	return wcs;
}

qse_mchar_t* qse_awk_wcstombsdup (qse_awk_t* awk, const qse_wchar_t* wcs, qse_size_t* _mbslen)
{
	qse_size_t mbslen, wcslen;
	qse_mchar_t* mbs;

	if (qse_wcstombswithcmgr(wcs, &wcslen, QSE_NULL, &mbslen, qse_awk_getcmgr(awk)) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	mbslen = mbslen + 1; /* for terminating null */

	mbs = qse_awk_allocmem(awk, QSE_SIZEOF(*mbs) * mbslen);
	if (!mbs) return QSE_NULL;

	qse_wcstombswithcmgr (wcs, &wcslen, mbs, &mbslen, qse_awk_getcmgr(awk));
	if (_mbslen) *_mbslen = mbslen;
	return mbs;
}


qse_wchar_t* qse_awk_mbsntowcsdup (qse_awk_t* awk, const qse_mchar_t* mbs, qse_size_t _mbslen, qse_size_t* _wcslen)
{
	qse_size_t mbslen, wcslen;
	qse_wchar_t* wcs;

	/* if i use qse_mbstowcsdupwithcmgr(), i cannot pinpoint the exact failure cause.
	 * let's do it differently. */
	mbslen = _mbslen;
	if (qse_mbsntowcsnwithcmgr(mbs, &mbslen, QSE_NULL, &wcslen, qse_awk_getcmgr(awk)) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	wcs = qse_awk_allocmem(awk, QSE_SIZEOF(*wcs) * (wcslen + 1));
	if (!wcs) return QSE_NULL;

	mbslen= _mbslen;
	qse_mbsntowcsnwithcmgr (mbs, &mbslen, wcs, &wcslen, qse_awk_getcmgr(awk));
	wcs[wcslen] = QSE_WT('\0');

	if (_wcslen) *_wcslen = wcslen;
	return wcs;
}


qse_mchar_t* qse_awk_wcsntombsdup (qse_awk_t* awk, const qse_wchar_t* wcs, qse_size_t _wcslen, qse_size_t* _mbslen)
{
	qse_size_t mbslen, wcslen;
	qse_mchar_t* mbs;

	wcslen = _wcslen;
	if (qse_wcsntombsnwithcmgr(wcs, &wcslen, QSE_NULL, &mbslen, qse_awk_getcmgr(awk)) <= -1)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	mbs = qse_awk_allocmem(awk, QSE_SIZEOF(*mbs) * (mbslen + 1));
	if (!mbs) return QSE_NULL;

	wcslen = _wcslen;
	qse_wcsntombsnwithcmgr (wcs, &wcslen, mbs, &mbslen, qse_awk_getcmgr(awk));
	mbs[mbslen] = QSE_MT('\0');

	if (_mbslen) *_mbslen = mbslen;
	return mbs;
}

/* ========================================================================= */
#undef awk_strxtoint
#undef awk_strtoflt
#undef awk_strxtoflt
#undef char_t
#undef cint_t
#undef AWK_ISSPACE
#undef AWK_ISDIGIT
#undef _T

#define awk_strxtoint qse_awk_mbsxtoint
#define awk_strtoflt qse_awk_mbstoflt
#define awk_strxtoflt qse_awk_mbsxtoflt
#define char_t qse_mchar_t
#define cint_t qse_mcint_t
#define AWK_ISSPACE QSE_AWK_ISMSPACE
#define AWK_ISDIGIT QSE_AWK_ISMDIGIT
#define _T QSE_MT
#include "misc-imp.h"

/* ------------------------------------------------------------------------- */
#undef awk_strxtoint
#undef awk_strtoflt
#undef awk_strxtoflt
#undef char_t
#undef cint_t
#undef AWK_ISSPACE
#undef AWK_ISDIGIT
#undef _T
/* ------------------------------------------------------------------------- */

#define awk_strxtoint qse_awk_wcsxtoint
#define awk_strtoflt qse_awk_wcstoflt
#define awk_strxtoflt qse_awk_wcsxtoflt
#define char_t qse_wchar_t
#define cint_t qse_wcint_t
#define AWK_ISSPACE QSE_AWK_ISWSPACE
#define AWK_ISDIGIT QSE_AWK_ISWDIGIT
#define _T QSE_WT
#include "misc-imp.h"

#undef awk_strxtoint
#undef awk_strtoflt
#undef awk_strxtoflt
#undef char_t
#undef cint_t
#undef AWK_ISSPACE
#undef AWK_ISDIGIT
#undef _T
/* ========================================================================= */


qse_size_t qse_awk_inttostr (
	qse_awk_t* awk, qse_awk_int_t value, 
	int radix, const qse_char_t* prefix, qse_char_t* buf, qse_size_t size)
{
	qse_awk_int_t t, rem;
	qse_size_t len, ret, i;
	qse_size_t prefix_len;

	prefix_len = (prefix != QSE_NULL)? qse_strlen(prefix): 0;

	t = value;
	if (t == 0)
	{
		/* zero */
		if (buf == QSE_NULL) 
		{
			/* if buf is not given, 
			 * return the number of bytes required */
			return prefix_len + 1;
		}

		if (size < prefix_len+1) 
		{
			/* buffer too small */
			return (qse_size_t)-1;
		}

		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
		buf[prefix_len] = QSE_T('0');
		if (size > prefix_len+1) buf[prefix_len+1] = QSE_T('\0');
		return prefix_len+1;
	}

	/* non-zero values */
	len = prefix_len;
	if (t < 0) { t = -t; len++; }
	while (t > 0) { len++; t /= radix; }

	if (buf == QSE_NULL)
	{
		/* if buf is not given, return the number of bytes required */
		return len;
	}

	if (size < len) return (qse_size_t)-1; /* buffer too small */
	if (size > len) buf[len] = QSE_T('\0');
	ret = len;

	t = value;
	if (t < 0) t = -t;

	while (t > 0) 
	{
		rem = t % radix;
		if (rem >= 10)
			buf[--len] = (qse_char_t)rem + QSE_T('a') - 10;
		else
			buf[--len] = (qse_char_t)rem + QSE_T('0');
		t /= radix;
	}

	if (value < 0) 
	{
		for (i = 1; i <= prefix_len; i++) 
		{
			buf[i] = prefix[i-1];
			len--;
		}
		buf[--len] = QSE_T('-');
	}
	else
	{
		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
	}

	return ret;
}

qse_char_t* qse_awk_rtx_strtok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, 
	const qse_char_t* delim, qse_cstr_t* tok)
{
	return qse_awk_rtx_strxntok (
		rtx, s, qse_strlen(s), delim, qse_strlen(delim), tok);
}

qse_char_t* qse_awk_rtx_strxtok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_cstr_t* tok)
{
	return qse_awk_rtx_strxntok (
		rtx, s, len, delim, qse_strlen(delim), tok);
}

qse_char_t* qse_awk_rtx_strntok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, 
	const qse_char_t* delim, qse_size_t delim_len,
	qse_cstr_t* tok)
{
	return qse_awk_rtx_strxntok (
		rtx, s, qse_strlen(s), delim, delim_len, tok);
}

qse_char_t* qse_awk_rtx_strxntok (
	qse_awk_rtx_t* rtx, const qse_char_t* s, qse_size_t len,
	const qse_char_t* delim, qse_size_t delim_len, qse_cstr_t* tok)
{
	const qse_char_t* p = s, *d;
	const qse_char_t* end = s + len;	
	const qse_char_t* sp = QSE_NULL, * ep = QSE_NULL;
	const qse_char_t* delim_end = delim + delim_len;
	qse_char_t c; 
	int delim_mode;

#define __DELIM_NULL      0
#define __DELIM_EMPTY     1
#define __DELIM_SPACES    2
#define __DELIM_NOSPACES  3
#define __DELIM_COMPOSITE 4
	if (delim == QSE_NULL) delim_mode = __DELIM_NULL;
	else 
	{
		delim_mode = __DELIM_EMPTY;

		for (d = delim; d < delim_end; d++) 
		{
			if (QSE_AWK_ISSPACE(rtx->awk,*d)) 
			{
				if (delim_mode == __DELIM_EMPTY)
					delim_mode = __DELIM_SPACES;
				else if (delim_mode == __DELIM_NOSPACES)
				{
					delim_mode = __DELIM_COMPOSITE;
					break;
				}
			}
			else
			{
				if (delim_mode == __DELIM_EMPTY)
					delim_mode = __DELIM_NOSPACES;
				else if (delim_mode == __DELIM_SPACES)
				{
					delim_mode = __DELIM_COMPOSITE;
					break;
				}
			}
		}

		/* TODO: verify the following statement... */
		if (delim_mode == __DELIM_SPACES && 
		    delim_len == 1 && 
		    delim[0] != QSE_T(' ')) delim_mode = __DELIM_NOSPACES;
	}		
	
	if (delim_mode == __DELIM_NULL) 
	{ 
		/* when QSE_NULL is given as "delim", it trims off the 
		 * leading and trailing spaces characters off the source
		 * string "s" eventually. */

		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
		while (p < end) 
		{
			c = *p;

			if (!QSE_AWK_ISSPACE(rtx->awk,c)) 
			{
				if (sp == QSE_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == __DELIM_EMPTY)
	{
		/* each character in the source string "s" becomes a token. */
		if (p < end)
		{
			c = *p;
			sp = p;
			ep = p++;
		}
	}
	else if (delim_mode == __DELIM_SPACES) 
	{
		/* each token is delimited by space characters. all leading
		 * and trailing spaces are removed. */

		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
		while (p < end) 
		{
			c = *p;
			if (QSE_AWK_ISSPACE(rtx->awk,c)) break;
			if (sp == QSE_NULL) sp = p;
			ep = p++;
		}
		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
	}
	else if (delim_mode == __DELIM_NOSPACES)
	{
		/* each token is delimited by one of charaters 
		 * in the delimeter set "delim". */
		if (rtx->gbl.ignorecase)
		{
			while (p < end) 
			{
				c = QSE_AWK_TOUPPER(rtx->awk, *p);
				for (d = delim; d < delim_end; d++) 
				{
					if (c == QSE_AWK_TOUPPER(rtx->awk,*d)) goto exit_loop;
				}

				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
		else
		{
			while (p < end) 
			{
				c = *p;
				for (d = delim; d < delim_end; d++) 
				{
					if (c == *d) goto exit_loop;
				}

				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
	}
	else /* if (delim_mode == __DELIM_COMPOSITE) */ 
	{
		/* each token is delimited by one of non-space charaters
		 * in the delimeter set "delim". however, all space characters
		 * surrounding the token are removed */
		while (p < end && QSE_AWK_ISSPACE(rtx->awk,*p)) p++;
		if (rtx->gbl.ignorecase)
		{
			while (p < end) 
			{
				c = QSE_AWK_TOUPPER(rtx->awk, *p);
				if (QSE_AWK_ISSPACE(rtx->awk,c)) 
				{
					p++;
					continue;
				}
				for (d = delim; d < delim_end; d++) 
				{
					if (c == QSE_AWK_TOUPPER(rtx->awk,*d))
						goto exit_loop;
				}
				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
		else
		{
			while (p < end) 
			{
				c = *p;
				if (QSE_AWK_ISSPACE(rtx->awk,c)) 
				{
					p++;
					continue;
				}
				for (d = delim; d < delim_end; d++) 
				{
					if (c == *d) goto exit_loop;
				}
				if (sp == QSE_NULL) sp = p;
				ep = p++;
			}
		}
	}

exit_loop:
	if (sp == QSE_NULL) 
	{
		tok->ptr = QSE_NULL;
		tok->len = (qse_size_t)0;
	}
	else 
	{
		tok->ptr = (qse_char_t*)sp;
		tok->len = ep - sp + 1;
	}

	/* if QSE_NULL is returned, this function should not be called again */
	if (p >= end) return QSE_NULL;
	if (delim_mode == __DELIM_EMPTY || 
	    delim_mode == __DELIM_SPACES) return (qse_char_t*)p;
	return (qse_char_t*)++p;
}

qse_char_t* qse_awk_rtx_strxntokbyrex (
	qse_awk_rtx_t* rtx, 
	const qse_char_t* str, qse_size_t len,
	const qse_char_t* substr, qse_size_t sublen,
	void* rex, qse_cstr_t* tok,
	qse_awk_errnum_t* errnum)
{
	int n;
	qse_size_t i;
	qse_cstr_t match, s, cursub, realsub;

	s.ptr = (qse_char_t*)str;
	s.len = len;

	cursub.ptr = (qse_char_t*)substr;
	cursub.len = sublen;

	realsub.ptr = (qse_char_t*)substr;
	realsub.len = sublen;

	while (cursub.len > 0)
	{
		n = qse_awk_matchrex (
			rtx->awk, rex, rtx->gbl.ignorecase,
			&s, &cursub, &match, QSE_NULL, errnum);
		if (n == -1) return QSE_NULL;
		if (n == 0)
		{
			/* no match has been found. 
			 * return the entire string as a token */
			tok->ptr = realsub.ptr;
			tok->len = realsub.len;
			*errnum = QSE_AWK_ENOERR;
			return QSE_NULL; 
		}

		QSE_ASSERT (n == 1);

		if (match.len == 0)
		{
			/* the match length is zero. */
			cursub.ptr++;
			cursub.len--;
		}
		else if (rtx->gbl.striprecspc > 0 || (rtx->gbl.striprecspc < 0 && (rtx->awk->opt.trait & QSE_AWK_STRIPRECSPC)))
		{
			/* match at the beginning of the input string */
			if (match.ptr == substr) 
			{
				for (i = 0; i < match.len; i++)
				{
					if (!QSE_AWK_ISSPACE(rtx->awk, match.ptr[i]))
						goto exit_loop;
				}

				/* the match that is all spaces at the 
				 * beginning of the input string is skipped */
				cursub.ptr += match.len;
				cursub.len -= match.len;

				/* adjust the substring by skipping the leading
				 * spaces and retry matching */
				realsub.ptr = (qse_char_t*)substr + match.len;
				realsub.len -= match.len;
			}
			else break;
		}
		else break;
	}

exit_loop:
	if (cursub.len <= 0)
	{
		tok->ptr = realsub.ptr;
		tok->len = realsub.len;
		*errnum = QSE_AWK_ENOERR;
		return QSE_NULL; 
	}

	tok->ptr = realsub.ptr;
	tok->len = match.ptr - realsub.ptr;

	for (i = 0; i < match.len; i++)
	{
		if (!QSE_AWK_ISSPACE(rtx->awk, match.ptr[i]))
		{
			/* the match contains a non-space character. */
			*errnum = QSE_AWK_ENOERR;
			return (qse_char_t*)match.ptr+match.len;
		}
	}

	/* the match is all spaces */
	*errnum = QSE_AWK_ENOERR;
	if (rtx->gbl.striprecspc > 0 || (rtx->gbl.striprecspc < 0 && (rtx->awk->opt.trait & QSE_AWK_STRIPRECSPC)))
	{
		/* if the match reached the last character in the input string,
		 * it returns QSE_NULL to terminate tokenization. */
		return (match.ptr+match.len >= substr+sublen)? 
			QSE_NULL: ((qse_char_t*)match.ptr+match.len);
	}
	else
	{
		/* if the match went beyond the the last character in the input 
		 * string, it returns QSE_NULL to terminate tokenization. */
		return (match.ptr+match.len > substr+sublen)? 
			QSE_NULL: ((qse_char_t*)match.ptr+match.len);
	}
}

qse_char_t* qse_awk_rtx_strxnfld (
	qse_awk_rtx_t* rtx, qse_char_t* str, qse_size_t len,
	qse_char_t fs, qse_char_t ec, qse_char_t lq, qse_char_t rq,
	qse_cstr_t* tok)
{
	qse_char_t* p = str;
	qse_char_t* end = str + len;
	int escaped = 0, quoted = 0;
	qse_char_t* ts; /* token start */
	qse_char_t* tp; /* points to one char past the last token char */
	qse_char_t* xp; /* points to one char past the last effective char */

	/* skip leading spaces */
	while (p < end && QSE_ISSPACE(*p)) p++;

	/* initialize token pointers */
	ts = tp = xp = p; 

	while (p < end)
	{
		char c = *p;

		if (escaped)
		{
			*tp++ = c; xp = tp; p++;
			escaped = 0;
		}
		else
		{
			if (c == ec)
			{
				escaped = 1;
				p++;
			}
			else if (quoted)
			{
				if (c == rq)
				{
					quoted = 0;
					p++;
				}
				else
				{
					*tp++ = c; xp = tp; p++;
				}
			}
			else 
			{
				if (c == fs)
				{
					tok->ptr = ts;
					tok->len = xp - ts;
					p++;

					if (QSE_ISSPACE(fs))
					{
						while (p < end && *p == fs) p++;
						if (p >= end) return QSE_NULL;
					}

					return p;
				}
		
				if (c == lq)
				{
					quoted = 1;
					p++;
				}
				else
				{
					*tp++ = c; p++;
					if (!QSE_ISSPACE(c)) xp = tp; 
				}
			}
		}
	}

	if (escaped) 
	{
		/* if it is still escaped, the last character must be 
		 * the escaper itself. treat it as a normal character */
		*xp++ = ec;
	}
	
	tok->ptr = ts;
	tok->len = xp - ts;
	return QSE_NULL;
}

static QSE_INLINE int rexerr_to_errnum (int err)
{
	switch (err)
	{
		case QSE_REX_ENOERR:   return QSE_AWK_ENOERR;
		case QSE_REX_ENOMEM:   return QSE_AWK_ENOMEM;
	 	case QSE_REX_ENOCOMP:  return QSE_AWK_EREXBL;
	 	case QSE_REX_ERECUR:   return QSE_AWK_EREXRECUR;
	 	case QSE_REX_ERPAREN:  return QSE_AWK_EREXRPAREN;
	 	case QSE_REX_ERBRACK:  return QSE_AWK_EREXRBRACK;
	 	case QSE_REX_ERBRACE:  return QSE_AWK_EREXRBRACE;
	 	case QSE_REX_ECOLON:   return QSE_AWK_EREXCOLON;
	 	case QSE_REX_ECRANGE:  return QSE_AWK_EREXCRANGE;
	 	case QSE_REX_ECCLASS:  return QSE_AWK_EREXCCLASS;
	 	case QSE_REX_EBOUND:   return QSE_AWK_EREXBOUND;
	 	case QSE_REX_ESPCAWP:  return QSE_AWK_EREXSPCAWP;
	 	case QSE_REX_EPREEND:  return QSE_AWK_EREXPREEND;
		default:               return QSE_AWK_EINTERN;
	}
}

int qse_awk_buildrex (
	qse_awk_t* awk, const qse_char_t* ptn, qse_size_t len, 
	qse_awk_errnum_t* errnum, void** code, void** icode)
{
#if defined(USE_REX)
	qse_rex_errnum_t err;
	void* p;

	if (code || icode)
	{
		p = qse_buildrex (
			qse_awk_getmmgr(awk), awk->opt.depth.s.rex_build,
			((awk->opt.trait & QSE_AWK_REXBOUND)? 0: QSE_REX_NOBOUND),
			ptn, len, &err
		);
		if (p == QSE_NULL) 
		{
			*errnum = rexerr_to_errnum(err);
			return -1;
		}
	
		if (code) *code = p;
		if (icode) *icode = p;
	}

	return 0;
#else
	qse_tre_t* tre = QSE_NULL; 
	qse_tre_t* itre = QSE_NULL;
	int opt = QSE_TRE_EXTENDED;

	if (!(awk->opt.trait & QSE_AWK_REXBOUND)) opt |= QSE_TRE_NOBOUND;

	if (code)
	{
		tre = qse_tre_open(qse_awk_getmmgr(awk), 0);
		if (tre == QSE_NULL)
		{
			*errnum = QSE_AWK_ENOMEM;
			return -1;
		}

		if (qse_tre_compx (tre, ptn, len, QSE_NULL, opt) <= -1)
		{
#if 0 /* TODO */

			if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM) *errnum = QSE_AWK_ENOMEM;
			else
				SETERR1 (awk, QSE_AWK_EREXBL, str->ptr, str->len, loc);
#endif
			*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
				QSE_AWK_ENOMEM: QSE_AWK_EREXBL;
			qse_tre_close (tre);
			return -1;
		}
	}

	if (icode) 
	{
		itre = qse_tre_open(qse_awk_getmmgr(awk), 0);
		if (itre == QSE_NULL)
		{
			if (tre) qse_tre_close (tre);
			*errnum = QSE_AWK_ENOMEM;
			return -1;
		}

		/* ignorecase is a compile option for TRE */
		if (qse_tre_compx (itre, ptn, len, QSE_NULL, opt | QSE_TRE_IGNORECASE) <= -1)
		{
#if 0 /* TODO */

			if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM) *errnum = QSE_AWK_ENOMEM;
			else
				SETERR1 (awk, QSE_AWK_EREXBL, str->ptr, str->len, loc);
#endif
			*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
				QSE_AWK_ENOMEM: QSE_AWK_EREXBL;
			qse_tre_close (itre);
			if (tre) qse_tre_close (tre);
			return -1;
		}
	}

	if (code) *code = tre;
	if (icode) *icode = itre;
	return 0;
#endif
}

#if !defined(USE_REX)
static int matchtre (
	qse_awk_t* awk, qse_tre_t* tre, int opt, 
	const qse_cstr_t* str, qse_cstr_t* mat, 
	qse_cstr_t submat[9], qse_awk_errnum_t* errnum)
{
	int n;
	/*qse_tre_match_t match[10] = { { 0, 0 }, };*/
	qse_tre_match_t match[10];

	QSE_MEMSET (match, 0, QSE_SIZEOF(match));
	n = qse_tre_execx(tre, str->ptr, str->len, match, QSE_COUNTOF(match), opt);
	if (n <= -1)
	{
		if (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMATCH) return 0;

#if 0 /* TODO: */
		*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
			QSE_AWK_ENOMEM: QSE_AWK_EREXMA;
		SETERR0 (sed, errnum, loc);
#endif
		*errnum = (QSE_TRE_ERRNUM(tre) == QSE_TRE_ENOMEM)? 
			QSE_AWK_ENOMEM: QSE_AWK_EREXMA;
		return -1;
	}

	QSE_ASSERT (match[0].rm_so != -1);
	if (mat)
	{
		mat->ptr = &str->ptr[match[0].rm_so];
		mat->len = match[0].rm_eo - match[0].rm_so;
	}

	if (submat)
	{
		int i;

		/* you must intialize submat before you pass into this 
		 * function because it can abort filling */
		for (i = 1; i < QSE_COUNTOF(match); i++)
		{
			if (match[i].rm_so != -1) 
			{
				submat[i-1].ptr = &str->ptr[match[i].rm_so];
				submat[i-1].len = match[i].rm_eo - match[i].rm_so;
			}
		}
	}
	return 1;
}
#endif

int qse_awk_matchrex (
	qse_awk_t* awk, void* code, int icase,
	const qse_cstr_t* str, const qse_cstr_t* substr,
	qse_cstr_t* match, qse_cstr_t submat[9], qse_awk_errnum_t* errnum)
{
#if defined(USE_REX)
	int x;
	qse_rex_errnum_t err;

	/* submatch is not supported */
	x = qse_matchrex (
		qse_awk_getmmgr(awk), awk->opt.depth.s.rex_match, code, 
		(icase? QSE_REX_IGNORECASE: 0), str, substr, match, &err);
	if (x <= -1) *errnum = rexerr_to_errnum(err);
	return x;
#else
	int x;
	int opt = QSE_TRE_BACKTRACKING; /* TODO: option... QSE_TRE_BACKTRACKING ??? */

	x = matchtre (
		awk, code,
		((str->ptr == substr->ptr)? opt: (opt | QSE_TRE_NOTBOL)),
		substr, match, submat, errnum
	);
	return x;
#endif
}

void qse_awk_freerex (qse_awk_t* awk, void* code, void* icode)
{
	if (code)
	{
#if defined(USE_REX)
		qse_freerex ((awk)->mmgr, code);
#else
		qse_tre_close (code);
#endif
	}

	if (icode && icode != code)
	{
#if defined(USE_REX)
		qse_freerex ((awk)->mmgr, icode);
#else
		qse_tre_close (icode);
#endif
	}
}

int qse_awk_rtx_matchrex (
	qse_awk_rtx_t* rtx, qse_awk_val_t* val,
	const qse_cstr_t* str, const qse_cstr_t* substr, qse_cstr_t* match, qse_cstr_t submat[9])
{
	void* code;
	int icase, x;
	qse_awk_errnum_t awkerr;
#if defined(USE_REX)
	qse_rex_errnum_t rexerr;
#endif

	icase = rtx->gbl.ignorecase;

	if (QSE_AWK_RTX_GETVALTYPE (rtx, val) == QSE_AWK_VAL_REX)
	{
		code = ((qse_awk_val_rex_t*)val)->code[icase];
	}
	else 
	{
		/* convert to a string and build a regular expression */
		qse_cstr_t tmp;

		tmp.ptr = qse_awk_rtx_getvalstr (rtx, val, &tmp.len);
		if (tmp.ptr == QSE_NULL) return -1;

		x = icase? qse_awk_buildrex (rtx->awk, tmp.ptr, tmp.len, &awkerr, QSE_NULL, &code):
		           qse_awk_buildrex (rtx->awk, tmp.ptr, tmp.len, &awkerr, &code, QSE_NULL);
		qse_awk_rtx_freevalstr (rtx, val, tmp.ptr);
		if (x <= -1)
		{
			qse_awk_rtx_seterrnum (rtx, awkerr, QSE_NULL);
			return -1;
		}
	}
	
#if defined(USE_REX)
	/* submatch not supported */
	x = qse_matchrex (
		qse_awk_rtx_getmmgr(rtx), rtx->awk->opt.depth.s.rex_match,
		code, (icase? QSE_REX_IGNORECASE: 0),
		str, substr, match, &rexerr);
	if (x <= -1) qse_awk_rtx_seterrnum (rtx, rexerr_to_errnum(rexerr), QSE_NULL);
#else
	x = matchtre (
		rtx->awk, code,
		((str->ptr == substr->ptr)? QSE_TRE_BACKTRACKING: (QSE_TRE_BACKTRACKING | QSE_TRE_NOTBOL)),
		substr, match, submat, &awkerr
	);
	if (x <= -1) qse_awk_rtx_seterrnum (rtx, awkerr, QSE_NULL);
#endif

	if (QSE_AWK_RTX_GETVALTYPE(rtx, val) == QSE_AWK_VAL_REX) 
	{
		/* nothing to free */
	}
	else
	{
		if (icase) 
			qse_awk_freerex (rtx->awk, QSE_NULL, code);
		else
			qse_awk_freerex (rtx->awk, code, QSE_NULL);
	}

	return x;
}

void* qse_awk_rtx_allocmem (qse_awk_rtx_t* rtx, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC (rtx->awk, size);
	if (ptr == QSE_NULL) qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}

void* qse_awk_rtx_reallocmem (qse_awk_rtx_t* rtx, void* ptr, qse_size_t size)
{
	void* nptr = QSE_AWK_REALLOC (rtx->awk, ptr, size);
	if (nptr == QSE_NULL) qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return nptr;
}

void* qse_awk_rtx_callocmem (qse_awk_rtx_t* rtx, qse_size_t size)
{
	void* ptr = QSE_AWK_ALLOC (rtx->awk, size);
	if (ptr) QSE_MEMSET (ptr, 0, size);
	else qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
	return ptr;
}






qse_wchar_t* qse_awk_rtx_mbstowcsdup (qse_awk_rtx_t* rtx, const qse_mchar_t* mbs, qse_size_t* _wcslen)
{
	qse_size_t mbslen, wcslen;
	qse_wchar_t* wcs;

	/* if i use qse_mbstowcsdupwithcmgr(), i cannot pinpoint the exact failure cause.
	 * let's do it differently. */
	if (qse_mbstowcswithcmgr(mbs, &mbslen, QSE_NULL, &wcslen, qse_awk_rtx_getcmgr(rtx)) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	wcslen = wcslen + 1; /* for terminating null */

	wcs = qse_awk_rtx_allocmem(rtx, QSE_SIZEOF(*wcs) * wcslen);
	if (!wcs) return QSE_NULL;

	qse_mbstowcswithcmgr (mbs, &mbslen, wcs, &wcslen, qse_awk_rtx_getcmgr(rtx));
	if (_wcslen) *_wcslen = wcslen;
	return wcs;
}

qse_mchar_t* qse_awk_rtx_wcstombsdup (qse_awk_rtx_t* rtx, const qse_wchar_t* wcs, qse_size_t* _mbslen)
{
	qse_size_t mbslen, wcslen;
	qse_mchar_t* mbs;

	if (qse_wcstombswithcmgr(wcs, &wcslen, QSE_NULL, &mbslen, qse_awk_rtx_getcmgr(rtx)) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	mbslen = mbslen + 1; /* for terminating null */

	mbs = qse_awk_rtx_allocmem(rtx, QSE_SIZEOF(*mbs) * mbslen);
	if (!mbs) return QSE_NULL;

	qse_wcstombswithcmgr (wcs, &wcslen, mbs, &mbslen, qse_awk_rtx_getcmgr(rtx));
	if (_mbslen) *_mbslen = mbslen;
	return mbs;
}


qse_wchar_t* qse_awk_rtx_mbsntowcsdup (qse_awk_rtx_t* rtx, const qse_mchar_t* mbs, qse_size_t _mbslen, qse_size_t* _wcslen)
{
	qse_size_t mbslen, wcslen;
	qse_wchar_t* wcs;

	/* if i use qse_mbstowcsdupwithcmgr(), i cannot pinpoint the exact failure cause.
	 * let's do it differently. */
	mbslen = _mbslen;
	if (qse_mbsntowcsnwithcmgr(mbs, &mbslen, QSE_NULL, &wcslen, qse_awk_rtx_getcmgr(rtx)) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	wcs = qse_awk_rtx_allocmem(rtx, QSE_SIZEOF(*wcs) * (wcslen + 1));
	if (!wcs) return QSE_NULL;

	mbslen= _mbslen;
	qse_mbsntowcsnwithcmgr (mbs, &mbslen, wcs, &wcslen, qse_awk_rtx_getcmgr(rtx));
	wcs[wcslen] = QSE_WT('\0');

	if (_wcslen) *_wcslen = wcslen;
	return wcs;
}


qse_mchar_t* qse_awk_rtx_wcsntombsdup (qse_awk_rtx_t* rtx, const qse_wchar_t* wcs, qse_size_t _wcslen, qse_size_t* _mbslen)
{
	qse_size_t mbslen, wcslen;
	qse_mchar_t* mbs;

	wcslen = _wcslen;
	if (qse_wcsntombsnwithcmgr(wcs, &wcslen, QSE_NULL, &mbslen, qse_awk_rtx_getcmgr(rtx)) <= -1)
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_EINVAL, QSE_NULL);
		return QSE_NULL;
	}

	mbs = qse_awk_rtx_allocmem(rtx, QSE_SIZEOF(*mbs) * (mbslen + 1));
	if (!mbs) return QSE_NULL;

	wcslen = _wcslen;
	qse_wcsntombsnwithcmgr (wcs, &wcslen, mbs, &mbslen, qse_awk_rtx_getcmgr(rtx));
	mbs[mbslen] = QSE_MT('\0');

	if (_mbslen) *_mbslen = mbslen;
	return mbs;
}
