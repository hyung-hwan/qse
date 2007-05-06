/*
 * $Id: Awk.cpp,v 1.6 2007/05/04 10:25:14 bacon Exp $
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
		if (awk == ASE_NULL && open() == -1) return -1;

		ase_awk_srcios_t srcios;

		srcios.in = sourceReader;
		srcios.out = sourceWriter;
		srcios.custom_data = this;

		return ase_awk_parse (awk, ASE_NULL);
	}

	int Awk::run (const char_t* main, const char_t** args)
	{
		if (awk == ASE_NULL) 
		{
			// TODO: SET ERROR INFO
			return -1;
		}

		//return ase_awk_run (awk, main);
		return 0;
	}

	int Awk::open ()
	{
		ase_awk_prmfns_t prmfns;

		prmfns.mmgr.malloc      = malloc;
		prmfns.mmgr.realloc     = realloc;
		prmfns.mmgr.free        = free;
		prmfns.mmgr.custom_data = this;

		prmfns.ccls.is_upper    = isUpper;
		prmfns.ccls.is_lower    = isLower;
		prmfns.ccls.is_alpha    = isAlpha;
		prmfns.ccls.is_digit    = isDigit;
		prmfns.ccls.is_xdigit   = isXdigit;
		prmfns.ccls.is_alnum    = isAlnum;
		prmfns.ccls.is_space    = isSpace;
		prmfns.ccls.is_print    = isPrint;
		prmfns.ccls.is_graph    = isGraph;
		prmfns.ccls.is_cntrl    = isCntrl;
		prmfns.ccls.is_punct    = isPunct;
		prmfns.ccls.to_upper    = toUpper;
		prmfns.ccls.to_lower    = toLower;
		prmfns.ccls.custom_data = this;

		/*
		int (Awk::*ptr) (void*, ase_char_t*, ase_size_t, const ase_char_t*, ...) = &Awk::sprintf;
		(this->*ptr) (ASE_NULL, ASE_NULL, 0, ASE_NULL);
		*/

		prmfns.misc.pow         = pow;
		prmfns.misc.sprintf     = sprintf;
		prmfns.misc.dprintf     = dprintf;
		prmfns.misc.custom_data = this;

		awk = ase_awk_open (&prmfns, ASE_NULL);
		if (awk == ASE_NULL)
		{
			// TODO: SET ERROR INFO
			return -1;
		}

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

	Awk::ssize_t Awk::sourceReader (
		int cmd, void* arg, char_t* data, size_t count)
	{
		Awk* awk = (Awk*)arg;
	
		if (cmd == ASE_AWK_IO_OPEN) 
		{
			return awk->openSource (Awk::SOURCE_READ);
		}
		else if (cmd == ASE_AWK_IO_CLOSE)
		{
			return awk->closeSource (Awk::SOURCE_READ);
		}
		else if (cmd == ASE_AWK_IO_READ)
		{
			return awk->readSource (data, count);
		}
	
		return -1;
	}

	Awk::ssize_t Awk::sourceWriter (
		int cmd, void* arg, char_t* data, size_t count)
	{
		Awk* awk = (Awk*)arg;
	
		if (cmd == ASE_AWK_IO_OPEN) 
		{
			return awk->openSource (Awk::SOURCE_WRITE);
		}
		else if (cmd == ASE_AWK_IO_CLOSE)
		{
			return awk->closeSource (Awk::SOURCE_WRITE);
		}
		else if (cmd == ASE_AWK_IO_WRITE)
		{
			return awk->writeSource (data, count);
		}
	
		return -1;
	}
	
	void* Awk::malloc (void* custom, size_t n)
	{
		return ((Awk*)custom)->malloc (n);
	}

	void* Awk::realloc (void* custom, void* ptr, size_t n)
	{
		return ((Awk*)custom)->realloc (ptr, n);
	}

	void Awk::free (void* custom, void* ptr)
	{
		((Awk*)custom)->free (ptr);
	}

	Awk::bool_t Awk::isUpper (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isUpper (c);
	}
	
	Awk::bool_t Awk::isLower (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isLower (c);
	}
	
	Awk::bool_t Awk::isAlpha (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isAlpha (c);
	}
	
	Awk::bool_t Awk::isDigit (void* custom, cint_t c)  
	{ 
		return ((Awk*)custom)->isDigit (c);
	}
	
	Awk::bool_t Awk::isXdigit (void* custom, cint_t c) 
	{ 
		return ((Awk*)custom)->isXdigit (c);
	}
	
	Awk::bool_t Awk::isAlnum (void* custom, cint_t c)
	{ 
		return ((Awk*)custom)->isAlnum (c);
	}
	
	Awk::bool_t Awk::isSpace (void* custom, cint_t c)
	{ 
		return ((Awk*)custom)->isSpace (c);
	}
	
	Awk::bool_t Awk::isPrint (void* custom, cint_t c)
	{ 
		return ((Awk*)custom)->isPrint (c);
	}
	
	Awk::bool_t Awk::isGraph (void* custom, cint_t c)
	{
		return ((Awk*)custom)->isGraph (c);
	}
	
	Awk::bool_t Awk::isCntrl (void* custom, cint_t c)
	{
		return ((Awk*)custom)->isCntrl (c);
	}
	
	Awk::bool_t Awk::isPunct (void* custom, cint_t c)
	{
		return ((Awk*)custom)->isPunct (c);
	}
	
	Awk::cint_t Awk::toUpper (void* custom, cint_t c)
	{
		return ((Awk*)custom)->toUpper (c);
	}
	
	Awk::cint_t Awk::toLower (void* custom, cint_t c)
	{
		return ((Awk*)custom)->toLower (c);
	}

	Awk::real_t Awk::pow (void* custom, real_t x, real_t y)
	{
		return ((Awk*)custom)->pow (x, y);
	}
		
	int Awk::sprintf (void* custom, char_t* buf, size_t size,
	                  const char_t* fmt, ...)
	{
		va_list ap;
		va_start (ap, fmt);
		int n = ((Awk*)custom)->vsprintf (buf, size, fmt, ap);
		va_end (ap);
		return n;
	}

	void Awk::dprintf (void* custom, const char_t* fmt, ...)
	{
		va_list ap;
		va_start (ap, fmt);
		((Awk*)custom)->vdprintf (fmt, ap);
		va_end (ap);
	}

}
