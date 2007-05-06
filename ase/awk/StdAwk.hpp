/*
 * $Id: StdAwk.hpp,v 1.2 2007/05/05 16:32:46 bacon Exp $
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

	protected:
		int sin ();
		int cos ();
		int tan ();
	};

}

#endif


