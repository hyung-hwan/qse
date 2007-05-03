/*
 * $Id: Awk.cpp,v 1.2 2007/05/01 12:39:22 bacon Exp $
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
		if (awk == ASE_NULL)
		{
			/*awk = ase_awk_open (*/
		}

		return ase_awk_parse (awk, ASE_NULL);
	}

	int Awk::run (/*const ase_char_t* main*/)
	{
		if (awk == ASE_NULL)
		{
		}

		//return ase_awk_run (awk, main);
		return 0;
	}

}

