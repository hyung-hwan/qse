/*
 * $Id: Awk.cpp 341 2008-08-20 10:58:19Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

static void print_error (const qse_awk_loc_t& loc, const qse_char_t* msg)
{
	if (loc.lin > 0 || loc.col > 0)
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s at LINE %lu COLUMN %lu\n"), msg, loc.lin, loc.col);
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

	// prepare a variable to hold the return value
	QSE::StdAwk::Value r;

	// call the 'pa' function
	if (awk.call (QSE_T("pa"), &r, arg, 1) <= -1) return -1;

	// output the result in various types
	qse_printf (QSE_T("RESULT: (int) [%lld]\n"), (long long)r.toInt());
	qse_printf (QSE_T("        (real)[%Lf]\n"), (long double)r.toReal());
	qse_printf (QSE_T("        (str) [%s]\n"), r.toStr(QSE_NULL));

	// get the value of 'FOO'
	if (awk.getGlobal (foo, foov) <= -1) return -1;
	qse_printf (QSE_T("FOO:    (int) [%lld]\n"), (long long)foov.toInt());
	qse_printf (QSE_T("        (real)[%Lf]\n"), (long double)foov.toReal());
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
			(int)idx.len, idx.ptr, (long long)v.toInt());
		
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
		QSE::StdAwk::OPT_MAPTOVAR | 
		QSE::StdAwk::OPT_RESET);

	if (ret >= 0) ret = run_awk (awk);
	if (ret <= -1) 
	{
		qse_awk_loc_t loc = awk.getErrorLocation();
		print_error (loc, awk.getErrorMessage());
	}

	awk.close ();
	return -1;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc,argv,awk_main);
}
