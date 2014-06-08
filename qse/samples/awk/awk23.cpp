#include <qse/awk/StdAwk.hpp>
#include <qse/cmn/sio.h>
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

	const char_t* getOutput () { return this->output.c_str(); }

protected:
	String input; // console input buffer 
	const char_t* inptr;
	const char_t* inend;

	String output; // console output buffer
	
	int openConsole (Console& io) 
	{ 
		if (io.getMode() == Console::READ)
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
		// or the nextofile statement. just return failure.
		return -1; 
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
	if (awk.addArgument (QSE_T("awk23")) <= -1) return -1;

	// prepare a string to print lines with A in the fourth column
	MyAwk::SourceString in (QSE_T("$4 == \"A\" { print $2, $1, $3; }")); 
	
	// parse the script.
	if (awk.parse (in, MyAwk::Source::NONE) == QSE_NULL) return -1;
	MyAwk::Value r;

	awk.setInput (instr); // locate the input string
	int x = awk.loop (&r); // execute the BEGIN, pattern-action, END blocks.

	if (x >= 0)
	{
		qse_printf (QSE_T("%s"), awk.getOutput()); // print the console output
		qse_printf (QSE_T("-----------------------------\n"));

		awk.setInput (instr2);

		// reset the runtime context so that the next loop() method
		// is performed over a new console stream.
		if (awk.resetRunContext() == QSE_NULL) return -1;

		int x = awk.loop (&r);

		if (x >= 0)
		{
			qse_printf (QSE_T("%s"), awk.getOutput());
			qse_printf (QSE_T("-----------------------------\n"));
		}
	}

	return x;
}

int main (int argc, char* argv[])
{
	MyAwk awk;

	qse_openstdsios ();
	int ret = awk.open ();
	if (ret >= 0) ret = run_awk (awk);

	if (ret <= -1) 
	{
		MyAwk::loc_t loc = awk.getErrorLocation();
		print_error (loc, awk.getErrorMessage());
	}

	awk.close ();
	qse_closestdsios ();
	return ret;
}

