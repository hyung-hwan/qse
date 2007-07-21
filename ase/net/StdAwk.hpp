/*
 * $Id: StdAwk.hpp,v 1.3 2007/07/19 14:35:10 bacon Exp $
 */

#include <ase/net/Awk.hpp>

namespace ASE
{
	namespace Net
	{
	
		public ref class StdAwk: Awk
		{
		public:
			StdAwk ();
			~StdAwk ();

		protected:
			int openFile (File^ file);
			int closeFile (File^ file);
			int readFile (File^ file, cli::array<char_t>^ buf, int len);
			int writeFile (File^ file, cli::array<char_t>^ buf, int len);
			int flushFile (File^ file);
			
		};

	}
}
