/*
 * $Id: Awk.cpp,v 1.59 2007/09/25 07:17:30 bacon Exp $
 *
 * {License}
 */


#include <ase/awk/Awk.hpp>
#include <ase/awk/val.h>
#include <ase/cmn/str.h>
#include <ase/cmn/mem.h>

/////////////////////////////////
ASE_BEGIN_NAMESPACE(ASE)
/////////////////////////////////

//////////////////////////////////////////////////////////////////
// Awk::Source
//////////////////////////////////////////////////////////////////

Awk::Source::Source (Mode mode): mode (mode), handle (ASE_NULL)
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

const Awk::extio_t* Awk::Extio::getRawExtio () const
{
	return extio;
}

const Awk::run_t* Awk::Extio::getRawRun () const
{
	return extio->run;
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
	clear ();
}

void Awk::Argument::clear ()
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
		this->val = ASE_NULL;
	}
}

void* Awk::Argument::operator new (size_t n, awk_t* awk) throw ()
{
	void* ptr = ase_awk_malloc (awk, ASE_SIZEOF(awk) + n);
	if (ptr == ASE_NULL) return ASE_NULL;

	*(awk_t**)ptr = awk;
	return (char*)ptr+ASE_SIZEOF(awk);
}

void* Awk::Argument::operator new[] (size_t n, awk_t* awk) throw ()
{
	void* ptr = ase_awk_malloc (awk, ASE_SIZEOF(awk) + n);
	if (ptr == ASE_NULL) return ASE_NULL;

	*(awk_t**)ptr = awk;
	return (char*)ptr+ASE_SIZEOF(awk);
}

#if !defined(__BORLANDC__)
void Awk::Argument::operator delete (void* ptr, awk_t* awk)
{
	ase_awk_free (awk, (char*)ptr-ASE_SIZEOF(awk));
}

void Awk::Argument::operator delete[] (void* ptr, awk_t* awk)
{
	ase_awk_free (awk, (char*)ptr-ASE_SIZEOF(awk));
}
#endif

void Awk::Argument::operator delete (void* ptr)
{
	void* p = (char*)ptr-ASE_SIZEOF(awk_t*);
	ase_awk_free (*(awk_t**)p, p);
}

void Awk::Argument::operator delete[] (void* ptr)
{
	void* p = (char*)ptr-ASE_SIZEOF(awk_t*);
	ase_awk_free (*(awk_t**)p, p);
}

int Awk::Argument::init (run_t* run, val_t* v)
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
	else if (v->type == ASE_AWK_VAL_MAP)
	{
		// TODO: support this propertly...
	}

	ase_awk_refdownval (run, v);
	this->run = ASE_NULL;
	this->val = ASE_NULL;
	return -1;
}

Awk::long_t Awk::Argument::toInt () const
{
	if (this->run == ASE_NULL || this->val == ASE_NULL) return 0;
	return this->inum;
}

Awk::real_t Awk::Argument::toReal () const
{
	if (this->run == ASE_NULL || this->val == ASE_NULL) return 0.0;
	return this->rnum;
}

