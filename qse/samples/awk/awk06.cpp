/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

static void print_error (
	const QSE::StdAwk::loc_t& loc, const QSE::StdAwk::char_t* msg)
{
	if (loc.lin > 0 || loc.col > 0)
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s at LINE %lu COLUMN %lu\n"), msg, loc.lin, loc.col);
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
	if (arg[1].setReal (run, 2) <= -1) return -1;
	if (awk.call (QSE_T("div"), &ret, arg, 2) <= -1) return -1;

	// output the result in various types
	qse_printf (QSE_T("RESULT: (int) [%lld]\n"), (long long)ret.toInt());
	qse_printf (QSE_T("        (real) [%Lf]\n"), (long double)ret.toReal());
	qse_printf (QSE_T("        (str) [%s]\n"), ret.toStr(QSE_NULL));

	// ret = sine (ret);
	arg[0] = ret;
	if (awk.call (QSE_T("sine"), &ret, arg, 1) <= -1) return -1;

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
	return qse_runmain (argc,argv,awk_main);
}
