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

#include <qse/cmn/tio.h>
#include <qse/cmn/mbwc.h> 
#include "mem.h"

#define STATUS_OUTPUT_DYNBUF (1 << 0)
#define STATUS_INPUT_DYNBUF  (1 << 1)
#define STATUS_INPUT_ILLSEQ  (1 << 2)
#define STATUS_INPUT_EOF     (1 << 3)

static int detach_in (qse_tio_t* tio, int fini);
static int detach_out (qse_tio_t* tio, int fini);

qse_tio_t* qse_tio_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, int flags)
{
	qse_tio_t* tio;

	tio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_tio_t) + xtnsize);
	if (tio)
	{
		if (qse_tio_init (tio, mmgr, flags) <= -1)
		{
			QSE_MMGR_FREE (mmgr, tio);
			return QSE_NULL;
		}

		else QSE_MEMSET (QSE_XTN(tio), 0, xtnsize);
	}
	return tio;
}

int qse_tio_close (qse_tio_t* tio)
{
	int n = qse_tio_fini (tio);
	QSE_MMGR_FREE (tio->mmgr, tio);
	return n;
}

int qse_tio_init (qse_tio_t* tio, qse_mmgr_t* mmgr, int flags)
{
	QSE_MEMSET (tio, 0, QSE_SIZEOF(*tio));

	tio->mmgr = mmgr;
	tio->cmgr = qse_getdflcmgr();

	tio->flags = flags;

	/*
	tio->input_func = QSE_NULL;
	tio->input_arg = QSE_NULL;
	tio->output_func = QSE_NULL;
	tio->output_arg = QSE_NULL;

	tio->status = 0;
	tio->inbuf_cur = 0;
	tio->inbuf_len = 0;
	tio->outbuf_len = 0;
	*/

	tio->errnum = QSE_TIO_ENOERR;
	return 0;
}

int qse_tio_fini (qse_tio_t* tio)
{
	int ret = 0;

	qse_tio_flush (tio); /* don't care about the result */
	if (detach_in (tio, 1) <= -1) ret = -1;
	if (detach_out (tio, 1) <= -1) ret = -1;

	return ret;
}

qse_mmgr_t* qse_tio_getmmgr (qse_tio_t* tio)
{
	return tio->mmgr;
}

void* qse_tio_getxtn (qse_tio_t* tio)
{
	return QSE_XTN (tio);
}

qse_tio_errnum_t qse_tio_geterrnum (const qse_tio_t* tio)
{
	return tio->errnum;
}

void qse_tio_seterrnum (qse_tio_t* tio, qse_tio_errnum_t errnum)
{
	tio->errnum = errnum;
}

qse_cmgr_t* qse_tio_getcmgr (qse_tio_t* tio)
{
	return tio->cmgr;
}

void qse_tio_setcmgr (qse_tio_t* tio, qse_cmgr_t* cmgr)
{
	tio->cmgr = cmgr;
}

int qse_tio_attachin (
	qse_tio_t* tio, qse_tio_io_impl_t input,
	qse_mchar_t* bufptr, qse_size_t bufcapa)
{
	qse_mchar_t* xbufptr;

	if (input == QSE_NULL || bufcapa < QSE_TIO_MININBUFCAPA) 
	{
		tio->errnum = QSE_TIO_EINVAL;
		return -1;
	}

	if (qse_tio_detachin(tio) <= -1) return -1;

	QSE_ASSERT (tio->in.fun == QSE_NULL);

	xbufptr = bufptr;
	if (xbufptr == QSE_NULL)
	{
		xbufptr = QSE_MMGR_ALLOC (
			tio->mmgr, QSE_SIZEOF(qse_mchar_t) * bufcapa);
		if (xbufptr == QSE_NULL)
		{
			tio->errnum = QSE_TIO_ENOMEM;
			return -1;	
		}
	}

	tio->errnum = QSE_TIO_ENOERR;
	if (input (tio, QSE_TIO_OPEN, QSE_NULL, 0) <= -1) 
	{
		if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
		if (xbufptr != bufptr) QSE_MMGR_FREE (tio->mmgr, xbufptr);
		return -1;
	}

	/* if i defined tio->io[2] instead of tio->in and tio-out, 
	 * i would be able to shorten code amount. but fields to initialize
	 * are not symmetric between input and output.
	 * so it's just a bit clumsy that i repeat almost the same code
	 * in qse_tio_attachout().
	 */

	tio->in.fun = input;
	tio->in.buf.ptr = xbufptr;
	tio->in.buf.capa = bufcapa;

	tio->status &= ~(STATUS_INPUT_ILLSEQ | STATUS_INPUT_EOF);
	tio->inbuf_cur = 0;
	tio->inbuf_len = 0;

	if (xbufptr != bufptr) tio->status |= STATUS_INPUT_DYNBUF;
	return 0;
}