const Awk::char_t* Awk::Argument::toStr (size_t* len) const
{
	if (this->run == ASE_NULL || this->val == ASE_NULL) 
	{
		*len = 0;
		return ASE_NULL;
	}

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

//////////////////////////////////////////////////////////////////
// Awk::Return
//////////////////////////////////////////////////////////////////

Awk::Return::Return (awk_t* awk): awk(awk), type (ASE_AWK_VAL_NIL)
{
}

Awk::Return::~Return ()
{
	clear ();
}

Awk::val_t* Awk::Return::toVal (run_t* run) const
{
	ASE_ASSERT (run != ASE_NULL);

	switch (this->type)
	{
		case ASE_AWK_VAL_NIL:
			return ase_awk_val_nil;

		case ASE_AWK_VAL_INT:
			return ase_awk_makeintval (run, this->v.inum);

		case ASE_AWK_VAL_REAL:
			return ase_awk_makerealval (run, this->v.rnum);

		case ASE_AWK_VAL_STR:
			return ase_awk_makestrval (
				run, this->v.str.ptr, this->v.str.len);
	}

	return ASE_NULL;
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

int Awk::Return::set (const char_t* ptr, size_t len)
{
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
		ase_awk_free (awk, this->v.str.ptr);
		this->v.str.ptr = ASE_NULL;
		this->v.str.len = 0;
	}

	this->type = ASE_AWK_VAL_NIL;
}

//////////////////////////////////////////////////////////////////
// Awk::Run
//////////////////////////////////////////////////////////////////

Awk::Run::Run (Awk* awk): 
	awk (awk), run (ASE_NULL), callbackFailed (false)
{
}

Awk::Run::Run (Awk* awk, run_t* run): 
	awk (awk), run (run), callbackFailed (false)
{
	ASE_ASSERT (this->run != ASE_NULL);
}

Awk::Run::~Run ()
{
}

int Awk::Run::stop () const
{
	ASE_ASSERT (this->run != ASE_NULL);
	return ase_awk_stop (this->run);
}

Awk::ErrorCode Awk::Run::getErrorCode () const
{
	ASE_ASSERT (this->run != ASE_NULL);
	return (ErrorCode)ase_awk_getrunerrnum (this->run);
}

Awk::size_t Awk::Run::getErrorLine () const
{
	ASE_ASSERT (this->run != ASE_NULL);
	return ase_awk_getrunerrlin (this->run);
}

const Awk::char_t* Awk::Run::getErrorMessage () const
{
	ASE_ASSERT (this->run != ASE_NULL);
	return ase_awk_getrunerrmsg (this->run);
}

int Awk::Run::setGlobal (int id, long_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	ase_awk_val_t* tmp = ase_awk_makeintval (run, v);
	if (tmp == ASE_NULL)
	{
		ase_awk_setrunerror (
			this->run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return -1;
	}

	ase_awk_refupval (run, tmp);
	int n = ase_awk_setglobal (this->run, id, tmp);
	ase_awk_refdownval (run, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, real_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	ase_awk_val_t* tmp = ase_awk_makerealval (run, v);
	if (tmp == ASE_NULL)
	{
		ase_awk_setrunerror (
			this->run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return -1;
	}

	ase_awk_refupval (run, tmp);
	int n = ase_awk_setglobal (this->run, id, tmp);
	ase_awk_refdownval (run, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, const char_t* ptr, size_t len)
{
	ASE_ASSERT (run != ASE_NULL);

	ase_awk_val_t* tmp = ase_awk_makestrval (run, ptr, len);
	if (tmp == ASE_NULL)
	{
		ase_awk_setrunerror (
			this->run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return -1;
	}

	ase_awk_refupval (run, tmp);
	int n = ase_awk_setglobal (this->run, id, tmp);
	ase_awk_refdownval (run, tmp);
	return n;
}

int Awk::Run::getGlobal (int id, Argument& global) const
{
	ASE_ASSERT (run != ASE_NULL);

	global.clear ();
	return global.init (run, ase_awk_getglobal(this->run,id));
}

//////////////////////////////////////////////////////////////////
// Awk
//////////////////////////////////////////////////////////////////

Awk::Awk (): awk (ASE_NULL), functionMap (ASE_NULL), 
	sourceIn (Source::READ), sourceOut (Source::WRITE),
	errnum (ERR_NOERR), errlin (0), runCallback (false)

{
	this->errmsg[0] = ASE_T('\0');
}

Awk::~Awk ()
{
}

Awk::ErrorCode Awk::getErrorCode () const
{
	return this->errnum;
}

Awk::size_t Awk::getErrorLine () const
{
	return this->errlin;
}

const Awk::char_t* Awk::getErrorMessage () const
{
	return this->errmsg;
}

void Awk::setError (
	ErrorCode code, size_t line, const char_t* arg, size_t len)
{
	if (awk != ASE_NULL)
	{
		ase_cstr_t x = { arg, len };
		ase_awk_seterror (awk, code, line, &x, 1);
		retrieveError ();
	}
	else
	{
		this->errnum = code;
		this->errlin = line;

		const char_t* es = ase_awk_geterrstr (ASE_NULL, code);
		ase_strxcpy (this->errmsg, ASE_COUNTOF(this->errmsg), es);
	}
}

void Awk::setError (
	ErrorCode code, size_t line, const char_t* msg)
{
	if (awk != ASE_NULL)
	{
		ase_awk_seterrmsg (awk, code, line, msg);
		retrieveError ();
	}
	else
	{
		this->errnum = code;
		this->errlin = line;
		ase_strxcpy (this->errmsg, ASE_COUNTOF(this->errmsg), msg);
	}
}

void Awk::clearError ()
{
	this->errnum = ERR_NOERR;
	this->errlin = 0;
	this->errmsg[0] = ASE_T('\0');
}

void Awk::retrieveError ()
{
	if (this->awk == ASE_NULL) 
	{
		clearError ();
	}
	else
	{
		int num;
		const char_t* msg;

		ase_awk_geterror (this->awk, &num, &this->errlin, &msg);
		this->errnum = (ErrorCode)num;
		ase_strxcpy (this->errmsg, ASE_COUNTOF(this->errmsg), msg);
	}
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
		setError (ERR_NOMEM);
		return -1;
	}

	functionMap = ase_awk_map_open (
		this, 512, 70, freeFunctionMapValue, awk);
	if (functionMap == ASE_NULL)
	{
		ase_awk_close (awk);
		awk = ASE_NULL;

		setError (ERR_NOMEM);
		return -1;
	}

	int opt = 
		OPT_IMPLICIT |
		OPT_EXPLICIT | 
		OPT_UNIQUEFN | 
		OPT_IDIV |
		OPT_SHADING | 
		OPT_SHIFT | 
		OPT_EXTIO | 
		OPT_BLOCKLESS | 
		OPT_BASEONE | 
		OPT_STRIPSPACES | 
		OPT_NEXTOFILE |
		OPT_ARGSTOMAIN;
	ase_awk_setoption (awk, opt);

	runCallback = false;
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

	clearError ();
	runCallback = false;
}

void Awk::setOption (int opt)
{
	ASE_ASSERT (awk != ASE_NULL);
	ase_awk_setoption (awk, opt);
}

int Awk::getOption () const
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_getoption (awk);
}

void Awk::setMaxDepth (int ids, size_t depth)
{
	ASE_ASSERT (awk != ASE_NULL);
	ase_awk_setmaxdepth (awk, ids, depth);
}

Awk::size_t Awk::getMaxDepth (int id) const
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_getmaxdepth (awk, id);
}

const Awk::char_t* Awk::getErrorString (ErrorCode num) const
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_geterrstr (awk, (int)num);
}

int Awk::setErrorString (ErrorCode num, const char_t* str)
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_seterrstr (awk, (int)num, str);
}

int Awk::setWord (const char_t* ow, const char_t* nw)
{
	return setWord (ow, ase_strlen(ow), nw, ase_strlen(nw));
}

int Awk::setWord (
	const char_t* ow, ase_size_t owl,
	const char_t* nw, ase_size_t nwl)
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_setword (awk, ow, owl, nw, nwl);
}

