/*
 * $Id$
 * 
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/awk/Awk.hpp>
#include <qse/cmn/str.h>
#include "../cmn/mem.h"
#include "awk.h"

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

int Awk::RIOBase::getUflags () const
{
	return this->riod->uflags;
}

void Awk::RIOBase::setUflags (int uflags)
{
	this->riod->uflags = uflags;
}

Awk::RIOBase::operator Awk* () const 
{
	return this->run->awk;
}

Awk::RIOBase::operator Awk::awk_t* () const 
{
	QSE_ASSERT (qse_awk_rtx_getawk(this->run->rtx) == this->run->awk->getHandle());
	return this->run->awk->getHandle();
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

Awk::Pipe::CloseMode Awk::Pipe::getCloseMode () const
{
	return (CloseMode)riod->rwcmode;
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
		qse_awk_freemem ((awk_t*)this, filename);
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

int Awk::Console::setFNR (int_t fnr)
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

Awk::Value::IndexIterator Awk::Value::IndexIterator::END;

const Awk::char_t* Awk::Value::getEmptyStr()
{
	static const Awk::char_t* EMPTY_STRING = QSE_T("");
	return EMPTY_STRING;
}

Awk::Value::IntIndex::IntIndex (int_t x)
{
	ptr = buf;
	len = 0;

#define NTOC(n) (QSE_T("0123456789")[n])

	int base = 10;
	int_t last = x % base;
	int_t y = 0;
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
	void* ptr = qse_awk_rtx_allocmem (run->rtx, QSE_SIZEOF(run) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(Run**)ptr = run;
	return (char*)ptr+QSE_SIZEOF(run);
}

void* Awk::Value::operator new[] (size_t n, Run* run) throw () 
{
	void* ptr = qse_awk_rtx_allocmem (run->rtx, QSE_SIZEOF(run) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(Run**)ptr = run;
	return (char*)ptr+QSE_SIZEOF(run);
}

#if !defined(__BORLANDC__) && !defined(__WATCOMC__)
void Awk::Value::operator delete (void* ptr, Run* run) 
{
	// this placement delete is to be called when the constructor 
	// throws an exception and it's caught by the caller.
	qse_awk_rtx_freemem (run->rtx, (char*)ptr-QSE_SIZEOF(run));
}

void Awk::Value::operator delete[] (void* ptr, Run* run) 
{
	// this placement delete is to be called when the constructor 
	// throws an exception and it's caught by the caller.
	qse_awk_rtx_freemem (run->rtx, (char*)ptr-QSE_SIZEOF(run));
}
#endif

void Awk::Value::operator delete (void* ptr) 
{
	// this delete is to be called for normal destruction.
	void* p = (char*)ptr-QSE_SIZEOF(Run*);
	qse_awk_rtx_freemem ((*(Run**)p)->rtx, p);
}

void Awk::Value::operator delete[] (void* ptr) 
{
	// this delete is to be called for normal destruction.
	void* p = (char*)ptr-QSE_SIZEOF(Run*);
	qse_awk_rtx_freemem ((*(Run**)p)->rtx, p);
}

Awk::Value::Value (): run (QSE_NULL), val (qse_getawknilval()) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
}

Awk::Value::Value (Run& run): run (&run), val (qse_getawknilval()) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
}

Awk::Value::Value (Run* run): run (run), val (qse_getawknilval()) 
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
			qse_awk_rtx_freemem (run->rtx, cached.str.ptr);
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
			qse_awk_rtx_freemem (run->rtx, cached.str.ptr);
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
			qse_awk_rtx_freemem (run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}

		run = QSE_NULL;
		val = qse_getawknilval();
	}
}

Awk::Value::operator Awk::int_t () const
{
	int_t v;
	if (getInt (&v) <= -1) v = 0;
	return v;
}

Awk::Value::operator Awk::flt_t () const
{
	flt_t v;
	if (getFlt (&v) <= -1) v = 0.0;
	return v;
}

Awk::Value::operator const Awk::char_t* () const
{
	const Awk::char_t* ptr;
	size_t len;
	if (Awk::Value::getStr (&ptr, &len) <= -1) ptr = getEmptyStr();
	return ptr;
}

int Awk::Value::getInt (int_t* v) const
{
	int_t lv = 0;

	QSE_ASSERT (this->val != QSE_NULL);

	if (run != QSE_NULL)
	{
		int n = qse_awk_rtx_valtoint (this->run->rtx, this->val, &lv);
		if (n <= -1) 
		{
			run->awk->retrieveError (this->run);
			return -1;
		}
	}

	*v = lv;
	return 0;
}

int Awk::Value::getFlt (flt_t* v) const
{
	flt_t rv = 0;

	QSE_ASSERT (this->val != QSE_NULL);

	if (this->run)
	{
		int n = qse_awk_rtx_valtoflt (this->run->rtx, this->val, &rv);
		if (n <= -1)
		{
			run->awk->retrieveError (this->run);
			return -1;
		}
	}

	*v = rv;
	return 0;
}

int Awk::Value::getNum (int_t* lv, flt_t* fv) const
{
	QSE_ASSERT (this->val != QSE_NULL);

	if (this->run != QSE_NULL)
	{
		int n = qse_awk_rtx_valtonum (this->run->rtx, this->val, lv, fv);
		if (n <= -1)
		{
			run->awk->retrieveError (this->run);
			return -1;
		}
		return n;
	}

	*lv = 0;
	return 0;
}

int Awk::Value::getStr (const char_t** str, size_t* len) const
{
	const char_t* p = getEmptyStr();
	size_t l = 0;

	QSE_ASSERT (this->val != QSE_NULL);

	if (this->run != QSE_NULL)
	{
		if (this->val->type == QSE_AWK_VAL_STR)
		{
			p = ((qse_awk_val_str_t*)this->val)->val.ptr;
			l = ((qse_awk_val_str_t*)this->val)->val.len;
		}
		else
		{
			if (cached.str.ptr == QSE_NULL)
			{
				qse_awk_rtx_valtostr_out_t out;
				out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
				if (qse_awk_rtx_valtostr (this->run->rtx, this->val, &out) <= -1)
				{
					run->awk->retrieveError (this->run);
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
	if (this->run == QSE_NULL) 
	{
		/* no runtime context assoicated. unfortunately, i can't 
		 * set an error number for the same reason */
		return -1;
	}
	return setVal (this->run, v);
}

