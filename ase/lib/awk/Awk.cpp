/*
 * $Id: Awk.cpp 468 2008-12-10 10:19:59Z baconevi $
 *
 * {License}
 */


#include <ase/awk/Awk.hpp>
#include <ase/cmn/str.h>
#include "../cmn/mem.h"

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

int Awk::Console::setFNR (long_t fnr)
{
	ase_awk_val_t* tmp;
	int n;

	tmp = ase_awk_makeintval (extio->run, fnr);
	if (tmp == ASE_NULL) return -1;

	ase_awk_refupval (extio->run, tmp);
	n = ase_awk_setglobal (extio->run, ASE_AWK_GLOBAL_FNR, tmp);
	ase_awk_refdownval (extio->run, tmp);

	return n;
}

Awk::Console::Mode Awk::Console::getMode () const
{
	return (Mode)extio->mode;
}

//////////////////////////////////////////////////////////////////
// Awk::Argument
//////////////////////////////////////////////////////////////////

Awk::Argument::Argument (Run& run): run (&run), val (ASE_NULL)
{
	this->inum = 0;
	this->rnum = 0.0;

	this->str.ptr = ASE_NULL;
	this->str.len = 0;
}

Awk::Argument::Argument (Run* run): run (run), val (ASE_NULL)
{
	this->inum = 0;
	this->rnum = 0.0;

	this->str.ptr = ASE_NULL;
	this->str.len = 0;
}

Awk::Argument::Argument (): run (ASE_NULL), val (ASE_NULL)
{
	this->inum = 0;
	this->rnum = 0.0;

	this->str.ptr = ASE_NULL;
	this->str.len = 0;
}

Awk::Argument::~Argument ()
{
	clear ();
}

void Awk::Argument::clear ()
{
	if (this->val == ASE_NULL)
	{
		/* case 1. not initialized.
		 * case 2. initialized with the second init.
		 * none of the cases creates a new string so the sttring
		 * that str.ptr is pointing at doesn't have to be freed */
		this->str.ptr = ASE_NULL;
		this->str.len = 0;
	}
	else if (ASE_AWK_VAL_TYPE(this->val) == ASE_AWK_VAL_MAP)
	{
		ASE_ASSERT (this->run != ASE_NULL);

		/* when the value is a map, str.ptr and str.len are
		 * used for index iteration in getFirstIndex & getNextIndex */
		ase_awk_refdownval (this->run->run, this->val);
		this->val = ASE_NULL;
	}
	else
	{
		ASE_ASSERT (this->run != ASE_NULL);

		if (this->str.ptr != ASE_NULL)
		{
			if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_STR)
			{
				awk_t* awk = this->run->awk->awk;
				ase_awk_free (awk, this->str.ptr);
			}

			this->str.ptr = ASE_NULL;
			this->str.len = 0;
		}

		if (this->val != ASE_NULL) 
		{
			ase_awk_refdownval (this->run->run, this->val);
			this->val = ASE_NULL;
		}

	}

	this->rnum = 0.0;
	this->inum = 0;
}

void* Awk::Argument::operator new (size_t n, awk_t* awk) throw ()
{
	void* ptr = ase_awk_alloc (awk, ASE_SIZEOF(awk) + n);
	if (ptr == ASE_NULL) return ASE_NULL;

	*(awk_t**)ptr = awk;
	return (char*)ptr+ASE_SIZEOF(awk);
}

