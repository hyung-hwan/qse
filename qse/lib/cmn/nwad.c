/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include "mem.h"

#if 0
int qse_mbstonwad (const qse_mchar_t* str, qse_nwad_t* nwad)
{
	const qse_mchar_t* p;
	qse_mcstr_t tmp;
	qse_nwad_t tmpad;

	QSE_MEMSET (&tmpad, 0, QSE_SIZEOF(tmpad));

	p = str;
	if (*p == QSE_MT('['))
	{
		/* IPv6 address */
		tmp.ptr = ++p; /* skip [ and remember the position */
		while (*p != QSE_MT('\0') && *p != QSE_MT('%') && *p != QSE_MT(']')) p++;

		if (*p == QSE_MT('\0')) return -1;

		tmp.len = p - tmp.ptr;
		if (*p == QSE_MT('%'))
		{
			/* handle scope id */
			qse_uint32_t x;

			p++; /* skip % */

			if (!(*p >= QSE_MT('0') && *p <= QSE_MT('9'))) return -1;
			tmpad.u.in6.scope = 0;
			do
			{
				x = tmpad.u.in6.scope * 10 + (*p - QSE_MT('0'));
				if (x < tmpad.u.in6.scope) return -1; /* overflow */
				tmpad.u.in6.scope = x;
				p++;
			}
			while (*p >= QSE_MT('0') && *p <= QSE_MT('9'));

			if (*p != QSE_MT(']')) return -1;
		}
		p++; /* skip ] */

		if (qse_mbsntoipad6 (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) return -1;
		tmpad.type = QSE_NWAD_IN6;
	}
	else
	{
		/* host name or IPv4 address */
		tmp.ptr = p;
		while (*p != QSE_MT('\0') && *p != QSE_MT(':')) p++;
		tmp.len = p - tmp.ptr;

		if (qse_mbsntoipad4 (tmp.ptr, tmp.len, &tmpad.u.in4.addr) <= -1)
		{
			/* check if it is an IPv6 address not enclosed in []. 
			 * the port number can't be specified in this format. */

			while (*p != QSE_MT('\0') && *p != QSE_MT('%')) p++;
			tmp.len = p - tmp.ptr;

			if (qse_mbsntoipad6 (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) 
				return -1;

			if (*p == QSE_MT('%'))
			{
				/* handle scope id */
				qse_uint32_t x;

				p++; /* skip % */
				if (!(*p >= QSE_MT('0') && *p <= QSE_MT('9'))) return -1;
				tmpad.u.in6.scope = 0;
				do
				{
					x = tmpad.u.in6.scope * 10 + (*p - QSE_MT('0'));
					if (x < tmpad.u.in6.scope) return -1; /* overflow */
					tmpad.u.in6.scope = x;
					p++;
				}
				while (*p >= QSE_MT('0') && *p <= QSE_MT('9'));
			}

			if (*p != QSE_MT('\0')) return -1;

			tmpad.type = QSE_NWAD_IN6;
			goto done;
		}

		tmpad.type = QSE_NWAD_IN4;
	}

	if (*p == QSE_MT(':')) 
	{
		/* port number */
		qse_uint32_t port = 0;

		p++; /* skip : */

		for (tmp.ptr = p; *p >= QSE_MT('0') && *p <= QSE_MT('9'); p++)
			port = port * 10 + (*p - QSE_MT('0'));

		tmp.len = p - tmp.ptr;
		if (tmp.len <= 0 || tmp.len >= 6 || 
		    port > QSE_TYPE_MAX(qse_uint16_t)) return -1;

		if (tmpad.type ==  QSE_NWAD_IN4)
			tmpad.u.in4.port = qse_hton16 (port);
		else
			tmpad.u.in6.port = qse_hton16 (port);
	}

done:
	if (nwad) *nwad = tmpad;
	return 0;
}
#endif

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

			if (!(p < end && *p >= QSE_MT('0') && *p <= QSE_MT('9'))) return -1;
			tmpad.u.in6.scope = 0;
			do
			{
				x = tmpad.u.in6.scope * 10 + (*p - QSE_MT('0'));
				if (x < tmpad.u.in6.scope) return -1; /* overflow */
				tmpad.u.in6.scope = x;
				p++;
			}
			while (p < end && *p >= QSE_MT('0') && *p <= QSE_MT('9'));

			if (p >= end || *p != QSE_MT(']')) return -1;
		}
		p++; /* skip ] */

		if (qse_mbsntoipad6 (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) return -1;
		tmpad.type = QSE_NWAD_IN6;
	}
	else
	{
		/* host name or IPv4 address */
		tmp.ptr = p;
		while (p < end && *p != QSE_MT(':')) p++;
		tmp.len = p - tmp.ptr;

		if (qse_mbsntoipad4 (tmp.ptr, tmp.len, &tmpad.u.in4.addr) <= -1)
		{
			if (p >= end || *p != QSE_MT(':')) return -1;
			
			/* check if it is an IPv6 address not enclosed in []. 
			 * the port number can't be specified in this format. */

			while (p < end && *p != QSE_MT('%')) p++;
			tmp.len = p - tmp.ptr;

			if (qse_mbsntoipad6 (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) 
				return -1;

			if (p < end && *p == QSE_MT('%'))
			{
				/* handle scope id */
				qse_uint32_t x;

				p++; /* skip % */
				if (!(p < end && *p >= QSE_MT('0') && *p <= QSE_MT('9'))) return -1;
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

			if (!(p < end && *p >= QSE_WT('0') && *p <= QSE_WT('9'))) return -1;
			tmpad.u.in6.scope = 0;
			do
			{
				x = tmpad.u.in6.scope * 10 + (*p - QSE_WT('0'));
				if (x < tmpad.u.in6.scope) return -1; /* overflow */
				tmpad.u.in6.scope = x;
				p++;
			}
			while (p < end && *p >= QSE_WT('0') && *p <= QSE_WT('9'));

			if (p >= end || *p != QSE_WT(']')) return -1;
		}
		p++; /* skip ] */

		if (qse_wcsntoipad6 (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) return -1;
		tmpad.type = QSE_NWAD_IN6;
	}
	else
	{
		/* host name or IPv4 address */
		tmp.ptr = p;
		while (p < end && *p != QSE_WT(':')) p++;
		tmp.len = p - tmp.ptr;

		if (qse_wcsntoipad4 (tmp.ptr, tmp.len, &tmpad.u.in4.addr) <= -1)
		{
			if (p >= end || *p != QSE_WT(':')) return -1;

			/* check if it is an IPv6 address not enclosed in []. 
			 * the port number can't be specified in this format. */

			while (p < end && *p != QSE_WT('%')) p++;
			tmp.len = p - tmp.ptr;

			if (qse_wcsntoipad6 (tmp.ptr, tmp.len, &tmpad.u.in6.addr) <= -1) 
				return -1;

			if (p < end && *p == QSE_WT('%'))
			{
				/* handle scope id */
				qse_uint32_t x;

				p++; /* skip % */
				if (!(p < end && *p >= QSE_WT('0') && *p <= QSE_WT('9'))) return -1;
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

qse_size_t qse_nwadtombs (const qse_nwad_t* nwad, qse_mchar_t* buf, qse_size_t len)
{
	qse_size_t xlen = 0;

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
			if (xlen + 1 < len)
			{
				xlen = qse_ipad4tombs (&nwad->u.in4.addr, buf, len);
				if (xlen + 1 < len && nwad->u.in4.port != 0) 
				{
					buf[xlen++] = QSE_MT(':');
					if (xlen + 1 < len)
					{
						xlen += qse_fmtuintmaxtombs (
							&buf[xlen], len - xlen, 
							qse_ntoh16(nwad->u.in4.port),
							10, 0, QSE_WT('\0'), QSE_NULL);
					}
				}
			}
			break;

		case QSE_NWAD_IN6:
			if (xlen + 1 < len)
			{
				if (nwad->u.in6.port != 0) buf[xlen++] = QSE_MT('[');	

				if (xlen + 1 < len)
				{
					xlen += qse_ipad6tombs (&nwad->u.in6.addr, buf, len);
					if (xlen + 1 < len && nwad->u.in6.scope != 0)
					{
						buf[xlen++] = QSE_MT('%');
						if (xlen + 1 < len)
						{
							xlen += qse_fmtuintmaxtombs (
								&buf[xlen], len - xlen, 
								nwad->u.in6.scope, 10, 0, QSE_WT('\0'), QSE_NULL);
						}
					}
	
					if (xlen + 1 < len && nwad->u.in6.port != 0) 
					{
						buf[xlen++] = QSE_MT(']');	
						if (xlen + 1 < len) 
						{
							buf[xlen++] = QSE_MT(':');
							if (xlen + 1 < len)
							{
								xlen += qse_fmtuintmaxtombs (
									&buf[xlen], len - xlen, 
									qse_ntoh16(nwad->u.in6.port),
									10, 0, QSE_WT('\0'), QSE_NULL);
							}
						}
					}
				}

			}
			break;

	}

	if (xlen + 1 < len) buf[xlen] = QSE_MT('\0');
	return xlen;
}

qse_size_t qse_nwadtowcs (const qse_nwad_t* nwad, qse_wchar_t* buf, qse_size_t len)
{
	qse_size_t xlen = 0;

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
			if (xlen + 1 < len)
			{
				xlen = qse_ipad4towcs (&nwad->u.in4.addr, buf, len);
				if (xlen + 1 < len && nwad->u.in4.port != 0) 
				{
					buf[xlen++] = QSE_WT(':');
					if (xlen + 1 < len)
					{
						xlen += qse_fmtuintmaxtowcs (
							&buf[xlen], len - xlen, 
							qse_ntoh16(nwad->u.in4.port),
							10, 0, QSE_WT('\0'), QSE_NULL);
					}
				}
			}
			break;

		case QSE_NWAD_IN6:
			if (xlen + 1 < len)
			{
				if (nwad->u.in6.port != 0) buf[xlen++] = QSE_WT('[');	

				if (xlen + 1 < len)
				{
					xlen += qse_ipad6towcs (&nwad->u.in6.addr, &buf[xlen], len - xlen);
					if (xlen + 1 < len && nwad->u.in6.scope != 0)
					{
						buf[xlen++] = QSE_WT('%');
						if (xlen + 1 < len)
						{
							xlen += qse_fmtuintmaxtowcs (
								&buf[xlen], len - xlen, 
								nwad->u.in6.scope, 10, 0, QSE_WT('\0'), QSE_NULL);
						}
					}

					if (xlen + 1 < len && nwad->u.in6.port != 0) 
					{
						buf[xlen++] = QSE_WT(']');	
						if (xlen + 1 < len) 
						{
							buf[xlen++] = QSE_WT(':');
							if (xlen + 1 < len)
							{
								xlen += qse_fmtuintmaxtowcs (
									&buf[xlen], len - xlen, 
									qse_ntoh16(nwad->u.in6.port),
									10, 0, QSE_WT('\0'), QSE_NULL);
							}
						}
					}
				}

			}
			break;

	}

	if (xlen < len) buf[xlen] = QSE_WT('\0');
	return xlen;
}
