/*
 * $Id: Awk.hpp,v 1.1 2007-04-22 13:51:44 bacon Exp $
 */

#ifndef _ASE_AWK_AWK_HPP_
#define _ASE_AWK_AWK_HPP_

#include <ase/awk/awk.h>

namespace ASE
{

	class Awk
	{
	public:
		Awk ();
		~Awk ();
		
		int parse ();
		int run ();

	private:
		ase_awk_t* awk;
	};

}

#endif
