#include <qse/awk/StdAwk.hpp>
#include <iostream>

#if defined(QSE_CHAR_IS_MCHAR)
#       define xcout std::cout
#else
#       define xcout std::wcout
#endif

struct MyAwk: public QSE::StdAwk 
{ 
	~MyAwk () { QSE::StdAwk::close (); } 
};

int main (int argc, char* argv[])
{
	MyAwk awk;
	MyAwk::Value r;
	MyAwk::SourceString in (QSE_T("BEGIN { print \"hello, world\" }"));

	if (awk.open () <= -1 || // initialize an awk object
	    awk.addArgument (QSE_T("awk21")) <= -1 || // set ARGV[0]
	    awk.parse (in, MyAwk::Source::NONE) == QSE_NULL || // parse the script
	    awk.loop (&r) <= -1) goto oops;

	// no need to close anything since the destructor performs it
	return 0;

oops:
	xcout << QSE_T("ERR: ") << awk.getErrorMessage() << std::endl; \
	return -1; \
}
