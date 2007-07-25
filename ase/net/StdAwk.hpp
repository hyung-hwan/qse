/*
 * $Id: StdAwk.hpp,v 1.4 2007/07/20 09:23:37 bacon Exp $
 */

#include <ase/net/Awk.hpp>

namespace ASE
{
	namespace Net
	{
	
		public ref class StdAwk abstract: public Awk
		{
		public:
			StdAwk ();
			~StdAwk ();

		public protected:
			// File
			virtual int OpenFile (File^ file) override;
			virtual int CloseFile (File^ file) override;
			virtual int ReadFile (
				File^ file, cli::array<char_t>^ buf, int len) override;
			virtual int WriteFile (
				File^ file, cli::array<char_t>^ buf, int len) override;
			virtual int FlushFile (File^ file) override;

			// Pipe
			virtual int OpenPipe (Pipe^ pipe) override;
			virtual int ClosePipe (Pipe^ pipe) override;
			virtual int ReadPipe (
				Pipe^ pipe, cli::array<char_t>^ buf, int len) override;
			virtual int WritePipe (
				Pipe^ pipe, cli::array<char_t>^ buf, int len) override;
			virtual int FlushPipe (Pipe^ pipe) override;
		};
	}
}