void* Awk::Argument::operator new[] (size_t n, awk_t* awk) throw ()
{
	void* ptr = ase_awk_alloc (awk, ASE_SIZEOF(awk) + n);
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

int Awk::Argument::init (val_t* v)
{
	// this method is used internally only
	// and should never be called more than once 
	ASE_ASSERT (this->val == ASE_NULL);
	ASE_ASSERT (v != ASE_NULL);

	ase_awk_refupval (this->run->run, v);
	this->val = v;

	if (ASE_AWK_VAL_TYPE(v) == ASE_AWK_VAL_STR)
	{
		int n = ase_awk_valtonum (
			this->run->run, v, &this->inum, &this->rnum);
		if (n == 0) 
		{
			this->rnum = (ase_real_t)this->inum;
			this->str.ptr = ((ase_awk_val_str_t*)this->val)->buf;
			this->str.len = ((ase_awk_val_str_t*)this->val)->len;
			return 0;
		}
		else if (n == 1) 
		{
			this->inum = (ase_long_t)this->rnum;
			this->str.ptr = ((ase_awk_val_str_t*)this->val)->buf;
			this->str.len = ((ase_awk_val_str_t*)this->val)->len;
			return 0;
		}
	}
	else if (ASE_AWK_VAL_TYPE(v) == ASE_AWK_VAL_INT)
	{
		this->inum = ((ase_awk_val_int_t*)v)->val;
		this->rnum = (ase_real_t)((ase_awk_val_int_t*)v)->val;

		this->str.ptr = ase_awk_valtostr (
			this->run->run, v, 0, ASE_NULL, &this->str.len);
		if (this->str.ptr != ASE_NULL) return 0;
	}
	else if (ASE_AWK_VAL_TYPE(v) == ASE_AWK_VAL_REAL)
	{
		this->inum = (ase_long_t)((ase_awk_val_real_t*)v)->val;
		this->rnum = ((ase_awk_val_real_t*)v)->val;

		this->str.ptr = ase_awk_valtostr (
			this->run->run, v, 0, ASE_NULL, &this->str.len);
		if (this->str.ptr != ASE_NULL) return 0;
	}
	else if (ASE_AWK_VAL_TYPE(v) == ASE_AWK_VAL_NIL)
	{
		this->inum = 0;
		this->rnum = 0.0;

		this->str.ptr = ase_awk_valtostr (
			this->run->run, v, 0, ASE_NULL, &this->str.len);
		if (this->str.ptr != ASE_NULL) return 0;
	}
	else if (ASE_AWK_VAL_TYPE(v) == ASE_AWK_VAL_MAP)
	{
		this->inum = 0;
		this->rnum = 0.0;
		this->str.ptr = ASE_NULL;
		this->str.len = 0;
		return 0;
	}

	// an error has occurred
	ase_awk_refdownval (this->run->run, v);
	this->val = ASE_NULL;
	return -1;
}

int Awk::Argument::init (const char_t* str, size_t len)
{
	ASE_ASSERT (this->val == ASE_NULL);

	this->str.ptr = (char_t*)str;
	this->str.len = len;

	if (ase_awk_strtonum (this->run->run, 
		str, len, &this->inum, &this->rnum) == 0)
	{
		this->rnum = (real_t)this->inum;
	}
	else
	{
		this->inum = (long_t)this->rnum;
	}

	return 0;
}


Awk::long_t Awk::Argument::toInt () const
{
	return this->inum;
}

Awk::real_t Awk::Argument::toReal () const
{
	return this->rnum;
}

const Awk::char_t* Awk::Argument::toStr (size_t* len) const
{

	if (this->val != ASE_NULL && 
	    ASE_AWK_VAL_TYPE(this->val) == ASE_AWK_VAL_MAP)
	{
		*len = 0;
		return ASE_T("");
	}
	else if (this->str.ptr == ASE_NULL)
	{
		*len = 0;
		return ASE_T("");
	}
	else
	{
		*len = this->str.len;
		return this->str.ptr;
	}
}

bool Awk::Argument::isIndexed () const
{
	if (this->val == ASE_NULL) return false;
	return ASE_AWK_VAL_TYPE(this->val) == ASE_AWK_VAL_MAP;
}

int Awk::Argument::getIndexed (const char_t* idxptr, Awk::Argument& val) const
{
	return getIndexed (idxptr, ase_strlen(idxptr), val);
}

int Awk::Argument::getIndexed (
	const char_t* idxptr, size_t idxlen, Awk::Argument& val) const
{
	val.clear ();

	// not initialized yet. val is just nil. not an error
	if (this->val == ASE_NULL) return 0;
	// not a map. val is just nil. not an error 
	if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_MAP) return 0;

	// get the value from the map.
	ase_awk_val_map_t* m = (ase_awk_val_map_t*)this->val;
	pair_t* pair = ase_map_search (m->map, idxptr, idxlen);

	// the key is not found. it is not an error. val is just nil 
	if (pair == ASE_NULL) return 0; 

	// if val.init fails, it should return an error 
	return val.init ((val_t*)ASE_MAP_VPTR(pair));
}

