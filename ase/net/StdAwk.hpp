/*
 * $Id: StdAwk.hpp,v 1.5 2007/08/20 14:19:58 bacon Exp $
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

		protected:
			bool Sin (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Cos (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Tan (System::String^ name, array<Argument^>^ args, Return^ ret);

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
