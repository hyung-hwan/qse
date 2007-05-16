/*
 * $Id: Awk.cpp,v 1.28 2007/05/14 08:40:13 bacon Exp $
 */

#include <ase/awk/Awk.hpp>
#include <ase/awk/val.h>
#include <ase/cmn/str.h>
#include <ase/cmn/mem.h>

namespace ASE
{

	//////////////////////////////////////////////////////////////////
	// Awk::Source
	//////////////////////////////////////////////////////////////////

	Awk::Source::Source (Mode mode): mode (mode)
	{
	}

	Awk::Source::Mode Awk::Source::getMode () const
	{
		return this->mode;
	}

	const void* Awk::Source::getHandle () const
	{
		return this->handle;
	}

	void Awk::Source::setHandle (void* handle)
	{
		this->handle = handle;
	}

	//////////////////////////////////////////////////////////////////
	// Awk::Extio
	//////////////////////////////////////////////////////////////////
	
	Awk::Extio::Extio (extio_t* extio): extio (extio)
	{
	}

	const Awk::char_t* Awk::Extio::getName () const
	{
		return extio->name;
	}

	const void* Awk::Extio::getHandle () const
	{
		return extio->handle;
	}

	void Awk::Extio::setHandle (void* handle)
	{
		extio->handle = handle;
	}

	Awk::run_t* Awk::Extio::getRun () const
	{
		return extio->run;
	}

	Awk::awk_t* Awk::Extio::getAwk () const
	{
		return ase_awk_getrunawk(extio->run);
	}

	//////////////////////////////////////////////////////////////////
	// Awk::Pipe
	//////////////////////////////////////////////////////////////////

	Awk::Pipe::Pipe (extio_t* extio): Extio(extio)
	{
	}

	Awk::Pipe::Mode Awk::Pipe::getMode () const
	{
		return (Mode)extio->mode;
	}

	//////////////////////////////////////////////////////////////////
	// Awk::File
	//////////////////////////////////////////////////////////////////
	
	Awk::File::File (extio_t* extio): Extio(extio)
	{
	}

	Awk::File::Mode Awk::File::getMode () const
	{
		return (Mode)extio->mode;
	}

	//////////////////////////////////////////////////////////////////
	// Awk::Console
	//////////////////////////////////////////////////////////////////
	
	Awk::Console::Console (extio_t* extio): Extio(extio), filename(ASE_NULL)
	{
	}

	Awk::Console::~Console ()
	{
		if (filename != ASE_NULL)
		{
			ase_awk_free (ase_awk_getrunawk(extio->run), filename);
		}
	}

	int Awk::Console::setFileName (const char_t* name)
	{
		if (extio->mode == READ)
		{
			return ase_awk_setfilename (
				extio->run, name, ase_strlen(name));
		}
		else
		{
			return ase_awk_setofilename (
				extio->run, name, ase_strlen(name));
		}
	}

	Awk::Console::Mode Awk::Console::getMode () const
	{
		return (Mode)extio->mode;
	}

	//////////////////////////////////////////////////////////////////
	// Awk::Argument
	//////////////////////////////////////////////////////////////////

	Awk::Argument::Argument (): run (ASE_NULL), val (ASE_NULL)
	{
		this->str.ptr = ASE_NULL;
		this->str.len = 0;
	}

	Awk::Argument::~Argument ()
	{
		if (this->str.ptr != ASE_NULL)
		{
			ASE_ASSERT (this->run != ASE_NULL);
			ase_awk_free (
				ase_awk_getrunawk(this->run), this->str.ptr);
			this->str.ptr = ASE_NULL;
			this->str.len = 0;
		}

		if (this->val != ASE_NULL) 
		{
			ASE_ASSERT (this->run != ASE_NULL);
			ase_awk_refdownval (this->run, this->val);
		}
	}

