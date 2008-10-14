/*
 * $Id: tio_put.c,v 1.2 2005/12/26 07:41:48 bacon Exp $
 */

#include <ase/cmn/tio.h>

ase_ssize_t ase_tio_putc (ase_tio_t* tio, ase_char_t c)
{
#ifndef ASE_CHAR_IS_MCHAR
	ase_size_t n, i;
	ase_mchar_t mc[50];
#endif

	if (tio->outbuf_len >= ASE_COUNTOF(tio->outbuf)) 
	{
		/* maybe, previous flush operation has failed a few 
		 * times previously. so the buffer is full.
		 */
		tio->errnum = ASE_TIO_ENOSPC;	
		return -1;
	}

#ifdef ASE_CHAR_IS_MCHAR
	tio->outbuf[tio->outbuf_len++] = c;	
	if (tio->outbuf_len >= ASE_COUNTOF(tio->outbuf))
		return ase_tio_flush (tio);
#else

	n = ase_wctomc (c, mc, ASE_COUNTOF(mc));
	if (n == 0) 
	{
		tio->errnum = ASE_TIO_EILSEQ;
		return -1;
	}

	for (i = 0; i < n; i++) 
	{
		tio->outbuf[tio->outbuf_len++] = mc[i];
		if (tio->outbuf_len >= ASE_COUNTOF(tio->outbuf)) 
		{
			if (ase_tio_flush(tio) == -1) return -1;
		}
	}		
#endif

	if (c == ASE_CHAR('\n') && tio->outbuf_len > 0) 
	{
		if (ase_tio_flush(tio) == -1) return -1;
	}

	return 1;
}

ase_ssize_t ase_tio_puts (ase_tio_t* tio, const ase_char_t* str)
{
	ase_ssize_t n;
	const ase_char_t* p;

	for (p = str; *p != ASE_CHAR('\0'); p++) 
	{
		n = ase_tio_putc (tio, *p);
		if (n == -1) return -1;
		if (n == 0) break;
	}

	return p - str;
}

ase_ssize_t ase_tio_putsx (ase_tio_t* tio, const ase_char_t* str, ase_size_t size)
{
	ase_ssize_t n;
	const ase_char_t* p, * end;

	if (size == 0) return 0;

	p = str; end = str + size;
	while (p < end) 
	{
		n = ase_tio_putc (tio, *p);
		if (n == -1) return -1;
		if (n == 0) break;

		p++;
	}

	return p - str;
}

#if 0
ase_ssize_t ase_tio_putsn (ase_tio_t* tio, ...)
{
	ase_ssize_t n;
	ase_va_list ap;

	ase_va_start (ap, tio);
	n = ase_tio_putsv (tio, ap);
	ase_va_end (ap);

	return n;
}

ase_ssize_t ase_tio_putsxn (ase_tio_t* tio, ...)
{
	ase_ssize_t n;
	ase_va_list ap;

	ase_va_start (ap, tio);
	n = ase_tio_putsxv (tio, ap);
	ase_va_end (ap);

	return n;
}

ase_ssize_t ase_tio_putsv (ase_tio_t* tio, ase_va_list ap)
{
	const ase_char_t* p;
	ase_size_t n, total = 0;

	while ((p = ase_va_arg (ap, const ase_char_t*)) != ASE_NULL) 
	{
		if (p[0] == ASE_CHAR('\0')) continue;

		n = ase_tio_puts (tio, p);
		if (n == -1) return -1;
		if (n == 0) break;

		total += n;
	}

	return total;
}

ase_ssize_t ase_tio_putsxv (ase_tio_t* tio, ase_va_list ap)
{
	const ase_char_t* p;
	ase_size_t len, n, total = 0;

	while ((p = ase_va_arg (ap, const ase_char_t*)) != ASE_NULL) 
	{
		len = ase_va_arg (ap, ase_size_t);
		if (len == 0) continue;

		n = ase_tio_putsx (tio, p, len);
		if (n == -1) return -1;
		if (n == 0) break;

		total += n;
	}

	return total;
}
#endif
