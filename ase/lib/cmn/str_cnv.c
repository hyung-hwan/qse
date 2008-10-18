/*
 * $Id: str_cnv.c 430 2008-10-17 11:43:20Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/str.h>

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif

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
	ase_size_t len, wlen;
	for (len = 0; *mbs++ != '\0'; len++);


	if (*wcslen <= 0) return 0;
	if (*wcslen == 1) 
	{
		wcs[0] = L'\0';
		return 0;
	}

	/* because ase_mbtowc needs the length, we get the lenght of mbs 
	 * and pass it to ase_mbsntowcsn */
	wlen = *wcslen - 1;
	len = ase_mbsntowcsn (mbs, len, wcs, &wlen);

	wcs[wlen] = L'\0';
	*wcslen = wlen;
/* TODO: wcslen should include the length including null? */
	return len;
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

ase_size_t wcstombs (
	const ase_wchar_t* wcs, ase_mchar_t* mbs, ase_size_t* mbslen)
{
	const ase_wchar_t* p = wcs;
	ase_size_t len = *mbslen;

	while (*p != ASE_T('\0') && len > 1) 
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
	if (len > 0) *mbs = '\0';

	/* returns the number of characters handled.
	 * the caller can check if the return value is as large is wcslen
	 * for an error. */
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

