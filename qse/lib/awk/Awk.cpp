/*
 * $Id: Awk.cpp 259 2009-08-20 11:28:03Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/awk/Awk.hpp>
#include <qse/cmn/str.h>
#include "../cmn/mem.h"
#include "awk.h"

// enable this once addFunction() is extended with argument spec (rxv...).
//#define PASS_BY_REFERENCE

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

//////////////////////////////////////////////////////////////////
// Awk::Source
//////////////////////////////////////////////////////////////////

struct xtn_t
{
	Awk* awk;
};

struct rxtn_t
{
	Awk::Run* run;
};

Awk::NoSource Awk::Source::NONE;

//////////////////////////////////////////////////////////////////
// Awk::RIO
//////////////////////////////////////////////////////////////////

Awk::RIOBase::RIOBase (Run* run, rio_arg_t* riod): run (run), riod (riod)
{
}

const Awk::char_t* Awk::RIOBase::getName () const
{
	return this->riod->name;
}

const void* Awk::RIOBase::getHandle () const
{
	return this->riod->handle;
}

void Awk::RIOBase::setHandle (void* handle)
{
	this->riod->handle = handle;
}

Awk::RIOBase::operator Awk* () const 
{
	return this->run->awk;
}

Awk::RIOBase::operator Awk::awk_t* () const 
{
	QSE_ASSERT (qse_awk_rtx_getawk(this->run->rtx) == this->run->awk->awk);
	return this->run->awk->awk;
}

Awk::RIOBase::operator Awk::rio_arg_t* () const
{
	return this->riod;
}

Awk::RIOBase::operator Awk::Run* () const
{
	return this->run;
}

Awk::RIOBase::operator Awk::rtx_t* () const
{
	return this->run->rtx;
}

//////////////////////////////////////////////////////////////////
// Awk::Pipe
//////////////////////////////////////////////////////////////////

Awk::Pipe::Pipe (Run* run, rio_arg_t* riod): RIOBase (run, riod)
{
}

Awk::Pipe::Mode Awk::Pipe::getMode () const
{
	return (Mode)riod->mode;
}

//////////////////////////////////////////////////////////////////
// Awk::File
//////////////////////////////////////////////////////////////////

Awk::File::File (Run* run, rio_arg_t* riod): RIOBase (run, riod)
{
}

Awk::File::Mode Awk::File::getMode () const
{
	return (Mode)riod->mode;
}

//////////////////////////////////////////////////////////////////
// Awk::Console
//////////////////////////////////////////////////////////////////

Awk::Console::Console (Run* run, rio_arg_t* riod): 
	RIOBase (run, riod), filename (QSE_NULL)
{
}

Awk::Console::~Console ()
{
	if (filename != QSE_NULL)
	{
		qse_awk_free ((awk_t*)this, filename);
	}
}

int Awk::Console::setFileName (const char_t* name)
{
	if (this->getMode() == READ)
	{
		return qse_awk_rtx_setfilename (
			this->run->rtx, name, qse_strlen(name));
	}
	else
	{
		return qse_awk_rtx_setofilename (
			this->run->rtx, name, qse_strlen(name));
	}
}

int Awk::Console::setFNR (long_t fnr)
{
	val_t* tmp;
	int n;

	tmp = qse_awk_rtx_makeintval (this->run->rtx, fnr);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (this->run->rtx, tmp);
	n = qse_awk_rtx_setgbl (this->run->rtx, QSE_AWK_GBL_FNR, tmp);
	qse_awk_rtx_refdownval (this->run->rtx, tmp);

	return n;
}

Awk::Console::Mode Awk::Console::getMode () const
{
	return (Mode)riod->mode;
}

//////////////////////////////////////////////////////////////////
// Awk::Value
//////////////////////////////////////////////////////////////////

const Awk::char_t* Awk::Value::EMPTY_STRING = QSE_T("");
Awk::Value::IndexIterator Awk::Value::IndexIterator::END;

Awk::Value::IntIndex::IntIndex (long_t x)
{
	ptr = buf;
	len = 0;

#define NTOC(n) ((n) + QSE_T('0'))

	int base = 10;
        long_t last = x % base;
        long_t y = 0;
        int dig = 0;

	if (x < 0) buf[len++] = QSE_T('-');

	x = x / base;
	if (x < 0) x = -x;

	while (x > 0)
	{
		y = y * base + (x % base);
		x = x / base;
		dig++;
	}

        while (y > 0)
        {
		buf[len++] = NTOC (y % base);
		y = y / base;
		dig--;
	}

	while (dig > 0)
	{
		dig--;
		buf[len++] = QSE_T('0');
	}
	if (last < 0) last = -last;
	buf[len++] = NTOC(last);

	buf[len] = QSE_T('\0');

#undef NTOC
}

void* Awk::Value::operator new (size_t n, Run* run) throw ()
{
	void* ptr = qse_awk_rtx_alloc (run->rtx, QSE_SIZEOF(run) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(Run**)ptr = run;
	return (char*)ptr+QSE_SIZEOF(run);
}

void* Awk::Value::operator new[] (size_t n, Run* run) throw () 
{
	void* ptr = qse_awk_rtx_alloc (run->rtx, QSE_SIZEOF(run) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(Run**)ptr = run;
	return (char*)ptr+QSE_SIZEOF(run);
}

#if !defined(__BORLANDC__)
void Awk::Value::operator delete (void* ptr, Run* run) 
{
	qse_awk_rtx_free (run->rtx, (char*)ptr-QSE_SIZEOF(run));
}

void Awk::Value::operator delete[] (void* ptr, Run* run) 
{
	qse_awk_rtx_free (run->rtx, (char*)ptr-QSE_SIZEOF(run));
}
#endif

void Awk::Value::operator delete (void* ptr) 
{
	void* p = (char*)ptr-QSE_SIZEOF(Run*);
	qse_awk_rtx_free ((*(Run**)p)->rtx, p);
}

void Awk::Value::operator delete[] (void* ptr) 
{
	void* p = (char*)ptr-QSE_SIZEOF(Run*);
	qse_awk_rtx_free ((*(Run**)p)->rtx, p);
}

Awk::Value::Value (): run (QSE_NULL), val (qse_awk_val_nil) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
}

Awk::Value::Value (Run& run): run (&run), val (qse_awk_val_nil) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
}

Awk::Value::Value (Run* run): run (run), val (qse_awk_val_nil) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
}

Awk::Value::Value (const Value& v): run (v.run), val (v.val)
{
	if (run != QSE_NULL)
		qse_awk_rtx_refupval (run->rtx, val);

	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
}

Awk::Value::~Value ()
{
	if (run != QSE_NULL)
	{
		qse_awk_rtx_refdownval (run->rtx, val);
		if (cached.str.ptr != QSE_NULL)
			qse_awk_rtx_free (run->rtx, cached.str.ptr);
	}
}

Awk::Value& Awk::Value::operator= (const Value& v)
{
	if (this == &v) return *this;

	if (run != QSE_NULL)
	{
		qse_awk_rtx_refdownval (run->rtx, val);
		if (cached.str.ptr != QSE_NULL)
		{
			qse_awk_rtx_free (run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}
	}

	run = v.run;
	val = v.val;

	if (run != QSE_NULL)
		qse_awk_rtx_refupval (run->rtx, val);

	return *this;
}
	
void Awk::Value::clear ()
{
	if (run != QSE_NULL)
	{
		qse_awk_rtx_refdownval (run->rtx, val);

		if (cached.str.ptr != QSE_NULL)
		{
			qse_awk_rtx_free (run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}

		run = QSE_NULL;
		val = qse_awk_val_nil;
	}
}

Awk::Value::operator Awk::long_t () const
{
	long_t v;
	if (getInt (&v) <= -1) v = 0;
	return v;
}

Awk::Value::operator Awk::real_t () const
{
	real_t v;
	if (getReal (&v) <= -1) v = 0.0;
	return v;
}

Awk::Value::operator const Awk::char_t* () const
{
	const Awk::char_t* ptr;
	size_t len;
	if (Awk::Value::getStr (&ptr, &len) <= -1) ptr = QSE_T("");
	return ptr;
}

int Awk::Value::getInt (long_t* v) const
{
	long_t lv = 0;

	QSE_ASSERT (val != QSE_NULL);

	if (run != QSE_NULL && 
	    val->type != QSE_AWK_VAL_NIL &&
	    val->type != QSE_AWK_VAL_MAP)
	{
		real_t rv;
		int n = qse_awk_rtx_valtonum (run->rtx, val, &lv, &rv);
		if (n <= -1) 
		{
			run->awk->retrieveError (run);
			return -1;
		}
		if (n >= 1) lv = rv;
	}

	*v = lv;
	return 0;
}

int Awk::Value::getReal (real_t* v) const
{
	real_t rv = 0;

	QSE_ASSERT (val != QSE_NULL);

	if (run != QSE_NULL && 
	    val->type != QSE_AWK_VAL_NIL &&
	    val->type != QSE_AWK_VAL_MAP)
	{
		long_t lv;
		int n = qse_awk_rtx_valtonum (run->rtx, val, &lv, &rv);
		if (n <= -1)
		{
			run->awk->retrieveError (run);
			return -1;
		}
		if (n == 0) rv = lv;
	}

	*v = rv;
	return 0;
}

int Awk::Value::getStr (const char_t** str, size_t* len) const
{
	const char_t* p = EMPTY_STRING;
	size_t l = 0;

	QSE_ASSERT (val != QSE_NULL);

	if (run != QSE_NULL && 
	    val->type != QSE_AWK_VAL_NIL &&
	    val->type != QSE_AWK_VAL_MAP)
	{
		if (val->type == QSE_AWK_VAL_STR)
		{
			p = ((qse_awk_val_str_t*)val)->ptr;
			l = ((qse_awk_val_str_t*)val)->len;
		}
		else
		{
			if (cached.str.ptr == QSE_NULL)
			{
				qse_awk_rtx_valtostr_out_t out;
				out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
				if (qse_awk_rtx_valtostr (
					run->rtx, val, &out) == QSE_NULL)
				{
					run->awk->retrieveError (run);
					return -1;
				}

				p = out.u.cpldup.ptr;
				l = out.u.cpldup.len;

				cached.str.ptr = out.u.cpldup.ptr;
				cached.str.len = out.u.cpldup.len;
			}
			else
			{
				p = cached.str.ptr;
				l = cached.str.len;
			}
		}
	}

	*str = p;
	*len = l;

	return 0;
}

int Awk::Value::setVal (val_t* v)
{
	if (run == QSE_NULL) return -1;
	return setVal (run, v);
}

int Awk::Value::setVal (Run* r, val_t* v)
{
	if (run != QSE_NULL)
	{
		qse_awk_rtx_refdownval (run->rtx, val);
		if (cached.str.ptr != QSE_NULL)
		{
			qse_awk_rtx_free (run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}
	}

	QSE_ASSERT (cached.str.ptr == QSE_NULL);
	qse_awk_rtx_refupval (r->rtx, v);

	run = r;
	val = v;

	return 0;
}

int Awk::Value::setInt (long_t v)
{
	if (run == QSE_NULL) return -1;
	return setInt (run, v);
}

int Awk::Value::setInt (Run* r, long_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makeintval (r->rtx, v);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = setVal (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setReal (real_t v)
{
	if (run == QSE_NULL) return -1;
	return setReal (run, v);
}

int Awk::Value::setReal (Run* r, real_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makerealval (r->rtx, v);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}
			
	int n = setVal (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setStr (const char_t* str, size_t len)
{
	if (run == QSE_NULL) return -1;
	return setStr (run, str, len);
}

int Awk::Value::setStr (Run* r, const char_t* str, size_t len)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makestrval (r->rtx, str, len);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = setVal (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setStr (const char_t* str)
{
	if (run == QSE_NULL) return -1;
	return setStr (run, str);
}

int Awk::Value::setStr (Run* r, const char_t* str)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makestrval0 (r->rtx, str);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = setVal (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setIndexedVal (const Index& idx, val_t* v)
{
	if (run == QSE_NULL) return -1;
	return setIndexedVal (run, idx, v);
}

int Awk::Value::setIndexedVal (Run* r, const Index& idx, val_t* v)
{
	QSE_ASSERT (r != QSE_NULL);

	if (val->type != QSE_AWK_VAL_MAP)
	{
		/* the previous value is not a map. 
		 * a new map value needs to be created first */
		val_t* map = qse_awk_rtx_makemapval (r->rtx);
		if (map == QSE_NULL) 
		{
			r->awk->retrieveError (r);
			return -1;
		}

		qse_awk_rtx_refupval (r->rtx, map);
		qse_awk_rtx_refupval (r->rtx, v);

		/* update the map with a given value */
		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)map)->map, 
			(char_t*)idx.ptr, idx.len, v, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (r->rtx, v);
			qse_awk_rtx_refdownval (r->rtx, map);
			r->setError (ERR_NOMEM);
			r->awk->retrieveError (r);
			return -1;
		}

		if (run != QSE_NULL)
			qse_awk_rtx_refdownval (run->rtx, val);

		run = r;
		val = map;
	}
	else
	{
		QSE_ASSERT (run != QSE_NULL);

		// if the previous value is a map, things are a bit simpler 
		// however it needs to check if the runtime context matches
		// with the previous one.
		if (run != r) 
		{
			// it can't span across multiple runtime contexts
			run->setError (ERR_INVAL);
			run->awk->retrieveError (run);
			return -1;
		}

		qse_awk_rtx_refupval (r->rtx, v);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)val)->map, 
			(char_t*)idx.ptr, idx.len, v, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (r->rtx, v);
			run->setError (ERR_NOMEM);
			run->awk->retrieveError (run);
			return -1;
		}
	}

	return 0;
}