	int Awk::Argument::init (run_t* run, ase_awk_val_t* v)
	{
		// this method is used internally only
		// and should never be called more than once 
		ASE_ASSERT (this->run == ASE_NULL && this->val == ASE_NULL);
		ASE_ASSERT (run != ASE_NULL && v != ASE_NULL);

		ase_awk_refupval (run, v);
		this->run = run;
		this->val = v;

		if (v->type == ASE_AWK_VAL_STR)
		{
			int n = ase_awk_valtonum (
				run, v, &this->inum, &this->rnum);
			if (n == 0) 
			{
				this->rnum = (ase_real_t)this->inum;
				return 0;
			}
			else if (n == 1) 
			{
				this->inum = (ase_long_t)this->rnum;
				return 0;
			}
		}
		else if (v->type == ASE_AWK_VAL_INT)
		{
			this->inum = ((ase_awk_val_int_t*)v)->val;
			this->rnum = (ase_real_t)((ase_awk_val_int_t*)v)->val;

			this->str.ptr = ase_awk_valtostr (run, v, 0, ASE_NULL, &this->str.len);
			if (this->str.ptr != ASE_NULL) return 0;
		}
		else if (v->type == ASE_AWK_VAL_REAL)
		{
			this->inum = (ase_long_t)((ase_awk_val_real_t*)v)->val;
			this->rnum = ((ase_awk_val_real_t*)v)->val;

			this->str.ptr = ase_awk_valtostr (run, v, 0, ASE_NULL, &this->str.len);
			if (this->str.ptr != ASE_NULL) return 0;
		}
		else if (v->type == ASE_AWK_VAL_NIL)
		{
			this->inum = 0;
			this->rnum = 0.0;

			this->str.ptr = ase_awk_valtostr (run, v, 0, ASE_NULL, &this->str.len);
			if (this->str.ptr != ASE_NULL) return 0;
		}

		ase_awk_refdownval (run, v);
		this->run = ASE_NULL;
		this->val = ASE_NULL;
		return -1;
	}

	Awk::long_t Awk::Argument::toInt () const
	{
		ASE_ASSERT (this->run != ASE_NULL && this->val != ASE_NULL);
		return this->inum;
	}

	Awk::real_t Awk::Argument::toReal () const
	{
		ASE_ASSERT (this->run != ASE_NULL && this->val != ASE_NULL);
		return this->rnum;
	}

	const Awk::char_t* Awk::Argument::toStr (size_t* len) const
	{
		ASE_ASSERT (this->run != ASE_NULL && this->val != ASE_NULL);

		if (this->str.ptr != ASE_NULL)
		{
			*len = this->str.len;
			return this->str.ptr;
		}
		else
		{
			ASE_ASSERT (val->type == ASE_AWK_VAL_STR);
			*len = ((ase_awk_val_str_t*)this->val)->len;
			return ((ase_awk_val_str_t*)this->val)->buf;
		}
	}

	Awk::run_t* Awk::Argument::getRun () const
	{
		ASE_ASSERT (this->run != ASE_NULL);
		return this->run;
	}

	Awk::awk_t* Awk::Argument::getAwk () const
	{
		ASE_ASSERT (this->run != ASE_NULL);
		return ase_awk_getrunawk (this->run);
	}

	//////////////////////////////////////////////////////////////////
	// Awk::Return
	//////////////////////////////////////////////////////////////////

	Awk::Return::Return (run_t* run): run (run), type (ASE_AWK_VAL_NIL)
	{
	}

	Awk::Return::~Return ()
	{
		clear ();
	}

	ase_awk_val_t* Awk::Return::toVal () const
	{
		switch (this->type)
		{
			case ASE_AWK_VAL_NIL:
				return ase_awk_val_nil;

			case ASE_AWK_VAL_INT:
				return ase_awk_makeintval (this->run, this->v.inum);

			case ASE_AWK_VAL_REAL:
				return ase_awk_makerealval (this->run, this->v.rnum);

			case ASE_AWK_VAL_STR:
				return ase_awk_makestrval (
					this->run, this->v.str.ptr, this->v.str.len);
		}

		return ASE_NULL;
	}

	Awk::run_t* Awk::Return::getRun () const
	{
		return this->run;
	}

	Awk::awk_t* Awk::Return::getAwk () const
	{
		return ase_awk_getrunawk (this->run);
	}

	int Awk::Return::set (long_t v)
	{
		clear ();

		this->type = ASE_AWK_VAL_INT;
		this->v.inum = v;

		return 0;
	}

	int Awk::Return::set (real_t v)
	{
		clear ();

		this->type = ASE_AWK_VAL_REAL;
		this->v.rnum = v;

		return 0;
	}

