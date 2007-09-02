/*
 * $Id: StdAwk.hpp,v 1.6 2007/08/26 14:33:38 bacon Exp $
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
			System::Random^ random;

			bool Sin (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Cos (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Tan (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Atan (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Atan2 (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Log (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Exp (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Sqrt (System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Int (System::String^ name, array<Argument^>^ args, Return^ ret);
			//bool Rand (System::String^ name, array<Argument^>^ args, Return^ ret);
			//bool Srand (System::String^ name, array<Argument^>^ args, Return^ ret);

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
