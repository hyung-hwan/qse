/*
 * $Id: Awk.cpp,v 1.1 2007/04/30 05:47:33 bacon Exp $
 */

#include <ase/awk/Awk.hpp>

namespace ASE
{

	Awk::Awk (): awk (ASE_NULL)
	{
	}

	Awk::~Awk ()
	{
		if (awk != ASE_NULL) 
		{
			ase_awk_close (awk);
			awk = ASE_NULL;
		}
	}

	int Awk::parse ()
	{
		return ase_awk_parse (awk, ASE_NULL);
	}

	int Awk::run (/*const ase_char_t* main*/)
	{
		//return ase_awk_parse (awk, main);
		return 0;
	}

}

