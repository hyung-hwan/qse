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

static void print_error (unsigned long line, const qse_char_t* msg)
{
	if (line > 0)
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s at LINE %lu\n"), msg, line);
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
	);

	QSE::StdAwk::SourceString in (script);
	QSE::StdAwk::SourceFile out (QSE_T("awk06.out"));

	// parse the script and deparse it to awk06.out
	run = awk.parse (&in, &out);
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
	if (arg[1].setReal (run, 2) <= -1) return -1;
	if (awk.call (QSE_T("div"), &ret, arg, 2) <= -1) return -1;

	// output the result in various types
	qse_printf (QSE_T("RESULT: (int) [%lld]\n"), (long long)ret.toInt());
	qse_printf (QSE_T("        (real) [%Lf]\n"), (long double)ret.toReal());
	qse_printf (QSE_T("        (str) [%s]\n"), ret.toStr(QSE_NULL));

	return 0;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	QSE::StdAwk awk;

	int ret = awk.open();

	if (ret >= 0) ret = run_awk (awk);
	if (ret <= -1) print_error (awk.getErrorLine(), awk.getErrorMessage());

	awk.close ();
	return -1;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc,argv,awk_main);
}