int Awk::Value::setVal (Run* r, val_t* v)
{
	if (this->run != QSE_NULL)
	{
		qse_awk_rtx_refdownval (this->run->rtx, val);
		if (cached.str.ptr != QSE_NULL)
		{
			qse_awk_rtx_freemem (this->run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}
	}

	QSE_ASSERT (cached.str.ptr == QSE_NULL);
	qse_awk_rtx_refupval (r->rtx, v);

	this->run = r;
	this->val = v;

	return 0;
}

int Awk::Value::setInt (int_t v)
{
	if (this->run == QSE_NULL) 
	{
		/* no runtime context assoicated. unfortunately, i can't 
		 * set an error number for the same reason */
		return -1;
	}
	return setInt (this->run, v);
}

int Awk::Value::setInt (Run* r, int_t v)
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

int Awk::Value::setFlt (flt_t v)
{
	if (this->run == QSE_NULL) 
	{
		/* no runtime context assoicated. unfortunately, i can't 
		 * set an error number for the same reason */
		return -1;
	}
	return setFlt (this->run, v);
}

int Awk::Value::setFlt (Run* r, flt_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makefltval (r->rtx, v);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}
			
	int n = setVal (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setStr (const char_t* str, size_t len, bool numeric)
{
	if (this->run == QSE_NULL) 
	{
		/* no runtime context assoicated. unfortunately, i can't 
		 * set an error number for the same reason */
		return -1; 
	}
	return setStr (this->run, str, len, numeric);
}

int Awk::Value::setStr (Run* r, const char_t* str, size_t len, bool numeric)
{
	val_t* tmp;

	cstr_t cstr;
	cstr.ptr = str;
	cstr.len = len;

	tmp = numeric? qse_awk_rtx_makenstrvalwithcstr (r->rtx, &cstr):
	               qse_awk_rtx_makestrvalwithcstr (r->rtx, &cstr);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = setVal (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setStr (const char_t* str, bool numeric)
{
	if (this->run == QSE_NULL) return -1;
	return setStr (this->run, str, numeric);
}

int Awk::Value::setStr (Run* r, const char_t* str, bool numeric)
{
	val_t* tmp;
	tmp = numeric? qse_awk_rtx_makenstrvalwithstr (r->rtx, str):
	               qse_awk_rtx_makestrvalwithstr (r->rtx, str);
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
	if (this->run == QSE_NULL) return -1;
	return setIndexedVal (this->run, idx, v);
}

int Awk::Value::setIndexedVal (Run* r, const Index& idx, val_t* v)
{
	QSE_ASSERT (r != QSE_NULL);

	if (val->type != QSE_AWK_VAL_MAP)
	{
		// the previous value is not a map. 
		// a new map value needs to be created first.
		val_t* map = qse_awk_rtx_makemapval (r->rtx);
		if (map == QSE_NULL) 
		{
			r->awk->retrieveError (r);
			return -1;
		}

		qse_awk_rtx_refupval (r->rtx, map);

		// update the map with a given value 
		if (qse_awk_rtx_setmapvalfld (
			r->rtx, map, idx.ptr, idx.len, v) == QSE_NULL)
		{
			qse_awk_rtx_refdownval (r->rtx, map);
			r->awk->retrieveError (r);
			return -1;
		}

		// free the previous value
		if (this->run) 
		{
			// if val is not nil, this->run can't be NULL
			qse_awk_rtx_refdownval (this->run->rtx, val);
		}

		this->run = r;
		this->val = map;
	}
	else
	{
		QSE_ASSERT (run != QSE_NULL);

		// if the previous value is a map, things are a bit simpler 
		// however it needs to check if the runtime context matches
		// with the previous one.
		if (this->run != r) 
		{
			// it can't span across multiple runtime contexts
			this->run->setError (QSE_AWK_EINVAL);
			this->run->awk->retrieveError (run);
			return -1;
		}

		// update the map with a given value 
		if (qse_awk_rtx_setmapvalfld (
			r->rtx, val, idx.ptr, idx.len, v) == QSE_NULL)
		{
			r->awk->retrieveError (r);
			return -1;
		}
	}

	return 0;
}

int Awk::Value::setIndexedInt (const Index& idx, int_t v)
{
	if (run == QSE_NULL) return -1;
	return setIndexedInt (run, idx, v);
}

int Awk::Value::setIndexedInt (Run* r, const Index& idx, int_t v)
{
	val_t* tmp = qse_awk_rtx_makeintval (r->rtx, v);
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

int Awk::Value::setIndexedFlt (const Index& idx, flt_t v)
{
	if (run == QSE_NULL) return -1;
	return setIndexedFlt (run, idx, v);
}

int Awk::Value::setIndexedFlt (Run* r, const Index& idx, flt_t v)
{
	val_t* tmp = qse_awk_rtx_makefltval (r->rtx, v);
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

int Awk::Value::setIndexedStr (const Index& idx, const char_t* str, size_t len, bool numeric)
{
	if (run == QSE_NULL) return -1;
	return setIndexedStr (run, idx, str, len, numeric);
}

int Awk::Value::setIndexedStr (
	Run* r, const Index& idx, const char_t* str, size_t len, bool numeric)
{
	val_t* tmp;

	cstr_t cstr;
	cstr.ptr = str;
	cstr.len = len;

	tmp = numeric? qse_awk_rtx_makenstrvalwithcstr (r->rtx, &cstr):
	               qse_awk_rtx_makestrvalwithcstr (r->rtx, &cstr);
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

int Awk::Value::setIndexedStr (const Index& idx, const char_t* str, bool numeric)
{
	if (run == QSE_NULL) return -1;
	return setIndexedStr (run, idx, str, numeric);
}

int Awk::Value::setIndexedStr (Run* r, const Index& idx, const char_t* str, bool numeric)
{
	val_t* tmp;
	tmp = numeric? qse_awk_rtx_makenstrvalwithstr (r->rtx, str):
	               qse_awk_rtx_makestrvalwithstr (r->rtx, str);
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
	val_t* fv = qse_awk_rtx_getmapvalfld (
		this->run->rtx, val, (char_t*)idx.ptr, idx.len);

	// the key is not found. it is not an error. v is just nil 
	if (fv == QSE_NULL)
	{
		v->clear ();
		return 0; 
	}

	// if v.set fails, it should return an error 
	return v->setVal (this->run, fv);
}

Awk::Value::IndexIterator Awk::Value::getFirstIndex (Index* idx) const
{
	QSE_ASSERT (this->val != QSE_NULL);

	if (this->val->type != QSE_AWK_VAL_MAP) return IndexIterator::END;

	QSE_ASSERT (this->run != QSE_NULL);

	Awk::Value::IndexIterator itr;
	qse_awk_val_map_itr_t* iptr;

	iptr = qse_awk_rtx_getfirstmapvalitr (this->run->rtx, this->val, &itr);
	if (iptr == QSE_NULL) return IndexIterator::END; // no more key

	idx->set (QSE_AWK_VAL_MAP_ITR_KEY(iptr));

	return itr;
}

Awk::Value::IndexIterator Awk::Value::getNextIndex (
	Index* idx, const IndexIterator& curitr) const
{
	QSE_ASSERT (val != QSE_NULL);

	if (val->type != QSE_AWK_VAL_MAP) return IndexIterator::END;

	QSE_ASSERT (this->run != QSE_NULL);

	Awk::Value::IndexIterator itr (curitr);
	qse_awk_val_map_itr_t* iptr;

	iptr = qse_awk_rtx_getnextmapvalitr (this->run->rtx, this->val, &itr);
	if (iptr == QSE_NULL) return IndexIterator::END; // no more key

	idx->set (QSE_AWK_VAL_MAP_ITR_KEY(iptr));

	return itr;
}

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

bool Awk::Run::isStop () const
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_isstop (this->rtx)? true: false;
}

Awk::errnum_t Awk::Run::getErrorNumber () const 
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_geterrnum (this->rtx);
}

Awk::loc_t Awk::Run::getErrorLocation () const 
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return *qse_awk_rtx_geterrloc (this->rtx);
}

const Awk::char_t* Awk::Run::getErrorMessage () const  
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_geterrmsg (this->rtx);
}

void Awk::Run::setError (errnum_t code, const cstr_t* args, const loc_t* loc)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_awk_rtx_seterror (this->rtx, code, args, loc);
}

void Awk::Run::setErrorWithMessage (
	errnum_t code, const char_t* msg, const loc_t* loc)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	errinf_t errinf;

	QSE_MEMSET (&errinf, 0, QSE_SIZEOF(errinf));
	errinf.num = code;
	if (loc == QSE_NULL) errinf.loc = *loc;
	qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), msg);

	qse_awk_rtx_seterrinf (this->rtx, &errinf);
}

