#include <qse/sed/StdSed.hpp>
#include <qse/cmn/main.h>
#include <qse/cmn/sio.h>
#include <iostream>
#include "sed00.h"

#if defined(QSE_CHAR_IS_MCHAR)
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
	init_sed_sample_locale ();
	return qse_runmain (argc, argv, sed_main);
}
