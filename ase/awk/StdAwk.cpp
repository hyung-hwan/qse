/*
 * $Id: StdAwk.cpp,v 1.6 2007/05/09 16:07:44 bacon Exp $
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
			int opt = 
				ASE_AWK_IMPLICIT |
				ASE_AWK_EXPLICIT | 
				ASE_AWK_UNIQUEFN | 
				ASE_AWK_IDIV |
				ASE_AWK_SHADING | 
				ASE_AWK_SHIFT | 
				ASE_AWK_EXTIO | 
				ASE_AWK_BLOCKLESS | 
				ASE_AWK_STRBASEONE | 
				ASE_AWK_STRIPSPACES | 
				ASE_AWK_NEXTOFILE /*|
				ASE_AWK_ARGSTOMAIN*/;
			ase_awk_setoption (awk, opt);

			addFunction (ASE_T("sin"), 1, 1, (FunctionHandler)&StdAwk::sin);
			addFunction (ASE_T("cos"), 1, 1, (FunctionHandler)&StdAwk::cos);
			addFunction (ASE_T("tan"), 1, 1, (FunctionHandler)&StdAwk::tan);
		}

		return n;
	}

	int StdAwk::sin (size_t nargs, const Value* args, Value* ret)
	{
		return 0;
	}

	int StdAwk::cos (size_t nargs, const Value* args, Value* ret)
	{
		return 0;
	}

	int StdAwk::tan (size_t nargs, const Value* args, Value* ret)
	{
		return 0;
	}
}
