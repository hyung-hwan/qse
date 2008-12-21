/*
 * $Id: tio_put.c,v 1.2 2005/12/26 07:41:48 bacon Exp $
 */

#include <qse/cmn/tio.h>

qse_ssize_t qse_tio_putc (qse_tio_t* tio, qse_char_t c)
{
#ifndef QSE_CHAR_IS_MCHAR
	qse_size_t n, i;
	qse_mchar_t mc[50];
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
		return qse_tio_flush (tio);
#else

	n = qse_wctomb (c, mc, QSE_COUNTOF(mc));
	if (n == 0) 
	{
		tio->errnum = QSE_TIO_EILCHR;
		return -1;
	}
	else if (n > QSE_COUNTOF(mc))
	{
		tio->errnum = QSE_TIO_ENOSPC;
		return -1;
	}

	for (i = 0; i < n; i++) 
	{
		tio->outbuf[tio->outbuf_len++] = mc[i];
		if (tio->outbuf_len >= QSE_COUNTOF(tio->outbuf)) 
		{
			if (qse_tio_flush(tio) == -1) return -1;
		}
	}		
#endif

	if (c == QSE_T('\n') && tio->outbuf_len > 0) 
	{
		if (qse_tio_flush(tio) == -1) return -1;
	}

	return 1;
}

qse_ssize_t qse_tio_puts (qse_tio_t* tio, const qse_char_t* str)
{
	qse_ssize_t n;
	const qse_char_t* p;

	for (p = str; *p != QSE_T('\0'); p++) 
	{
		n = qse_tio_putc (tio, *p);
		if (n == -1) return -1;
		if (n == 0) break;
	}

	return p - str;
}

qse_ssize_t qse_tio_putsx (qse_tio_t* tio, const qse_char_t* str, qse_size_t size)
{
	qse_ssize_t n;
	const qse_char_t* p, * end;

	if (size == 0) return 0;

	p = str; end = str + size;
	while (p < end) 
	{
		n = qse_tio_putc (tio, *p);
		if (n == -1) return -1;
		if (n == 0) break;

		p++;
	}

	return p - str;
}

#if 0
qse_ssize_t qse_tio_putsn (qse_tio_t* tio, ...)
{
	qse_ssize_t n;
	qse_va_list ap;

	qse_va_start (ap, tio);
	n = qse_tio_putsv (tio, ap);
	qse_va_end (ap);

	return n;
}

qse_ssize_t qse_tio_putsxn (qse_tio_t* tio, ...)
{
	qse_ssize_t n;
	qse_va_list ap;

	qse_va_start (ap, tio);
	n = qse_tio_putsxv (tio, ap);
	qse_va_end (ap);

	return n;
}

qse_ssize_t qse_tio_putsv (qse_tio_t* tio, qse_va_list ap)
{
	const qse_char_t* p;
	qse_size_t n, total = 0;

	while ((p = qse_va_arg (ap, const qse_char_t*)) != QSE_NULL) 
	{
		if (p[0] == QSE_T('\0')) continue;

		n = qse_tio_puts (tio, p);
		if (n == -1) return -1;
		if (n == 0) break;

		total += n;
	}

	return total;
}

qse_ssize_t qse_tio_putsxv (qse_tio_t* tio, qse_va_list ap)
{
	const qse_char_t* p;
	qse_size_t len, n, total = 0;

	while ((p = qse_va_arg (ap, const qse_char_t*)) != QSE_NULL) 
	{
		len = qse_va_arg (ap, qse_size_t);
		if (len == 0) continue;

		n = qse_tio_putsx (tio, p, len);
		if (n == -1) return -1;
		if (n == 0) break;

		total += n;
	}

	return total;
}
#endif
