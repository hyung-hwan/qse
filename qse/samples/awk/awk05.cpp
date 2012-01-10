/*
 * $Id: Awk.cpp 341 2008-08-20 10:58:19Z baconevi $
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
