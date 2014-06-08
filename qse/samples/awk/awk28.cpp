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
#include <qse/cmn/sio.h>
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

class MyConsoleHandler: public StdAwk::Console::Handler
{
public:
	// this class defines a console handler that can be
	// registered into an awk object.

	void setInput (const StdAwk::char_t* instr)
	{
		this->input = instr;
		this->inptr = this->input.c_str();
		this->inend = inptr + this->input.length();
	}

	const StdAwk::char_t* getOutput () { return this->output.c_str(); }

protected:
	String input; // console input buffer 
	const StdAwk::char_t* inptr;
	const StdAwk::char_t* inend;

	String output; // console output buffer
	
	int open (StdAwk::Console& io) 
	{ 
		if (io.getMode() == StdAwk::Console::READ)
		{
			this->inptr = this->input.c_str();
			this->inend = inptr + this->input.length();
		}
		else
		{
			this->output.clear ();
		}

		return 1; // return open-success
	}

	int close (StdAwk::Console& io) 
	{ 
		return 0; // return success
	} 

	int flush (StdAwk::Console& io) 
	{
		// there is nothing to flush since a string buffer
		// is used for a console output. just return success.
		return 0; 
	} 
	int next (StdAwk::Console& io) 
	{ 
		// this stripped-down awk doesn't honor the nextfile statement
		// or the nextofile statement. just return failure.
		return -1; 
	} 

	StdAwk::ssize_t read (StdAwk::Console& io, StdAwk::char_t* data, size_t size) 
	{
		if (this->inptr >= this->inend) return 0; // EOF
		size_t x = qse_strxncpy (data, size, inptr, inend - inptr);
		this->inptr += x;
		return x;
	}

	StdAwk::ssize_t write (StdAwk::Console& io, const StdAwk::char_t* data, size_t size) 
	{
		try { this->output.append (data, size); }
		catch (...) 
		{ 
			((StdAwk::Run*)io)->setError (QSE_AWK_ENOMEM);
			return -1; 
		}
		return size;
	}
};

static void print_error (
	const StdAwk::loc_t& loc, const StdAwk::char_t* msg)
{
	if (loc.line > 0 || loc.colm > 0)
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s at LINE %lu COLUMN %lu\n"), msg, loc.line, loc.colm);
	else
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), msg);
	
}

static int run_awk (StdAwk& awk)
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

	const qse_char_t* instr2 = QSE_T(
		"aardvark     555-5553     1200/300          A\n"
		"alpo-net     555-3412     2400/1200/300     B\n"
		"barfly       555-7685     1200/300          C\n"
		"bites        555-1675     2400/1200/300     A\n"
		"camelot      555-0542     300               C\n"
		"core         555-2912     1200/300          B\n"
		"fooey        555-1234     2400/1200/300     A\n"
		"foot         555-6699     1200/300          A\n"
		"macfoo       555-6480     1200/300          B\n"
		"sdace        555-3430     2400/1200/300     B\n"
		"sabafoo      555-2127     1200/300          A\n");

	// ARGV[0]
	if (awk.addArgument (QSE_T("awk14")) <= -1) return -1;

	// prepare a string to print lines with A in the fourth column
	StdAwk::SourceString in (QSE_T("$4 == \"A\" { print $2, $1, $3; }")); 
	
	// parse the script.
	if (awk.parse (in, StdAwk::Source::NONE) == QSE_NULL) return -1;
	StdAwk::Value r;

	MyConsoleHandler* mch = (MyConsoleHandler*)awk.getConsoleHandler();

	mch->setInput (instr); // locate the input string
	int x = awk.loop (&r); // execute the BEGIN, pattern-action, END blocks.

	if (x >= 0)
	{
		qse_printf (QSE_T("%s"), mch->getOutput()); // print the console output
		qse_printf (QSE_T("-----------------------------\n"));

		mch->setInput (instr2);

		// reset the runtime context so that the next loop() method
		// is performed over a new console stream.
		if (awk.resetRunContext() == QSE_NULL) return -1;

		int x = awk.loop (&r);

		if (x >= 0)
		{
			qse_printf (QSE_T("%s"), mch->getOutput());
			qse_printf (QSE_T("-----------------------------\n"));
		}
	}

	return x;
}

static int awk_main (int argc, qse_char_t* argv[])
{
	
	MyConsoleHandler mch;
	StdAwk awk;

	int ret = awk.open ();
	if (ret >= 0) 
	{
		awk.setConsoleHandler (&mch);
		ret = run_awk (awk);
	}

	if (ret <= -1) 
	{
		StdAwk::loc_t loc = awk.getErrorLocation();
		print_error (loc, awk.getErrorMessage());
	}

	awk.close ();
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;
	qse_openstdsios ();

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
	}

	x = qse_runmain (argc,argv,awk_main);
	qse_closestdsios ();
	return x;
}
