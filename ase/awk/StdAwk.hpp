/*
 * $Id: StdAwk.hpp,v 1.3 2007/05/06 06:55:05 bacon Exp $
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
		int sin ();
		int cos ();
		int tan ();
	};

}

#endif


