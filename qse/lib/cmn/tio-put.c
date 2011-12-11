/*
 * $Id: tio-put.c 566 2011-09-11 12:44:56Z hyunghwan.chung $
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

#include <qse/cmn/tio.h>
#include <qse/cmn/chr.h>

static qse_ssize_t tio_putc (qse_tio_t* tio, qse_char_t c, int* flush_needed)
{
#ifdef QSE_CHAR_IS_WCHAR
	qse_size_t n, i;
	qse_mchar_t mc[QSE_MBLEN_MAX]; 
#endif

	if (tio->outbuf_len >= QSE_COUNTOF(tio->outbuf)) 
	{
		/* maybe, previous flush operation has failed a few 
		 * times previously. so the buffer is full.
		 */
		tio->errnum = QSE_TIO_ENOSPC;	
		return -1;
	}

#ifdef QSE_CHAR_IS_MCHAR

	tio->outbuf[tio->outbuf_len++] = c;	
	if (tio->outbuf_len >= QSE_COUNTOF(tio->outbuf))
	{
		*flush_needed = 0;
		return qse_tio_flush (tio);
	}

#else /*  QSE_CHAR_IS_WCHAR */

	n = qse_wcrtomb (c, mc, QSE_COUNTOF(mc), &tio->mbstate.out);
	if (n == 0) 
	{
		if (tio->flags & QSE_TIO_IGNOREMBWCERR) 
		{
			/* return 1 as if c has been written successfully */
			return 1;
		}

		tio->errnum = QSE_TIO_EILCHR;
		return -1;
	}
	else if (n > QSE_COUNTOF(mc))
	{
		if (tio->flags & QSE_TIO_IGNOREMBWCERR) return 1;
		tio->errnum = QSE_TIO_ENOSPC;
		return -1;
	}


	for (i = 0; i < n; i++) 
	{
		tio->outbuf[tio->outbuf_len++] = mc[i];
		if (tio->outbuf_len >= QSE_COUNTOF(tio->outbuf)) 
		{
			*flush_needed = 0;
			if (qse_tio_flush (tio) <= -1) return -1;
		}
	}		

#endif

	if (c == QSE_T('\n') && tio->outbuf_len > 0) 
	{
		/*if (qse_tio_flush (tio) <= -1) return -1;*/
		*flush_needed = 1;
	}

	return 1;
}

qse_ssize_t qse_tio_write (qse_tio_t* tio, const qse_char_t* str, qse_size_t size)
{
	qse_ssize_t n;
	const qse_char_t* p;
	int flush_needed = 0;

	if (size == 0) return 0;

	p = str;

	if (size == (qse_size_t)-1)
	{
/* TODO: should not write more than than QSE_TYPE_MAX(qse_ssize_t) */
		while (*p != QSE_T('\0'))
		{
			n = tio_putc (tio, *p, &flush_needed);
			if (n <= -1) return -1;
			if (n == 0) break;
			p++;
		}
	}
	else
	{
		const qse_char_t* end;

/* TODO: size should not be longer than QSE_TYPE_MAX(qse_ssize_t) */
		end = str + size;
		while (p < end) 
		{
			n = tio_putc (tio, *p, &flush_needed);
			if (n <= -1) return -1;
			if (n == 0) break;
			p++;
		}
	}

	if (flush_needed && qse_tio_flush(tio) <= -1) return -1;
	return p - str;
}

qse_ssize_t qse_tio_writemstr (
	qse_tio_t* tio, const qse_mchar_t* mptr, qse_size_t mlen)
{
	const qse_mchar_t* xptr, * xend;
	qse_size_t capa;
	int nl = 0;

	if (tio->outbuf_len >= QSE_COUNTOF(tio->outbuf)) 
	{
		/* maybe, previous flush operation has failed a few 
		 * times previously. so the buffer is full.
		 */
		tio->errnum = QSE_TIO_ENOSPC;	
		return -1;
	}

	/* adjust mlen for the type difference between the parameter
	 * and the return value */
	if (mlen > QSE_TYPE_MAX(qse_ssize_t)) mlen = QSE_TYPE_MAX(qse_ssize_t);

	/* handle the parts that can't fit into the internal buffer */
	while (mlen >= (capa = QSE_COUNTOF(tio->outbuf) - tio->outbuf_len))
	{
		for (xend = xptr + capa; xptr < xend; xptr++)
			tio->outbuf[tio->outbuf_len++] = *xptr;
		if (qse_tio_flush (tio) == -1) return -1;
		mlen -= capa;
	}

	/* handle the last part that can fit into the internal buffer */
	for (xend = xptr + mlen; xptr < xend; xptr++)
	{
		/* TODO: support different line terminating characeter */
		if (*xptr == QSE_MT('\n')) nl = 1; 
		tio->outbuf[tio->outbuf_len++] = *xptr;
	}

	/* if the last part contains a new line, flush the internal
	 * buffer. note that this flushes characters after nl also.*/
	if (nl && qse_tio_flush (tio) == -1) return -1;

	/* returns the number multi-bytes characters handled */
	return xptr - mptr;
}

#if 0
qse_ssize_t qse_tio_writewstr (
	qse_tio_t* tio, const qse_wchar_t* wptr, qse_size_t wlen)
{

	if (wlen > QSE_TYPE_MAX(qse_ssize_t)) wlen = QSE_TYPE_MAX(qse_ssize_t);

	while (1)
	{
		qse_size_t capa, mcnt, wcnt;

		capa = QSE_COUNTOF(tio->outbuf) - tio->outbuf_len;
		mcnt = capa;
		wcnt = qse_wcsntombsn (wptr, wlen, &tio->outbuf[tio->outbuf_len], &mcnt);

		if (wcnt == wlen)
		{
			/* process the all*/
		}

		if (mcnt >= capa) 
		{
			/* wcsntombsn doesn't null-terminate the result. *	/
			/* buffer is not large enough. flush first before continuing */
			if (qse_tio_flush (tio) == -1) return -1;
			continue;	
		}
		else if (wcnt != wlen)
		{
			/* invalid wide-character is included. */
			if (tio->flags & QSE_TIO_IGNOREMBWCERR)
			{
			}
		}
}
#endif
