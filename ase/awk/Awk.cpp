/*
 * $Id: Awk.cpp,v 1.4 2007/05/02 15:07:33 bacon Exp $
 */

#include <ase/awk/Awk.hpp>

namespace ASE
{
	Awk::Awk (): awk (ASE_NULL)
	{
	}

	Awk::~Awk ()
	{
		close ();
	}

	int Awk::parse ()
	{
		if (awk == ASE_NULL)
		{
			/*awk = ase_awk_open (*/
		}

		ase_awk_srcios_t srcios;

		srcios.in = sourceReader;
		srcios.out = sourceWriter;
		srcios.custom_data = this;

		return ase_awk_parse (awk, ASE_NULL);
	}

	int Awk::run (const ase_char_t* main, const ase_char_t** args)
	{
		if (awk == ASE_NULL) 
		{
			// TODO: SET ERROR INFO
			return -1;
		}

		//return ase_awk_run (awk, main);
		return 0;
	}

	void Awk::close ()
	{
		if (awk != ASE_NULL) 
		{
			ase_awk_close (awk);
			awk = ASE_NULL;
		}
	}

	ase_ssize_t Awk::sourceReader (
		int cmd, void* arg, ase_char_t* data, ase_size_t count)
	{
		ASE::Awk* awk = (ASE::Awk*)arg;
	
		if (cmd == ASE_AWK_IO_OPEN) 
		{
			return awk->openSource (ASE::Awk::SOURCE_READ);
		}
		else if (cmd == ASE_AWK_IO_CLOSE)
		{
			return awk->closeSource (ASE::Awk::SOURCE_READ);
		}
		else if (cmd == ASE_AWK_IO_READ)
		{
			return awk->readSource (data, count);
		}
	
		return -1;
	}
	
	ase_ssize_t Awk::sourceWriter (
		int cmd, void* arg, ase_char_t* data, ase_size_t count)
	{
		ASE::Awk* awk = (ASE::Awk*)arg;
	
		if (cmd == ASE_AWK_IO_OPEN) 
		{
			return awk->openSource (ASE::Awk::SOURCE_WRITE);
		}
		else if (cmd == ASE_AWK_IO_CLOSE)
		{
			return awk->closeSource (ASE::Awk::SOURCE_WRITE);
		}
		else if (cmd == ASE_AWK_IO_WRITE)
		{
			return awk->writeSource (data, count);
		}
	
		return -1;
	}
	
}
