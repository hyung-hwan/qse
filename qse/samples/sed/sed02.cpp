/**
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include <qse/sed/StdSed.hpp>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <iostream>

#include <locale.h>
#if defined(_WIN32)
#	include <stdio.h>
#    include <windows.h>
#endif


#ifdef QSE_CHAR_IS_MCHAR
#	define xcout std::cout
#else
#	define xcout std::wcout
#endif

int sed_main (int argc, qse_char_t* argv[])
{
	if (argc <  2 || argc > 4)
	{
		xcout << QSE_T("USAGE: ") << argv[0] <<
		         QSE_T(" command-string [input-file [output-file]]") << std::endl;
		return -1;
	}

	QSE::StdSed sed;

	if (sed.open () == -1)
	{
		xcout << QSE_T("ERR: cannot open") << std::endl;
		return -1;
	}


	QSE::StdSed::StringStream sstream (argv[1]);
	if (sed.compile (sstream) == -1)
	{
		xcout << QSE_T("ERR: cannot compile - ") << sed.getErrorMessage() << std::endl;
		sed.close ();
		return -1;
	}

	qse_char_t* infile = (argc >= 3)? argv[2]: QSE_NULL;
	qse_char_t* outfile = (argc >= 4)? argv[3]: QSE_NULL;
	QSE::StdSed::FileStream fstream (infile, outfile);

	if (sed.execute (fstream) == -1)
	{
		xcout << QSE_T("ERR: cannot execute - ") << sed.getErrorMessage() << std::endl;
		sed.close ();
		return -1;
	}

	sed.close ();
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
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
	return qse_runmain (argc, argv, sed_main);
}
