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

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "mem-prv.h"

qse_mchar_t* qse_mbstrmx (qse_mchar_t* str, int opt)
{
	qse_mchar_t* p = str;

	if (*p != QSE_MT('\0')) 
	{
		qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISMSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (*p != QSE_MT('\0'));


		if (e)
		{
			if (opt & QSE_MBSTRMX_RIGHT) e[1] = QSE_MT('\0');
			if (opt & QSE_MBSTRMX_LEFT) str = s;
		}
		else
		{
			/* the entire string need to ne deleted */
			if ((opt & QSE_MBSTRMX_RIGHT) || 
			    (opt & QSE_MBSTRMX_LEFT)) str[0] = QSE_MT('\0');
		}
	}

	return str;
}

qse_mchar_t* qse_mbsxtrmx (qse_mchar_t* str, qse_size_t* len, int opt)
{
	qse_mchar_t* p = str, * end = str + *len;

	if (p < end)
	{
		qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISMSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (p < end);

		if (e)
		{
			if (opt & QSE_MBSTRMX_RIGHT) 
			{
				*len -= end - e - 1;
			}
			if (opt & QSE_MBSTRMX_LEFT) 
			{
				*len -= s - str;
				str = s;
			}
		}
		else
		{
			/* the entire string need to ne deleted */
			if ((opt & QSE_MBSTRMX_RIGHT) || 
			    (opt & QSE_MBSTRMX_LEFT)) *len = 0;
		}
	}

	return str;
}


qse_wchar_t* qse_wcstrmx (qse_wchar_t* str, int opt)
{
	qse_wchar_t* p = str;

	if (*p != QSE_MT('\0')) 
	{
		qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISWSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (*p != QSE_MT('\0'));


		if (e)
		{
			if (opt & QSE_WCSTRMX_RIGHT) e[1] = QSE_MT('\0');
			if (opt & QSE_WCSTRMX_LEFT) str = s;
		}
		else
		{
			/* the entire string need to be deleted */
			if ((opt & QSE_WCSTRMX_RIGHT) || 
			    (opt & QSE_WCSTRMX_LEFT)) str[0] = QSE_MT('\0');
		}
	}

	return str;
}

qse_wchar_t* qse_wcsxtrmx (qse_wchar_t* str, qse_size_t* len, int opt)
{
	qse_wchar_t* p = str, * end = str + *len;

	if (p < end)
	{
		qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

		do
		{
			if (!QSE_ISWSPACE(*p))
			{
				if (s == QSE_NULL) s = p;
				e = p;
			}
			p++;
		}
		while (p < end);

		if (e)
		{
			if (opt & QSE_WCSTRMX_RIGHT) 
			{
				*len -= end - e - 1;
			}
			if (opt & QSE_WCSTRMX_LEFT) 
			{
				*len -= s - str;
				str = s;
			}
		}
		else
		{
			/* the entire string need to ne deleted */
			if ((opt & QSE_WCSTRMX_RIGHT) || 
			    (opt & QSE_WCSTRMX_LEFT)) *len = 0;
		}
	}

	return str;
}

/* -------------------------------------------------------------- */

qse_size_t qse_mbstrm (qse_mchar_t* str)
{
	qse_mchar_t* p = str;
	qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_MT('\0')) 
	{
		if (!QSE_ISMSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e)
	{
		e[1] = QSE_MT('\0');
		if (str != s)
			QSE_MEMMOVE (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	str[0] = QSE_MT('\0');
	return 0;
}

qse_size_t qse_mbsxtrm (qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t* p = str, * end = str + len;
	qse_mchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (p < end) 
	{
		if (!QSE_ISMSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		/* do not insert a terminating null */
		/*e[1] = QSE_MT('\0');*/
		if (str != s)
			QSE_MEMMOVE (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	/* do not insert a terminating null */
	/*str[0] = QSE_MT('\0');*/
	return 0;
}

qse_size_t qse_wcstrm (qse_wchar_t* str)
{
	qse_wchar_t* p = str;
	qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_MT('\0')) 
	{
		if (!QSE_ISWSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e)
	{
		e[1] = QSE_MT('\0');
		if (str != s)
			QSE_MEMMOVE (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	str[0] = QSE_MT('\0');
	return 0;
}

qse_size_t qse_wcsxtrm (qse_wchar_t* str, qse_size_t len)
{
	qse_wchar_t* p = str, * end = str + len;
	qse_wchar_t* s = QSE_NULL, * e = QSE_NULL;

	while (p < end) 
	{
		if (!QSE_ISWSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		/* do not insert a terminating null */
		/*e[1] = QSE_MT('\0');*/
		if (str != s)
			QSE_MEMMOVE (str, s, (e - s + 2) * QSE_SIZEOF(*str));
		return e - s + 1;
	}

	/* do not insert a terminating null */
	/*str[0] = QSE_MT('\0');*/
	return 0;
}