int Awk::Run::setGlobal (int id, int_t v)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	val_t* tmp = qse_awk_rtx_makeintval (this->rtx, v);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (this->rtx, tmp);
	int n = qse_awk_rtx_setgbl (this->rtx, id, tmp);
	qse_awk_rtx_refdownval (this->rtx, tmp);
	return n;
}

int Awk::Run::setGlobal (int id, flt_t v)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	val_t* tmp = qse_awk_rtx_makefltval (this->rtx, v);
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

Awk::Awk (Mmgr* mmgr): 
	Mmged (mmgr), awk (QSE_NULL), functionMap (QSE_NULL), 
	source_reader (QSE_NULL), source_writer (QSE_NULL),
	pipe_handler (QSE_NULL), file_handler (QSE_NULL), 
	console_handler (QSE_NULL), runctx (this)

{
	QSE_MEMSET (&errinf, 0, QSE_SIZEOF(errinf));
	errinf.num = QSE_AWK_ENOERR;
}

Awk::operator Awk::awk_t* () const 
{
	return this->awk;
}

const Awk::char_t* Awk::getErrorString (errnum_t num) const 
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (dflerrstr != QSE_NULL);
	return dflerrstr (awk, num);
}

const Awk::char_t* Awk::xerrstr (const awk_t* a, errnum_t num) 
{
	Awk* awk = *(Awk**)QSE_XTN(a);
	return awk->getErrorString (num);
}


