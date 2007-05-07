/*
 * $Id: StdAwk.cpp,v 1.3 2007/05/06 06:55:05 bacon Exp $
 */

#include <ase/awk/StdAwk.hpp>

namespace ASE
{

	StdAwk::StdAwk ()
	{
	}

	int StdAwk::open ()
	{
		int n = Awk::open ();
		if (n == 0)
		{
			/*
			addFunction (ASE_T("sin"), 1, 1, (FunctionHandler)&StdAwk::sin);
			addFunction (ASE_T("cos"), 1, 1, (FunctionHandler)&StdAwk::cos);
			addFunction (ASE_T("tan"), 1, 1, (FunctionHandler)&StdAwk::tan);
			*/
		}

		return n;
	}

	int StdAwk::sin ()
	{
		return 0;
	}

	int StdAwk::cos ()
	{
		return 0;
	}

	int StdAwk::tan ()
	{
		return 0;
	}
}