int Awk::Value::setIndexedInt (const Index& idx, long_t v)
{
	if (run == QSE_NULL) return -1;
	return setIndexedInt (run, idx, v);
}

int Awk::Value::setIndexedInt (Run* r, const Index& idx, long_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makeintval (r->rtx, v);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = setIndexedVal (r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedReal (const Index& idx, real_t v)
{
	if (run == QSE_NULL) return -1;
	return setIndexedReal (run, idx, v);
}

int Awk::Value::setIndexedReal (Run* r, const Index& idx, real_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makerealval (r->rtx, v);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = setIndexedVal (r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedStr (const Index& idx, const char_t* str, size_t len)
{
	if (run == QSE_NULL) return -1;
	return setIndexedStr (run, idx, str, len);
}

int Awk::Value::setIndexedStr (
	Run* r, const Index& idx, const char_t* str, size_t len)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makestrval (r->rtx, str, len);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = setIndexedVal (r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedStr (const Index& idx, const char_t* str)
{
	if (run == QSE_NULL) return -1;
	return setIndexedStr (run, idx, str);
}

int Awk::Value::setIndexedStr (Run* r, const Index& idx, const char_t* str)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makestrval0 (r->rtx, str);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = setIndexedVal (r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}


bool Awk::Value::isIndexed () const
{
	QSE_ASSERT (val != QSE_NULL);
	return val->type == QSE_AWK_VAL_MAP;
}

int Awk::Value::getIndexed (const Index& idx, Value* v) const
{
	QSE_ASSERT (val != QSE_NULL);

	// not a map. v is just nil. not an error 
	if (val->type != QSE_AWK_VAL_MAP) 
	{	
		v->clear ();
		return 0;
	}

	// get the value from the map.
	qse_awk_val_map_t* m = (qse_awk_val_map_t*)val;
	pair_t* pair = qse_map_search (m->map, idx.ptr, idx.len);

	// the key is not found. it is not an error. v is just nil 
	if (pair == QSE_NULL) 
	{
		v->clear ();
		return 0; 
	}

	// if v.set fails, it should return an error 
	return v->setVal (run, (val_t*)QSE_MAP_VPTR(pair));
}

Awk::Value::IndexIterator Awk::Value::getFirstIndex (Index* idx) const
{
	QSE_ASSERT (val != QSE_NULL);

	if (val->type != QSE_AWK_VAL_MAP) return IndexIterator::END;

	size_t buckno;
	qse_awk_val_map_t* m = (qse_awk_val_map_t*)val;
	pair_t* pair = qse_map_getfirstpair (m->map, &buckno);
	if (pair == QSE_NULL) return IndexIterator::END; // no more key

	idx->ptr = (const char_t*)QSE_MAP_KPTR(pair);
	idx->len = QSE_MAP_KLEN(pair);

	return IndexIterator (pair, buckno);	
}

Awk::Value::IndexIterator Awk::Value::getNextIndex (
	Index* idx, const IndexIterator& iter) const
{
	QSE_ASSERT (val != QSE_NULL);

	if (val->type != QSE_AWK_VAL_MAP) return IndexIterator::END;

	qse_awk_val_map_t* m = (qse_awk_val_map_t*)val;

	pair_t* pair = (pair_t*)iter.pair;
	size_t buckno = iter.buckno;
		
	pair = qse_map_getnextpair (m->map, pair, &buckno);
	if (pair == QSE_NULL) return IndexIterator::END;

	idx->ptr = (const char_t*)QSE_MAP_KPTR(pair); 
	idx->len = QSE_MAP_KLEN(pair);

	return IndexIterator (pair, buckno);	
}

#if 0
int Awk::Argument::getFirstIndex (Awk::Argument& val) const
{
	val.clear ();

	if (this->val == QSE_NULL) return -1;
	if (this->val->type != QSE_AWK_VAL_MAP) return -1;

	qse_size_t buckno;
	qse_awk_val_map_t* m = (qse_awk_val_map_t*)this->val;
	pair_t* pair = qse_map_getfirstpair (m->map, &buckno);
	if (pair == QSE_NULL) return 0; // no more key

	if (val.init (
		(qse_char_t*)QSE_MAP_KPTR(pair),
		QSE_MAP_KLEN(pair)) == -1) return -1;

	// reuse the string field as an interator.
	this->str.ptr = (char_t*)pair;
	this->str.len = buckno;

	return 1;
}

int Awk::Argument::getNextIndex (Awk::Argument& val) const
{
	val.clear ();

	if (this->val == QSE_NULL) return -1;
	if (this->val->type != QSE_AWK_VAL_MAP) return -1;

	qse_awk_val_map_t* m = (qse_awk_val_map_t*)this->val;

	pair_t* pair = (pair_t*)this->str.ptr;
	qse_size_t buckno = this->str.len;
		
	pair = qse_map_getnextpair (m->map, pair, &buckno);
	if (pair == QSE_NULL) return 0;

	if (val.init (
		(qse_char_t*)QSE_MAP_KPTR(pair),
		QSE_MAP_KLEN(pair)) == -1) return -1;

	// reuse the string field as an interator.
	this->str.ptr = (char_t*)pair;
	this->str.len = buckno;
	return 1;
}

#endif

//////////////////////////////////////////////////////////////////
// Awk::Run
//////////////////////////////////////////////////////////////////

Awk::Run::Run (Awk* awk): awk (awk), rtx (QSE_NULL)
{
}

Awk::Run::Run (Awk* awk, rtx_t* rtx): awk (awk), rtx (rtx)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
}

Awk::Run::~Run ()
{
}

Awk::Run::operator Awk* () const 
{
	return this->awk;
}

Awk::Run::operator Awk::rtx_t* () const 
{
	return this->rtx;
}

void Awk::Run::stop () const 
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_awk_rtx_stop (this->rtx);
}

bool Awk::Run::shouldStop () const
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_shouldstop (this->rtx)? true: false;
}

Awk::ErrorNumber Awk::Run::getErrorNumber () const 
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return (ErrorNumber)qse_awk_rtx_geterrnum (this->rtx);
}

Awk::size_t Awk::Run::getErrorLine () const 
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_geterrlin (this->rtx);
}