Awk::errnum_t Awk::getErrorNumber () const 
{
	return this->errinf.num;
}

Awk::loc_t Awk::getErrorLocation () const 
{
	return this->errinf.loc;
}

const Awk::char_t* Awk::getErrorMessage () const 
{
	return this->errinf.msg;
}

void Awk::setError (errnum_t code, const cstr_t* args, const loc_t* loc)
{
	if (awk != QSE_NULL)
	{
		qse_awk_seterror (awk, code, args, loc);
		retrieveError ();
	}
	else
	{
		QSE_MEMSET (&errinf, 0, QSE_SIZEOF(errinf));
		errinf.num = code;
		if (loc != QSE_NULL) errinf.loc = *loc;
		qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), 
			QSE_T("not ready to set an error message"));
	}
}

void Awk::setErrorWithMessage (errnum_t code, const char_t* msg, const loc_t* loc)
{
	QSE_MEMSET (&errinf, 0, QSE_SIZEOF(errinf));

	errinf.num = code;
	if (loc != QSE_NULL) errinf.loc = *loc;
	qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), msg);

	if (awk != QSE_NULL) qse_awk_seterrinf (awk, &errinf);
}

void Awk::clearError ()
{
	QSE_MEMSET (&errinf, 0, QSE_SIZEOF(errinf));
	errinf.num = QSE_AWK_ENOERR;
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
	Awk::htb_t* map, void* dptr, Awk::size_t dlen)
{
	Awk* awk = *(Awk**) QSE_XTN (map);
	qse_awk_freemem ((Awk::awk_t*)*awk, dptr);
}