int Awk::unsetWord (const char_t* ow)
{
	return unsetWord (ow, ase_strlen(ow));
}

int Awk::unsetWord (const char_t* ow, ase_size_t owl)
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_setword (awk, ow, owl, ASE_NULL, 0);
}

int Awk::unsetAllWords ()
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_setword (awk, ASE_NULL, 0, ASE_NULL, 0);
}

int Awk::parse ()
{
	ASE_ASSERT (awk != ASE_NULL);

	ase_awk_srcios_t srcios;

	srcios.in = sourceReader;
	srcios.out = sourceWriter;
	srcios.custom_data = this;

	int n = ase_awk_parse (awk, &srcios);
	if (n == -1) retrieveError ();
	return n;
}

int Awk::run (const char_t* main, const char_t** args, size_t nargs)
{
	ASE_ASSERT (awk != ASE_NULL);

	size_t i;
	ase_awk_runios_t runios;
	ase_awk_runcbs_t runcbs;
	ase_awk_runarg_t* runarg = ASE_NULL;
	Run runForCallback (this);

	runios.pipe        = pipeHandler;
	runios.coproc      = ASE_NULL;
	runios.file        = fileHandler;
	runios.console     = consoleHandler;
	runios.custom_data = this;

	if (runCallback)
	{
		runcbs.on_start     = onRunStart;
		runcbs.on_end       = onRunEnd;
		runcbs.on_return    = onRunReturn;
		runcbs.on_statement = onRunStatement;
		runcbs.custom_data  = &runForCallback;
	}

	if (nargs > 0)
	{
		runarg = (ase_awk_runarg_t*) ase_awk_malloc (
			awk, ASE_SIZEOF(ase_awk_runarg_t)*(nargs+1));

		if (runarg == ASE_NULL)
		{
			setError (ERR_NOMEM);
			return -1;
		}

		for (i = 0; i < nargs; i++)
		{
			runarg[i].len = ase_strlen (args[i]);
			runarg[i].ptr = ase_awk_strxdup (awk, args[i], runarg[i].len);
			if (runarg[i].ptr == ASE_NULL)
			{
				while (i > 0) ase_awk_free (awk, runarg[--i].ptr);
				ase_awk_free (awk, runarg);
				setError (ERR_NOMEM);
				return -1;
			}
		}

		runarg[i].ptr = ASE_NULL;
		runarg[i].len = 0;
	}
	
	int n = ase_awk_run (
		awk, main, &runios, 
		(runCallback? &runcbs: ASE_NULL), 
		runarg, this);
	if (n == -1) retrieveError ();

	if (runarg != ASE_NULL) 
	{
		while (i > 0) ase_awk_free (awk, runarg[--i].ptr);
		ase_awk_free (awk, runarg);
	}

	return n;
}