const Awk::char_t* Awk::Run::getErrorMessage () const  
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_geterrmsg (this->rtx);
}

void Awk::Run::setError (ErrorNumber code, const cstr_t* args, size_t line)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_awk_rtx_seterror (this->rtx, (errnum_t)code, args, line);
}

void Awk::Run::setErrorWithMessage (
	ErrorNumber code, const char_t* msg, size_t line)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	errinf_t errinf;

	errinf.num = (errnum_t)code;
	errinf.lin = line;
	qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), msg);

	qse_awk_rtx_seterrinf (this->rtx, &errinf);
}

int Awk::Run::setGlobal (int id, long_t v)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	val_t* tmp = qse_awk_rtx_makeintval (this->rtx, v);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (this->rtx, tmp);
	int n = qse_awk_rtx_setgbl (this->rtx, id, tmp);
	qse_awk_rtx_refdownval (this->rtx, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, real_t v)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	val_t* tmp = qse_awk_rtx_makerealval (this->rtx, v);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (this->rtx, tmp);
	int n = qse_awk_rtx_setgbl (this->rtx, id, tmp);
	qse_awk_rtx_refdownval (this->rtx, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, const char_t* ptr, size_t len)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	val_t* tmp = qse_awk_rtx_makestrval (this->rtx, ptr, len);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (this->rtx, tmp);
	int n = qse_awk_rtx_setgbl (this->rtx, id, tmp);
	qse_awk_rtx_refdownval (this->rtx, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, const Value& gbl)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_setgbl (this->rtx, id, (val_t*)gbl);
}

