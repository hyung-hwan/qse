/*
 * $Id: Awk.cpp,v 1.18 2007/08/21 14:24:37 bacon Exp $
 */

#include "stdafx.h"
#include "Awk.hpp"

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <stdlib.h>
#include <math.h>

#include <msclr/auto_gcroot.h>
#include <msclr/gcroot.h>

using System::Runtime::InteropServices::GCHandle;	

extern "C" void outputxxx (const wchar_t* x);

namespace ASE
{

	class MojoAwk: protected Awk
	{
	public:
		MojoAwk (): wrapper(nullptr)
		{	
		}

		~MojoAwk ()
		{
		}

		int open (ASE::Net::Awk^ wrapper)
		{
			this->wrapper = wrapper;
			int n = Awk::open ();
			this->wrapper = nullptr;
			return n;
		}

		void close (ASE::Net::Awk^ wrapper)
		{
			this->wrapper = wrapper;
			Awk::close ();
			this->wrapper = nullptr;	
		}

		int getOption (ASE::Net::Awk^ wrapper) const
		{
			this->wrapper = wrapper;
			int n = Awk::getOption ();
			this->wrapper = nullptr;
			return n;
		}

		void setOption (ASE::Net::Awk^ wrapper, int opt)
		{
			this->wrapper = wrapper;
			Awk::setOption (opt);
			this->wrapper = nullptr;
		}

		int parse (ASE::Net::Awk^ wrapper)
		{
			this->wrapper = wrapper;
			int n = Awk::parse ();
			this->wrapper = nullptr;
			return n;
		}

		int run (ASE::Net::Awk^ wrapper, const char_t* main = ASE_NULL, 
		         const char_t** args = ASE_NULL, size_t nargs = 0)
		{
			this->wrapper = wrapper;
			int n = Awk::run (main, args, nargs);
			this->wrapper = nullptr;
			return n;
		}

		int setWord (ASE::Net::Awk^ wrapper, const char_t* ow, size_t olen, const char_t* nw, size_t nlen)
		{
			this->wrapper = wrapper;
			int n = Awk::setWord (ow, olen, nw, nlen);
			this->wrapper = nullptr;
			return n;
		}

		int unsetWord (ASE::Net::Awk^ wrapper, const char_t* ow, size_t olen)
		{
			this->wrapper = wrapper;
			int n = Awk::unsetWord (ow, olen);
			this->wrapper = nullptr;
			return n;
		}

		int unsetAllWords (ASE::Net::Awk^ wrapper)
		{
			this->wrapper = wrapper;
			int n = Awk::unsetAllWords ();
			this->wrapper = nullptr;
			return n;
		}

		void setMaxDepth (ASE::Net::Awk^ wrapper, int ids, size_t depth)
		{
			this->wrapper = wrapper;
			Awk::setMaxDepth (ids, depth);
			this->wrapper = nullptr;
		}

		size_t getMaxDepth (ASE::Net::Awk^ wrapper, int id) const
		{
			this->wrapper = wrapper;
			size_t n = Awk::getMaxDepth (id);
			this->wrapper = nullptr;
			return n;
		}

		void enableRunCallback (ASE::Net::Awk^ wrapper)
		{
			this->wrapper = wrapper;
			Awk::enableRunCallback ();
			this->wrapper = nullptr;
		}

		void disableRunCallback (ASE::Net::Awk^ wrapper)
		{
			this->wrapper = wrapper;
			Awk::disableRunCallback ();
			this->wrapper = nullptr;
		}

		void onRunStart (const Run& run)
		{
			if (wrapper->OnRunStart != nullptr)
			{
				wrapper->OnRunStart ();
			}
		}
		void onRunEnd (const Run& run)
		{
			if (wrapper->OnRunEnd != nullptr)
			{
				wrapper->OnRunEnd ();
			}
		}
		void onRunReturn (const Run& run, const Argument& ret)
		{
			if (wrapper->OnRunReturn != nullptr)
			{
				wrapper->OnRunReturn ();
			}
		}

