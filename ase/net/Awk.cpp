/*
 * $Id: Awk.cpp,v 1.9 2007/07/20 09:23:37 bacon Exp $
 */

#include "stdafx.h"
#include "Awk.hpp"

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <stdlib.h>
#include <math.h>

#include <msclr/auto_gcroot.h>

using System::Runtime::InteropServices::GCHandle;	

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
			if (io.getMode() == Source::READ)
			{
				if (wrapper->SourceInputStream == nullptr)
				{
					return -1;
				}

				if (!wrapper->SourceInputStream->CanRead)
				{
					wrapper->SourceInputStream->Close ();
					return -1;
				}

				System::IO::StreamReader^ reader = gcnew System::IO::StreamReader (wrapper->SourceInputStream);
				GCHandle gh = GCHandle::Alloc (reader);
				System::IntPtr^ ip = GCHandle::ToIntPtr(gh);
				io.setHandle (ip->ToPointer());
			}
			else
			{
				if (wrapper->SourceOutputStream == nullptr)
				{
					return -1;
				}

				if (!wrapper->SourceOutputStream->CanWrite)
				{
					wrapper->SourceOutputStream->Close ();
					return -1;
				}

				System::IO::StreamWriter^ writer = gcnew System::IO::StreamWriter (wrapper->SourceOutputStream);
				GCHandle gh = GCHandle::Alloc (writer);
				System::IntPtr^ ip = GCHandle::ToIntPtr(gh);
				io.setHandle (ip->ToPointer());
			}

			return 1;
		}

		int closeSource (Source& io) 
		{
			System::IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			if (io.getMode() == Source::READ)
			{
				System::IO::StreamReader^ reader = (System::IO::StreamReader^)gh.Target;
				reader->Close ();
			}
			else
			{
				System::IO::StreamWriter^ writer = (System::IO::StreamWriter^)gh.Target;
				writer->Close ();
			}
			
			gh.Free ();
			return 0;
		}

		ssize_t readSource (Source& io, char_t* buf, size_t len) 
		{
			System::IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			System::IO::StreamReader^ reader = (System::IO::StreamReader^)gh.Target;

			cli::array<char_t>^ b = gcnew cli::array<char_t>(len);
			int n = reader->Read (b, 0, len);
			for (int i = 0; i < n; i++) buf[i] =  b[i];

			return n;
		}

		ssize_t writeSource (Source& io, char_t* buf, size_t len)
		{
			System::IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			System::IO::StreamWriter^ writer = (System::IO::StreamWriter^)gh.Target;

			cli::array<char_t>^ b = gcnew cli::array<char_t>(len);
			for (int i = 0; i < (int)len; i++) b[i] =  buf[i];
			writer->Write (b, 0, len);

			return len;
		}

		int openPipe (Pipe& io) 
		{
			ASE::Net::Awk::Pipe^ nio = gcnew ASE::Net::Awk::Pipe (
				gcnew System::String (io.getName ()),
				(ASE::Net::Awk::Pipe::MODE)io.getMode());

			GCHandle gh = GCHandle::Alloc (nio);
			io.setHandle (GCHandle::ToIntPtr(gh).ToPointer());

			try { return wrapper->OpenPipe (nio); }
			catch (...) 
			{ 
				gh.Free ();
				io.setHandle (NULL);
				return -1; 
			
			}
		}

		int closePipe (Pipe& io) 
		{
			IntPtr ip ((void*)io.getHandle ());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->ClosePipe (
					(ASE::Net::Awk::Pipe^)gh.Target);
			}
			catch (...) { return -1; }
			finally { gh.Free (); }
		}

		ssize_t readPipe  (Pipe& io, char_t* buf, size_t len) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			
			try
			{
				b = gcnew cli::array<char_t> (len);
				int n = wrapper->ReadPipe (
					(ASE::Net::Awk::Pipe^)gh.Target, b, len);
				for (int i = 0; i < n; i++) buf[i] = b[i];
				return n;
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
		}

		ssize_t writePipe (Pipe& io, char_t* buf, size_t len) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			try
			{
				b = gcnew cli::array<char_t> (len);
				for (int i = 0; i < len; i++) b[i] = buf[i];
				return wrapper->WritePipe (
					(ASE::Net::Awk::Pipe^)gh.Target, b, len);
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
		}

		int flushPipe (Pipe& io) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->FlushPipe (
					(ASE::Net::Awk::Pipe^)gh.Target);
			}
			catch (...) { return -1; }
		}

		int openFile (File& io) 
		{	
			ASE::Net::Awk::File^ nio = gcnew ASE::Net::Awk::File (
				gcnew System::String (io.getName ()),
				(ASE::Net::Awk::File::MODE)io.getMode());

			GCHandle gh = GCHandle::Alloc (nio);
			io.setHandle (GCHandle::ToIntPtr(gh).ToPointer());

			try { return wrapper->OpenFile (nio); }
			catch (...)
			{
				gh.Free ();
				io.setHandle (NULL);
				return -1;
			}
		}

		int closeFile (File& io) 
		{
			IntPtr ip ((void*)io.getHandle ());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->CloseFile (
					(ASE::Net::Awk::File^)gh.Target);
			}
			catch (...) { return -1; }
			finally { gh.Free (); }
		}

		ssize_t readFile (File& io, char_t* buf, size_t len) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			try
			{
				b = gcnew cli::array<char_t> (len);
				int n = wrapper->ReadFile (
					(ASE::Net::Awk::File^)gh.Target, b, len);
				for (int i = 0; i < n; i++) buf[i] = b[i];
				return n;
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
		}

		ssize_t writeFile (File& io, char_t* buf, size_t len) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			try
			{
				b = gcnew cli::array<char_t> (len);
				for (int i = 0; i < len; i++) b[i] = buf[i];
				return wrapper->WriteFile (
					(ASE::Net::Awk::File^)gh.Target, b, len);
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
		}

		int flushFile (File& io) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->FlushFile (
					(ASE::Net::Awk::File^)gh.Target);
			}
			catch (...) { return -1; }
		}

		int openConsole (Console& io) 
		{	
			ASE::Net::Awk::Console^ nio = gcnew ASE::Net::Awk::Console (
				gcnew System::String (io.getName ()),
				(ASE::Net::Awk::Console::MODE)io.getMode());

			GCHandle gh = GCHandle::Alloc (nio);
			io.setHandle (GCHandle::ToIntPtr(gh).ToPointer());

			try { return wrapper->OpenConsole (nio); }
			catch (...)
			{
				gh.Free ();
				io.setHandle (NULL);
				return -1;
			}
		}

		int closeConsole (Console& io) 
		{
			IntPtr ip ((void*)io.getHandle ());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->CloseConsole (
					(ASE::Net::Awk::Console^)gh.Target);
			}
			catch (...) { return -1; }
			finally { gh.Free (); }
		}

		ssize_t readConsole (Console& io, char_t* buf, size_t len) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			try
			{
				b = gcnew cli::array<char_t> (len);
				int n = wrapper->ReadConsole (
					(ASE::Net::Awk::Console^)gh.Target, b, len);
				for (int i = 0; i < n; i++) buf[i] = b[i];
				return n;
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
		}

		ssize_t writeConsole (Console& io, char_t* buf, size_t len) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			try
			{
				b = gcnew cli::array<char_t> (len);
				for (int i = 0; i < len; i++) b[i] = buf[i];
				return wrapper->WriteConsole (
					(ASE::Net::Awk::Console^)gh.Target, b, len);
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
		}

		int flushConsole (Console& io) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->FlushConsole (
					(ASE::Net::Awk::Console^)gh.Target);
			}
			catch (...) { return -1; }
		}

		int nextConsole (Console& io) 
		{
			IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->NextConsole (
					(ASE::Net::Awk::Console^)gh.Target);
			}
			catch (...) { return -1; }
		}

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
		msclr::auto_gcroot<ASE::Net::Awk^> wrapper;
	};

	namespace Net
	{
		Awk::Awk ()
		{
			awk = new ASE::StubAwk (this);
			if (awk->open () == -1)
			{
				// TODO:...
				//throw new AwkException ("cannot open awk");
			}
		}

		Awk::~Awk ()
		{
			Close ();
			delete awk;
		}

		void Awk::Close ()
		{
			awk->close ();
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
			cli::pin_ptr<const wchar_t> nptr = PtrToStringChars(name);
			return awk->deleteFunction (nptr) == 0;
		}

		int Awk::DispatchFunction (System::String^ name)
		{
			return 0;
		}

	}
}