int Awk::Argument::getIndexed (long_t idx, Argument& val) const
{
	val.clear ();

	// not initialized yet. val is just nil. not an error
	if (this->val == ASE_NULL) return 0;

	// not a map. val is just nil. not an error 
	if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_MAP) return 0;

	char_t ri[128];

	int rl = Awk::sprintf (
		this->run->awk, ri, ASE_COUNTOF(ri), 
	#if ASE_SIZEOF_LONG_LONG > 0
		ASE_T("%lld"), (long long)idx
	#elif ASE_SIZEOF___INT64 > 0
		ASE_T("%I64d"), (__int64)idx
	#elif ASE_SIZEOF_LONG > 0
		ASE_T("%ld"), (long)idx
	#elif ASE_SIZEOF_INT > 0
		ASE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		run->setError (ERR_INTERN, 0, ASE_NULL, 0);
		return -1;
	}

	// get the value from the map.
	ase_awk_val_map_t* m = (ase_awk_val_map_t*)this->val;
	pair_t* pair = ase_map_search (m->map, ri, rl);

	// the key is not found. it is not an error. val is just nil 
	if (pair == ASE_NULL) return 0; 

	// if val.init fails, it should return an error 
	return val.init ((val_t*)ASE_MAP_VPTR(pair));
}

int Awk::Argument::getFirstIndex (Awk::Argument& val) const
{
	val.clear ();

	if (this->val == ASE_NULL) return -1;
	if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_MAP) return -1;

	ase_size_t buckno;
	ase_awk_val_map_t* m = (ase_awk_val_map_t*)this->val;
	pair_t* pair = ase_map_getfirstpair (m->map, &buckno);
	if (pair == ASE_NULL) return 0; // no more key

	if (val.init (
		(ase_char_t*)ASE_MAP_KPTR(pair),
		ASE_MAP_KLEN(pair)) == -1) return -1;

	// reuse the string field as an interator.
	this->str.ptr = (char_t*)pair;
	this->str.len = buckno;

	return 1;
}

int Awk::Argument::getNextIndex (Awk::Argument& val) const
{
	val.clear ();

	if (this->val == ASE_NULL) return -1;
	if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_MAP) return -1;

	ase_awk_val_map_t* m = (ase_awk_val_map_t*)this->val;

	pair_t* pair = (pair_t*)this->str.ptr;
	ase_size_t buckno = this->str.len;
		
	pair = ase_map_getnextpair (m->map, pair, &buckno);
	if (pair == ASE_NULL) return 0;

	if (val.init (
		(ase_char_t*)ASE_MAP_KPTR(pair),
		ASE_MAP_KLEN(pair)) == -1) return -1;

	// reuse the string field as an interator.
	this->str.ptr = (char_t*)pair;
	this->str.len = buckno;
	return 1;
}

//////////////////////////////////////////////////////////////////
// Awk::Return
//////////////////////////////////////////////////////////////////

Awk::Return::Return (Run& run): run(&run), val(ase_awk_val_nil)
{
}

Awk::Return::Return (Run* run): run(run), val(ase_awk_val_nil)
{
}

Awk::Return::~Return ()
{
	clear ();
}

Awk::val_t* Awk::Return::toVal () const
{
	return this->val; 
}

Awk::Return::operator Awk::val_t* () const 
{
	return this->val; 
}

int Awk::Return::set (long_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	ase_awk_val_t* x = ase_awk_makeintval (this->run->run, v);
	if (x == ASE_NULL) return -1;

	ase_awk_refdownval (this->run->run, this->val);
	this->val = x;
	ase_awk_refupval (this->run->run, this->val);

	return 0;
}

int Awk::Return::set (real_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	ase_awk_val_t* x = ase_awk_makerealval (this->run->run, v);
	if (x == ASE_NULL) return -1;

	ase_awk_refdownval (this->run->run, this->val);
	this->val = x;
	ase_awk_refupval (this->run->run, this->val);

	return 0;
}

int Awk::Return::set (const char_t* ptr, size_t len)
{
	ASE_ASSERT (this->run != ASE_NULL);

	ase_awk_val_t* x = ase_awk_makestrval (this->run->run, ptr, len);
	if (x == ASE_NULL) return -1;

	ase_awk_refdownval (this->run->run, this->val);
	this->val = x;
	ase_awk_refupval (this->run->run, this->val);
	return 0;
}

