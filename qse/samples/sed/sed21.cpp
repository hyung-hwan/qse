#include <qse/sed/StdSed.hpp>
#include <qse/cmn/main.h>
#include <iostream>
#include "sed00.h"

#if defined(QSE_CHAR_IS_MCHAR)
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

	if (sed.open () <= -1)
	{
		xcout << QSE_T("ERR: cannot open") << std::endl;
		return -1;
	}


	QSE::StdSed::StringStream sstream (argv[1]);
	if (sed.compile (sstream) <= -1)
	{
		xcout << QSE_T("ERR: cannot compile - ") << sed.getErrorMessage() << std::endl;
		sed.close ();
		return -1;
	}

	qse_char_t* infile = (argc >= 3)? argv[2]: QSE_NULL;
	qse_char_t* outfile = (argc >= 4)? argv[3]: QSE_NULL;
	QSE::StdSed::FileStream fstream (infile, outfile);

	if (sed.execute (fstream) <= -1)
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
	int x;
	init_sed_sample_locale ();
	x = qse_runmain (argc, argv, sed_main);
	return x;
}
