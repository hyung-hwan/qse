/*
 * $Id: assert.cpp,v 1.2 2007/07/16 11:12:12 bacon Exp $
 */

#include "stdafx.h"

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

