/*
 * $Id$
 */

#include <ase/cmn/chr.h>
#include "mem.h"

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

ase_size_t ase_mblen (const ase_mchar_t* mb, ase_size_t mblen)
{
#ifdef HAVE_MBRLEN
	size_t n;
	mbstate_t mbs = { 0 };

	n = mbrlen (mb, mblen, &mbs);
	if (n == 0) return 1; /* a null character */

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mblen + 1; /* incomplete sequence */

	return (ase_size_t)n;
#else
	#error #### NOT SUPPORTED ####
#endif
}

ase_size_t ase_mbtowc (const ase_mchar_t* mb, ase_size_t mblen, ase_wchar_t* wc)
{
#ifdef HAVE_MBRTOWC
	size_t n;
	mbstate_t mbs = { 0 };

	n = mbrtowc (wc, mb, mblen, &mbs);
	if (n == 0) 
	{
		*wc = ASE_WT('\0');
		return 1;
	}

	if (n == (size_t)-1) return 0; /* invalid sequence */
	if (n == (size_t)-2) return mblen + 1; /* incomplete sequence */
	return (ase_size_t)n;
#else
	#error #### NOT SUPPORTED ####
#endif
}

ase_size_t ase_wctomb (ase_wchar_t wc, ase_mchar_t* mb, ase_size_t mblen)
{
#ifdef HAVE_WCRTOMB
	size_t n;
	mbstate_t mbs = { 0 };

/* man mbsinit
 * For 8-bit encodings, all states are equivalent to  the  initial  state.
 * For multibyte encodings like UTF-8, EUC-*, BIG5 or SJIS, the wide char‐
 * acter to  multibyte  conversion  functions  never  produce  non-initial
 * states,  but  the multibyte to wide-character conversion functions like
 * mbrtowc(3) do produce non-initial states when interrupted in the middle
 * of a character.
 */

	if (mblen < MB_CUR_MAX)
	{
		ase_mchar_t buf[MB_CUR_MAX];

		n = wcrtomb (buf, wc, &mbs);
		if (n > mblen) return mblen + 1; /* buffer to small */
		if (n == (size_t)-1) return 0; /* illegal character */

		ASE_MEMCPY (mb, buf, mblen);
	}
	else
	{
		n = wcrtomb (mb, wc, &mbs);
		if (n > mblen) return mblen + 1; /* buffer to small */
		if (n == (size_t)-1) return 0; /* illegal character */
	}

	return n;
#else
	#error #### NOT SUPPORTED ####
#endif
}