int Awk::open () 
{
	QSE_ASSERT (awk == QSE_NULL && functionMap == QSE_NULL);

	qse_awk_prm_t prm;

	QSE_MEMSET (&prm, 0, QSE_SIZEOF(prm));
	prm.math.pow = pow;
	prm.math.mod = mod;
	prm.math.sin = sin;
	prm.math.cos = cos;
	prm.math.tan = tan;
	prm.math.atan = atan;
	prm.math.atan2 = atan2;
	prm.math.log = log;
	prm.math.log10 = log10;
	prm.math.exp = exp;
	prm.math.sqrt = sqrt;
	prm.modopen  = modopen;
	prm.modclose = modclose;
	prm.modsym  = modsym;

	awk = qse_awk_open (this->getMmgr(), QSE_SIZEOF(xtn_t), &prm);
	if (awk == QSE_NULL)
	{
		setError (QSE_AWK_ENOMEM);
		return -1;
	}

	// associate this Awk object with the underlying awk object
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	xtn->awk = this;

	dflerrstr = qse_awk_geterrstr (awk);
	qse_awk_seterrstr (awk, xerrstr);

	functionMap = qse_htb_open (
		qse_awk_getmmgr(awk), QSE_SIZEOF(this), 512, 70,
		QSE_SIZEOF(qse_char_t), 1
	);
	if (functionMap == QSE_NULL)
	{
		qse_awk_close (awk);
		awk = QSE_NULL;

		setError (QSE_AWK_ENOMEM);
		return -1;
	}

	*(Awk**)QSE_XTN(functionMap) = this;

	static qse_htb_style_t style =
	{
		{
			QSE_HTB_COPIER_INLINE,
			QSE_HTB_COPIER_DEFAULT 
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			free_function_map_value
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};
	qse_htb_setstyle (functionMap, &style);

	return 0;
}

void Awk::close () 
{
	fini_runctx ();
	clearArguments ();

	if (functionMap != QSE_NULL)
	{
		qse_htb_close (functionMap);
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
		setError (QSE_AWK_EINVAL);
		return QSE_NULL;
	}

	fini_runctx ();

	source_reader = &in;
	source_writer = (&out == &Source::NONE)? QSE_NULL: &out;

	qse_awk_sio_t sio;
	sio.in = readSource;
	sio.out = (source_writer == QSE_NULL)? QSE_NULL: writeSource;

	int n = qse_awk_parse (awk, &sio);
	if (n <= -1) 
	{
		retrieveError ();
		return QSE_NULL;
	}

	if (init_runctx() <= -1) return QSE_NULL;
	return &this->runctx;
}

Awk::Run* Awk::resetRunContext ()
{
	if (this->runctx.rtx)
	{
		fini_runctx ();
		if (init_runctx() <= -1) return QSE_NULL;
		return &this->runctx;
	}
	else return QSE_NULL;
}

int Awk::loop (Value* ret)
{
	QSE_ASSERT (this->awk != QSE_NULL);
	QSE_ASSERT (this->runctx.rtx != QSE_NULL);

	val_t* rv = qse_awk_rtx_loop (this->runctx.rtx);
	if (rv == QSE_NULL) 
	{
		retrieveError (&this->runctx);
		return -1;
	}

	ret->setVal (&this->runctx, rv);
	qse_awk_rtx_refdownval (this->runctx.rtx, rv);
	
	return 0;
}

int Awk::call (
	const char_t* name, Value* ret,
	const Value* args, size_t nargs)
{
	QSE_ASSERT (this->awk != QSE_NULL);
	QSE_ASSERT (this->runctx.rtx != QSE_NULL);

	val_t* buf[16];
	val_t** ptr = QSE_NULL;

	if (args != QSE_NULL)
	{
		if (nargs <= QSE_COUNTOF(buf)) ptr = buf;
		else
		{
			ptr = (val_t**) qse_awk_allocmem (
				awk, QSE_SIZEOF(val_t*) * nargs);
			if (ptr == QSE_NULL)
			{
				this->runctx.setError (QSE_AWK_ENOMEM);
				retrieveError (&this->runctx);
				return -1;
			}
		}

		for (size_t i = 0; i < nargs; i++) ptr[i] = (val_t*)args[i];
	}

	val_t* rv = qse_awk_rtx_call (this->runctx.rtx, name, ptr, nargs);

	if (ptr != QSE_NULL && ptr != buf) qse_awk_freemem (awk, ptr);

	if (rv == QSE_NULL) 
	{
		retrieveError (&this->runctx);
		return -1;
	}

	ret->setVal (&this->runctx, rv);

	qse_awk_rtx_refdownval (this->runctx.rtx, rv);
	return 0;
}

void Awk::stop () 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_stopall (awk);
}

