/*
 * $Id: StdAwk.hpp,v 1.7 2007/05/11 16:25:38 bacon Exp $
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
		int sin (Return* ret, const Argument* args, size_t nargs);
		int cos (Return* ret, const Argument* args, size_t nargs);
		int tan (Return* ret, const Argument* args, size_t nargs);
		int atan2 (Return* ret, const Argument* args, size_t nargs);
		int log (Return* ret, const Argument* args, size_t nargs);
		int exp (Return* ret, const Argument* args, size_t nargs);
		int sqrt (Return* ret, const Argument* args, size_t nargs);
		int fnint (Return* ret, const Argument* args, size_t nargs);
		int rand (Return* ret, const Argument* args, size_t nargs);
		int srand (Return* ret, const Argument* args, size_t nargs);
		int systime (Return* ret, const Argument* args, size_t nargs);
		int strftime (Return* ret, const Argument* args, size_t nargs);
		int strfgmtime (Return* ret, const Argument* args, size_t nargs);
		int system (Return* ret, const Argument* args, size_t nargs);

	protected:
		unsigned int seed; 
	};

}

#endif


