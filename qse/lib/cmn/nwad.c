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

#include <qse/cmn/nwad.h>
#include <qse/cmn/hton.h>
#include <qse/cmn/nwif.h>
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/mbwc.h>
#include "mem.h"

int qse_nwadequal (const qse_nwad_t* x, const qse_nwad_t* y)
{
	if (x->type != y->type) return 0;

	switch (x->type)
	{
		case QSE_NWAD_IN4:
			return (x->u.in4.port == y->u.in4.port &&
			        QSE_MEMCMP (&x->u.in4.addr, &y->u.in4.addr, QSE_SIZEOF(x->u.in4.addr)) == 0)? 1: 0;

		case QSE_NWAD_IN6:
			return (x->u.in6.port == y->u.in6.port &&
			        x->u.in6.scope == y->u.in6.scope &&
			        QSE_MEMCMP (&x->u.in6.addr, &y->u.in6.addr, QSE_SIZEOF(x->u.in6.addr)) == 0)? 1: 0;

		case QSE_NWAD_LOCAL:
			return qse_strcmp (x->u.local.path, y->u.local.path) == 0;

		default: 
			/* can't compare */
			return -1;
	}
}

void qse_clearnwad (qse_nwad_t* nwad, qse_nwad_type_t type)
{
	QSE_MEMSET (nwad, 0, QSE_SIZEOF(*nwad));
	nwad->type = type;
}

void qse_setnwadport (qse_nwad_t* nwad, qse_uint16_t port)
{
	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
			nwad->u.in4.port = port;
			break;

		case QSE_NWAD_IN6:
			nwad->u.in6.port = port;
			break;

		case QSE_NWAD_LOCAL:
			/* no port for QSE_NWAD_LOCAL */
			break;
	}
}

qse_uint16_t qse_getnwadport (qse_nwad_t* nwad)
{
	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
			return nwad->u.in4.port;

		case QSE_NWAD_IN6:
			return nwad->u.in6.port;

		case QSE_NWAD_LOCAL:
		default:
			return 0;
	}
}

int qse_mbstonwad (const qse_mchar_t* str, qse_nwad_t* nwad)
{
	return qse_mbsntonwad (str, qse_mbslen(str), nwad);
}

