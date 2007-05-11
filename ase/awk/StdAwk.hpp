/*
 * $Id: StdAwk.hpp,v 1.5 2007/05/09 16:07:44 bacon Exp $
 */

#ifndef _ASE_AWK_STDAWK_HPP_
#define _ASE_AWK_STDAWK_HPP_

#include <ase/awk/Awk.hpp>


namespace ASE
{
	class StdAwk: public Awk
	{
	public:
		StdAwk ();

		int open ();

	protected:
		int sin (size_t nargs, const Value* args, Value* ret);
		int cos (size_t nargs, const Value* args, Value* ret);
		int tan (size_t nargs, const Value* args, Value* ret);
	};

}

#endif


