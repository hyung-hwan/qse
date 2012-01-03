/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/stdio.h>
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
		"	reset ret;\n"
		"	for (i in x) { print i, \"=>\", x[i]; ret += x[i]; }\n"
		"	return ret + FOO++;\n"
		"}\n"
		"function pb (x) {\n"
		"	reset ret;\n"
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

	// allow returning a map from a function and enable 'reset'
	awk.setOption (
		awk.getOption() | 
		QSE_AWK_MAPTOVAR |
		QSE_AWK_RESET);

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
#if defined(_WIN32)
     char locale[100];
	UINT codepage = GetConsoleOutputCP();	
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgr (qse_utf8cmgr);
	}
	else
	{
     	sprintf (locale, ".%u", (unsigned int)codepage);
     	setlocale (LC_ALL, locale);
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
     setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif
	return qse_runmain (argc,argv,awk_main);
}
