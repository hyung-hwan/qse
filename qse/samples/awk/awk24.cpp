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

	QSE::StdAwk::SourceString in (QSE_T(
		"function pa (x) {\n"
		"	@reset ret;\n"
		"	for (i in x) { print i, \"=>\", x[i]; ret += x[i]; }\n"
		"	return ret + FOO++;\n"
		"}\n"
		"function pb (x) {\n"
		"	@reset ret;\n"
		"	for (i in x) { ret[-i] = -x[i]; }\n"
		"	return ret;\n"
		"}"
	));

	// add a global variable 'FOO'
	int foo = awk.addGlobal (QSE_T("FOO"));
	if (foo <= -1) return -1;

	// parse the script and perform no deparsing
	run = awk.parse (in, QSE::StdAwk::Source::NONE);
	if (run == QSE_NULL) return -1;

	// set 'FOO' to 100000
	QSE::StdAwk::Value foov (run);
	if (foov.setInt (100000) <= -1) return -1;
	if (awk.setGlobal (foo, foov) <= -1) return -1;

	// prepare an indexed parameter 
	QSE::StdAwk::Value arg[1];
	for (int i = 1; i <= 5; i++)
	{
		if (arg[0].setIndexedInt (run, 
			QSE::StdAwk::Value::IntIndex(i), i*20) <= -1) return -1;
	}
	if (arg[0].setIndexedStr (run, 
		QSE::StdAwk::Value::IntIndex(99), QSE_T("-2345")) <= -1) return -1;

	QSE::StdAwk::Value dummy;
	if (dummy.setStr (run, QSE_T("4567")) <= -1) return -1;
	if (arg[0].setIndexedVal (run, 
		QSE::StdAwk::Value::IntIndex(999), dummy) <= -1) return -1;

	// prepare a variable to hold the return value
	QSE::StdAwk::Value r;

	// call the 'pa' function
	if (awk.call (QSE_T("pa"), &r, arg, 1) <= -1) return -1;

	// output the result in various types
	qse_printf (QSE_T("RESULT: (int) [%lld]\n"), (long long)r.toInt());
	qse_printf (QSE_T("        (flt)[%Lf]\n"), (long double)r.toFlt());
	qse_printf (QSE_T("        (str) [%s]\n"), r.toStr(QSE_NULL));

	// get the value of 'FOO'
	if (awk.getGlobal (foo, foov) <= -1) return -1;
	qse_printf (QSE_T("FOO:    (int) [%lld]\n"), (long long)foov.toInt());
	qse_printf (QSE_T("        (flt)[%Lf]\n"), (long double)foov.toFlt());
	qse_printf (QSE_T("        (str) [%s]\n"), foov.toStr(QSE_NULL));

	// call the 'pb' function
	if (awk.call (QSE_T("pb"), &r, arg, QSE_COUNTOF(arg)) <= -1) return -1;

	// output the returned map.
	QSE_ASSERT (r.isIndexed());

	QSE::StdAwk::Value::IndexIterator iter;
	QSE::StdAwk::Value::Index idx;
	QSE::StdAwk::Value v;

	qse_printf (QSE_T("RESULT:\n"));

	iter = r.getFirstIndex (&idx);
	while (iter != QSE::StdAwk::Value::IndexIterator::END)
	{
		if (r.getIndexed (idx, &v) <= -1) return -1;
		
		qse_printf (QSE_T("\t[%.*s]=>[%lld]\n"), 
			(int)idx.length(), idx.pointer(), 
			(long long)v.toInt()
		);
		
		iter = r.getNextIndex (&idx, iter);
	}

	return 0;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	QSE::StdAwk awk;

	int ret = awk.open();

	// allow returning a map from a function
	awk.setTrait (awk.getTrait() | QSE_AWK_FLEXMAP);

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