int qse_mbsntonwad (const qse_mchar_t* str, qse_size_t len, qse_nwad_t* nwad)
{
	const qse_mchar_t* p;
	const qse_mchar_t* end;
	qse_mcstr_t tmp;
	qse_nwad_t tmpad;

	QSE_MEMSET (&tmpad, 0, QSE_SIZEOF(tmpad));

	p = str;
	end = str + len;

	if (p >= end) return -1;

	if (*p == QSE_MT('@') && len >= 2)
	{
		/* the string begins with @. it's a local name */
	#if defined(QSE_CHAR_IS_MCHAR)
		qse_mbsxncpy (tmpad.u.local.path, QSE_COUNTOF(tmpad.u.local.path), p + 1, len - 1);
	#else
		qse_size_t mbslen = len - 1;
		qse_size_t wcslen = QSE_COUNTOF(tmpad.u.local.path) - 1;
		if (qse_mbsntowcsn (p + 1, &mbslen, tmpad.u.local.path, &wcslen) <= -1) return -1;
		tmpad.u.local.path[wcslen] = QSE_WT('\0');
	#endif

		tmpad.type = QSE_NWAD_LOCAL;
		goto done;
	}

	if (*p == QSE_MT('['))
	{
		/* IPv6 address */
		tmp.ptr = ++p; /* skip [ and remember the position */
		while (p < end && *p != QSE_MT('%') && *p != QSE_MT(']')) p++;

		if (p >= end) return -1;

		tmp.len = p - tmp.ptr;
		if (*p == QSE_MT('%'))
		{
			/* handle scope id */
			qse_uint32_t x;

			p++; /* skip % */

			if (p >= end)
			{
				/* premature end */
				return -1;
			}

			if (*p >= QSE_MT('0') && *p <= QSE_MT('9')) 
			{
				/* numeric scope id */
				tmpad.u.in6.scope = 0;
				do
				{
					x = tmpad.u.in6.scope * 10 + (*p - QSE_MT('0'));
					if (x < tmpad.u.in6.scope) return -1; /* overflow */
					tmpad.u.in6.scope = x;
					p++;
				}
				while (p < end && *p >= QSE_MT('0') && *p <= QSE_MT('9'));
			}
			else
			{
				/* interface name as a scope id? */
				const qse_mchar_t* stmp = p;
				unsigned int index;
				do p++; while (p < end && *p != QSE_MT(']'));
				if (qse_nwifmbsntoindex (stmp, p - stmp, &index) <= -1) return -1;
				tmpad.u.in6.scope = index;
			}

			if (p >= end || *p != QSE_MT(']')) return -1;
		}
		p++; /* skip ] */

		if (qse_mbsntoip6ad (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) return -1;
		tmpad.type = QSE_NWAD_IN6;
	}
	else
	{
		/* host name or IPv4 address */
		tmp.ptr = p;
		while (p < end && *p != QSE_MT(':')) p++;
		tmp.len = p - tmp.ptr;

		if (qse_mbsntoip4ad (tmp.ptr, tmp.len, &tmpad.u.in4.addr) <= -1)
		{
			if (p >= end || *p != QSE_MT(':')) return -1;
			
			/* check if it is an IPv6 address not enclosed in []. 
			 * the port number can't be specified in this format. */

			while (p < end && *p != QSE_MT('%')) p++;
			tmp.len = p - tmp.ptr;

			if (qse_mbsntoip6ad (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) 
				return -1;

			if (p < end && *p == QSE_MT('%'))
			{
				/* handle scope id */
				qse_uint32_t x;

				p++; /* skip % */

				if (p >= end)
				{
					/* premature end */
					return -1;
				}

				if (*p >= QSE_MT('0') && *p <= QSE_MT('9')) 
				{
					/* numeric scope id */
					tmpad.u.in6.scope = 0;
					do
					{
						x = tmpad.u.in6.scope * 10 + (*p - QSE_MT('0'));
						if (x < tmpad.u.in6.scope) return -1; /* overflow */
						tmpad.u.in6.scope = x;
						p++;
					}
					while (p < end && *p >= QSE_MT('0') && *p <= QSE_MT('9'));
				}
				else
				{
					/* interface name as a scope id? */
					const qse_mchar_t* stmp = p;
					unsigned int index;
					do p++; while (p < end);
					if (qse_nwifmbsntoindex (stmp, p - stmp, &index) <= -1) return -1;
					tmpad.u.in6.scope = index;
				}
			}

			if (p < end) return -1;

			tmpad.type = QSE_NWAD_IN6;
			goto done;
		}

		tmpad.type = QSE_NWAD_IN4;
	}

	if (p < end && *p == QSE_MT(':')) 
	{
		/* port number */
		qse_uint32_t port = 0;

		p++; /* skip : */

		tmp.ptr = p;
		while (p < end && *p >= QSE_MT('0') && *p <= QSE_MT('9'))
		{
			port = port * 10 + (*p - QSE_MT('0'));
			p++;
		}

		tmp.len = p - tmp.ptr;
		if (tmp.len <= 0 || tmp.len >= 6 || 
		    port > QSE_TYPE_MAX(qse_uint16_t)) return -1;

		if (tmpad.type == QSE_NWAD_IN4)
			tmpad.u.in4.port = qse_hton16 (port);
		else
			tmpad.u.in6.port = qse_hton16 (port);
	}

done:
	if (nwad) *nwad = tmpad;
	return 0;
}

int qse_wcstonwad (const qse_wchar_t* str, qse_nwad_t* nwad)
{
	return qse_wcsntonwad (str, qse_wcslen(str), nwad);
}

int qse_wcsntonwad (const qse_wchar_t* str, qse_size_t len, qse_nwad_t* nwad)
{
	const qse_wchar_t* p;
	const qse_wchar_t* end;
	qse_wcstr_t tmp;
	qse_nwad_t tmpad;

	QSE_MEMSET (&tmpad, 0, QSE_SIZEOF(tmpad));

	p = str;
	end = str + len;

	if (p >= end) return -1;

	if (*p == QSE_WT('@') && len >= 2)
	{
		/* the string begins with @. it's a local name */
	#if defined(QSE_CHAR_IS_MCHAR)
		qse_size_t wcslen = len - 1;
		qse_size_t mbslen = QSE_COUNTOF(tmpad.u.local.path) - 1;
		if (qse_wcsntombsn (p + 1, &wcslen, tmpad.u.local.path, &mbslen) <= -1) return -1;
		tmpad.u.local.path[mbslen] = QSE_MT('\0');
	#else
		qse_wcsxncpy (tmpad.u.local.path, QSE_COUNTOF(tmpad.u.local.path), p + 1, len - 1);
	#endif

		tmpad.type = QSE_NWAD_LOCAL;
		goto done;
	}

	if (*p == QSE_WT('['))
	{
		/* IPv6 address */
		tmp.ptr = ++p; /* skip [ and remember the position */
		while (p < end && *p != QSE_WT('%') && *p != QSE_WT(']')) p++;

		if (p >= end) return -1;

		tmp.len = p - tmp.ptr;
		if (*p == QSE_WT('%'))
		{
			/* handle scope id */
			qse_uint32_t x;

			p++; /* skip % */

			if (p >= end)
			{
				/* premature end */
				return -1;
			}

			if (*p >= QSE_WT('0') && *p <= QSE_WT('9')) 
			{
				/* numeric scope id */
				tmpad.u.in6.scope = 0;
				do
				{
					x = tmpad.u.in6.scope * 10 + (*p - QSE_WT('0'));
					if (x < tmpad.u.in6.scope) return -1; /* overflow */
					tmpad.u.in6.scope = x;
					p++;
				}
				while (p < end && *p >= QSE_WT('0') && *p <= QSE_WT('9'));
			}
			else
			{
				/* interface name as a scope id? */
				const qse_wchar_t* stmp = p;
				unsigned int index;
				do p++; while (p < end && *p != QSE_WT(']'));
				if (qse_nwifwcsntoindex (stmp, p - stmp, &index) <= -1) return -1;
				tmpad.u.in6.scope = index;
			}

			if (p >= end || *p != QSE_WT(']')) return -1;
		}
		p++; /* skip ] */

		if (qse_wcsntoip6ad (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) return -1;
		tmpad.type = QSE_NWAD_IN6;
	}
	else
	{
		/* host name or IPv4 address */
		tmp.ptr = p;
		while (p < end && *p != QSE_WT(':')) p++;
		tmp.len = p - tmp.ptr;

		if (qse_wcsntoip4ad (tmp.ptr, tmp.len, &tmpad.u.in4.addr) <= -1)
		{
			if (p >= end || *p != QSE_WT(':')) return -1;

			/* check if it is an IPv6 address not enclosed in []. 
			 * the port number can't be specified in this format. */

			while (p < end && *p != QSE_WT('%')) p++;
			tmp.len = p - tmp.ptr;

			if (qse_wcsntoip6ad (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) 
				return -1;

			if (p < end && *p == QSE_WT('%'))
			{
				/* handle scope id */
				qse_uint32_t x;

				p++; /* skip % */

				if (p >= end)
				{
					/* premature end */
					return -1;
				}

				if (*p >= QSE_WT('0') && *p <= QSE_WT('9')) 
				{
					/* numeric scope id */
					tmpad.u.in6.scope = 0;
					do
					{
						x = tmpad.u.in6.scope * 10 + (*p - QSE_WT('0'));
						if (x < tmpad.u.in6.scope) return -1; /* overflow */
						tmpad.u.in6.scope = x;
						p++;
					}
					while (p < end && *p >= QSE_WT('0') && *p <= QSE_WT('9'));
				}
				else
				{
					/* interface name as a scope id? */
					const qse_wchar_t* stmp = p;
					unsigned int index;
					do p++; while (p < end);
					if (qse_nwifwcsntoindex (stmp, p - stmp, &index) <= -1) return -1;
					tmpad.u.in6.scope = index;
				}
			}

			if (p < end) return -1;

			tmpad.type = QSE_NWAD_IN6;
			goto done;
		}

		tmpad.type = QSE_NWAD_IN4;
	}

	if (p < end && *p == QSE_WT(':')) 
	{
		/* port number */
		qse_uint32_t port = 0;

		p++; /* skip : */

		tmp.ptr = p;
		while (p < end && *p >= QSE_WT('0') && *p <= QSE_WT('9'))
		{
			port = port * 10 + (*p - QSE_WT('0'));
			p++;
		}

		tmp.len = p - tmp.ptr;
		if (tmp.len <= 0 || tmp.len >= 6 || 
		    port > QSE_TYPE_MAX(qse_uint16_t)) return -1;

		if (tmpad.type == QSE_NWAD_IN4)
			tmpad.u.in4.port = qse_hton16 (port);
		else
			tmpad.u.in6.port = qse_hton16 (port);
	}

done:
	if (nwad) *nwad = tmpad;
	return 0;
}

qse_size_t qse_nwadtombs (
	const qse_nwad_t* nwad, qse_mchar_t* buf, qse_size_t len, int flags)
{
	qse_size_t xlen = 0;

	/* unsupported types will result in an empty string */

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
			if (flags & QSE_NWADTOMBS_ADDR)
			{
				if (xlen + 1 >= len) goto done;
				xlen += qse_ip4adtombs (&nwad->u.in4.addr, buf, len);
			}

			if (flags & QSE_NWADTOMBS_PORT)
			{
				if (!(flags & QSE_NWADTOMBS_ADDR) || 
				    nwad->u.in4.port != 0)
				{
					if (flags & QSE_NWADTOMBS_ADDR)
					{
						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_MT(':');
					}

					if (xlen + 1 >= len) goto done;
					xlen += qse_fmtuintmaxtombs (
						&buf[xlen], len - xlen, 
						qse_ntoh16(nwad->u.in4.port),
						10, 0, QSE_MT('\0'), QSE_NULL);
				}
			}
			break;

		case QSE_NWAD_IN6:
			if (flags & QSE_NWADTOMBS_PORT)
			{
				if (!(flags & QSE_NWADTOMBS_ADDR) || 
				    nwad->u.in6.port != 0)
				{
					if (flags & QSE_NWADTOMBS_ADDR)
					{
						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_MT('[');	
					}
				}
			}

			if (flags & QSE_NWADTOMBS_ADDR)
			{

				if (xlen + 1 >= len) goto done;
				xlen += qse_ip6adtombs (&nwad->u.in6.addr, &buf[xlen], len - xlen);
			
				if (nwad->u.in6.scope != 0)
				{
					int tmp;

					if (xlen + 1 >= len) goto done;
					buf[xlen++] = QSE_MT('%');

					if (xlen + 1 >= len) goto done;

					tmp = qse_nwifindextombs (nwad->u.in6.scope, &buf[xlen], len - xlen);
					if (tmp <= -1)
					{
						xlen += qse_fmtuintmaxtombs (
							&buf[xlen], len - xlen, 
							nwad->u.in6.scope, 10, 0, QSE_MT('\0'), QSE_NULL);
					}
					else xlen += tmp;
				}
			}

			if (flags & QSE_NWADTOMBS_PORT)
			{
				if (!(flags & QSE_NWADTOMBS_ADDR) || 
				    nwad->u.in6.port != 0) 
				{
					if (flags & QSE_NWADTOMBS_ADDR)
					{
						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_MT(']');

						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_MT(':');
					}

					if (xlen + 1 >= len) goto done;
					xlen += qse_fmtuintmaxtombs (
						&buf[xlen], len - xlen, 
						qse_ntoh16(nwad->u.in6.port),
						10, 0, QSE_MT('\0'), QSE_NULL);
				}
			}

			break;

		case QSE_NWAD_LOCAL:
			if (flags & QSE_NWADTOMBS_ADDR)
			{
				if (xlen + 1 >= len) goto done;
				buf[xlen++] = QSE_MT('@');

			#if defined(QSE_CHAR_IS_MCHAR)
				if (xlen + 1 >= len) goto done;
				xlen += qse_mbsxcpy (&buf[xlen], len - xlen, nwad->u.local.path);
			#else
				if (xlen + 1 >= len) goto done;
				else
				{
					qse_size_t wcslen, mbslen = len - xlen;
					qse_wcstombs (nwad->u.local.path, &wcslen, &buf[xlen], &mbslen);
					/* i don't care about conversion errors */
					xlen += mbslen;
				}
			#endif
			}

			break;
	}

done:
	if (xlen < len) buf[xlen] = QSE_MT('\0');
	return xlen;
}


qse_size_t qse_nwadtowcs (
	const qse_nwad_t* nwad, qse_wchar_t* buf, qse_size_t len, int flags)
{
	qse_size_t xlen = 0;

	/* unsupported types will result in an empty string */

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
			if (flags & QSE_NWADTOWCS_ADDR)
			{
				if (xlen + 1 >= len) goto done;
				xlen += qse_ip4adtowcs (&nwad->u.in4.addr, buf, len);
			}

			if (flags & QSE_NWADTOWCS_PORT)
			{
				if (!(flags & QSE_NWADTOMBS_ADDR) || 
				    nwad->u.in4.port != 0)
				{
					if (flags & QSE_NWADTOMBS_ADDR)
					{
						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_WT(':');
					}

					if (xlen + 1 >= len) goto done;
					xlen += qse_fmtuintmaxtowcs (
						&buf[xlen], len - xlen, 
						qse_ntoh16(nwad->u.in4.port),
						10, 0, QSE_WT('\0'), QSE_NULL);
				}
			}
			break;

		case QSE_NWAD_IN6:
			if (flags & QSE_NWADTOWCS_PORT)
			{
				if (!(flags & QSE_NWADTOMBS_ADDR) || 
				    nwad->u.in6.port != 0)
				{
					if (flags & QSE_NWADTOMBS_ADDR)
					{
						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_WT('[');	
					}
				}
			}

			if (flags & QSE_NWADTOWCS_ADDR)
			{
				if (xlen + 1 >= len) goto done;
				xlen += qse_ip6adtowcs (&nwad->u.in6.addr, &buf[xlen], len - xlen);
			
				if (nwad->u.in6.scope != 0)
				{
					int tmp;

					if (xlen + 1 >= len) goto done;
					buf[xlen++] = QSE_WT('%');

					if (xlen + 1 >= len) goto done;

					tmp = qse_nwifindextowcs (nwad->u.in6.scope, &buf[xlen], len - xlen);
					if (tmp <= -1)
					{
						xlen += qse_fmtuintmaxtowcs (
							&buf[xlen], len - xlen, 
							nwad->u.in6.scope, 10, 0, QSE_WT('\0'), QSE_NULL);
					}
					else xlen += tmp;
				}
			}

			if (flags & QSE_NWADTOWCS_PORT)
			{
				if (!(flags & QSE_NWADTOMBS_ADDR) || 
				    nwad->u.in6.port != 0) 
				{
					if (flags & QSE_NWADTOMBS_ADDR)
					{
						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_WT(']');

						if (xlen + 1 >= len) goto done;
						buf[xlen++] = QSE_WT(':');
					}

					if (xlen + 1 >= len) goto done;
					xlen += qse_fmtuintmaxtowcs (
						&buf[xlen], len - xlen, 
						qse_ntoh16(nwad->u.in6.port),
						10, 0, QSE_WT('\0'), QSE_NULL);
				}
			}

			break;

		case QSE_NWAD_LOCAL:
			if (flags & QSE_NWADTOMBS_ADDR)
			{
				if (xlen + 1 >= len) goto done;
				buf[xlen++] = QSE_WT('@');

			#if defined(QSE_CHAR_IS_MCHAR)
				if (xlen + 1 >= len) goto done;
				else
				{
					qse_size_t wcslen = len - xlen, mbslen;
					qse_mbstowcs (nwad->u.local.path, &mbslen, &buf[xlen], &wcslen);
					/* i don't care about conversion errors */
					xlen += wcslen;
				}
			#else
				if (xlen + 1 >= len) goto done;
				xlen += qse_wcsxcpy (&buf[xlen], len - xlen, nwad->u.local.path);
			#endif
			}
	}

done:
	if (xlen < len) buf[xlen] = QSE_WT('\0');
	return xlen;
}