	int Awk::Return::set (char_t* ptr, size_t len)
	{
		awk_t* awk = ase_awk_getrunawk(this->run);
		char_t* tmp = ase_awk_strxdup (awk, ptr, len);
		if (tmp == ASE_NULL) return -1;

		clear ();

		this->type = ASE_AWK_VAL_STR;
		this->v.str.ptr = tmp;
		this->v.str.len = len;

		return 0;
	}

	void Awk::Return::clear ()
	{
		if (this->type == ASE_AWK_VAL_STR)
		{
			ASE_ASSERT (this->v.str.ptr != ASE_NULL);
			awk_t* awk = ase_awk_getrunawk(this->run);
			ase_awk_free (awk, this->v.str.ptr);
			this->v.str.ptr = ASE_NULL;
			this->v.str.len = 0;
		}

		this->type = ASE_AWK_VAL_NIL;
	}


	//////////////////////////////////////////////////////////////////
	// Awk::Run
	//////////////////////////////////////////////////////////////////
	
	Awk::Run::Run (run_t* run): run (run)
	{
	}

	//////////////////////////////////////////////////////////////////
	// Awk
	//////////////////////////////////////////////////////////////////

	Awk::Awk (): awk (ASE_NULL), functionMap (ASE_NULL), 
		sourceIn (Source::READ), sourceOut (Source::WRITE)
	{
	}

	Awk::~Awk ()
	{
	}

	int Awk::parse ()
	{
		ASE_ASSERT (awk != ASE_NULL);

		ase_awk_srcios_t srcios;

		srcios.in = sourceReader;
		srcios.out = sourceWriter;
		srcios.custom_data = this;

		return ase_awk_parse (awk, &srcios);
	}

	int Awk::run (const char_t* main, const char_t** args, size_t nargs)
	{
		ASE_ASSERT (awk != ASE_NULL);

		size_t i;
		ase_awk_runios_t runios;
		ase_awk_runcbs_t runcbs;
		ase_awk_runarg_t* runarg = ASE_NULL;

		runios.pipe        = pipeHandler;
		runios.coproc      = ASE_NULL;
		runios.file        = fileHandler;
		runios.console     = consoleHandler;
		runios.custom_data = this;

		runcbs.on_start     = onRunStart;
		runcbs.on_end       = onRunEnd;
		runcbs.on_return    = ASE_NULL;
		runcbs.on_statement = ASE_NULL;
		runcbs.custom_data  = this;

		if (nargs > 0)
		{
			runarg = (ase_awk_runarg_t*) ase_awk_malloc (
				awk, ASE_SIZEOF(ase_awk_runarg_t)*(nargs+1));

			if (runarg == ASE_NULL)
			{
				// TODO: SET ERROR INFO
				return -1;
			}

			for (i = 0; i < nargs; i++)
			{
				runarg[i].len = ase_strlen (args[i]);
				runarg[i].ptr = ase_awk_strxdup (awk, args[i], runarg[i].len);
				if (runarg[i].ptr == ASE_NULL)
				{
					if (i > 0)
					{
						for (i-- ; i > 0; i--)
						{
							ase_awk_free (awk, runarg[i].ptr);
						}
					}

					// TODO: SET ERROR INFO
					return -1;
				}
			}

			runarg[i].ptr = ASE_NULL;
			runarg[i].len = 0;
		}

		int n = ase_awk_run (
			awk, main, &runios, &runcbs, runarg, this);

		if (runarg != ASE_NULL) 
		{
			for (i--; i > 0; i--)
			{
				ase_awk_free (awk, runarg[i].ptr);
			}
			ase_awk_free (awk, runarg);
		}

		return n;
	}

	int Awk::open ()
	{
		ASE_ASSERT (awk == ASE_NULL && functionMap == ASE_NULL);

		ase_awk_prmfns_t prmfns;

		prmfns.mmgr.malloc      = allocMem;
		prmfns.mmgr.realloc     = reallocMem;
		prmfns.mmgr.free        = freeMem;
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

		prmfns.misc.pow         = pow;
		prmfns.misc.sprintf     = sprintf;
		prmfns.misc.dprintf     = dprintf;
		prmfns.misc.custom_data = this;

		awk = ase_awk_open (&prmfns, this);
		if (awk == ASE_NULL)
		{
			// TODO: SET ERROR INFO
			return -1;
		}

		functionMap = ase_awk_map_open (
			this, 512, freeFunctionMapValue, awk);
		if (functionMap == ASE_NULL)
		{
			// TODO: set ERROR INFO -> ENOMEM...
			ase_awk_close (awk);
			awk = ASE_NULL;
			return -1;
		}

		return 0;
	}

