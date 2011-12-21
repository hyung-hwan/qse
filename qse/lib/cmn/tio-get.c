/*
 * $Id: tio-get.c 566 2011-09-11 12:44:56Z hyunghwan.chung $
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
#include "mem.h"

#define STATUS_ILLSEQ  (1 << 0)
#define STATUS_EOF     (1 << 1)

qse_ssize_t qse_tio_readmbs (qse_tio_t* tio, qse_mchar_t* buf, qse_size_t size)
{
	qse_size_t nread;
	qse_ssize_t n;

	/*QSE_ASSERT (tio->input_func != QSE_NULL);*/
	if (tio->input_func == QSE_NULL) 
	{
		tio->errnum = QSE_TIO_ENOINF;
		return -1;
	}

	/* note that this function doesn't check if
	 * tio->input_status is set with STATUS_ILLSEQ
	 * since this function can simply return the next
	 * available byte. */

	if (size > QSE_TYPE_MAX(qse_ssize_t)) size = QSE_TYPE_MAX(qse_ssize_t);

	nread = 0;
	while (nread < size)
	{
		if (tio->inbuf_cur >= tio->inbuf_len) 
		{
			n = tio->input_func (
				QSE_TIO_IO_DATA, tio->input_arg,
				tio->inbuf, QSE_COUNTOF(tio->inbuf));
			if (n == 0) break;
			if (n <= -1) 
			{
				tio->errnum = QSE_TIO_EINPUT;
				return -1;
			}

			tio->inbuf_cur = 0;
			tio->inbuf_len = (qse_size_t)n;
		}

		do
		{
			buf[nread] = tio->inbuf[tio->inbuf_cur++];
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
		if (tio->input_status & STATUS_EOF) n = 0;
		else
		{
			n = tio->input_func (
				QSE_TIO_IO_DATA, tio->input_arg,
				&tio->inbuf[tio->inbuf_len], QSE_COUNTOF(tio->inbuf) - tio->inbuf_len);
		}
		if (n == 0) 
		{
			tio->input_status |= STATUS_EOF;

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
			tio->errnum = QSE_TIO_EINPUT;
			return -1;
		}

		tio->inbuf_len += n;
	}

	mlen = tio->inbuf_len - tio->inbuf_cur;
	wlen = bufsize;

	x = qse_mbsntowcsnupto (&tio->inbuf[tio->inbuf_cur], &mlen, buf, &wlen, QSE_WT('\n'));
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
			QSE_MEMCPY (&tio->inbuf[0], 
			            &tio->inbuf[tio->inbuf_cur],
			            tio->inbuf_len * QSE_SIZEOF(tio->inbuf[0]));
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
			tio->input_status |= STATUS_ILLSEQ;
		}
	}
	
	return wlen;
}

qse_ssize_t qse_tio_readwcs (qse_tio_t* tio, qse_wchar_t* buf, qse_size_t size)
{
	qse_size_t nread = 0;
	qse_ssize_t n;

	/*QSE_ASSERT (tio->input_func != QSE_NULL);*/
	if (tio->input_func == QSE_NULL) 
	{
		tio->errnum = QSE_TIO_ENOINF;
		return -1;
	}

	if (size > QSE_TYPE_MAX(qse_ssize_t)) size = QSE_TYPE_MAX(qse_ssize_t);

	while (nread < size)
	{
		if (tio->input_status & STATUS_ILLSEQ) 
		{
			tio->input_status &= ~STATUS_ILLSEQ;
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
