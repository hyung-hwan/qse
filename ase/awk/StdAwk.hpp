/*
 * $Id: StdAwk.hpp,v 1.11 2007/05/23 14:15:16 bacon Exp $
 */

#ifndef _ASE_AWK_STDAWK_HPP_
#define _ASE_AWK_STDAWK_HPP_

#include <ase/awk/Awk.hpp>

namespace ASE
{
	class StdAwk: public Awk
	{
	public:
		StdAwk ();

		int open ();

	protected:

		// builtin functions 
		int sin (Return* ret, const Argument* args, size_t nargs);
		int cos (Return* ret, const Argument* args, size_t nargs);
		int tan (Return* ret, const Argument* args, size_t nargs);
		int atan2 (Return* ret, const Argument* args, size_t nargs);
		int log (Return* ret, const Argument* args, size_t nargs);
		int exp (Return* ret, const Argument* args, size_t nargs);
		int sqrt (Return* ret, const Argument* args, size_t nargs);
		int fnint (Return* ret, const Argument* args, size_t nargs);
		int rand (Return* ret, const Argument* args, size_t nargs);
		int srand (Return* ret, const Argument* args, size_t nargs);
		int systime (Return* ret, const Argument* args, size_t nargs);
		int strftime (Return* ret, const Argument* args, size_t nargs);
		int strfgmtime (Return* ret, const Argument* args, size_t nargs);
		int system (Return* ret, const Argument* args, size_t nargs);

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

}

#endif


