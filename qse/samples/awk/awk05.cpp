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
	// ARGV[0]
	if (awk.addArgument (QSE_T("awk05")) <= -1) return -1;

	// ARGV[1] and/or the first console input file
	if (awk.addArgument (QSE_T("awk05.cpp")) <= -1) return -1;

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
	return qse_runmain (argc,argv,awk_main);
}
