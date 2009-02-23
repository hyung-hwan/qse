/*
 * $Id: tio_get.c 76 2009-02-22 14:18:06Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/tio.h>
#include <qse/cmn/chr.h>
#include "mem.h"

#define STATUS_GETC_EILSEQ  (1 << 0)

static qse_ssize_t tio_getc (qse_tio_t* tio, qse_char_t* c)
{
	qse_size_t left = 0;
	qse_ssize_t n;
	qse_char_t curc;

	/* TODO: more efficient way to check this?
	 *       maybe better to use QSE_ASSERT 
	 * QSE_ASSERT (tio->input_func != QSE_NULL);
	 */
	if (tio->input_func == QSE_NULL) 
	{
		tio->errnum = QSE_TIO_ENOINF;
		return -1;
	}

	if (tio->input_status & STATUS_GETC_EILSEQ) 
	{
		tio->input_status &= ~STATUS_GETC_EILSEQ;
		tio->errnum = QSE_TIO_EILSEQ;
		return -1;
	}

	if (tio->inbuf_curp >= tio->inbuf_len) 
	{
	getc_conv:
		n = tio->input_func (
			QSE_TIO_IO_DATA, tio->input_arg,
			&tio->inbuf[left], QSE_COUNTOF(tio->inbuf)-left);
		if (n == 0) 
		{
			if (tio->inbuf_curp < tio->inbuf_len)
			{
				/* gargage left in the buffer */
				tio->errnum = QSE_TIO_EICSEQ;
				return -1;
			}

			return 0;
		}
		if (n <= -1) 
		{
			tio->errnum = QSE_TIO_EINPUT;
			return -1;
		}

		tio->inbuf_curp = 0;
		tio->inbuf_len = (qse_size_t)n + left;	
	}

#ifdef QSE_CHAR_IS_MCHAR
	curc = tio->inbuf[tio->inbuf_curp++];
#else
	left = tio->inbuf_len - tio->inbuf_curp;

	n = qse_mbtowc (&tio->inbuf[tio->inbuf_curp], left, &curc);
	if (n == 0) 
	{
		/* illegal sequence */
		tio->inbuf_curp++; /* skip one byte */
		tio->errnum = QSE_TIO_EILSEQ;
		return -1;
	}
	if (n > left)
	{
		/* incomplete sequence */
		if (tio->inbuf_curp > 0)
		{
			QSE_MEMCPY (tio->inbuf, &tio->inbuf[tio->inbuf_curp], left);
			tio->inbuf_curp = 0;
			tio->inbuf_len = left;
		}
		goto getc_conv;
	}

	tio->inbuf_curp += n;
#endif

	*c = curc;
	return 1;
}

qse_ssize_t qse_tio_read (qse_tio_t* tio, qse_char_t* buf, qse_size_t size)
{
	qse_ssize_t n;
	qse_char_t* p, * end, c;

	if (size <= 0) return 0;

	p = buf; end = buf + size;
	while (p < end) 
	{
		n = tio_getc (tio, &c);
		if (n == -1) 
		{
			if (p > buf && tio->errnum == QSE_TIO_EILSEQ) 
			{
				tio->input_status |= STATUS_GETC_EILSEQ;
				break;
			}
			return -1;
		}
		if (n == 0) break;
		*p++ = c;

		/* TODO: support a different line breaker */
		if (c == QSE_T('\n')) break;
	}

	return p - buf;
}