int Awk::Run::getGlobal (int id, Value& g) const
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return g.setVal ((Run*)this, qse_awk_rtx_getgbl (this->rtx, id));
}

//////////////////////////////////////////////////////////////////
// Awk
//////////////////////////////////////////////////////////////////

Awk::Awk () : awk (QSE_NULL), functionMap (QSE_NULL), runctx (this)

{
	errinf.num = (errnum_t)ERR_NOERR;
	errinf.lin = 0;
	errinf.msg[0] = QSE_T('\0');
}

Awk::operator Awk::awk_t* () const 
{
	return this->awk;
}

const Awk::char_t* Awk::getErrorString (ErrorNumber num) const 
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (dflerrstr != QSE_NULL);
	return dflerrstr (awk, (errnum_t)num);
}

const Awk::char_t* Awk::xerrstr (awk_t* a, errnum_t num) 
{
	Awk* awk = *(Awk**)QSE_XTN(a);
	return awk->getErrorString ((ErrorNumber)num);
}


Awk::ErrorNumber Awk::getErrorNumber () const 
{
	return (ErrorNumber)this->errinf.num;
}

Awk::size_t Awk::getErrorLine () const 
{
	return this->errinf.lin;
}

const Awk::char_t* Awk::getErrorMessage () const 
{
	return this->errinf.msg;
}

