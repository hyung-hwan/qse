/*
 * $Id: StdAwk.cpp,v 1.3 2007/07/20 09:23:37 bacon Exp $
 */

#include "stdafx.h"
#include "misc.h"

#include <ase/net/StdAwk.hpp>
#include <stdio.h>
#include <tchar.h>
#include <vcclr.h>

namespace ASE
{
	namespace Net
	{

		StdAwk::StdAwk ()
		{
		}

		StdAwk::~StdAwk ()
		{
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
			if (file->Mode == File::MODE::READ)
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

			pipe->Handle = IntPtr ((void*)fp);
			return 1;
		}

		int StdAwk::ClosePipe (Pipe^ pipe)
		{
			IntPtr ip = (IntPtr)pipe->Handle;
			FILE* fp = (FILE*)ip.ToPointer();
			return (::_pclose (fp) == EOF)? -1: 0;
		}

		int StdAwk::ReadPipe (Pipe^ pipe, cli::array<char_t>^ buf, int len)
		{
			IntPtr ip = (IntPtr)pipe->Handle;
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
			IntPtr ip = (IntPtr)pipe->Handle;
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
						return -1;
					}
					left -= 1; ptr += 1;
				}
				else
				{
					int n = fprintf (fp, "%.*s", left, ptr);
					if (n < 0 || n > left) return -1;
					left -= n; ptr += n;
				}
			}

			::free (mbp);
			return len;
		}

		int StdAwk::FlushPipe (Pipe^ pipe)
		{
			IntPtr ip = (IntPtr)pipe->Handle;
			FILE* fp = (FILE*)ip.ToPointer();
			return (::fflush (fp) == EOF)? -1: 0;
		}
	}
}
