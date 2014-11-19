/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/sio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static void print_error (
	const QSE::StdAwk::loc_t& loc, const QSE::StdAwk::char_t* msg)
{
	if (loc.line > 0 || loc.colm > 0)
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s at LINE %lu COLUMN %lu\n"), msg, loc.line, loc.colm);
	else
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), msg);
	
}

static int run_awk (QSE::StdAwk& awk)
{
	// ARGV[0]
	if (awk.addArgument (QSE_T("awk05")) <= -1) return -1;

	// ARGV[1] and/or the first console input file
	if (awk.addArgument (QSE_T("Makefile")) <= -1) return -1;

	const qse_char_t* script = QSE_T(
		"BEGIN { print \">> PRINT ALL LINES WHOSE LENGTH IS GREATER THAN 0\"; }\n" 
		"length($0) > 0 { print $0; count++; }\n"
		"END { print \">> TOTAL\", count, \"LINES\"; }\n"
	);

	QSE::StdAwk::SourceString in (script);
	QSE::StdAwk::SourceFile out (QSE_T("awk05.out"));

	// parse the script string and deparse it to awk05.out.
	if (awk.parse (in, out) == QSE_NULL) return -1;

	QSE::StdAwk::Value r;
	// execute the BEGIN, pattern-action, END blocks.
	return awk.loop (&r);
}

static int awk_main (int argc, qse_char_t* argv[])
{
	QSE::StdAwk awk;

	int ret = awk.open ();
	if (ret >= 0) ret = run_awk (awk);

	if (ret <= -1) 
	{
		QSE::StdAwk::loc_t loc = awk.getErrorLocation();
		print_error (loc, awk.getErrorMessage());
	}

	awk.close ();
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;

	qse_openstdsios ();
	{
	#if defined(_WIN32)
		char locale[100];
		UINT codepage = GetConsoleOutputCP();	
		if (codepage == CP_UTF8)
		{
			/*SetConsoleOUtputCP (CP_UTF8);*/
			qse_setdflcmgrbyid (QSE_CMGR_UTF8);
		}
		else
		{
			sprintf (locale, ".%u", (unsigned int)codepage);
			setlocale (LC_ALL, locale);
			qse_setdflcmgrbyid (QSE_CMGR_SLMB);
		}
	#else
		setlocale (LC_ALL, "");
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	#endif
	}

	x = qse_runmain (argc,argv,awk_main);
	qse_closestdsios ();
	return x;
}