bool Awk::Return::isIndexed () const
{
	if (this->val == ASE_NULL) return false;
	return ASE_AWK_VAL_TYPE(this->val) == ASE_AWK_VAL_MAP;
}

int Awk::Return::setIndexed (const char_t* idx, size_t iln, long_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	int opt = this->run->awk->getOption();
	if ((opt & OPT_MAPTOVAR) == 0)
	{
		/* refer to run_return in run.c */
		this->run->setError (ERR_MAPNOTALLOWED, 0, ASE_NULL, 0);
		return -1;
	}

	if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_MAP)
	{
		ase_awk_val_t* x = ase_awk_makemapval (this->run->run);
		if (x == ASE_NULL) return -1;

		ase_awk_refupval (this->run->run, x);

		ase_awk_val_t* x2 = ase_awk_makeintval (this->run->run, v);
		if (x2 == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x);
			return -1;
		}

		ase_awk_refupval (this->run->run, x2);

		pair_t* pair = ase_map_upsert (
			((ase_awk_val_map_t*)x)->map,
			(char_t*)idx, iln, x2, 0);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x2);
			ase_awk_refdownval (this->run->run, x);
			this->run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
			return -1;
		}

		ase_awk_refdownval (this->run->run, this->val);
		this->val = x;
	}
	else
	{
		ase_awk_val_t* x2 = ase_awk_makeintval (this->run->run, v);
		if (x2 == ASE_NULL) return -1;

		ase_awk_refupval (this->run->run, x2);

		pair_t* pair = ase_map_upsert (
			((ase_awk_val_map_t*)this->val)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x2);
			this->run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
			return -1;
		}
	}

	return 0;
}

int Awk::Return::setIndexed (const char_t* idx, size_t iln, real_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	int opt = this->run->awk->getOption();
	if ((opt & OPT_MAPTOVAR) == 0)
	{
		/* refer to run_return in run.c */
		this->run->setError (ERR_MAPNOTALLOWED, 0, ASE_NULL, 0);
		return -1;
	}

	if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_MAP)
	{
		ase_awk_val_t* x = ase_awk_makemapval (this->run->run);
		if (x == ASE_NULL) return -1;

		ase_awk_refupval (this->run->run, x);

		ase_awk_val_t* x2 = ase_awk_makerealval (this->run->run, v);
		if (x2 == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x);
			return -1;
		}

		ase_awk_refupval (this->run->run, x2);

		pair_t* pair = ase_map_upsert (
			((ase_awk_val_map_t*)x)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x2);
			ase_awk_refdownval (this->run->run, x);
			this->run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
			return -1;
		}

		ase_awk_refdownval (this->run->run, this->val);
		this->val = x;
	}
	else
	{
		ase_awk_val_t* x2 = ase_awk_makerealval (this->run->run, v);
		if (x2 == ASE_NULL) return -1;

		ase_awk_refupval (this->run->run, x2);

		pair_t* pair = ase_map_upsert (
			((ase_awk_val_map_t*)this->val)->map,
			(char_t*)idx, iln, x2, 0);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x2);
			this->run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
			return -1;
		}
	}

	return 0;
}

int Awk::Return::setIndexed (const char_t* idx, size_t iln, const char_t* str, size_t sln)
{
	ASE_ASSERT (this->run != ASE_NULL);

	int opt = this->run->awk->getOption();
	if ((opt & OPT_MAPTOVAR) == 0)
	{
		/* refer to run_return in run.c */
		this->run->setError (ERR_MAPNOTALLOWED, 0, ASE_NULL, 0);
		return -1;
	}

	if (ASE_AWK_VAL_TYPE(this->val) != ASE_AWK_VAL_MAP)
	{
		ase_awk_val_t* x = ase_awk_makemapval (this->run->run);
		if (x == ASE_NULL) return -1;

		ase_awk_refupval (this->run->run, x);

		ase_awk_val_t* x2 = ase_awk_makestrval (this->run->run, str, sln);
		if (x2 == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x);
			return -1;
		}

		ase_awk_refupval (this->run->run, x2);

		pair_t* pair = ase_map_upsert (
			((ase_awk_val_map_t*)x)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x2);
			ase_awk_refdownval (this->run->run, x);
			this->run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
			return -1;
		}

		ase_awk_refdownval (this->run->run, this->val);
		this->val = x;
	}
	else
	{
		ase_awk_val_t* x2 = ase_awk_makestrval (this->run->run, str, sln);
		if (x2 == ASE_NULL) return -1;

		ase_awk_refupval (this->run->run, x2);

		pair_t* pair = ase_map_upsert (
			((ase_awk_val_map_t*)this->val)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == ASE_NULL)
		{
			ase_awk_refdownval (this->run->run, x2);
			this->run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
			return -1;
		}
	}

	return 0;
}