int Awk::dispatchFunction (run_t* run, const char_t* name, size_t len)
{
	pair_t* pair;
	awk_t* awk;

	awk = ase_awk_getrunawk (run);

	pair = ase_awk_map_get (functionMap, name, len);
	if (pair == ASE_NULL) 
	{
		ase_cstr_t errarg;

		errarg.ptr = name;
		errarg.len = len;

		ase_awk_setrunerror (
			run, ASE_AWK_EFNNONE, 0, &errarg, 1);
		return -1;
	}

	FunctionHandler handler;
       	handler = *(FunctionHandler*)ASE_AWK_PAIR_VAL(pair);	

	size_t i, nargs = ase_awk_getnargs(run);

	//Argument* args = ASE_NULL;
	//try { args = new Argument [nargs]; } catch (...)  {}
	Argument* args = new(awk) Argument[nargs];
	if (args == ASE_NULL) 
	{
		ase_awk_setrunerror (
			run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return -1;
	}

	for (i = 0; i < nargs; i++)
	{
		val_t* v = ase_awk_getarg (run, i);
		if (args[i].init (run, v) == -1)
		{
			ase_awk_setrunerror (
				run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
			delete[] args;
			return -1;
		}
	}
	
	Run runForFunction (this, run);
	Return ret (awk);

	int n = (this->*handler) (runForFunction, ret, args, nargs, name, len);

	delete[] args;

	if (n <= -1) 
	{
		/* this is really the handler error. the underlying engine 
		 * will take care of the error code. */
		return -1;
	}

	val_t* r = ret.toVal (run);
	if (r == ASE_NULL) 
	{
		ase_awk_setrunerror (
			run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

int Awk::addGlobal (const char_t* name)
{
	ASE_ASSERT (awk != ASE_NULL);

	int n = ase_awk_addglobal (awk, name, ase_strlen(name));
	if (n == -1) retrieveError ();
	return n;
}

int Awk::deleteGlobal (const char_t* name)
{
	ASE_ASSERT (awk != ASE_NULL);
	int n = ase_awk_delglobal (awk, name, ase_strlen(name));
	if (n == -1) retrieveError ();
	return n;
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
		setError (ERR_NOMEM);
		return -1;
	}

	//ase_memcpy (tmp, &handler, ASE_SIZEOF(handler));
	*tmp = handler;
	
	size_t nameLen = ase_strlen(name);

	void* p = ase_awk_addfunc (awk, name, nameLen,
	                          0, minArgs, maxArgs, ASE_NULL, 
	                          functionHandler);
	if (p == ASE_NULL) 
	{
		ase_awk_free (awk, tmp);
		retrieveError ();
		return -1;
	}

	pair_t* pair = ase_awk_map_put (functionMap, name, nameLen, tmp);
	if (pair == ASE_NULL)
	{
		ase_awk_delfunc (awk, name, nameLen);
		ase_awk_free (awk, tmp);

		setError (ERR_NOMEM);
		return -1;
	}

	return 0;
}

int Awk::deleteFunction (const char_t* name)
{
	ASE_ASSERT (awk != ASE_NULL);

	size_t nameLen = ase_strlen(name);

	int n = ase_awk_delfunc (awk, name, nameLen);
	if (n == 0) ase_awk_map_remove (functionMap, name, nameLen);
	else retrieveError ();

	return n;
}

void Awk::enableRunCallback ()
{
	runCallback = true;
}

void Awk::disableRunCallback ()
{
	runCallback = false;
}

void Awk::onRunStart (const Run& run)
{
}

void Awk::onRunEnd (const Run& run)
{
}

void Awk::onRunReturn (const Run& run, const Argument& ret)
{
}

void Awk::onRunStatement (const Run& run, size_t line)
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
	Run* r = (Run*)custom;

	// the actual run_t value for the run-time callback is set here.
	// r here refers to runForCallback declared in Awk::run and is 
	// different from a Run instance available from intrinsic function 
	// handlers (runForFunction in dispatchFunction). however, all methods 
	// of the Run class will still work as intended in all places once 
	// r->run is set properly here.
	// NOTE: I admit this strategy is ugly.
	r->run = run; 

	r->callbackFailed = false;
	r->awk->onRunStart (*r);
}

void Awk::onRunEnd (run_t* run, int errnum, void* custom)
{
	Run* r = (Run*)custom;

	if (errnum == ERR_NOERR && r->callbackFailed)
	{
		ase_awk_setrunerror (
			r->run, ASE_AWK_ENOMEM, 0, ASE_NULL, 0);
	}

	r->awk->onRunEnd (*r);
}

void Awk::onRunReturn (run_t* run, val_t* ret, void* custom)
{
	Run* r = (Run*)custom;
	if (r->callbackFailed) return;

	Argument x;
	if (x.init (run, ret) == -1)
	{
		r->callbackFailed = true;		
	}
	else
	{
		r->awk->onRunReturn (*r, x);
	}
}

void Awk::onRunStatement (run_t* run, size_t line, void* custom)
{
	Run* r = (Run*)custom;
	if (r->callbackFailed) return;
	r->awk->onRunStatement (*r, line);
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

/////////////////////////////////
ASE_END_NAMESPACE(ASE)
/////////////////////////////////

