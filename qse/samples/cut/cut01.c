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

#include <qse/cut/std.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

void print_usage (QSE_FILE* s, const qse_char_t* argv0)
{
	qse_fprintf (s, QSE_T("USAGE: %s selector [input-file [output-file]]\n"), argv0);
	qse_fprintf (s, QSE_T("Sample selectors:\n"));
	qse_fprintf (s, QSE_T("f1-3,5,D:, - select 1st to 3rd fields and 5th field delimited by :\n"));
	qse_fprintf (s, QSE_T("             delimit output with ,\n"));
	qse_fprintf (s, QSE_T("c7,10 - select 7th and 10th characters\n"));
}

int cut_main (int argc, qse_char_t* argv[])
{
	qse_cut_t* cut = QSE_NULL;
	qse_char_t* infile;
	qse_char_t* outfile;
	int ret = -1;

	if (argc <  2 || argc > 4)
	{
		print_usage (QSE_STDERR, argv[0]);
		return -1;
	}

	cut = qse_cut_openstd (0);
	if (cut == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open cut\n"));
		goto oops;
	}

	if (qse_cut_compstd (cut, argv[1]) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_cut_geterrmsg(cut));
		goto oops;
	}

	infile = (argc >= 3)? argv[2]: QSE_NULL;
	outfile = (argc >= 4)? argv[3]: QSE_NULL;

	if (qse_cut_execstd (cut, infile, outfile) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_cut_geterrmsg(cut));
		goto oops;
	}

oops:
	if (cut != QSE_NULL) qse_cut_close (cut);
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
	return qse_runmain (argc, argv, cut_main);
}

