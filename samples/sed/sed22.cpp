#include <qse/sed/StdSed.hpp>
#include <qse/cmn/main.h>
#include <qse/Exception.hpp>
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
	QSE_EXCEPTION(Error);

	MySed () { if (this->open() <= -1) QSE_THROW_WITH_MSG (Error, QSE_T("cannot open sed")); }
	~MySed () { this->close (); }

	void compile (const char_t* sptr)
	{
		QSE::StdSed::StringStream stream(sptr);
		if (QSE::StdSed::compile (stream) <= -1)
			QSE_THROW_WITH_MSG(Error, this->getErrorMessage());
	}

	void execute (Stream& stream)
	{
		if (QSE::StdSed::execute (stream) <= -1)
			QSE_THROW_WITH_MSG(Error, this->getErrorMessage());
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
		xcout << QSE_T("ERROR: ") << QSE_EXCEPTION_MSG(err) << std::endl;
		return -1;
	}

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;
	init_sed_sample_locale ();
	x = qse_run_main (argc, argv, sed_main);
	return x;
}