int Awk::init_runctx () 
{
	if (this->runctx.rtx) return 0;

	qse_awk_rio_t rio;

	rio.pipe    = pipeHandler;
	rio.file    = fileHandler;
	rio.console = consoleHandler;

	rtx_t* rtx = qse_awk_rtx_open (awk, QSE_SIZEOF(rxtn_t), &rio);
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
	if (this->runctx.rtx)
	{
		qse_awk_rtx_close (this->runctx.rtx);
		this->runctx.rtx = QSE_NULL;
	}
}

int Awk::getTrait () const 
{
	QSE_ASSERT (awk != QSE_NULL);
	int val;
	qse_awk_getopt (awk, QSE_AWK_TRAIT, &val);
	return val;
}

void Awk::setTrait (int trait) 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_setopt (awk, QSE_AWK_TRAIT, &trait);
}

Awk::size_t Awk::getMaxDepth (depth_t id) const 
{
	QSE_ASSERT (awk != QSE_NULL);

	size_t depth;
	qse_awk_getopt (awk, (qse_awk_opt_t)id, &depth);
	return depth;
}

void Awk::setMaxDepth (depth_t id, size_t depth) 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_setopt (awk, (qse_awk_opt_t)id, &depth);
}

int Awk::dispatch_function (Run* run, const fnc_info_t* fi)
{
	pair_t* pair;
	bool has_ref_arg = false;

	pair = qse_htb_search (functionMap, fi->name.ptr, fi->name.len);
	if (pair == QSE_NULL) 
	{
		run->setError (QSE_AWK_EFUNNF, (cstr_t*)&fi->name);
		return -1;
	}

	FunctionHandler handler;
       	handler = *(FunctionHandler*)QSE_HTB_VPTR(pair);	

	size_t i, nargs = qse_awk_rtx_getnargs(run->rtx);

	Value buf[16];
	Value* args;
	if (nargs <= QSE_COUNTOF(buf)) args = buf;
	else
	{
		args = new(run) Value[nargs];
		if (args == QSE_NULL) 
		{
			run->setError (QSE_AWK_ENOMEM);
			return -1;
		}
	}

	for (i = 0; i < nargs; i++)
	{
		int xx;
		val_t* v = qse_awk_rtx_getarg (run->rtx, i);

		if (v->type == QSE_AWK_VAL_REF)
		{
			qse_awk_val_ref_t* ref = (qse_awk_val_ref_t*)v;

			switch (ref->id)
			{
				case qse_awk_val_ref_t::QSE_AWK_VAL_REF_POS:
				{
					qse_size_t idx = (qse_size_t)ref->adr;

					if (idx == 0)
					{
						xx = args[i].setStr (run, 
							QSE_STR_PTR(&run->rtx->inrec.line),
							QSE_STR_LEN(&run->rtx->inrec.line));
					}
					else if (idx <= run->rtx->inrec.nflds)
					{
						xx = args[i].setStr (run,
							run->rtx->inrec.flds[idx-1].ptr,
							run->rtx->inrec.flds[idx-1].len);
					}
					else
					{
						xx = args[i].setStr (run, QSE_T(""), (size_t)0);
					}
					break;
				}

				case qse_awk_val_ref_t::QSE_AWK_VAL_REF_GBL:
				{
					qse_size_t idx = (qse_size_t)ref->adr;
					qse_awk_val_t* val = (qse_awk_val_t*)RTX_STACK_GBL (run->rtx, idx);
					xx = args[i].setVal (run, val);
					break;
				}
	
				default:
					xx = args[i].setVal (run, *(ref->adr));
					break;
			}
			has_ref_arg = true;
		}
		else
		{
			xx = args[i].setVal (run, v);
		}

		if (xx <= -1)
		{
			run->setError (QSE_AWK_ENOMEM);
			if (args != buf) delete[] args;
			return -1;
		}
	}
	
	Value ret (run);

	int n;

	try { n = (this->*handler) (*run, ret, args, nargs, fi); }
	catch (...) { n = -1; }

	if (n >= 0 && has_ref_arg)
	{
		for (i = 0; i < nargs; i++)
		{
			QSE_ASSERTX (args[i].run == run, 
				"Do NOT change the run field from function handler");

			val_t* v = qse_awk_rtx_getarg (run->rtx, i);
			if (v->type == QSE_AWK_VAL_REF)
			{
				if (qse_awk_rtx_setrefval (run->rtx, (qse_awk_val_ref_t*)v, args[i].toVal()) <= -1)
				{
					n = -1;
					break;
				}
			}
		}
	}

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
		ptr = (qse_xstr_t*) qse_awk_reallocmem (
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
			qse_awk_freemem (awk, this->ptr[--this->len].ptr);

		qse_awk_freemem (awk, this->ptr);
		this->ptr = QSE_NULL;
		this->capa = 0;
	}
}