static int detach_in (qse_tio_t* tio, int fini)
{
	int ret = 0;

	if (tio->in.fun)
	{
		tio->errnum = QSE_TIO_ENOERR;
		if (tio->in.fun (tio, QSE_TIO_CLOSE, QSE_NULL, 0) <= -1) 
		{
			if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;

			/* returning with an error here allows you to retry detaching */
			if (!fini) return -1; 

			/* otherwise, you can't retry since the input handler information
			 * is reset below */
			ret = -1; 
		}

		if (tio->status & STATUS_INPUT_DYNBUF) 
		{
			QSE_MMGR_FREE (tio->mmgr, tio->in.buf.ptr);
			tio->status &= ~STATUS_INPUT_DYNBUF;
		}

		tio->in.fun = QSE_NULL;
		tio->in.buf.ptr = QSE_NULL;
		tio->in.buf.capa = 0;
	}
		
	return ret;
}

int qse_tio_detachin (qse_tio_t* tio)
{
	return detach_in (tio, 0);
}

int qse_tio_attachout (
	qse_tio_t* tio, qse_tio_io_impl_t output, 
	qse_mchar_t* bufptr, qse_size_t bufcapa)
{
	qse_mchar_t* xbufptr;

	if (output == QSE_NULL || bufcapa < QSE_TIO_MINOUTBUFCAPA)  
	{
		tio->errnum = QSE_TIO_EINVAL;
		return -1;
	}

	if (qse_tio_detachout(tio) == -1) return -1;

	QSE_ASSERT (tio->out.fun == QSE_NULL);

	xbufptr = bufptr;
	if (xbufptr == QSE_NULL)
	{
		xbufptr = QSE_MMGR_ALLOC (
			tio->mmgr, QSE_SIZEOF(qse_mchar_t) * bufcapa);
		if (xbufptr == QSE_NULL)
		{
			tio->errnum = QSE_TIO_ENOMEM;
			return -1;	
		}
	}

	tio->errnum = QSE_TIO_ENOERR;
	if (output (tio, QSE_TIO_OPEN, QSE_NULL, 0) <= -1) 
	{
		if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
		if (xbufptr != bufptr) QSE_MMGR_FREE (tio->mmgr, xbufptr);
		return -1;
	}

	tio->out.fun = output;
	tio->out.buf.ptr = xbufptr;
	tio->out.buf.capa = bufcapa;

	tio->outbuf_len = 0;

	if (xbufptr != bufptr) tio->status |= STATUS_OUTPUT_DYNBUF;
	return 0;
}

static int detach_out (qse_tio_t* tio, int fini)
{
	int ret = 0;

	if (tio->out.fun)
	{
		qse_tio_flush (tio); /* don't care about the result */

		tio->errnum = QSE_TIO_ENOERR;
		if (tio->out.fun (tio, QSE_TIO_CLOSE, QSE_NULL, 0) <= -1) 
		{
			if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
			/* returning with an error here allows you to retry detaching */
			if (!fini) return -1;

			/* otherwise, you can't retry since the input handler information
			 * is reset below */
			ret = -1;
		}
	
		if (tio->status & STATUS_OUTPUT_DYNBUF) 
		{
			QSE_MMGR_FREE (tio->mmgr, tio->out.buf.ptr);
			tio->status &= ~STATUS_OUTPUT_DYNBUF;
		}

		tio->out.fun = QSE_NULL;
		tio->out.buf.ptr = QSE_NULL;
		tio->out.buf.capa = 0;
	}
		
	return ret;
}

int qse_tio_detachout (qse_tio_t* tio)
{
	return detach_out (tio, 0);
}

