/*
 * $Id: StdAwk.cpp,v 1.2 2007/05/05 16:32:46 bacon Exp $
 */

#include <ase/awk/StdAwk.hpp>

namespace ASE
{

	StdAwk::StdAwk ()
	{
		addFunction (ASE_T("sin"), 1, 1, (FunctionHandler)&StdAwk::sin);
		addFunction (ASE_T("cos"), 1, 1, (FunctionHandler)&StdAwk::cos);
		addFunction (ASE_T("tan"), 1, 1, (FunctionHandler)&StdAwk::tan);
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