void Awk::setError (ErrorNumber code, const cstr_t* args, size_t line)
{
	if (awk != QSE_NULL)
	{
		qse_awk_seterror (awk, (errnum_t)code, args, line);
		retrieveError ();
	}
	else
	{
		errinf.num = (errnum_t)code;
		errinf.lin = line;
		qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), 
			QSE_T("not ready to set an error message"));
	}
}

void Awk::setErrorWithMessage (ErrorNumber code, const char_t* msg, size_t line)
{
	if (awk != QSE_NULL)
	{
		errinf.num = (errnum_t)code;
		errinf.lin = line;
		qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), msg);
		qse_awk_seterrinf (awk, &errinf);
	}
	else
	{
		errinf.num = (errnum_t)code;
		errinf.lin = line;
		qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), msg);
	}
}

void Awk::clearError ()
{
	errinf.num = (errnum_t)ERR_NOERR;
	errinf.lin = 0;
	errinf.msg[0] = QSE_T('\0');
}

void Awk::retrieveError ()
{
	if (this->awk == QSE_NULL) 
	{
		clearError ();
	}
	else
	{
		qse_awk_geterrinf (this->awk, &errinf);
	}
}

void Awk::retrieveError (Run* run)
{
	QSE_ASSERT (run != QSE_NULL);
	if (run->rtx == QSE_NULL) return;
	qse_awk_rtx_geterrinf (run->rtx, &errinf);
}

static void free_function_map_value (
	Awk::map_t* map, void* dptr, Awk::size_t dlen)
{
	Awk* awk = *(Awk**) QSE_XTN (map);
	qse_awk_free ((Awk::awk_t*)*awk, dptr);
}