qse_ssize_t qse_tio_flush (qse_tio_t* tio)
{
	qse_size_t left, count;
	qse_ssize_t n;
	qse_mchar_t* cur;

	if (tio->out.fun == QSE_NULL)
	{
		tio->errnum = QSE_TIO_ENOUTF;
		return (qse_ssize_t)-1;
	}

	left = tio->outbuf_len;
	cur = tio->out.buf.ptr;
	while (left > 0) 
	{
		tio->errnum = QSE_TIO_ENOERR;
		n = tio->out.fun (tio, QSE_TIO_DATA, cur, left);
		if (n <= -1) 
		{
			if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
			if (cur != tio->out.buf.ptr)
			{
				QSE_MEMCPY (tio->out.buf.ptr, cur, left);
				tio->outbuf_len = left;
			}
			return -1;
		}
		if (n == 0) 
		{
			if (cur != tio->out.buf.ptr)
				QSE_MEMCPY (tio->out.buf.ptr, cur, left);
			break;
		}
	
		left -= n;
		cur += n;
	}

	count = tio->outbuf_len - left;
	tio->outbuf_len = left;

	return (qse_ssize_t)count;
}

void qse_tio_drain (qse_tio_t* tio)
{
	tio->status &= ~(STATUS_INPUT_ILLSEQ | STATUS_INPUT_EOF);
	tio->inbuf_cur = 0;
	tio->inbuf_len = 0;
	tio->outbuf_len = 0;
	tio->errnum = QSE_TIO_ENOERR;
}

/* ------------------------------------------------------------- */


qse_ssize_t qse_tio_readmbs (qse_tio_t* tio, qse_mchar_t* buf, qse_size_t size)
{
	qse_size_t nread;
	qse_ssize_t n;

	/*QSE_ASSERT (tio->in.fun != QSE_NULL);*/
	if (tio->in.fun == QSE_NULL) 
	{
		tio->errnum = QSE_TIO_ENINPF;
		return -1;
	}

	/* note that this function doesn't check if
	 * tio->status is set with STATUS_INPUT_ILLSEQ
	 * since this function can simply return the next
	 * available byte. */

	if (size > QSE_TYPE_MAX(qse_ssize_t)) size = QSE_TYPE_MAX(qse_ssize_t);

	nread = 0;
	while (nread < size)
	{
		if (tio->inbuf_cur >= tio->inbuf_len) 
		{
			tio->errnum = QSE_TIO_ENOERR;
			n = tio->in.fun (
				tio, QSE_TIO_DATA, 
				tio->in.buf.ptr, tio->in.buf.capa);
			if (n == 0) break;
			if (n <= -1) 
			{
				if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
				return -1;
			}

			tio->inbuf_cur = 0;
			tio->inbuf_len = (qse_size_t)n;
		}

		do
		{
			buf[nread] = tio->in.buf.ptr[tio->inbuf_cur++];
			/* TODO: support a different line terminator */
			if (buf[nread++] == QSE_MT('\n')) goto done;
		}
		while (tio->inbuf_cur < tio->inbuf_len && nread < size);
	}

done:
	return nread;
}

