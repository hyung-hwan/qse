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
	QSE::StdAwk::Run* run;

	const qse_char_t* script = QSE_T(
		"function add (a, b) { return a + b }\n"
		"function mul (a, b) { return a * b }\n"
		"function div (a, b) { return a / b }\n"
		"function sine (a) { return sin(a) }\n"
	);

	QSE::StdAwk::SourceString in (script);
	QSE::StdAwk::SourceFile out (QSE_T("awk06.out"));

	// parse the script and deparse it to awk06.out
	run = awk.parse (in, out);
	if (run == QSE_NULL) return -1;

	QSE::StdAwk::Value arg[2];
	if (arg[0].setInt (run, -20) <= -1) return -1;
	if (arg[1].setStr (run, QSE_T("51")) <= -1) return -1;

	// ret = add (-20, 51) 
	QSE::StdAwk::Value ret;
	if (awk.call (QSE_T("add"), &ret, arg, 2) <= -1) return -1;

	// ret = mul (ret, 51);
	arg[0] = ret;
	if (awk.call (QSE_T("mul"), &ret, arg, 2) <= -1) return -1;

	// ret = div (ret, 2);
	arg[0] = ret;
	if (arg[1].setFlt (run, 2) <= -1) return -1;
	if (awk.call (QSE_T("div"), &ret, arg, 2) <= -1) return -1;

	// output the result in various types
	qse_printf (QSE_T("RESULT: (int) [%lld]\n"), (long long)ret.toInt());
	qse_printf (QSE_T("        (flt) [%Lf]\n"), (long double)ret.toFlt());
	qse_printf (QSE_T("        (str) [%s]\n"), ret.toStr(QSE_NULL));

	// ret = sine (ret);
	arg[0] = ret;
	if (awk.call (QSE_T("sine"), &ret, arg, 1) <= -1) return -1;

	// output the result in various types
	qse_printf (QSE_T("RESULT: (int) [%lld]\n"), (long long)ret.toInt());
	qse_printf (QSE_T("        (flt) [%Lf]\n"), (long double)ret.toFlt());
	qse_printf (QSE_T("        (str) [%s]\n"), ret.toStr(QSE_NULL));

	return 0;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	QSE::StdAwk awk;

	int ret = awk.open();

	if (ret >= 0) ret = run_awk (awk);
	if (ret <= -1) 
	{
		QSE::StdAwk::loc_t loc = awk.getErrorLocation();
		print_error (loc, awk.getErrorMessage());
	}

	awk.close ();
	return -1;
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
