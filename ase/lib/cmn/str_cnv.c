/*
 * $Id: str_cnv.c 432 2008-10-20 11:22:02Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/str.h>

int ase_strtoi (const ase_char_t* str)
{
	int v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

long ase_strtol (const ase_char_t* str)
{
	long v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

unsigned int ase_strtoui (const ase_char_t* str)
{
	unsigned int v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

unsigned long ase_strtoul (const ase_char_t* str)
{
	unsigned long v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

int ase_strxtoi (const ase_char_t* str, ase_size_t len)
{
	int v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

long ase_strxtol (const ase_char_t* str, ase_size_t len)
{
	long v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

unsigned int ase_strxtoui (const ase_char_t* str, ase_size_t len)
{
	unsigned int v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

unsigned long ase_strxtoul (const ase_char_t* str, ase_size_t len)
{
	unsigned long v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_int_t ase_strtoint (const ase_char_t* str)
{
	ase_int_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_long_t ase_strtolong (const ase_char_t* str)
{
	ase_long_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_uint_t ase_strtouint (const ase_char_t* str)
{
	ase_uint_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_ulong_t ase_strtoulong (const ase_char_t* str)
{
	ase_ulong_t v;
	ASE_STRTONUM (v, str, ASE_NULL, 10);
	return v;
}

ase_int_t ase_strxtoint (const ase_char_t* str, ase_size_t len)
{
	ase_int_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_long_t ase_strxtolong (const ase_char_t* str, ase_size_t len)
{
	ase_long_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_uint_t ase_strxtouint (const ase_char_t* str, ase_size_t len)
{
	ase_uint_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_ulong_t ase_strxtoulong (const ase_char_t* str, ase_size_t len)
{
	ase_ulong_t v;
	ASE_STRXTONUM (v, str, len, ASE_NULL, 10);
	return v;
}

ase_size_t ase_mbstowcs (
	const ase_mchar_t* mbs, ase_wchar_t* wcs, ase_size_t* wcslen)
{
	ase_size_t wlen, mlen;
	const ase_mchar_t* mp;

	/* get the length of mbs and pass it to ase_mbsntowcsn as 
	 * ase_mbtowc called by ase_mbsntowcsn needs it. */
	for (mp = mbs; *mp != '\0'; mp++);

	if (*wcslen <= 0) 
	{
		/* buffer too small. cannot null-terminate it */
		return 0;
	}
	if (*wcslen == 1) 
	{
		wcs[0] = L'\0';
		return 0;
	}

	wlen = *wcslen - 1;
	mlen = ase_mbsntowcsn (mbs, mp - mbs, wcs, &wlen);

	wcs[wlen] = L'\0';
	*wcslen = wlen;

	return mlen;
}

ase_size_t ase_mbsntowcsn (
	const ase_mchar_t* mbs, ase_size_t mbslen,
	ase_wchar_t* wcs, ase_size_t* wcslen)
{
	ase_size_t mlen = mbslen, n;
	const ase_mchar_t* p;
	ase_wchar_t* q, * qend ;

	qend = wcs + *wcslen;

	for (p = mbs, q = wcs; mlen > 0 && q < qend; p += n, mlen -= n) 
	{
		n = ase_mbtowc (p, mlen, q);
		if (n == 0 || n > mlen)
		{
			/* wrong sequence or insufficient input */
			break;
		}

		q++;
	}

	*wcslen = q - wcs;
	return p - mbs; /* returns the number of bytes processed */
}

ase_size_t ase_wcstombs (
	const ase_wchar_t* wcs, ase_mchar_t* mbs, ase_size_t* mbslen)
{
	const ase_wchar_t* p = wcs;
	ase_size_t rem = *mbslen;

	while (*p != ASE_T('\0') && rem > 1) 
	{
		ase_size_t n = ase_wctomb (*p, mbs, rem);
		if (n == 0 || n > rem)
		{
			/* illegal character or buffer not enough */
			break;
		}

		if (rem == n) 
		{
			/* the buffer is full without the space for a 
			 * terminating null. should stop processing further
			 * excluding this last character emitted. */
			break;
		}

		mbs += n; rem -= n; p++;
	}

	*mbslen -= rem; 

	/* null-terminate the multibyte sequence if it has sufficient space */
	if (rem > 0) *mbs = '\0';

	/* returns the number of characters handled. */
	return p - wcs; 
}

ase_size_t ase_wcsntombsn (
	const ase_wchar_t* wcs, ase_size_t wcslen,
	ase_mchar_t* mbs, ase_size_t* mbslen)
{
	const ase_wchar_t* p = wcs;
	const ase_wchar_t* end = wcs + wcslen;
	ase_size_t len = *mbslen;

	while (p < end && len > 0) 
	{
		ase_size_t n = ase_wctomb (*p, mbs, len);
		if (n == 0 || n > len)
		{
			/* illegal character or buffer not enough */
			break;
		}
		mbs += n; len -= n; p++;
	}

	*mbslen -= len; 

	/* returns the number of characters handled.
	 * the caller can check if the return value is as large is wcslen
	 * for an error. */
	return p - wcs; 
}

