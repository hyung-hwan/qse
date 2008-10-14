/*
 * $Id: tio_get.c,v 1.8 2005/12/26 07:41:48 bacon Exp $
 */

#include <ase/cmn/tio.h>
#include "mem.h"

#define STATUS_GETC_EILSEQ  (1 << 0)

ase_ssize_t ase_tio_getc (ase_tio_t* tio, ase_char_t* c)
{
	ase_size_t left = 0;
	ase_ssize_t n;
	ase_char_t curc;
#ifndef ASE_CHAR_IS_MCHAR
	ase_size_t seqlen;
#endif

	/* TODO: more efficient to check this...
	 *       maybe better to use ASE_ASSERT 
	 * ASE_ASSERT (tio->input_func != ASE_NULL);
	 */
	if (tio->input_func == ASE_NULL) 
	{
		tio->errnum = ASE_TIO_ENOINF;
		return -1;
	}

	if (tio->input_status & STATUS_GETC_EILSEQ) 
	{
		tio->input_status &= ~STATUS_GETC_EILSEQ;
		tio->errnum = ASE_TIO_EILSEQ;
		return -1;
	}

	if (tio->inbuf_curp >= tio->inbuf_len) 
	{
getc_conv:
		n = tio->input_func (
			ASE_TIO_IO_DATA, tio->input_arg,
			&tio->inbuf[left], ASE_COUINTOF(tio->inbuf) - left);
		if (n == 0) return 0;
		if (n <= -1) 
		{
			tio->errnum = ASE_TIO_EINPUT;
			return -1;
		}

		tio->inbuf_curp = 0;
		tio->inbuf_len = (ase_size_t)n + left;	
	}

#ifdef ASE_CHAR_IS_MCHAR
	curc = tio->inbuf[tio->inbuf_curp++];
#else
	seqlen = ase_mcseqlen (tio->inbuf[tio->inbuf_curp]);
	if (seqlen == 0) {
		tio->inbuf_curp++;  /* skip one byte */
		tio->errnum = ASE_TIO_EILSEQ;
		return -1;
	}

	left = tio->inbuf_len - tio->inbuf_curp;
	if (left < seqlen) {
		ase_memcpy (tio->inbuf, &tio->inbuf[tio->inbuf_curp], left);
		tio->inbuf_curp = 0;
		tio->inbuf_len = left;
		goto getc_conv;
	}
	
	n = ase_mctowc (&tio->inbuf[tio->inbuf_curp], seqlen, &curc);
	if (n == 0) {
		tio->inbuf_curp++; /* skip one byte */
		tio->errnum = ASE_TIO_EILSEQ;
		return -1;
	}

	tio->inbuf_curp += n;
#endif

	*c = curc;
	return 1;
}

ase_ssize_t ase_tio_gets (ase_tio_t* tio, ase_char_t* buf, ase_size_t size)
{
	ase_ssize_t n;

	if (size <= 0) return 0;
	n = ase_tio_getsx (tio, buf, size - 1);
	if (n == -1) return -1;
	buf[n] = ASE_CHAR('\0');
	return n;
}

ase_ssize_t ase_tio_getsx (ase_tio_t* tio, ase_char_t* buf, ase_size_t size)
{
	ase_ssize_t n;
	ase_char_t* p, * end, c;

	if (size <= 0) return 0;

	p = buf; end = buf + size;
	while (p < end) 
	{
		n = ase_tio_getc (tio, &c);
		if (n == -1) 
		{
			if (p > buf && tio->errnum == ASE_TIO_EILSEQ) 
			{
				tio->input_status |= STATUS_GETC_EILSEQ;
				break;
			}
			return -1;
		}
		if (n == 0) break;
		*p++ = c;

		if (c == ASE_CHAR('\n')) break;
	}

	return p - buf;
}

ase_ssize_t ase_tio_getstr (ase_tio_t* tio, ase_str_t* buf)
{
	ase_ssize_t n;
	ase_char_t c;

	ase_str_clear (buf);

	for (;;) 
	{
		n = ase_tio_getc (tio, &c);
		if (n == -1) 
		{
			if (ASE_STR_LEN(buf) > 0 && tio->errnum == ASE_TIO_EILSEQ) 
			{
				tio->input_status |= STATUS_GETC_EILSEQ;
				break;
			}
			return -1;
		}
		if (n == 0) break;

		if (ase_str_ccat(buf, c) == (ase_size_t)-1) 
		{
			tio->errnum = ASE_TIO_ENOMEM;
			return -1;
		}

		if (c == ASE_CHAR('\n')) break;
	}

	return ASE_STR_LEN(buf);
}
