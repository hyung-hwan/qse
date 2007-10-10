/*
 * $Id: StdAwk.cpp,v 1.16 2007/10/08 09:43:15 bacon Exp $
 *
 * {License}
 */

#include "stdafx.h"
#include "misc.h"

#include <ase/net/StdAwk.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <vcclr.h>
#include <time.h>

#pragma warning(disable:4996)

namespace ASE
{
	namespace Net
	{

		StdAwk::StdAwk ()
		{
			random_seed = (gcnew System::Random)->Next (System::Int32::MinValue, System::Int32::MaxValue);
			random = gcnew System::Random (random_seed);

			// TODO: exception/error handling....
			AddFunction ("sin", 1, 1, gcnew FunctionHandler (this, &StdAwk::Sin));
			AddFunction ("cos", 1, 1, gcnew FunctionHandler (this, &StdAwk::Cos));
			AddFunction ("tan", 1, 1, gcnew FunctionHandler (this, &StdAwk::Tan));
			AddFunction ("atan", 1, 1, gcnew FunctionHandler (this, &StdAwk::Atan));
			AddFunction ("atan2", 2, 2, gcnew FunctionHandler (this, &StdAwk::Atan2));
			AddFunction ("log", 1, 1, gcnew FunctionHandler (this, &StdAwk::Log));
			AddFunction ("exp", 1, 1, gcnew FunctionHandler (this, &StdAwk::Exp));
			AddFunction ("sqrt", 1, 1, gcnew FunctionHandler (this, &StdAwk::Sqrt));
			AddFunction ("int", 1, 1, gcnew FunctionHandler (this, &StdAwk::Int));
			AddFunction ("rand", 0, 0, gcnew FunctionHandler (this, &StdAwk::Rand));
			AddFunction ("srand", 1, 1, gcnew FunctionHandler (this, &StdAwk::Srand));
			AddFunction ("systime", 0, 0, gcnew FunctionHandler (this, &StdAwk::Systime));
			AddFunction ("strftime", 0, 2, gcnew FunctionHandler (this, &StdAwk::Strftime));
			AddFunction ("strfgmtime", 0, 2, gcnew FunctionHandler (this, &StdAwk::Strfgmtime));
		}

		StdAwk::~StdAwk ()
		{
		}

		bool StdAwk::Sin (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Sin (args[0]->RealValue));
		}

		bool StdAwk::Cos (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Cos (args[0]->RealValue));
		}

		bool StdAwk::Tan (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Tan (args[0]->RealValue));
		}

		bool StdAwk::Atan (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Atan (args[0]->RealValue));
		}

		bool StdAwk::Atan2 (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Atan2 (args[0]->RealValue, args[1]->RealValue));
		}

		bool StdAwk::Log (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Log (args[0]->RealValue));
		}

		bool StdAwk::Exp (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Exp (args[0]->RealValue));
		}

		bool StdAwk::Sqrt (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((real_t)System::Math::Sqrt (args[0]->RealValue));
		}

		bool StdAwk::Int (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set (args[0]->LongValue);
		}

		bool StdAwk::Rand (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((long_t)random->Next ());
		}

		bool StdAwk::Srand (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			int seed = (int)args[0]->LongValue;
			System::Random^ tmp = gcnew System::Random (seed);

			if (!ret->Set((long_t)tmp->Next())) return false;

			this->random_seed = seed;
			this->random = tmp;
			return true;
		}

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER>=1400)
	#define time_t __time64_t
	#define time _time64
	#define localtime _localtime64
	#define gmtime _gmtime64