int Awk::open () 
{
	QSE_ASSERT (awk == QSE_NULL && functionMap == QSE_NULL);

	qse_awk_prm_t prm;
	prm.pow     = pow;
	prm.sprintf = sprintf;

	awk = qse_awk_open ((qse_mmgr_t*)this, QSE_SIZEOF(xtn_t), &prm);
	if (awk == QSE_NULL)
	{
		setError (ERR_NOMEM);
		return -1;
	}

	// associate this Awk object with the underlying awk object
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	xtn->awk = this;

	dflerrstr = qse_awk_geterrstr (awk);
	qse_awk_seterrstr (awk, xerrstr);

	functionMap = qse_map_open (
		qse_awk_getmmgr(awk), QSE_SIZEOF(this), 512, 70);
	if (functionMap == QSE_NULL)
	{
		qse_awk_close (awk);
		awk = QSE_NULL;

		setError (ERR_NOMEM);
		return -1;
	}

	*(Awk**)QSE_XTN(functionMap) = this;
	qse_map_setcopier (functionMap, QSE_MAP_KEY, QSE_MAP_COPIER_INLINE);
	qse_map_setfreeer (functionMap, QSE_MAP_VAL, free_function_map_value);
	qse_map_setscale (functionMap, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	return 0;
}

void Awk::close () 
{
	fini_runctx ();
	clearArguments ();

	if (functionMap != QSE_NULL)
	{
		qse_map_close (functionMap);
		functionMap = QSE_NULL;
	}

	if (awk != QSE_NULL) 
	{
		qse_awk_close (awk);
		awk = QSE_NULL;
	}

	clearError ();
}

Awk::Run* Awk::parse (Source& in, Source& out) 
{
	QSE_ASSERT (awk != QSE_NULL);

	if (&in == &Source::NONE) 
	{
		setError (ERR_INVAL);
		return QSE_NULL;
	}

	fini_runctx ();

	sourceReader = &in;
	sourceWriter = (&out == &Source::NONE)? QSE_NULL: &out;

	qse_awk_sio_t sio;
	sio.in = readSource;
	sio.out = (sourceWriter == QSE_NULL)? QSE_NULL: writeSource;

	int n = qse_awk_parse (awk, &sio);
	if (n <= -1) 
	{
		retrieveError ();
		return QSE_NULL;
	}

	if (init_runctx () <= -1) return QSE_NULL;
	return &runctx;
}

int Awk::loop (Value* ret)
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	val_t* rv = qse_awk_rtx_loop (runctx.rtx);
	if (rv == QSE_NULL) 
	{
		retrieveError (&runctx);
		return -1;
	}

	ret->setVal (&runctx, rv);
	qse_awk_rtx_refdownval (runctx.rtx, rv);
	
	return 0;
}

int Awk::call (
	const char_t* name, Value* ret,
	const Value* args, size_t nargs)
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	val_t* buf[16];
	val_t** ptr = QSE_NULL;

	if (args != QSE_NULL)
	{
		if (nargs <= QSE_COUNTOF(buf)) ptr = buf;
		else
		{
			ptr = (val_t**) qse_awk_alloc (
				awk, QSE_SIZEOF(val_t*) * nargs);
			if (ptr == QSE_NULL)
			{
				runctx.setError (ERR_NOMEM);
				retrieveError (&runctx);
				return -1;
			}
		}

		for (size_t i = 0; i < nargs; i++) ptr[i] = (val_t*)args[i];
	}

	val_t* rv = qse_awk_rtx_call (runctx.rtx, name, ptr, nargs);

	if (ptr != QSE_NULL && ptr != buf) qse_awk_free (awk, ptr);

	if (rv == QSE_NULL) 
	{
		retrieveError (&runctx);
		return -1;
	}

	ret->setVal (&runctx, rv);

	qse_awk_rtx_refdownval (runctx.rtx, rv);
	return 0;
}

void Awk::stop () 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_stopall (awk);
}

int Awk::init_runctx () 
{
	if (runctx.rtx != QSE_NULL) return 0;

	qse_awk_rio_t rio;

	rio.pipe    = pipeHandler;
	rio.file    = fileHandler;
	rio.console = consoleHandler;

	rtx_t* rtx = qse_awk_rtx_open (
		awk, QSE_SIZEOF(rxtn_t), &rio, (qse_cstr_t*)runarg.ptr);
	if (rtx == QSE_NULL) 
	{
		retrieveError();
		return -1;
	}

	runctx.rtx = rtx;

	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	rxtn->run = &runctx;

	return 0;
}

void Awk::fini_runctx () 
{
	if (runctx.rtx != QSE_NULL)
	{
		qse_awk_rtx_close (runctx.rtx);
		runctx.rtx = QSE_NULL;
	}
}

int Awk::getOption () const 
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_getoption (awk);
}

void Awk::setOption (int opt) 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_setoption (awk, opt);
}

void Awk::setMaxDepth (int ids, size_t depth) 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_setmaxdepth (awk, ids, depth);
}

Awk::size_t Awk::getMaxDepth (int id) const 
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_getmaxdepth (awk, id);
}

