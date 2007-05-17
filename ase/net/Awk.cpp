/*
* $Id: Awk.cpp,v 1.1 2007/05/15 08:29:30 bacon Exp $
*/

#include "stdafx.h"
#include "Awk.hpp"

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <stdlib.h>
#include <math.h>

#include <msclr/auto_gcroot.h>


namespace ASE
{

	class StubAwk: public Awk
	{
	public:	
		StubAwk (NET::Awk^ wrapper): wrapper(wrapper)
		{		
		}

		int openSource (Source& io) 
		{ 
			NET::Awk::Source^ nio = gcnew NET::Awk::Source ();
			int n = wrapper->OpenSource (nio);
			// TODO: put nio back to io.
			return n;
		}

		int closeSource (Source& io) 
		{
			return 0;
		}

		ssize_t readSource (Source& io, char_t* buf, size_t len) 
		{
			return 0;
		}

		ssize_t writeSource (Source& io, char_t* buf, size_t len)
		{
			return 0;
		}

		int openPipe (Pipe& io) {return 0; }
		int closePipe (Pipe& io) {return 0; }
		ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) {return 0; }
		ssize_t writePipe (Pipe& io, char_t* buf, size_t len) {return 0; }
		int flushPipe (Pipe& io) {return 0; }

		int openFile (File& io) {return 0; }
		int closeFile (File& io) {return 0; }
		ssize_t readFile (File& io, char_t* buf, size_t len) {return 0; }
		ssize_t writeFile (File& io, char_t* buf, size_t len) {return 0; }
		int flushFile (File& io) {return 0; }

		int openConsole (Console& io) {return 0; }
		int closeConsole (Console& io) {return 0; }
		ssize_t readConsole (Console& io, char_t* buf, size_t len) {return 0; }
		ssize_t writeConsole (Console& io, char_t* buf, size_t len) {return 0; }
		int flushConsole (Console& io) {return 0; }
		int nextConsole (Console& io) {return 0; }

		// primitive operations 
		void* allocMem   (size_t n) { return ::malloc (n); }
		void* reallocMem (void* ptr, size_t n) { return ::realloc (ptr, n); }
		void  freeMem    (void* ptr) { ::free (ptr); }

		bool_t isUpper  (cint_t c) { return ase_isupper (c); }
		bool_t isLower  (cint_t c) { return ase_islower (c); }
		bool_t isAlpha  (cint_t c) { return ase_isalpha (c); }
		bool_t isDigit  (cint_t c) { return ase_isdigit (c); }
		bool_t isXdigit (cint_t c) { return ase_isxdigit (c); }
		bool_t isAlnum  (cint_t c) { return ase_isalnum (c); }
		bool_t isSpace  (cint_t c) { return ase_isspace (c); }
		bool_t isPrint  (cint_t c) { return ase_isprint (c); }
		bool_t isGraph  (cint_t c) { return ase_isgraph (c); }
		bool_t isCntrl  (cint_t c) { return ase_iscntrl (c); }
		bool_t isPunct  (cint_t c) { return ase_ispunct (c); }
		cint_t toUpper  (cint_t c) { return ase_toupper (c); }
		cint_t toLower  (cint_t c) { return ase_tolower (c); }

		real_t pow (real_t x, real_t y) 
		{ 
			return ::pow (x, y); 
		}

		int vsprintf (char_t* buf, size_t size, const char_t* fmt, va_list arg) 
		{
			return ase_vsprintf (buf, size, fmt, arg);
		}

		void vdprintf (const char_t* fmt, va_list arg) 
		{
			ase_vfprintf (stderr, fmt, arg);
		}

	private:
		msclr::auto_gcroot<NET::Awk^>  wrapper;
	};

	namespace NET
	{

		Awk::Awk ()
		{
			awk = new ASE::StubAwk (this);
		}

		Awk::~Awk ()
		{
			delete awk;
		}

		bool Awk::Parse ()
		{
			return awk->parse () == 0;
		}

		bool Awk::Run ()
		{
			return awk->run () == 0;
		}

		bool Awk::AddFunction (System::String^ name, int minArgs, int maxArgs, FunctionHandler^ handler)
		{
			return false;
		}

		bool Awk::DeleteFunction (System::String^ name)
		{
			return false;
		}
	}
}