#endif

		bool StdAwk::Systime (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			return ret->Set ((long_t)::time(NULL));
		}

		bool StdAwk::Strftime (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			wchar_t buf[128]; 
			struct tm* tm;
			size_t len;
	       
			if (args->Length < 1) 
			{
				const wchar_t* fmt = L"%c";
				time_t t = (args->Length < 2)? ::time(NULL): (time_t)args[1]->LongValue;

				tm = ::localtime (&t);
				len = ::wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
			}
			else
			{
				cli::pin_ptr<const ASE::Awk::char_t> fmt = PtrToStringChars(args[0]->StringValue);

				time_t t = (args->Length < 2)? ::time(NULL): (time_t)args[1]->LongValue;

				tm = ::localtime (&t);
				len = ::wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
			}

			return ret->Set (gcnew System::String (buf, 0, len));
		}

		bool StdAwk::Strfgmtime (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret)
		{
			wchar_t buf[128]; 
			struct tm* tm;
			size_t len;
	       
			if (args->Length < 1) 
			{
				const wchar_t* fmt = L"%c";
				time_t t = (args->Length < 2)? ::time(NULL): (time_t)args[1]->LongValue;

				tm = ::gmtime (&t);
				len = ::wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
			}
			else
			{
				cli::pin_ptr<const ASE::Awk::char_t> fmt = PtrToStringChars(args[0]->StringValue);

				time_t t = (args->Length < 2)? ::time(NULL): (time_t)args[1]->LongValue;

				tm = ::gmtime (&t);
				len = ::wcsftime (buf, ASE_COUNTOF(buf), fmt, tm);
			}

			return ret->Set (gcnew System::String (buf, 0, len));
		}

		int StdAwk::OpenFile (File^ file)
		{
			System::IO::FileMode mode;
			System::IO::FileAccess access;
			System::IO::FileStream^ fs;

			if (file->Mode->Equals(File::MODE::READ))
			{
				mode = System::IO::FileMode::Open;
				access = System::IO::FileAccess::Read;

				fs = gcnew System::IO::FileStream (file->Name, mode, access);
				System::IO::StreamReader^ rd = gcnew System::IO::StreamReader (fs);
				file->Handle = rd;
			}
			else if (file->Mode->Equals(File::MODE::WRITE))
			{
				mode = System::IO::FileMode::Create;
				access = System::IO::FileAccess::Write;

				fs = gcnew System::IO::FileStream (file->Name, mode, access);
				System::IO::StreamWriter^ wr = gcnew System::IO::StreamWriter (fs);
				file->Handle = wr;
			}
			else /* File::MODE::APPEND */
			{
				mode = System::IO::FileMode::Append;
				access = System::IO::FileAccess::Write;

				fs = gcnew System::IO::FileStream (file->Name, mode, access);
				System::IO::StreamWriter^ wr = gcnew System::IO::StreamWriter (fs);
				file->Handle = wr;
			}

			return 1;
		}

		int StdAwk::CloseFile (File^ file)
		{
			if (file->Mode->Equals(File::MODE::READ))
			{
				System::IO::StreamReader^ sr = (System::IO::StreamReader^)file->Handle;
				sr->Close ();
			}
			else
			{
				System::IO::StreamWriter^ sw = (System::IO::StreamWriter^)file->Handle;
				sw->Close ();
			}
			return 0;
		}

		int StdAwk::ReadFile (File^ file, cli::array<char_t>^ buf, int len)
		{
			System::IO::StreamReader^ sr = (System::IO::StreamReader^)file->Handle;
			return sr->Read (buf, 0, len); 
		}

		int StdAwk::WriteFile (File^ file, cli::array<char_t>^ buf, int len)
		{
			System::IO::StreamWriter^ sw = (System::IO::StreamWriter^)file->Handle;
			sw->Write (buf, 0, len);
			return len;
		}

		int StdAwk::FlushFile (File^ file)
		{
			System::IO::StreamWriter^ sw = (System::IO::StreamWriter^)file->Handle;
			sw->Flush ();
			return 0;
		}


		int StdAwk::OpenPipe (Pipe^ pipe)
		{
			FILE* fp = NULL;

			cli::pin_ptr<const wchar_t> name =
				PtrToStringChars(pipe->Name);

			if (pipe->Mode->Equals(Pipe::MODE::READ))
			{
				fp = _wpopen (name, L"r");
			}
			else // Pipe::MODE::WRITE
			{
				fp = _wpopen (name, L"w");
			}

			if (fp == NULL) return -1;

			pipe->Handle = System::IntPtr ((void*)fp);
			return 1;
		}

		int StdAwk::ClosePipe (Pipe^ pipe)
		{
			System::IntPtr ip = (System::IntPtr)pipe->Handle;
			FILE* fp = (FILE*)ip.ToPointer();
			return (::_pclose (fp) == EOF)? -1: 0;
		}

		int StdAwk::ReadPipe (Pipe^ pipe, cli::array<char_t>^ buf, int len)
		{
			System::IntPtr ip = (System::IntPtr)pipe->Handle;
			FILE* fp = (FILE*)ip.ToPointer();

			int n = 0;

			while (n < len)
			{
				wint_t c = fgetwc (fp);
				if (c == WEOF) break;

				buf[n++] = c;
				if (c == L'\n') break;
			}

			return n;
		}

		int StdAwk::WritePipe (Pipe^ pipe, cli::array<char_t>^ buf, int len)
		{
			System::IntPtr ip = (System::IntPtr)pipe->Handle;
			FILE* fp = (FILE*)ip.ToPointer();
			int left;

			cli::pin_ptr<char_t> bp = &buf[0];

			/* somehow, fwprintf returns 0 when non-ascii 
			 * characters are included in the buffer.
			while (left > 0)
			{
				if (*bp == ASE_T('\0')) 
				{
					if (fputwc (*ptr, fp) == WEOF) 
					{
						return -1;
					}
					left -= 1; bp += 1;
				}
				else
				{
					int n = fwprintf (fp, L"%.*s", left, bp);
					if (n < 0 || n > left) return -1;
					left -= n; bp += n;
				}
			}*/

			/* so the scheme has been changed to the following */
			char* mbp = unicode_to_multibyte (bp, len, &left);
			if (mbp == NULL) return -1;

			char* ptr = mbp;
			while (left > 0)
			{
				if (*ptr == '\0')
				{
					if (fputwc (*ptr, fp) == WEOF) 
					{
						::free (mbp);
						return -1;
					}
					left -= 1; ptr += 1;
				}
				else
				{
					int n = fprintf (fp, "%.*s", left, ptr);
					if (n < 0 || n > left) 
					{
						::free (mbp);
						return -1;
					}
					left -= n; ptr += n;
				}
			}

			::free (mbp);
			return len;
		}

		int StdAwk::FlushPipe (Pipe^ pipe)
		{
			System::IntPtr ip = (System::IntPtr)pipe->Handle;
			FILE* fp = (FILE*)ip.ToPointer();
			return (::fflush (fp) == EOF)? -1: 0;
		}
	}
}
