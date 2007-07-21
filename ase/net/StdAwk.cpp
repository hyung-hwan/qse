/*
 * $Id: StdAwk.cpp,v 1.2 2007/07/19 14:35:10 bacon Exp $
 */

#include "stdafx.h"

#include <ase/net/StdAwk.hpp>

namespace ASE
{
	namespace Net
	{

		StdAwk::StdAwk ()
		{
			OpenFileHandler += gcnew OpenFile (this, &StdAwk::openFile);
			CloseFileHandler += gcnew CloseFile (this, &StdAwk::closeFile);
			ReadFileHandler += gcnew ReadFile (this, &StdAwk::readFile);
			WriteFileHandler += gcnew WriteFile (this, &StdAwk::writeFile);
			FlushFileHandler += gcnew FlushFile (this, &StdAwk::flushFile);
		}

		StdAwk::~StdAwk ()
		{
		}

		int StdAwk::openFile (File^ file)
		{
			System::IO::FileMode mode;
			System::IO::FileAccess access;
			System::IO::FileStream^ fs;

			try
			{
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
			catch (System::Exception^)
			{
				return -1;
			}
		}

		int StdAwk::closeFile (File^ file)
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

		int StdAwk::readFile (File^ file, cli::array<char_t>^ buf, int len)
		{
			System::IO::StreamReader^ sr = (System::IO::StreamReader^)file->Handle;
			try { return sr->Read (buf, 0, len); }
			catch (System::Exception^) { return -1;	}
		}

		int StdAwk::writeFile (File^ file, cli::array<char_t>^ buf, int len)
		{
			System::IO::StreamWriter^ sw = (System::IO::StreamWriter^)file->Handle;
			try
			{
				sw->Write (buf, 0, len);
				return len;
			}
			catch (System::Exception^) { return -1;	}
		}

		int StdAwk::flushFile (File^ file)
		{
			System::IO::StreamWriter^ sw = (System::IO::StreamWriter^)file->Handle;
			try
			{
				sw->Flush ();
				return 0;
			}
			catch (System::Exception^) { return -1; }
		}

	}
}
