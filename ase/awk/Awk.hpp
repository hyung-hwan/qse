/*
 * $Id: Awk.hpp,v 1.3 2007/05/02 15:07:33 bacon Exp $
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
		virtual ~Awk ();
		
		int parse ();
		int run (const ase_char_t* main, const ase_char_t** args);
		void close ();

		enum SourceMode
		{
			SOURCE_READ,
			SOURCE_WRITE
		};

	protected:
		virtual int openSource (SourceMode mode) = 0;
		virtual int closeSource (SourceMode mode) = 0;

		virtual ase_ssize_t readSource (
			ase_char_t* buf, ase_size_t len) = 0;
		virtual ase_ssize_t writeSource (
			ase_char_t* buf, ase_size_t len) = 0;

		static ase_ssize_t sourceReader (
        		int cmd, void* arg, ase_char_t* data, ase_size_t count);
		static ase_ssize_t sourceWriter (
        		int cmd, void* arg, ase_char_t* data, ase_size_t count);

	private:
		ase_awk_t* awk;
	};

}

#endif