		void onRunStatement (const Run& run, size_t line)
		{
			if (wrapper->OnRunStatement != nullptr)
			{
				wrapper->OnRunStatement ();
			}
		}

		int addFunction (
			ASE::Net::Awk^ wrapper,	const char_t* name,
			size_t minArgs, size_t maxArgs, FunctionHandler handler)
		{
			this->wrapper = wrapper;
			int n = Awk::addFunction (name, minArgs, maxArgs, handler);
			this->wrapper = nullptr;
			return n;
		}

		int deleteFunction (ASE::Net::Awk^ wrapper, const char_t* main)
		{
			this->wrapper = wrapper;
			int n = Awk::deleteFunction (main);
			this->wrapper = nullptr;
			return n;
		}

		int mojoFunctionHandler (
			Return* ret, const Argument* args, size_t nargs, 
			const char_t* name, size_t len)
		{
			
			return wrapper->DispatchFunction (ret, args, nargs, name, len)? 0: -1;
		}

		int openSource (Source& io) 
		{ 
			ASE::Net::Awk::Source^ nio = gcnew ASE::Net::Awk::Source (
				(ASE::Net::Awk::Source::MODE)io.getMode());

			GCHandle gh = GCHandle::Alloc (nio);
			io.setHandle (GCHandle::ToIntPtr(gh).ToPointer());

			try { return wrapper->OpenSource (nio); }
			catch (...) 
			{ 
				gh.Free ();
				io.setHandle (NULL);
				return -1; 	
			}
		}

		int closeSource (Source& io) 
		{
			System::IntPtr ip ((void*)io.getHandle ());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			try
			{
				return wrapper->CloseSource (
					(ASE::Net::Awk::Source^)gh.Target);
			}
			catch (...) { return -1; }
			finally { gh.Free (); }
		}

		ssize_t readSource (Source& io, char_t* buf, size_t len) 
		{
			System::IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			
			try
			{
				b = gcnew cli::array<char_t> (len);
				int n = wrapper->ReadSource (
					(ASE::Net::Awk::Source^)gh.Target, b, len);
				for (int i = 0; i < n; i++) buf[i] = b[i];
				return n;
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
		}

		ssize_t writeSource (Source& io, char_t* buf, size_t len)
		{
			System::IntPtr ip ((void*)io.getHandle());
			GCHandle gh = GCHandle::FromIntPtr (ip);

			cli::array<char_t>^ b = nullptr;
			try
			{
				b = gcnew cli::array<char_t> (len);
				for (int i = 0; i < len; i++) b[i] = buf[i];
				return wrapper->WriteSource (
					(ASE::Net::Awk::Source^)gh.Target, b, len);
			}
			catch (...) { return -1; }
			finally { if (b != nullptr) delete b; }
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
			System::IntPtr ip ((void*)io.getHandle ());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle ());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle ());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle());
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
			System::IntPtr ip ((void*)io.getHandle());
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