	void Awk::close ()
	{
		if (functionMap != ASE_NULL)
		{
			ase_awk_map_close (functionMap);
			functionMap = ASE_NULL;
		}

		if (awk != ASE_NULL) 
		{
			ase_awk_close (awk);
			awk = ASE_NULL;
		}
	}

	int Awk::dispatchFunction (
		run_t* run, const char_t* name, size_t len)
	{
		pair_t* pair;
		awk_t* awk;

		awk = ase_awk_getrunawk (run);

		pair = ase_awk_map_get (functionMap, name, len);
		if (pair == ASE_NULL) 
		{
			// TODO: SET ERROR INFO
			return -1;
		}

		FunctionHandler handler;
	       	handler = *(FunctionHandler*)ASE_AWK_PAIR_VAL(pair);	

		size_t i, nargs = ase_awk_getnargs(run);

		Argument* args = ASE_NULL;
		try { args = new Argument [nargs]; } catch (...)  {}
		if (args == ASE_NULL) 
		{
			// TODO: SET ERROR INFO
			return -1;
		}

		for (i = 0; i < nargs; i++)
		{
			ase_awk_val_t* v = ase_awk_getarg (run, i);
			if (args[i].init (run, v) == -1)
			{
				delete[] args;
				// TODO: SET ERROR INFO
				return -1;
			}
		}
		

		Return ret (run);
		int n = (this->*handler) (&ret, args, nargs);

		delete[] args;

		if (n <= -1) 
		{
			// TODO: SET ERROR INFO
			return -1;
		}	

		ase_awk_val_t* r = ret.toVal ();
		if (r == ASE_NULL) 
		{
			// TODO: SET ERROR INFO
			return -1;
		}

		ase_awk_setretval (run, r);
		return 0;
	}

	int Awk::addFunction (
		const char_t* name, size_t minArgs, size_t maxArgs, 
		FunctionHandler handler)
	{
		ASE_ASSERT (awk != ASE_NULL);

		FunctionHandler* tmp = (FunctionHandler*) 
			ase_awk_malloc (awk, ASE_SIZEOF(handler));
		if (tmp == ASE_NULL)
		{
			// TODO: SET ERROR INFO -> ENOMEM
			return -1;
		}

		//ase_memcpy (tmp, &handler, ASE_SIZEOF(handler));
		*tmp = handler;
		
		size_t nameLen = ase_strlen(name);

		void* p = ase_awk_addbfn (awk, name, nameLen,
		                          0, minArgs, maxArgs, ASE_NULL, 
		                          functionHandler);
		if (p == ASE_NULL) 
		{
			ase_awk_free (awk, tmp);
			return -1;
		}

		pair_t* pair = ase_awk_map_put (functionMap, name, nameLen, tmp);
		if (pair == ASE_NULL)
		{
			// TODO: SET ERROR INFO
			ase_awk_delbfn (awk, name, nameLen);
			ase_awk_free (awk, tmp);
			return -1;
		}

		return 0;
	}

	int Awk::deleteFunction (const char_t* name)
	{
		ASE_ASSERT (awk != ASE_NULL);

		size_t nameLen = ase_strlen(name);

		int n = ase_awk_delbfn (awk, name, nameLen);
		if (n == 0) ase_awk_map_remove (functionMap, name, nameLen);

		return n;
	}

	void Awk::onRunStart (const Run& run)
	{
	}

	void Awk::onRunEnd (const Run& run, int errnum)
	{
	}

	Awk::ssize_t Awk::sourceReader (
		int cmd, void* arg, char_t* data, size_t count)
	{
		Awk* awk = (Awk*)arg;
	
		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openSource (awk->sourceIn);
			case ASE_AWK_IO_CLOSE:
				return awk->closeSource (awk->sourceIn);
			case ASE_AWK_IO_READ:
				return awk->readSource (awk->sourceIn, data, count);
		}
	
