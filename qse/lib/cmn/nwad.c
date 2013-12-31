/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#include <qse/cmn/nwad.h>
#include <qse/cmn/hton.h>
#include <qse/cmn/nwif.h>
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
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

		default: 
			/* can't compare */
			return -1;
	}
}

void qse_clearnwad (qse_nwad_t* nwad, qse_nwad_type_t type)
{
	QSE_MEMSET (nwad,  0, QSE_SIZEOF(*nwad));
	nwad->type = type;
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

	}

done:
	if (xlen < len) buf[xlen] = QSE_WT('\0');
	return xlen;
}