int Awk::Return::setIndexed (long_t idx, long_t v)
{
	char_t ri[128];

	int rl = Awk::sprintf (
		this->run->awk, ri, ASE_COUNTOF(ri), 
	#if ASE_SIZEOF_LONG_LONG > 0
		ASE_T("%lld"), (long long)idx
	#elif ASE_SIZEOF___INT64 > 0
		ASE_T("%I64d"), (__int64)idx
	#elif ASE_SIZEOF_LONG > 0
		ASE_T("%ld"), (long)idx
	#elif ASE_SIZEOF_INT > 0
		ASE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		this->run->setError (ERR_INTERN, 0, ASE_NULL, 0);
		return -1;
	}

	return setIndexed (ri, rl, v);
}

int Awk::Return::setIndexed (long_t idx, real_t v)
{
	char_t ri[128];

	int rl = Awk::sprintf (
		this->run->awk, ri, ASE_COUNTOF(ri), 
	#if ASE_SIZEOF_LONG_LONG > 0
		ASE_T("%lld"), (long long)idx
	#elif ASE_SIZEOF___INT64 > 0
		ASE_T("%I64d"), (__int64)idx
	#elif ASE_SIZEOF_LONG > 0
		ASE_T("%ld"), (long)idx
	#elif ASE_SIZEOF_INT > 0
		ASE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		this->run->setError (ERR_INTERN, 0, ASE_NULL, 0);
		return -1;
	}

	return setIndexed (ri, rl, v);
}

int Awk::Return::setIndexed (long_t idx, const char_t* str, size_t sln)
{
	char_t ri[128];

	int rl = Awk::sprintf (
		this->run->awk, ri, ASE_COUNTOF(ri), 
	#if ASE_SIZEOF_LONG_LONG > 0
		ASE_T("%lld"), (long long)idx
	#elif ASE_SIZEOF___INT64 > 0
		ASE_T("%I64d"), (__int64)idx
	#elif ASE_SIZEOF_LONG > 0
		ASE_T("%ld"), (long)idx
	#elif ASE_SIZEOF_INT > 0
		ASE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		this->run->setError (ERR_INTERN, 0, ASE_NULL, 0);
		return -1;
	}

	return setIndexed (ri, rl, str, sln);
}

void Awk::Return::clear ()
{
	ase_awk_refdownval (this->run->run, this->val);
	this->val = ase_awk_val_nil;
}

//////////////////////////////////////////////////////////////////
// Awk::Run
//////////////////////////////////////////////////////////////////

Awk::Run::Run (Awk* awk): 
	awk (awk), run (ASE_NULL), callbackFailed (false)
{
}

Awk::Run::Run (Awk* awk, run_t* run): 
	awk (awk), run (run), callbackFailed (false), data (ASE_NULL)
{
	ASE_ASSERT (this->run != ASE_NULL);
}

Awk::Run::~Run ()
{
}

Awk::Run::operator Awk* () const 
{
	return this->awk;
}

Awk::Run::operator Awk::run_t* () const 
{
	return this->run;
}

void Awk::Run::stop () const
{
	ASE_ASSERT (this->run != ASE_NULL);
	ase_awk_stop (this->run);
}