int Awk::dispatch_function (Run* run, const cstr_t* name)
{
	pair_t* pair;

	pair = qse_map_search (functionMap, name->ptr, name->len);
	if (pair == QSE_NULL) 
	{
		run->setError (ERR_FUNNF, name);
		return -1;
	}

	FunctionHandler handler;
       	handler = *(FunctionHandler*)QSE_MAP_VPTR(pair);	

	size_t i, nargs = qse_awk_rtx_getnargs(run->rtx);

	Value buf[16];
	Value* args;
	if (nargs <= QSE_COUNTOF(buf)) args = buf;
	else
	{
		args = new(run) Value[nargs];
		if (args == QSE_NULL) 
		{
			run->setError (ERR_NOMEM);
			return -1;
		}
	}

	for (i = 0; i < nargs; i++)
	{
		val_t* v = qse_awk_rtx_getarg (run->rtx, i);
#ifdef PASS_BY_REFERENCE
		QSE_ASSERT (v->type == QSE_AWK_VAL_REF);
		val_t** ref = (val_t**)((qse_awk_val_ref_t*)v)->adr;
		if (args[i].setVal (run, *ref) == -1)
		{
			run->setError (ERR_NOMEM);
			if (args != buf) delete[] args;
			return -1;
		}
#else
		if (args[i].setVal (run, v) == -1)
		{
			run->setError (ERR_NOMEM);
			if (args != buf) delete[] args;
			return -1;
		}
#endif
	}
	
	Value ret (run);

	int n;

	try { n = (this->*handler) (*run, ret, args, nargs, name); }
	catch (...) { n = -1; }

#ifdef PASS_BY_REFERENCE
	if (n >= 0)
	{
		for (i = 0; i < nargs; i++)
		{
			QSE_ASSERTX (args[i].run == run, 
				"Do NOT change Run from function handler");

			val_t* v = qse_awk_rtx_getarg (run->rtx, i);
			val_t* nv = args[i].toVal();

			if (nv == v) continue;

			QSE_ASSERT (v->type == QSE_AWK_VAL_REF);
			val_t** ref = (val_t**)((qse_awk_val_ref_t*)v)->adr;
	
			qse_awk_rtx_refdownval (run->rtx, *ref);
			*ref = nv;
			qse_awk_rtx_refupval (run->rtx, *ref);
		}
	}
#endif
	if (args != buf) delete[] args;

	if (n <= -1) 
	{
		/* this is really the handler error. the underlying engine 
		 * will take care of the error code. */
		return -1;
	}

	qse_awk_rtx_setretval (run->rtx, ret.toVal());
	return 0;
}

int Awk::xstrs_t::add (awk_t* awk, const char_t* arg, size_t len) 
{
	if (this->len >= this->capa)
	{
		qse_xstr_t* ptr;
		size_t capa = this->capa;

		capa += 64;
		ptr = (qse_xstr_t*) qse_awk_realloc (
			awk, this->ptr, QSE_SIZEOF(qse_xstr_t)*(capa+1));
		if (ptr == QSE_NULL) return -1;

		this->ptr = ptr;
		this->capa = capa;
	}

	this->ptr[this->len].len = len;
	this->ptr[this->len].ptr = qse_awk_strxdup (awk, arg, len);
	if (this->ptr[this->len].ptr == QSE_NULL) return -1;

	this->len++;
	this->ptr[this->len].len = 0;
	this->ptr[this->len].ptr = QSE_NULL;

	return 0;
}

void Awk::xstrs_t::clear (awk_t* awk) 
{
	if (this->ptr != QSE_NULL)
	{
		while (this->len > 0)
			qse_awk_free (awk, this->ptr[--this->len].ptr);

		qse_awk_free (awk, this->ptr);
		this->ptr = QSE_NULL;
		this->capa = 0;
	}
}

int Awk::addArgument (const char_t* arg, size_t len) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = runarg.add (awk, arg, len);
	if (n <= -1) setError (ERR_NOMEM);
	return n;
}

int Awk::addArgument (const char_t* arg) 
{
	return addArgument (arg, qse_strlen(arg));
}

void Awk::clearArguments () 
{
	runarg.clear (awk);
}

int Awk::addGlobal (const char_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);

	int n = qse_awk_addgbl (awk, name, qse_strlen(name));
	if (n <= -1) retrieveError ();
	return n;
}

int Awk::deleteGlobal (const char_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_delgbl (awk, name, qse_strlen(name));
	if (n <= -1) retrieveError ();
	return n;
}

int Awk::setGlobal (int id, const Value& v) 
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	if (v.run != &runctx) 
	{
		setError (ERR_INVAL);
		return -1;
	}

	int n = runctx.setGlobal (id, v);
	if (n <= -1) retrieveError ();
	return n;
}

int Awk::getGlobal (int id, Value& v) 
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	int n = runctx.getGlobal (id, v);
	if (n <= -1) retrieveError ();
	return n;
}

int Awk::addFunction (
	const char_t* name, size_t minArgs, size_t maxArgs, 
	FunctionHandler handler) 
{
	QSE_ASSERT (awk != QSE_NULL);

	FunctionHandler* tmp = (FunctionHandler*) 
		qse_awk_alloc (awk, QSE_SIZEOF(handler));
	if (tmp == QSE_NULL)
	{
		setError (ERR_NOMEM);
		return -1;
	}

	//QSE_MEMCPY (tmp, &handler, QSE_SIZEOF(handler));
	*tmp = handler;
	
	size_t nameLen = qse_strlen(name);

	void* p = qse_awk_addfnc (
		awk, name, nameLen,
		0, minArgs, maxArgs,
#ifdef PASS_BY_REFERENCE
		QSE_T("R"), // pass all arguments by reference
#else
		QSE_NULL,
#endif
		functionHandler);
	if (p == QSE_NULL) 
	{
		qse_awk_free (awk, tmp);
		retrieveError ();
		return -1;
	}

	pair_t* pair = qse_map_upsert (
		functionMap, (char_t*)name, nameLen, tmp, 0);
	if (pair == QSE_NULL)
	{
		qse_awk_delfnc (awk, name, nameLen);
		qse_awk_free (awk, tmp);

		setError (ERR_NOMEM);
		return -1;
	}

	return 0;
}

