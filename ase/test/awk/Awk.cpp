/*
 * $Id: Awk.cpp,v 1.1 2007/05/02 15:07:33 bacon Exp $
 */

#include <ase/awk/Awk.h>

class TestAwk: public ASE::Awk
{
	int openSource (SourceMode mode)
	{
		return 1;
	}

	int closeSource (SourceMode mode)
	{
		return 0;
	}

	ase_ssize_t readSource (ase_char_t* buf, ase_size_t count)
	{
		return 0;
	}

	ase_ssize_t writeSource (ase_char_t* buf, ase_size_t count);
	{
		return 0;
	}
};

int main ()
{

	
}
