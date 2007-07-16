/*
* $Id: Awk.cpp,v 1.3 2007/07/15 16:31:59 bacon Exp $
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
		StubAwk (Net::Awk^ wrapper): wrapper(wrapper)
		{		
		}

		int stubFunctionHandler (
			Return* ret, const Argument* args, size_t nargs, 
			const char_t* name, size_t len)
		{
			System::String^ nm = gcnew System::String (name, 0, len);
			return wrapper->DispatchFunction (nm);
		}

		int openSource (Source& io) 
		{ 
			/*
			Net::Awk::Source^ nio = gcnew Net::Awk::Source ();
			int n = wrapper->OpenSource (nio);
			return n;
			*/
			if (io.getMode() == Source::READ)
			{
				//wrapper->SourceInputStream->BeginRead ();
			}
			else
			{
				//wrapper->SourceOutputStream->BeginWrite ();
			}
			return 1;
		}

		int closeSource (Source& io) 
		{
			//System::IO::Stream^ stream = io.getHandle();
			//stream->Close ();
			return 0;
		}

		ssize_t readSource (Source& io, char_t* buf, size_t len) 
		{
			//System::IO::Stream^ stream = io.getHandle();
			return 0;
		}

		ssize_t writeSource (Source& io, char_t* buf, size_t len)
		{
			//System::IO::Stream^ stream = io.getHandle();
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
		msclr::auto_gcroot<Net::Awk^> wrapper;
	};

	namespace Net
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

		bool Awk::AddFunction (
			System::String^ name, int minArgs, int maxArgs, 
			FunctionHandler^ handler)
		{
			cli::pin_ptr<const wchar_t> nptr = PtrToStringChars(name);
			return awk->addFunction (nptr, minArgs, maxArgs, 
				(ASE::Awk::FunctionHandler)&StubAwk::stubFunctionHandler) == 0;
		}

		bool Awk::DeleteFunction (System::String^ name)
		{
			pin_ptr<const wchar_t> nptr = PtrToStringChars(name);
			return awk->deleteFunction (nptr) == 0;
		}

		int Awk::DispatchFunction (System::String^ name)
		{
			return 0;
		}
	}
}