int Awk::addArgument (const char_t* arg, size_t len) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = runarg.add (awk, arg, len);
	if (n <= -1) setError (QSE_AWK_ENOMEM);
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
	int n = qse_awk_addgbl (awk, name);
	if (n <= -1) retrieveError ();
	return n;
}

int Awk::deleteGlobal (const char_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_delgbl (awk, name);
	if (n <= -1) retrieveError ();
	return n;
}

int Awk::findGlobal (const char_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_findgbl (awk, name);
	if (n <= -1) retrieveError ();
	return n;
}

int Awk::setGlobal (int id, const Value& v) 
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	if (v.run != &runctx) 
	{
		setError (QSE_AWK_EINVAL);
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
	const char_t* argSpec, FunctionHandler handler, int validOpts)
{
	QSE_ASSERT (awk != QSE_NULL);

	FunctionHandler* tmp = (FunctionHandler*) 
		qse_awk_callocmem (awk, QSE_SIZEOF(handler));
	if (tmp == QSE_NULL)
	{
		setError (QSE_AWK_ENOMEM);
		return -1;
	}

	*tmp = handler;
	
	fnc_spec_t spec;

	QSE_MEMSET (&spec, 0, QSE_SIZEOF(spec));
	spec.arg.min = minArgs;
	spec.arg.max = maxArgs;
	spec.arg.spec = argSpec;
	spec.impl = functionHandler;
	spec.trait = validOpts;

	qse_awk_fnc_t* fnc = qse_awk_addfnc (awk, name, &spec);
	if (fnc == QSE_NULL) 
	{
		qse_awk_freemem (awk, tmp);
		retrieveError ();
		return -1;
	}

	pair_t* pair = qse_htb_upsert (
		functionMap, (char_t*)name, qse_strlen(name), tmp, 0);
	if (pair == QSE_NULL)
	{
		qse_awk_delfnc (awk, name);
		qse_awk_freemem (awk, tmp);
		setError (QSE_AWK_ENOMEM);
		return -1;
	}

	return 0;
}

