/*
 * $Id: misc.cpp,v 1.1 2007/07/20 09:23:37 bacon Exp $
 */

#include "stdafx.h"
#include "misc.h"

#ifndef NDEBUG

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>

#pragma warning(disable:4996)
#pragma unmanaged

void ase_assert_abort (void)
{
	::abort ();
}

void ase_assert_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
#ifdef _WIN32
	int n;
	ase_char_t buf[1024];
#endif

	va_start (ap, fmt);

	n = _vsntprintf (buf, ASE_COUNTOF(buf), fmt, ap);
	if (n < 0) buf[ASE_COUNTOF(buf)-1] = ASE_T('\0');

	//ase_vprintf (fmt, ap);
	::MessageBox (NULL, buf, 
		ASE_T("ASSERTION FAILURE"), MB_OK|MB_ICONERROR);

	va_end (ap);
}
#endif

char* unicode_to_multibyte (const wchar_t* in, int inlen, int* outlen)
{
	int n;
	n = WideCharToMultiByte (CP_UTF8, 0, in, inlen, NULL, 0, NULL, 0);

	char* ptr = (char*)::malloc (sizeof(char)*n);
	if (ptr == NULL) return NULL;

	*outlen = WideCharToMultiByte (CP_UTF8, 0, in, inlen, ptr, n, NULL, 0);
	return ptr;
}