static QSE_INLINE qse_ssize_t tio_read_widechars (
	qse_tio_t* tio, qse_wchar_t* buf, qse_size_t bufsize)
{
	qse_size_t mlen, wlen;
	qse_ssize_t n;
	int x;

	if (tio->inbuf_cur >= tio->inbuf_len) 
	{
		tio->inbuf_cur = 0;
		tio->inbuf_len = 0;

	getc_conv:
		if (tio->status & STATUS_INPUT_EOF) n = 0;
		else
		{
			tio->errnum = QSE_TIO_ENOERR;
			n = tio->in.fun (
				tio, QSE_TIO_DATA,
				&tio->in.buf.ptr[tio->inbuf_len], 
				tio->in.buf.capa - tio->inbuf_len);
		}
		if (n == 0) 
		{
			tio->status |= STATUS_INPUT_EOF;

			if (tio->inbuf_cur < tio->inbuf_len)
			{
				/* no more input from the underlying input handler.
				 * but some incomplete bytes in the buffer. */
				if (tio->flags & QSE_TIO_IGNOREMBWCERR) 
				{
					/* tread them as illegal sequence */
					goto ignore_illseq;
				}
				else
				{
					tio->errnum = QSE_TIO_EICSEQ;
					return -1;
				}
			}

			return 0;
		}
		if (n <= -1) 
		{
			if (tio->errnum == QSE_TIO_ENOERR) tio->errnum = QSE_TIO_EOTHER;
			return -1;
		}

		tio->inbuf_len += n;
	}

	mlen = tio->inbuf_len - tio->inbuf_cur;
	wlen = bufsize;

	x = qse_mbsntowcsnuptowithcmgr (
		&tio->in.buf.ptr[tio->inbuf_cur],
		&mlen, buf, &wlen, QSE_WT('\n'), tio->cmgr);
	tio->inbuf_cur += mlen;

	if (x == -3)
	{
		/* incomplete sequence */
		if (wlen <= 0)
		{
			/* not even a single character was handled. 
			 * shift bytes in the buffer to the head. */
			QSE_ASSERT (mlen <= 0);
			tio->inbuf_len = tio->inbuf_len - tio->inbuf_cur;
			QSE_MEMCPY (&tio->in.buf.ptr[0], 
			            &tio->in.buf.ptr[tio->inbuf_cur],
			            tio->inbuf_len * QSE_SIZEOF(tio->in.buf.ptr[0]));
			tio->inbuf_cur = 0;
			goto getc_conv; /* and read more */
		}

		/* get going if some characters are handled */
	}
	else if (x == -2)
	{
		/* buffer not large enough */
		QSE_ASSERT (wlen > 0);
		
		/* the wide-character buffer is not just large enough to
		 * hold the entire conversion result. lets's go on so long as 
		 * 1 wide-character is produced though it may be inefficient.
		 */
	}
	else if (x <= -1)
	{
		/* illegal sequence */
		if (tio->flags & QSE_TIO_IGNOREMBWCERR)
		{
		ignore_illseq:
			tio->inbuf_cur++; /* skip one byte */
			buf[wlen++] = QSE_WT('?');
		}
		else if (wlen <= 0)
		{
			tio->errnum = QSE_TIO_EILSEQ;
			return -1;
		}
		else
		{
			/* some characters are already handled.
			 * mark that an illegal sequence encountered
			 * and carry on. */
			tio->status |= STATUS_INPUT_ILLSEQ;
		}
	}
	
	return wlen;
}

qse_ssize_t qse_tio_readwcs (qse_tio_t* tio, qse_wchar_t* buf, qse_size_t size)
{
	qse_size_t nread = 0;
	qse_ssize_t n;

	/*QSE_ASSERT (tio->in.fun != QSE_NULL);*/
	if (tio->in.fun == QSE_NULL) 
	{
		tio->errnum = QSE_TIO_ENINPF;
		return -1;
	}

	if (size > QSE_TYPE_MAX(qse_ssize_t)) size = QSE_TYPE_MAX(qse_ssize_t);

	while (nread < size)
	{
		if (tio->status & STATUS_INPUT_ILLSEQ) 
		{
			tio->status &= ~STATUS_INPUT_ILLSEQ;
			tio->errnum = QSE_TIO_EILSEQ;
			return -1;
		}
		
		n = tio_read_widechars (tio, &buf[nread], size - nread);
		if (n == 0) break;
		if (n <= -1) return -1;

		nread += n;
		if (buf[nread-1] == QSE_WT('\n')) break;
	}

	return nread;
}


/* ------------------------------------------------------------- */
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
			while (mptr[pos] != QSE_MT('\0')) 
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
			while (mptr[pos] != QSE_MT('\0')) 
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
				if (*xptr == QSE_MT('\n'))
				{
					nl = 1; 
					break;
				}

				tio->out.buf.ptr[tio->outbuf_len++] = *xptr;
			}

			/* continue copying without checking for nl */
			while (xptr < xend) tio->out.buf.ptr[tio->outbuf_len++] = *xptr++;
		}

		/* if the last part contains a new line, flush the internal
		 * buffer. note that this flushes characters after nl also.*/
		if (nl && qse_tio_flush (tio) <= -1) return -1;

		/* returns the number multi-byte characters handled */
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
		 * times previously. so the buffer is full. */
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
			 * it is not large enough in this case */
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