int Awk::deleteFunction (const char_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);

	int n = qse_awk_delfnc (awk, name);
	if (n == 0) qse_htb_delete (functionMap, name, qse_strlen(name));
	else retrieveError ();

	return n;
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
			return xtn->awk->source_reader->open (sdat);
		case QSE_AWK_SIO_CLOSE:
			return xtn->awk->source_reader->close (sdat);
		case QSE_AWK_SIO_READ:
			return xtn->awk->source_reader->read (sdat, data, count);
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
			return xtn->awk->source_writer->open (sdat);
		case QSE_AWK_SIO_CLOSE:
			return xtn->awk->source_writer->close (sdat);
		case QSE_AWK_SIO_WRITE:
			return xtn->awk->source_writer->write (sdat, data, count);
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
		if (awk->pipe_handler)
		{
			switch (cmd)
			{
				case QSE_AWK_RIO_OPEN:
					return awk->pipe_handler->open (pipe);
				case QSE_AWK_RIO_CLOSE:
					return awk->pipe_handler->close (pipe);
	
				case QSE_AWK_RIO_READ:
					return awk->pipe_handler->read (pipe, data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->pipe_handler->write (pipe, data, count);
	
				case QSE_AWK_RIO_FLUSH:
					return awk->pipe_handler->flush (pipe);
	
				default:
					return -1;
			}
		}
		else
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
		if (awk->file_handler)
		{
			switch (cmd)
			{
				case QSE_AWK_RIO_OPEN:
					return awk->file_handler->open (file);
				case QSE_AWK_RIO_CLOSE:
					return awk->file_handler->close (file);
	
				case QSE_AWK_RIO_READ:
					return awk->file_handler->read (file, data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->file_handler->write (file, data, count);
	
				case QSE_AWK_RIO_FLUSH:
					return awk->file_handler->flush (file);
	
				default:
					return -1;
			}
		}
		else
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
		if (awk->console_handler)
		{
			switch (cmd)
			{
				case QSE_AWK_RIO_OPEN:
					return awk->console_handler->open (console);
				case QSE_AWK_RIO_CLOSE:
					return awk->console_handler->close (console);

				case QSE_AWK_RIO_READ:
					return awk->console_handler->read (console, data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->console_handler->write (console, data, count);

				case QSE_AWK_RIO_FLUSH:
					return awk->console_handler->flush (console);
				case QSE_AWK_RIO_NEXT:
					return awk->console_handler->next (console);

				default:
					return -1;
			}
		}
		else
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
	}
	catch (...)
	{
		return -1;
	}
}

int Awk::openPipe  (Pipe& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::closePipe (Pipe& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

Awk::ssize_t Awk::readPipe (Pipe& io, char_t* buf, size_t len)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

Awk::ssize_t Awk::writePipe (Pipe& io, const char_t* buf, size_t len)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::flushPipe (Pipe& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::openFile  (File& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::closeFile (File& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

Awk::ssize_t Awk::readFile (File& io, char_t* buf, size_t len)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

Awk::ssize_t Awk::writeFile (File& io, const char_t* buf, size_t len)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::flushFile (File& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::openConsole  (Console& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::closeConsole (Console& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

Awk::ssize_t Awk::readConsole (Console& io, char_t* buf, size_t len)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

Awk::ssize_t Awk::writeConsole (Console& io, const char_t* buf, size_t len)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::flushConsole (Console& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::nextConsole (Console& io)
{
	((Run*)io)->setError (QSE_AWK_ENOIMPL);
	return -1;
}

int Awk::functionHandler (rtx_t* rtx, const fnc_info_t* fi)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	return rxtn->run->awk->dispatch_function (rxtn->run, fi);
}	
	
Awk::flt_t Awk::pow (awk_t* awk, flt_t x, flt_t y)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->pow (x, y);
}

Awk::flt_t Awk::mod (awk_t* awk, flt_t x, flt_t y)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->mod (x, y);
}

Awk::flt_t Awk::sin (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->sin (x);
}

Awk::flt_t Awk::cos (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->cos (x);
}

Awk::flt_t Awk::tan (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->tan (x);
}

Awk::flt_t Awk::atan (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->atan (x);
}

Awk::flt_t Awk::atan2 (awk_t* awk, flt_t x, flt_t y)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->atan2 (x, y);
}

Awk::flt_t Awk::log (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->log (x);
}

Awk::flt_t Awk::log10 (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->log10 (x);
}

Awk::flt_t Awk::exp (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->exp (x);
}

Awk::flt_t Awk::sqrt (awk_t* awk, flt_t x)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->sqrt (x);
}

void* Awk::modopen (awk_t* awk, const mod_spec_t* spec)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->modopen (spec);
}

void Awk::modclose (awk_t* awk, void* handle)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	xtn->awk->modclose (handle);
}

void* Awk::modsym (awk_t* awk, void* handle, const char_t* name)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);
	return xtn->awk->modsym (handle, name);
}
/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
