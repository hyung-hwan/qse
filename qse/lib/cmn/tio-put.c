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
#include <qse/cmn/mbwc.h>

qse_ssize_t qse_tio_writembs (
	qse_tio_t* tio, const qse_mchar_t* mptr, qse_size_t mlen)
{
	if (tio->outbuf_len >= tio->out.buf.capa) 
	{
		/* maybe, previous flush operation has failed a few 
		 * times previously. so the buffer is full.
		 */
		tio->errnum = QSE_TIO_ENOSPC;	
		return -1;
	}

	if (mlen == (qse_size_t)-1)
	{
		qse_size_t pos = 0;

		if (tio->flags & QSE_TIO_NOAUTOFLUSH)
		{
			while (mptr[pos]) 
			{
				tio->out.buf.ptr[tio->outbuf_len++] = mptr[pos++];
				if (tio->outbuf_len >= tio->out.buf.capa &&
				    qse_tio_flush (tio) <= -1) return -1;
				if (pos >= QSE_TYPE_MAX(qse_ssize_t)) break;
			}
		}
		else
		{
			int nl = 0;
			while (mptr[pos]) 
			{
				tio->out.buf.ptr[tio->outbuf_len++] = mptr[pos];
				if (tio->outbuf_len >= tio->out.buf.capa)
				{
					if (qse_tio_flush (tio) <= -1) return -1;
					nl = 0;
				}
				else if (mptr[pos] == QSE_T('\n')) nl = 1; 
				/* TODO: different line terminator */
				if (++pos >= QSE_TYPE_MAX(qse_ssize_t)) break;
			}
			if (nl && qse_tio_flush(tio) <= -1) return -1;
		}

		return pos;
	}
	else
	{	
		const qse_mchar_t* xptr, * xend;
		qse_size_t capa;
		int nl = 0;

		/* adjust mlen for the type difference between the parameter
		 * and the return value */
		if (mlen > QSE_TYPE_MAX(qse_ssize_t)) mlen = QSE_TYPE_MAX(qse_ssize_t);
		xptr = mptr;

		/* handle the parts that can't fit into the internal buffer */
		while (mlen >= (capa = tio->out.buf.capa - tio->outbuf_len))
		{
			for (xend = xptr + capa; xptr < xend; xptr++)
				tio->out.buf.ptr[tio->outbuf_len++] = *xptr;
			if (qse_tio_flush (tio) <= -1) return -1;
			mlen -= capa;
		}

		if (tio->flags & QSE_TIO_NOAUTOFLUSH)
		{
			/* handle the last part that can fit into the internal buffer */
			for (xend = xptr + mlen; xptr < xend; xptr++)
				tio->out.buf.ptr[tio->outbuf_len++] = *xptr;
		}
		else
		{
			/* handle the last part that can fit into the internal buffer */
			for (xend = xptr + mlen; xptr < xend; xptr++)
			{
				/* TODO: support different line terminating characeter */
				tio->out.buf.ptr[tio->outbuf_len++] = *xptr;
				if (*xptr == QSE_MT('\n')) 
				{
					nl = 1; 
					break;
				}
			}
			while (xptr < xend) tio->out.buf.ptr[tio->outbuf_len++] = *xptr++;
		}

		/* if the last part contains a new line, flush the internal
		 * buffer. note that this flushes characters after nl also.*/
		if (nl && qse_tio_flush (tio) <= -1) return -1;

		/* returns the number multi-bytes characters handled */
		return xptr - mptr;
	}
}

qse_ssize_t qse_tio_writewcs (
	qse_tio_t* tio, const qse_wchar_t* wptr, qse_size_t wlen)
{
	qse_size_t capa, wcnt, mcnt, xwlen;
	int n, nl = 0;

	if (tio->outbuf_len >= tio->out.buf.capa) 
	{
		/* maybe, previous flush operation has failed a few 
		 * times previously. so the buffer is full.
		 */
		tio->errnum = QSE_TIO_ENOSPC;	
		return -1;
	}

	if (wlen == (qse_size_t)-1) wlen = qse_wcslen(wptr);
	if (wlen > QSE_TYPE_MAX(qse_ssize_t)) wlen = QSE_TYPE_MAX(qse_ssize_t);

	xwlen = wlen;
	while (xwlen > 0)
	{
		capa = tio->out.buf.capa - tio->outbuf_len;
		wcnt = xwlen; mcnt = capa;

		n = qse_wcsntombsnwithcmgr (
			wptr, &wcnt, &tio->out.buf.ptr[tio->outbuf_len], &mcnt, tio->cmgr);
		tio->outbuf_len += mcnt;

		if (n == -2)
		{
			/* the buffer is not large enough to 
			 * convert more. so flush now and continue.
			 * note that the buffer may not be full though 
			 * it not large enough in this case */
			if (qse_tio_flush (tio) <= -1) return -1;
			nl = 0;
		}
		else 
		{
			if (tio->outbuf_len >= tio->out.buf.capa)
			{
				/* flush the full buffer regardless of conversion
				 * result. */
				if (qse_tio_flush (tio) <= -1) return -1;
				nl = 0;		  
			}

			if (n <= -1)
			{
				/* an invalid wide-character is encountered. */
				if (tio->flags & QSE_TIO_IGNOREMBWCERR)
				{
					/* insert a question mark for an illegal 
					 * character. */
					QSE_ASSERT (tio->outbuf_len < tio->out.buf.capa);
					tio->out.buf.ptr[tio->outbuf_len++] = QSE_MT('?');
					wcnt++; /* skip this illegal character */
					/* don't need to increment mcnt since
					 * it's not used below */
				}
				else
				{
					tio->errnum = QSE_TIO_EILCHR;
					return -1;
				}
			}
			else
			{
				if (!(tio->flags & QSE_TIO_NOAUTOFLUSH) && !nl)
				{
					/* checking for a newline this way looks damn ugly.
					 * TODO: how can i do this more elegantly? */
					qse_size_t i = wcnt;
					while (i > 0)
					{
						/* scan backward assuming a line terminator
						 * is typically at the back */
						if (wptr[--i] == QSE_WT('\n'))  
						{
							/* TOOD: differetn line terminator */
							nl = 1; 
							break;
						}
					}
				}
			}
		}
		wptr += wcnt; xwlen -= wcnt;
	}

	if (nl && qse_tio_flush (tio) <= -1) return -1;
	return wlen;
}