bool Awk::Run::isStop () const
{
	ASE_ASSERT (this->run != ASE_NULL);
	return ase_awk_isstop (this->run)? true: false;
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

void Awk::Run::setError (ErrorCode code)
{
	ASE_ASSERT (this->run != ASE_NULL);
	ase_awk_setrunerror (this->run, code, 0, ASE_NULL, 0);
}

void Awk::Run::setError (ErrorCode code, size_t line)
{
	ASE_ASSERT (this->run != ASE_NULL);
	ase_awk_setrunerror (this->run, code, line, ASE_NULL, 0);
}

void Awk::Run::setError (ErrorCode code, size_t line, const char_t* arg)
{
	ASE_ASSERT (this->run != ASE_NULL);
	ase_cstr_t x = { arg, ase_strlen(arg) };
	ase_awk_setrunerror (this->run, code, line, &x, 1);
}

void Awk::Run::setError (
	ErrorCode code, size_t line, const char_t* arg, size_t len)
{
	ASE_ASSERT (this->run != ASE_NULL);
	ase_cstr_t x = { arg, len };
	ase_awk_setrunerror (this->run, code, line, &x, 1);
}

void Awk::Run::setErrorWithMessage (
	ErrorCode code, size_t line, const char_t* msg)
{
	ASE_ASSERT (this->run != ASE_NULL);
	ase_awk_setrunerrmsg (this->run, code, line, msg);
}

int Awk::Run::setGlobal (int id, long_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	ase_awk_val_t* tmp = ase_awk_makeintval (run, v);
	if (tmp == ASE_NULL) return -1;

	ase_awk_refupval (run, tmp);
	int n = ase_awk_setglobal (this->run, id, tmp);
	ase_awk_refdownval (run, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, real_t v)
{
	ASE_ASSERT (this->run != ASE_NULL);

	ase_awk_val_t* tmp = ase_awk_makerealval (run, v);
	if (tmp == ASE_NULL) return -1;

	ase_awk_refupval (run, tmp);
	int n = ase_awk_setglobal (this->run, id, tmp);
	ase_awk_refdownval (run, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, const char_t* ptr, size_t len)
{
	ASE_ASSERT (run != ASE_NULL);

	ase_awk_val_t* tmp = ase_awk_makestrval (run, ptr, len);
	if (tmp == ASE_NULL) return -1;

	ase_awk_refupval (run, tmp);
	int n = ase_awk_setglobal (this->run, id, tmp);
	ase_awk_refdownval (run, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, const Return& global)
{
	ASE_ASSERT (this->run != ASE_NULL);

	return ase_awk_setglobal (this->run, id, global.toVal());
}

int Awk::Run::getGlobal (int id, Argument& global) const
{
	ASE_ASSERT (this->run != ASE_NULL);

	global.clear ();
	return global.init (ase_awk_getglobal(this->run,id));
}

void Awk::Run::setCustom (void* data)
{
	this->data = data;
}

void* Awk::Run::getCustom () const
{
	return this->data;
}

//////////////////////////////////////////////////////////////////
// Awk
//////////////////////////////////////////////////////////////////

Awk::Awk (): awk (ASE_NULL), functionMap (ASE_NULL),
	sourceIn (Source::READ), sourceOut (Source::WRITE),
	errnum (ERR_NOERR), errlin (0), runCallback (false)

{
	this->errmsg[0] = ASE_T('\0');

	mmgr.alloc   = allocMem;
	mmgr.realloc = reallocMem;
	mmgr.free    = freeMem;
	mmgr.data    = this;

	ccls.is = isType;
	ccls.to = transCase;
	ccls.data = this;

	prmfns.pow         = pow;
	prmfns.sprintf     = sprintf;
	prmfns.data = this;
}

Awk::~Awk ()
{
}

Awk::operator Awk::awk_t* () const
{
	return this->awk;
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

void Awk::setError (ErrorCode code)
{
	setError (code, 0, ASE_NULL, 0);
}

void Awk::setError (ErrorCode code, size_t line)
{
	setError (code, line, ASE_NULL, 0);
}

void Awk::setError (ErrorCode code, size_t line, const char_t* arg)
{
	setError (code, line, arg, ase_strlen(arg));
}

void Awk::setError (ErrorCode code, size_t line, const char_t* arg, size_t len)
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

void Awk::setErrorWithMessage (ErrorCode code, size_t line, const char_t* msg)
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

	awk = ase_awk_open (&mmgr, 0);
	if (awk == ASE_NULL)
	{
		setError (ERR_NOMEM);
		return -1;
	}

	ase_awk_setccls (awk, &ccls);
	ase_awk_setprmfns (awk, &prmfns);

	
	//functionMap = ase_map_open (
	//	this, 512, 70, freeFunctionMapValue, ASE_NULL, 
	//	ase_awk_getmmgr(awk));
	functionMap = ase_map_open (
		ase_awk_getmmgr(awk), ASE_SIZEOF(this), 512, 70);
	if (functionMap == ASE_NULL)
	{
		ase_awk_close (awk);
		awk = ASE_NULL;

		setError (ERR_NOMEM);
		return -1;
	}

	*(Awk**)ase_map_getextension(functionMap) = this;
	ase_map_setcopier (functionMap, ASE_MAP_KEY, ASE_MAP_COPIER_INLINE);
	ase_map_setfreeer (functionMap, ASE_MAP_VAL, freeFunctionMapValue);
	ase_map_setscale (functionMap, ASE_MAP_KEY, ASE_SIZEOF(ase_char_t));

	int opt = 
		OPT_IMPLICIT |
		OPT_EXTIO | 
		OPT_NEWLINE | 
		OPT_BASEONE |
		OPT_PABLOCK;
	ase_awk_setoption (awk, opt);

	runCallback = false;
	return 0;
}

void Awk::close ()
{
	if (functionMap != ASE_NULL)
	{
		ase_map_close (functionMap);
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

int Awk::getWord (
	const char_t* ow, ase_size_t owl,
	const char_t** nw, ase_size_t* nwl)
{
	ASE_ASSERT (awk != ASE_NULL);
	return ase_awk_getword (awk, ow, owl, nw, nwl);
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
	srcios.data = this;

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

	// make sure that the run field is set in Awk::onRunStart.
	Run runctx (this);

	runios.pipe        = pipeHandler;
	runios.coproc      = ASE_NULL;
	runios.file        = fileHandler;
	runios.console     = consoleHandler;
	runios.data = this;

	ASE_MEMSET (&runcbs, 0, ASE_SIZEOF(runcbs));
	runcbs.on_start = onRunStart;
	if (runCallback)
	{
		runcbs.on_end       = onRunEnd;
		runcbs.on_return    = onRunReturn;
		runcbs.on_statement = onRunStatement;
	}
	runcbs.data = &runctx;
	
	if (nargs > 0)
	{
		runarg = (ase_awk_runarg_t*) ase_awk_alloc (
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
	
	int n = ase_awk_run (awk, main, &runios, &runcbs, runarg, &runctx);
	if (n == -1) retrieveError ();

	if (runarg != ASE_NULL) 
	{
		while (i > 0) ase_awk_free (awk, runarg[--i].ptr);
		ase_awk_free (awk, runarg);
	}

	return n;
}

void Awk::stop ()
{
	ASE_ASSERT (awk != ASE_NULL);
	ase_awk_stopall (awk);
}

int Awk::dispatchFunction (Run* run, const char_t* name, size_t len)
{
	pair_t* pair;
	awk_t* awk = run->awk->awk;

	//awk = ase_awk_getrunawk (run);

	pair = ase_map_search (functionMap, name, len);
	if (pair == ASE_NULL) 
	{
		run->setError (ERR_FNNONE, 0, name, len);
		return -1;
	}

	FunctionHandler handler;
       	handler = *(FunctionHandler*)ASE_MAP_VPTR(pair);	

	size_t i, nargs = ase_awk_getnargs(run->run);

	//Argument* args = ASE_NULL;
	//try { args = new Argument [nargs]; } catch (...)  {}
	Argument* args = new(awk) Argument[nargs];
	if (args == ASE_NULL) 
	{
		run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
		return -1;
	}

	for (i = 0; i < nargs; i++)
	{
		args[i].run = run; // dirty late initialization 
		                   // due to c++ array creation limitation.

		val_t* v = ase_awk_getarg (run->run, i);
		if (args[i].init (v) == -1)
		{
			run->setError (ERR_NOMEM, 0, ASE_NULL, 0);
			delete[] args;
			return -1;
		}
	}
	
	Return ret (run);

	int n = (this->*handler) (*run, ret, args, nargs, name, len);

	delete[] args;

	if (n <= -1) 
	{
		/* this is really the handler error. the underlying engine 
		 * will take care of the error code. */
		return -1;
	}

	ase_awk_setretval (run->run, ret);
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
		ase_awk_alloc (awk, ASE_SIZEOF(handler));
	if (tmp == ASE_NULL)
	{
		setError (ERR_NOMEM);
		return -1;
	}

	//ASE_MEMCPY (tmp, &handler, ASE_SIZEOF(handler));
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

	pair_t* pair = ase_map_upsert (
		functionMap, (char_t*)name, nameLen, tmp, 0);
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
	if (n == 0) ase_map_delete (functionMap, name, nameLen);
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

void Awk::triggerOnRunStart (Run& run)
{
	if (runCallback) onRunStart (run);
}

void Awk::onRunStart (Run& run)
{
}

void Awk::onRunEnd (Run& run)
{
}

void Awk::onRunReturn (Run& run, const Argument& ret)
{
}

void Awk::onRunStatement (Run& run, size_t line)
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
	Awk* awk = (Awk*)extio->data;

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
	Awk* awk = (Awk*)extio->data;

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
	Awk* awk = (Awk*)extio->data;

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
	Run* ctx = (Run*) ase_awk_getruncustomdata (run);
	Awk* awk = ctx->awk;
	return awk->dispatchFunction (ctx, name, len);
}	

void Awk::freeFunctionMapValue (map_t* map, void* dptr, size_t dlen)
{
	//Awk* awk = (Awk*)owner;
	Awk* awk = *(Awk**)ase_map_getextension(map);
	ase_awk_free (awk->awk, dptr);
}

void Awk::onRunStart (run_t* run, void* data)
{
	Run* r = (Run*)data;

	// the actual run_t value for the run-time callback is set here.
	// r here refers to runctx declared in Awk::run. As onRunStart
	// is executed immediately after the run method is invoked,
	// the run field can be set safely here. This seems to be the 
	// only place to acquire the run_t value safely as Awk::run 
	// is blocking.
	r->run = run; 

	r->callbackFailed = false;
	r->awk->triggerOnRunStart (*r);
}

void Awk::onRunEnd (run_t* run, int errnum, void* data)
{
	Run* r = (Run*)data;

	if (errnum == ERR_NOERR && r->callbackFailed)
	{
		ase_awk_setrunerrnum (r->run, ERR_NOMEM);
	}

	r->awk->onRunEnd (*r);
}

void Awk::onRunReturn (run_t* run, val_t* ret, void* data)
{
	Run* r = (Run*)data;
	if (r->callbackFailed) return;

	Argument x (r);
	if (x.init (ret) == -1)
	{
		r->callbackFailed = true;		
	}
	else
	{
		r->awk->onRunReturn (*r, x);
	}
}

void Awk::onRunStatement (run_t* run, size_t line, void* data)
{
	Run* r = (Run*)data;
	if (r->callbackFailed) return;
	r->awk->onRunStatement (*r, line);
}

void* Awk::allocMem (void* data, size_t n)
{
	return ((Awk*)data)->allocMem (n);
}

void* Awk::reallocMem (void* data, void* ptr, size_t n)
{
	return ((Awk*)data)->reallocMem (ptr, n);
}

void Awk::freeMem (void* data, void* ptr)
{
	((Awk*)data)->freeMem (ptr);
}

Awk::bool_t Awk::isType (void* data, cint_t c, ase_ccls_type_t type) 
{
	return ((Awk*)data)->isType (c, (ccls_type_t)type);
}

Awk::cint_t Awk::transCase (void* data, cint_t c, ase_ccls_type_t type) 
{
	return ((Awk*)data)->transCase (c, (ccls_type_t)type);
}

Awk::real_t Awk::pow (void* data, real_t x, real_t y)
{
	return ((Awk*)data)->pow (x, y);
}
	
int Awk::sprintf (void* data, char_t* buf, size_t size,
                  const char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	int n = ((Awk*)data)->vsprintf (buf, size, fmt, ap);
	va_end (ap);
	return n;
}

/////////////////////////////////
ASE_END_NAMESPACE(ASE)
/////////////////////////////////