	protected:
		//msclr::auto_gcroot<ASE::Net::Awk^> wrapper;		
		mutable gcroot<ASE::Net::Awk^> wrapper;		
	};

	namespace Net
	{
		Awk::Awk ()
		{
			funcs = gcnew System::Collections::Hashtable();

			awk = new ASE::MojoAwk ();
			if (awk->open (this) == -1)
			{
				// TODO:...
				//throw new AwkException ("cannot open awk");
			}

			//option = (OPTION)(awk->getOption (this) | MojoAwk::OPT_CRLF);	
			option = (OPTION)(awk->getOption (this) | ASE::Awk::OPT_CRLF);	
			awk->setOption (this, (int)option);
		}

		Awk::~Awk ()
		{
System::Diagnostics::Debug::Print ("Awk::~Awk");

			if (awk != NULL)
			{
				awk->close (this);
				delete awk;
				awk = NULL;
			}

			if (funcs != nullptr)
			{
				funcs->Clear ();
				delete funcs;
				funcs = nullptr;
			}
		}

		Awk::!Awk ()
		{
System::Diagnostics::Debug::Print ("Awk::!Awk");
			if (awk != NULL)
			{
				awk->close (this);
				delete awk;
				awk = NULL;
			}
		}

		Awk::OPTION Awk::Option::get ()
		{
			if (awk != NULL) this->option = (OPTION)awk->getOption (this);
			return this->option; 
		}

		void Awk::Option::set (Awk::OPTION opt)
		{
			this->option = opt;
			if (awk != NULL) awk->setOption (this, (int)this->option);
		}

		void Awk::Close ()
		{
			if (awk != NULL) 
			{
				awk->close (this);
				delete awk;
				awk = NULL;
			}

			if (funcs != nullptr)
			{
				funcs->Clear ();
				delete funcs;
				funcs = nullptr;
			}
		}

		bool Awk::Parse ()
		{
			if (awk == NULL) return false;
			return awk->parse (this) == 0;	
		}

		bool Awk::Run ()
		{
			if (awk == NULL) return false;

			if (OnRunStart != nullptr || OnRunEnd != nullptr || 
				OnRunReturn != nullptr || OnRunStatement != nullptr)
			{
				awk->enableRunCallback (this);
			}

			return awk->run (this) == 0;
		}

		bool Awk::AddFunction (
			System::String^ name, int minArgs, int maxArgs, 
			FunctionHandler^ handler)
		{
			if (awk == NULL) return false;
			cli::pin_ptr<const ASE::Awk::char_t> nptr = PtrToStringChars(name);
			int n = awk->addFunction (this, nptr, minArgs, maxArgs, 
				(ASE::Awk::FunctionHandler)&MojoAwk::mojoFunctionHandler);
			if (n == 0) funcs->Add(name, handler);	
			return n == 0;
		}

		bool Awk::DeleteFunction (System::String^ name)
		{
			if (awk == NULL) return false;
			cli::pin_ptr<const ASE::Awk::char_t> nptr = PtrToStringChars(name);
			int n = awk->deleteFunction (this, nptr);
			if (n == 0) funcs->Remove (name);
			return n == 0;
		}

		bool Awk::DispatchFunction (ASE::Awk::Return* ret, 
			const ASE::Awk::Argument* args, size_t nargs, 
			const char_t* name, size_t len)
		{
			System::String^ nm = gcnew System::String (name, 0, len);

			FunctionHandler^ fh = (FunctionHandler^)funcs[nm];
			if (fh == nullptr) return false;
			
			cli::array<Argument^>^ arg_arr = gcnew cli::array<Argument^> (nargs);
			for (size_t i = 0; i < nargs; i++) arg_arr[i] = gcnew Argument(args[i]);

			Return^ r = gcnew Return (*ret);
			return fh(nm, arg_arr, r);
		}

		bool Awk::SetWord (System::String^ ow, System::String^ nw)
		{
			if (awk == NULL) return false;
			cli::pin_ptr<const ASE::Awk::char_t> optr = PtrToStringChars(ow);
			cli::pin_ptr<const ASE::Awk::char_t> nptr = PtrToStringChars(nw);
			return (awk->setWord (this, optr, ow->Length, nptr, nw->Length) == 0);
		}

		bool Awk::UnsetWord (System::String^ ow)
		{
			if (awk == NULL) return false;
			cli::pin_ptr<const ASE::Awk::char_t> optr = PtrToStringChars(ow);
			return (awk->unsetWord (this, optr, ow->Length) == 0);
		}

		bool Awk::UnsetAllWords ()
		{
			if (awk == NULL) return false;
			return (awk->unsetAllWords (this) == 0);
		}

		bool Awk::SetMaxDepth (DEPTH id, size_t depth)
		{
			if (awk == NULL) return false;
			awk->setMaxDepth (this, (int)id, depth);
			return true;
		}

		bool Awk::GetMaxDepth (DEPTH id, size_t* depth)
		{
			if (awk == NULL) return false;
			*depth = awk->getMaxDepth (this, (int)id);
			return true;
		}

	}
}

