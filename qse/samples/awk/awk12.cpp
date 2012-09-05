/*
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

#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/str.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <string>
#if defined(QSE_CHAR_IS_WCHAR)
typedef std::wstring String;
#else
typedef std::string String;
#endif

typedef QSE::StdAwk StdAwk;
typedef QSE::StdAwk::Run Run;
typedef QSE::StdAwk::Value Value;

class MyAwk: public StdAwk
{
public:
	//
	// this class overrides console methods to use
	// string buffers for console input and output.
	//
	MyAwk () { }
	~MyAwk () { close (); }

	void setInput (const char_t* instr)
	{
		this->input = instr;
		this->inptr = this->input.c_str();
		this->inend = inptr + this->input.length();
	}

	void clearOutput () { this->output.clear (); }
	const char_t* getOutput () { return this->output.c_str(); }

protected:
	String input; // console input buffer 
	const char_t* inptr;
	const char_t* inend;

	String output; // console output buffer
	
	int openConsole (Console& io) 
	{ 
		return 1; // return open-success
	}

	int closeConsole (Console& io) 
	{ 
		return 0; // return success
	} 

	int flushConsole (Console& io) 
	{
		// there is nothing to flush since a string buffer
		// is used for a console output. just return success.
		return 0; 
	} 
	int nextConsole (Console& io) 
	{ 
		// this stripped-down awk doesn't honor the nextfile statement
		// or the nextofile statement. just return success.
		return 0; 
	} 

	ssize_t readConsole (Console& io, char_t* data, size_t size) 
	{
		if (this->inptr >= this->inend) return 0; // EOF
		size_t x = qse_strxncpy (data, size, inptr, inend - inptr);
		this->inptr += x;
		return x;
	}

	ssize_t writeConsole (Console& io, const char_t* data, size_t size) 
	{
		try { this->output.append (data, size); }
		catch (...) 
		{ 
			((Run*)io)->setError (QSE_AWK_ENOMEM);
			return -1; 
		}
		return size;
	}
};

static void print_error (
	const MyAwk::loc_t& loc, const MyAwk::char_t* msg)
{
	if (loc.line > 0 || loc.colm > 0)
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s at LINE %lu COLUMN %lu\n"), msg, loc.line, loc.colm);
	else
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), msg);
	
}

static int run_awk (MyAwk& awk)
{
	// sample input string
	const qse_char_t* instr = QSE_T(
		"aardvark     555-5553     1200/300          B\n"
		"alpo-net     555-3412     2400/1200/300     A\n"
		"barfly       555-7685     1200/300          A\n"
		"bites        555-1675     2400/1200/300     A\n"
		"camelot      555-0542     300               C\n"
		"core         555-2912     1200/300          C\n"
		"fooey        555-1234     2400/1200/300     B\n"
		"foot         555-6699     1200/300          B\n"
		"macfoo       555-6480     1200/300          A\n"
		"sdace        555-3430     2400/1200/300     A\n"
		"sabafoo      555-2127     1200/300          C\n");

	// ARGV[0]
	if (awk.addArgument (QSE_T("awk12")) <= -1) return -1;

	// prepare a script to print the second and the first column
	MyAwk::SourceString in (QSE_T("{ print $2, $1; }")); 
	
	// parse the script.
	if (awk.parse (in, MyAwk::Source::NONE) == QSE_NULL) return -1;
	MyAwk::Value r;

	awk.setInput (instr); // locate the input string
	awk.clearOutput (); // clear the output string
	int x = awk.loop (&r); // execute the BEGIN, pattern-action, END blocks.

	if (x >= 0)
	{
		qse_printf (QSE_T("%s"), awk.getOutput()); // print the console output
		qse_printf (QSE_T("-----------------------------\n"));

		// prepare a string to print lines with A in the fourth column
		MyAwk::SourceString in2 (QSE_T("$4 == \"A\" { print $1; }")); 
		if (awk.parse (in2, MyAwk::Source::NONE) == QSE_NULL) return -1;

		awk.setInput (instr);
		awk.clearOutput ();

		int x = awk.loop (&r);

		if (x >= 0)
		{
			qse_printf (QSE_T("%s"), awk.getOutput());
			qse_printf (QSE_T("-----------------------------\n"));
		}
	}

	return x;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	MyAwk awk;

	int ret = awk.open ();
	if (ret >= 0) ret = run_awk (awk);

	if (ret <= -1) 
	{
		MyAwk::loc_t loc = awk.getErrorLocation();
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
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	return qse_runmain (argc,argv,awk_main);
}