		return -1;
	}

	Awk::ssize_t Awk::sourceWriter (
		int cmd, void* arg, char_t* data, size_t count)
	{
		Awk* awk = (Awk*)arg;
	
		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openSource (awk->sourceOut);
			case ASE_AWK_IO_CLOSE:
				return awk->closeSource (awk->sourceOut);
			case ASE_AWK_IO_WRITE:
				return awk->writeSource (awk->sourceOut, data, count);
		}
	
		return -1;
	}

	Awk::ssize_t Awk::pipeHandler (
		int cmd, void* arg, char_t* data, size_t count)
	{
		extio_t* extio = (extio_t*)arg;
		Awk* awk = (Awk*)extio->custom_data;

		ASE_ASSERT ((extio->type & 0xFF) == ASE_AWK_EXTIO_PIPE);

		Pipe pipe (extio);

		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openPipe (pipe);
			case ASE_AWK_IO_CLOSE:
				return awk->closePipe (pipe);

			case ASE_AWK_IO_READ:
				return awk->readPipe (pipe, data, count);
			case ASE_AWK_IO_WRITE:
				return awk->writePipe (pipe, data, count);

			case ASE_AWK_IO_FLUSH:
				return awk->flushPipe (pipe);

			case ASE_AWK_IO_NEXT:
				return -1;
		}

		return -1;
	}

	Awk::ssize_t Awk::fileHandler (
		int cmd, void* arg, char_t* data, size_t count)
	{
		extio_t* extio = (extio_t*)arg;
		Awk* awk = (Awk*)extio->custom_data;

		ASE_ASSERT ((extio->type & 0xFF) == ASE_AWK_EXTIO_FILE);

		File file (extio);

		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openFile (file);
			case ASE_AWK_IO_CLOSE:
				return awk->closeFile (file);

			case ASE_AWK_IO_READ:
				return awk->readFile (file, data, count);
			case ASE_AWK_IO_WRITE:
				return awk->writeFile (file, data, count);

			case ASE_AWK_IO_FLUSH:
				return awk->flushFile (file);

			case ASE_AWK_IO_NEXT:
				return -1;
		}

		return -1;
	}

	Awk::ssize_t Awk::consoleHandler (
		int cmd, void* arg, char_t* data, size_t count)
	{
		extio_t* extio = (extio_t*)arg;
		Awk* awk = (Awk*)extio->custom_data;

		ASE_ASSERT ((extio->type & 0xFF) == ASE_AWK_EXTIO_CONSOLE);

		Console console (extio);

		switch (cmd)
		{
			case ASE_AWK_IO_OPEN:
				return awk->openConsole (console);
			case ASE_AWK_IO_CLOSE:
				return awk->closeConsole (console);

			case ASE_AWK_IO_READ:
				return awk->readConsole (console, data, count);
			case ASE_AWK_IO_WRITE:
				return awk->writeConsole (console, data, count);

			case ASE_AWK_IO_FLUSH:
				return awk->flushConsole (console);
			case ASE_AWK_IO_NEXT:
				return awk->nextConsole (console);
		}

		return -1;
	}

	int Awk::functionHandler (
		run_t* run, const char_t* name, size_t len)
	{
		Awk* awk = (Awk*) ase_awk_getruncustomdata (run);
		return awk->dispatchFunction (run, name, len);
	}	

	void Awk::freeFunctionMapValue (void* owner, void* value)
	{
		Awk* awk = (Awk*)owner;
		ase_awk_free (awk->awk, value);
	}

	void Awk::onRunStart (run_t* run, void* custom)
	{
		Awk* awk = (Awk*)custom;
		awk->onRunStart (Run(run));
	}

	void Awk::onRunEnd (run_t* run, int errnum, void* custom)
	{
		Awk* awk = (Awk*)custom;
		awk->onRunEnd (Run(run), errnum);
	}

	void* Awk::allocMem (void* custom, size_t n)
	{
		return ((Awk*)custom)->allocMem (n);
	}

	void* Awk::reallocMem (void* custom, void* ptr, size_t n)
	{
		return ((Awk*)custom)->reallocMem (ptr, n);
	}

	void Awk::freeMem (void* custom, void* ptr)
	{
		((Awk*)custom)->freeMem (ptr);
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