int Awk::deleteFunction (const char_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);

	size_t nameLen = qse_strlen(name);

	int n = qse_awk_delfnc (awk, name, nameLen);
	if (n == 0) qse_map_delete (functionMap, name, nameLen);
	else retrieveError ();

	return n;
}

int Awk::getWord (const cstr_t* ow, cstr_t* nw)
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_getword (awk, ow, nw);
}

int Awk::setWord (const cstr_t* ow, const cstr_t* nw)
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_setword (awk, ow, nw);
}

int Awk::unsetWord (const cstr_t* w)
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_unsetword (awk, w);
}

void Awk::unsetAllWords () 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_unsetallwords (awk);
}

Awk::ssize_t Awk::readSource (
	awk_t* awk, sio_cmd_t cmd, sio_arg_t* arg,
	char_t* data, size_t count)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	Source::Data sdat (xtn->awk, Source::READ, arg);

	switch (cmd)
	{
		case QSE_AWK_SIO_OPEN:
			return xtn->awk->sourceReader->open (sdat);
		case QSE_AWK_SIO_CLOSE:
			return xtn->awk->sourceReader->close (sdat);
		case QSE_AWK_SIO_READ:
			return xtn->awk->sourceReader->read (sdat, data, count);
		default:
			return -1;
	}
}

Awk::ssize_t Awk::writeSource (
	awk_t* awk, qse_awk_sio_cmd_t cmd, sio_arg_t* arg,
	char_t* data, size_t count)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	Source::Data sdat (xtn->awk, Source::WRITE, arg);

	switch (cmd)
	{
		case QSE_AWK_SIO_OPEN:
			return xtn->awk->sourceWriter->open (sdat);
		case QSE_AWK_SIO_CLOSE:
			return xtn->awk->sourceWriter->close (sdat);
		case QSE_AWK_SIO_WRITE:
			return xtn->awk->sourceWriter->write (sdat, data, count);
		default:
			return -1;
	}
}

Awk::ssize_t Awk::pipeHandler (
	rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
	char_t* data, size_t count)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	Awk* awk = rxtn->run->awk;

	QSE_ASSERT ((riod->type & 0xFF) == QSE_AWK_RIO_PIPE);

	Pipe pipe (rxtn->run, riod);

	try
	{	
		switch (cmd)
		{
			case QSE_AWK_RIO_OPEN:
				return awk->openPipe (pipe);
			case QSE_AWK_RIO_CLOSE:
				return awk->closePipe (pipe);

			case QSE_AWK_RIO_READ:
				return awk->readPipe (pipe, data, count);
			case QSE_AWK_RIO_WRITE:
				return awk->writePipe (pipe, data, count);

			case QSE_AWK_RIO_FLUSH:
				return awk->flushPipe (pipe);

			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

Awk::ssize_t Awk::fileHandler (
	rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
	char_t* data, size_t count)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	Awk* awk = rxtn->run->awk;

	QSE_ASSERT ((riod->type & 0xFF) == QSE_AWK_RIO_FILE);

	File file (rxtn->run, riod);

	try
	{
		switch (cmd)
		{
			case QSE_AWK_RIO_OPEN:
				return awk->openFile (file);
			case QSE_AWK_RIO_CLOSE:
				return awk->closeFile (file);

			case QSE_AWK_RIO_READ:
				return awk->readFile (file, data, count);
			case QSE_AWK_RIO_WRITE:
				return awk->writeFile (file, data, count);

			case QSE_AWK_RIO_FLUSH:
				return awk->flushFile (file);

			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

Awk::ssize_t Awk::consoleHandler (
	rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
	char_t* data, size_t count)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	Awk* awk = rxtn->run->awk;

	QSE_ASSERT ((riod->type & 0xFF) == QSE_AWK_RIO_CONSOLE);

	Console console (rxtn->run, riod);

	try
	{
		switch (cmd)
		{
			case QSE_AWK_RIO_OPEN:
				return awk->openConsole (console);
			case QSE_AWK_RIO_CLOSE:
				return awk->closeConsole (console);

			case QSE_AWK_RIO_READ:
				return awk->readConsole (console, data, count);
			case QSE_AWK_RIO_WRITE:
				return awk->writeConsole (console, data, count);

			case QSE_AWK_RIO_FLUSH:
				return awk->flushConsole (console);
			case QSE_AWK_RIO_NEXT:
				return awk->nextConsole (console);

			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

int Awk::functionHandler (rtx_t* rtx, const cstr_t* name)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	return rxtn->run->awk->dispatch_function (rxtn->run, name);
}	

Awk::real_t Awk::pow (awk_t* awk, real_t x, real_t y)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->pow (x, y);
}
	
int Awk::sprintf (awk_t* awk, char_t* buf, size_t size,
                  const char_t* fmt, ...)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);

	va_list ap;
	va_start (ap, fmt);
	int n = xtn->awk->vsprintf (buf, size, fmt, ap);
	va_end (ap);
	return n;
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////