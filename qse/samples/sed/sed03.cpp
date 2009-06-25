/**
 * $Id$
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

#include <qse/sed/StdSed.hpp>
#include <qse/cmn/main.h>

#include <string>
#include <iostream>

#ifdef QSE_CHAR_IS_MCHAR
#	define xcout std::cout
#else
#	define xcout std::wcout
#endif

typedef std::basic_string<QSE::StdSed::char_t> xstring;
typedef QSE::StdSed StdSed; // added for doxygen cross-reference

//
// The MySed class provides a handier interface to QSE::StdSed by 
// reimplementing console I/O functions, combining compile() and execute(),
// and utilizing exception handling.
//
class MySed: protected QSE::StdSed
{
public:
	
	class Error
	{ 
	public:
		Error (const char_t* msg) throw (): msg (msg) { }
		const char_t* getMessage() const throw() { return msg; }
	protected:
		const char_t* msg;
	};

	MySed () 
	{ 
		if (QSE::StdSed::open() == -1)
			throw Error (QSE_T("cannot open")); 
	}

	~MySed () 
	{ 
		QSE::StdSed::close (); 
	}
	
	void run (const char_t* cmds, const char_t* in, xstring& out)
	{
		// remember an input string and an output string
		this->in = in; this->out = &out;

		// compile source commands and execute compiled commands.
		if (QSE::StdSed::compile (cmds) <= -1 || 
		    QSE::StdSed::execute () <= -1)
		{
			throw Error (QSE::StdSed::getErrorMessage());
		}
	}

protected:
	// override console I/O functions to perform I/O over strings.

	int openConsole (Console& io) 
	{ 
		if (io.getMode() == Console::WRITE) out->clear();
		return 1;
	}

	int closeConsole (Console& io) 
	{
		return 0;
	}

	ssize_t readConsole (Console& io, char_t* buf, size_t len)
	{
		ssize_t n = qse_strxcpy (buf, len, in);
		in += n; return n;
	}

	ssize_t writeConsole (Console& io, const char_t* buf, size_t len)
	{
		try { out->append (buf, len); return len; }
		catch (...) { QSE::Sed::setError (QSE_SED_ENOMEM); throw; }
	}

protected:
	const char_t* in;
	xstring* out;
};

int sed_main (int argc, qse_char_t* argv[])
{
	try
	{
		MySed sed;
		xstring out;

		sed.run (
			QSE_T("y/ABC/abc/;s/abc/def/g"),
			QSE_T("ABCDEFabcdef"), out);
		xcout << QSE_T("OUTPUT: ") << out << std::endl;
	}
	catch (MySed::Error& err)
	{
		xcout << QSE_T("ERROR: ") << err.getMessage() << std::endl;
		return -1;
	}

	return 0;
}

int qse_main (int argc, char* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}
