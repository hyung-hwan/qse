/*
 * $Id: StdAwk.hpp,v 1.8 2007/10/08 09:43:15 bacon Exp $
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
			int random_seed;
			System::Random^ random;

			bool Sin (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Cos (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Tan (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Atan (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Atan2 (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Log (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Exp (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Sqrt (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Int (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Rand (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Srand (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Systime (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Strftime (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);
			bool Strfgmtime (Context^ ctx, System::String^ name, array<Argument^>^ args, Return^ ret);

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
