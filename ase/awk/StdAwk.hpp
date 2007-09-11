/*
 * $Id: StdAwk.hpp,v 1.14 2007/09/07 05:40:16 bacon Exp $
 */

#ifndef _ASE_AWK_STDAWK_HPP_
#define _ASE_AWK_STDAWK_HPP_

#include <ase/awk/Awk.hpp>

/////////////////////////////////
ASE_BEGIN_NAMESPACE(ASE)
/////////////////////////////////

class StdAwk: public Awk
{
public:
	StdAwk ();
	int open ();

protected:

	// builtin functions 
	int sin (Return* ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len);
	int cos (Return* ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len);
	int tan (Return* ret, const Argument* args, size_t nargs, 
		const char_t* name, size_t len);
	int atan (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int atan2 (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int log (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int exp (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int sqrt (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int fnint (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int rand (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int srand (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int systime (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int strftime (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int strfgmtime (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);
	int system (Return* ret, const Argument* args, size_t nargs,
		const char_t* name, size_t len);

	// pipe io handlers 
	int openPipe (Pipe& io);
	int closePipe (Pipe& io);
	ssize_t readPipe  (Pipe& io, char_t* buf, size_t len);
	ssize_t writePipe (Pipe& io, char_t* buf, size_t len);
	int flushPipe (Pipe& io);

	// file io handlers 
	int openFile (File& io);
	int closeFile (File& io);
	ssize_t readFile (File& io, char_t* buf, size_t len);
	ssize_t writeFile (File& io, char_t* buf, size_t len);
	int flushFile (File& io);

	// primitive handlers 
	void* allocMem   (size_t n);
	void* reallocMem (void* ptr, size_t n);
	void  freeMem    (void* ptr);

	bool_t isUpper  (cint_t c);
	bool_t isLower  (cint_t c);
	bool_t isAlpha  (cint_t c);
	bool_t isDigit  (cint_t c);
	bool_t isXdigit (cint_t c);
	bool_t isAlnum  (cint_t c);
	bool_t isSpace  (cint_t c);
	bool_t isPrint  (cint_t c);
	bool_t isGraph  (cint_t c);
	bool_t isCntrl  (cint_t c);
	bool_t isPunct  (cint_t c);
	cint_t toUpper  (cint_t c);
	cint_t toLower  (cint_t c);

	real_t pow (real_t x, real_t y);
	int    vsprintf (char_t* buf, size_t size,
	                 const char_t* fmt, va_list arg);
	void   vdprintf (const char_t* fmt, va_list arg);

protected:
	unsigned int seed; 
};

/////////////////////////////////
ASE_END_NAMESPACE(ASE)
/////////////////////////////////

#endif


