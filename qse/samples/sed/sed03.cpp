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
#include <qse/cmn/sio.h>
#include <iostream>

#include <locale.h>
#if defined(_WIN32)
#	include <stdio.h>
#	include <windows.h>
#endif


#ifdef QSE_CHAR_IS_MCHAR
#	define xcout std::cout
#else
#	define xcout std::wcout
#endif

//
// The MySed class simplifies QSE::StdSed by utilizing exception handling.
//
class MySed: protected QSE::StdSed
{
public:
	class Error
	{ 
	public:
		Error (const char_t* msg) throw (): msg (msg) {}
		const char_t* getMessage() const throw() { return msg; }
	protected:
		const char_t* msg;
	};

	MySed () { if (open() <= -1) throw Error (QSE_T("cannot open")); }
	~MySed () { close (); }

	void compile (const char_t* sptr)
	{
		QSE::StdSed::StringStream stream(sptr);
		if (QSE::StdSed::compile (stream) <= -1)
			throw Error (getErrorMessage());
	}

	void execute (Stream& stream)
	{
		if (QSE::StdSed::execute (stream) <= -1)
			throw Error (getErrorMessage());
	}
};

int sed_main (int argc, qse_char_t* argv[])
{
	try
	{
		MySed sed;

		sed.compile (QSE_T("y/ABC/abc/;s/abc/def/g"));

		QSE::StdSed::StringStream stream (QSE_T("ABCDEFabcdef"));
		sed.execute (stream);

		xcout << QSE_T("INPUT: ") << stream.getInput() << std::endl;
		xcout << QSE_T("OUTPUT: ") << stream.getOutput() << std::endl;
	}
	catch (MySed::Error& err)
	{
		xcout << QSE_T("ERROR: ") << err.getMessage() << std::endl;
		return -1;
	}

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
		qse_setdflcmgr (qse_utf8cmgr);
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
