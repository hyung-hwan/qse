/*
 * $Id: StdAwk.hpp,v 1.4 2007/05/07 09:30:28 bacon Exp $
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
		Value* sin (size_t nargs, Value** args);
		Value* cos (size_t nargs, Value** args);
		Value* tan (size_t nargs, Value** args);
	};

}

#endif


