/*
 * $Id: Awk.cpp 228 2009-07-11 03:01:36Z hyunghwan.chung $
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

Awk::Value::Value (const Value& v): run (v.run), val (v.val)
{
	if (run != QSE_NULL)
		qse_awk_rtx_refupval (run->rtx, val);
}

Awk::Value::~Value ()
{
	if (run != QSE_NULL)
		qse_awk_rtx_refdownval (run->rtx, val);
}

Awk::Value& Awk::Value::operator= (const Value& v)
{
	if (run != QSE_NULL)
		qse_awk_rtx_refdownval (run->rtx, val);

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

		run = QSE_NULL;
		val = qse_awk_val_nil;
	}
}

Awk::Value::operator Awk::long_t () const
{
	long_t v;
	if (get (&v) <= -1) v = 0;
	return v;
}

Awk::Value::operator Awk::real_t () const
{
	real_t v;
	if (get (&v) <= -1) v = 0.0;
	return v;
}

Awk::Value::operator const Awk::char_t* () const
{
	const char_t* ptr;
	size_t len;
	if (get (&ptr, &len) <= -1) ptr = QSE_T("");
	return ptr;
}

int Awk::Value::get (long_t* v) const
{
	long_t lv = 0;

	if (run != QSE_NULL)
	{
		real_t rv;
		int n = qse_awk_rtx_valtonum (run->rtx, val, &lv, &rv);
		if (n <= -1) return -1;
		if (n >= 1) lv = rv;
	}

	*v = lv;
	return 0;
}

int Awk::Value::get (real_t* v) const
{
	real_t rv = 0;

	if (run != QSE_NULL)
	{
		long_t lv;
		int n = qse_awk_rtx_valtonum (run->rtx, val, &lv, &rv);
		if (n <= -1) return -1;
		if (n == 0) rv = lv;
	}

	*v = rv;
	return 0;
}

int Awk::Value::get (const char_t** str, size_t* len) const
{
#if 0
if v is a string, return the pointer and the length.
otherwise call valtostr.... how can i handle free then???

	real_t rv = 0;

	if (run != QSE_NULL)
	{
		long_t lv;
		int n = qse_awk_rtx_valtostr (run->rtx, val, &lv, &rv);
		if (n <= -1) return -1;
		if (n == 0) rv = lv;
	}

	*v = rv;
	return 0;
#endif
}

int Awk::Value::set (val_t* v)
{
	if (run == QSE_NULL) return -1;
	return set (run, v);
}

int Awk::Value::set (Run* r, val_t* v)
{
	if (run != QSE_NULL)
		qse_awk_rtx_refdownval (run->rtx, val);
	qse_awk_rtx_refupval (r->rtx, v);

	run = r;
	val = v;

	return 0;
}

int Awk::Value::set (long_t v)
{
	if (run == QSE_NULL) return -1;
	return set (run, v);
}

int Awk::Value::set (Run* r, long_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makeintval (r->rtx, v);
	if (tmp == QSE_NULL) return -1;

	int n = set (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::set (real_t v)
{
	if (run == QSE_NULL) return -1;
	return set (run, v);
}

int Awk::Value::set (Run* r, real_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makerealval (r->rtx, v);
	if (tmp == QSE_NULL) return -1;
			
	int n = set (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::set (const char_t* str, size_t len)
{
	if (run == QSE_NULL) return -1;
	return set (run, str, len);
}

int Awk::Value::set (Run* r, const char_t* str, size_t len)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makestrval (r->rtx, str, len);
	if (tmp == QSE_NULL) return -1;

	int n = set (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::set (const char_t* str)
{
	if (run == QSE_NULL) return -1;
	return set (run, str);
}

int Awk::Value::set (Run* r, const char_t* str)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makestrval0 (r->rtx, str);
	if (tmp == QSE_NULL) return -1;

	int n = set (r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setIndexed (const char_t* idx, size_t isz, val_t* v)
{
	if (run == QSE_NULL) return -1;
	return setIndexed (run, idx, isz, v);
}

int Awk::Value::setIndexed (Run* r, const char_t* idx, size_t isz, val_t* v)
{
	if (val->type != QSE_AWK_VAL_MAP)
	{
		/* the previous value is not a map. 
		 * a new map value needs to be created first */
		val_t* map = qse_awk_rtx_makemapval (run->rtx);
		if (map == QSE_NULL) return -1;

		qse_awk_rtx_refupval (r->rtx, map);
		qse_awk_rtx_refupval (r->rtx, v);

		/* update the map with a given value */
		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)map)->map, 
			(char_t*)idx, isz, v, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (r->rtx, v);
			qse_awk_rtx_refdownval (r->rtx, map);
			run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
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
			return -1;
		}

		qse_awk_rtx_refupval (r->rtx, v);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)val)->map, 
			(char_t*)idx, isz, v, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (r->rtx, v);
			run->setError (ERR_NOMEM);
			return -1;
		}
	}

	return 0;
}

int Awk::Value::setIndexed (const char_t* idx, size_t isz, long_t v)
{
	if (run == QSE_NULL) return -1;
	return setIndexed (run, idx, isz, v);
}

int Awk::Value::setIndexed (Run* r, const char_t* idx, size_t isz, long_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makeintval (r->rtx, v);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = setIndexed (r, idx, isz, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexed (const char_t* idx, size_t isz, real_t v)
{
	if (run == QSE_NULL) return -1;
	return setIndexed (run, idx, isz, v);
}

int Awk::Value::setIndexed (Run* r, const char_t* idx, size_t isz, real_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makerealval (r->rtx, v);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = setIndexed (r, idx, isz, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexed (const char_t* idx, size_t isz, const char_t* str, size_t len)
{
	if (run == QSE_NULL) return -1;
	return setIndexed (run, idx, isz, str, len);
}

int Awk::Value::setIndexed (Run* r, const char_t* idx, size_t isz, const char_t* str, size_t len)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makestrval (r->rtx, str, len);
	if (tmp == QSE_NULL) return -1;

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = setIndexed (r, idx, isz, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

bool Awk::Value::isIndexed () const
{
	QSE_ASSERT (val != QSE_NULL);
	return val->type == QSE_AWK_VAL_MAP;
}

int Awk::Value::getIndexed (const char_t* idx, size_t isz, Value& v) const
{
	QSE_ASSERT (val != QSE_NULL);

	// not a map. v is just nil. not an error 
	if (val->type != QSE_AWK_VAL_MAP) 
	{	
		v.clear ();
		return 0;
	}

	// get the value from the map.
	qse_awk_val_map_t* m = (qse_awk_val_map_t*)val;
	pair_t* pair = qse_map_search (m->map, idx, isz);

	// the key is not found. it is not an error. v is just nil 
	if (pair == QSE_NULL) 
	{
		v.clear ();
		return 0; 
	}

	// if v.set fails, it should return an error 
	return v.set (run, (val_t*)QSE_MAP_VPTR(pair));
}

//////////////////////////////////////////////////////////////////
// Awk::Argument
//////////////////////////////////////////////////////////////////

Awk::Argument::Argument (Run& run): run (&run), val (QSE_NULL)
{
	this->inum = 0;
	this->rnum = 0.0;

	this->str.ptr = QSE_NULL;
	this->str.len = 0;
}

Awk::Argument::Argument (Run* run): run (run), val (QSE_NULL)
{
	this->inum = 0;
	this->rnum = 0.0;

	this->str.ptr = QSE_NULL;
	this->str.len = 0;
}

Awk::Argument::Argument (): run (QSE_NULL), val (QSE_NULL)
{
	this->inum = 0;
	this->rnum = 0.0;

	this->str.ptr = QSE_NULL;
	this->str.len = 0;
}

Awk::Argument::~Argument ()
{
	clear ();
}

void Awk::Argument::clear ()
{
	if (this->val == QSE_NULL)
	{
		/* case 1. not initialized.
		 * case 2. initialized with the second init.
		 * none of the cases. create a new string so the sttring
		 * that str.ptr is pointing to doesn't have to be freed */
		this->str.ptr = QSE_NULL;
		this->str.len = 0;
	}
	else if (this->val->type == QSE_AWK_VAL_MAP)
	{
		QSE_ASSERT (this->run != QSE_NULL);

		/* when the value is a map, str.ptr and str.len are
		 * used for index iteration in getFirstIndex & getNextIndex */
		qse_awk_rtx_refdownval (this->run->rtx, this->val);
		this->val = QSE_NULL;
	}
	else
	{
		QSE_ASSERT (this->run != QSE_NULL);

		if (this->str.ptr != QSE_NULL)
		{
			if (this->val->type != QSE_AWK_VAL_STR)
			{
				awk_t* awk = this->run->awk->awk;
				qse_awk_free (awk, this->str.ptr);
			}

			this->str.ptr = QSE_NULL;
			this->str.len = 0;
		}

		if (this->val != QSE_NULL) 
		{
			qse_awk_rtx_refdownval (this->run->rtx, this->val);
			this->val = QSE_NULL;
		}

	}

	this->rnum = 0.0;
	this->inum = 0;
}

void* Awk::Argument::operator new (size_t n, awk_t* awk) throw ()
{
	void* ptr = qse_awk_alloc (awk, QSE_SIZEOF(awk) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(awk_t**)ptr = awk;
	return (char*)ptr+QSE_SIZEOF(awk);
}

void* Awk::Argument::operator new[] (size_t n, awk_t* awk) throw ()
{
	void* ptr = qse_awk_alloc (awk, QSE_SIZEOF(awk) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(awk_t**)ptr = awk;
	return (char*)ptr+QSE_SIZEOF(awk);
}

#if !defined(__BORLANDC__)
void Awk::Argument::operator delete (void* ptr, awk_t* awk)
{
	qse_awk_free (awk, (char*)ptr-QSE_SIZEOF(awk));
}

void Awk::Argument::operator delete[] (void* ptr, awk_t* awk)
{
	qse_awk_free (awk, (char*)ptr-QSE_SIZEOF(awk));
}
#endif

void Awk::Argument::operator delete (void* ptr)
{
	void* p = (char*)ptr-QSE_SIZEOF(awk_t*);
	qse_awk_free (*(awk_t**)p, p);
}

void Awk::Argument::operator delete[] (void* ptr)
{
	void* p = (char*)ptr-QSE_SIZEOF(awk_t*);
	qse_awk_free (*(awk_t**)p, p);
}

int Awk::Argument::init (val_t* v)
{
	// this method is used internally only
	// and should never be called more than once 
	QSE_ASSERT (this->val == QSE_NULL);
	QSE_ASSERT (v != QSE_NULL);

	qse_awk_rtx_refupval (this->run->rtx, v);
	this->val = v;

	if (v->type == QSE_AWK_VAL_STR)
	{
		int n = qse_awk_rtx_valtonum (
			this->run->rtx, v, &this->inum, &this->rnum);
		if (n == 0) 
		{
			this->rnum = (qse_real_t)this->inum;
			this->str.ptr = ((qse_awk_val_str_t*)this->val)->ptr;
			this->str.len = ((qse_awk_val_str_t*)this->val)->len;
			return 0;
		}
		else if (n == 1) 
		{
			this->inum = (qse_long_t)this->rnum;
			this->str.ptr = ((qse_awk_val_str_t*)this->val)->ptr;
			this->str.len = ((qse_awk_val_str_t*)this->val)->len;
			return 0;
		}
	}
	else if (v->type == QSE_AWK_VAL_INT)
	{
		this->inum = ((qse_awk_val_int_t*)v)->val;
		this->rnum = (qse_real_t)((qse_awk_val_int_t*)v)->val;

		this->str.ptr = qse_awk_rtx_valtocpldup (
			this->run->rtx, v, &this->str.len);
		if (this->str.ptr != QSE_NULL) return 0;
	}
	else if (v->type == QSE_AWK_VAL_REAL)
	{
		this->inum = (qse_long_t)((qse_awk_val_real_t*)v)->val;
		this->rnum = ((qse_awk_val_real_t*)v)->val;

		this->str.ptr = qse_awk_rtx_valtocpldup (
			this->run->rtx, v, &this->str.len);
		if (this->str.ptr != QSE_NULL) return 0;
	}
	else if (v->type == QSE_AWK_VAL_NIL)
	{
		this->inum = 0;
		this->rnum = 0.0;

		this->str.ptr = qse_awk_rtx_valtocpldup (
			this->run->rtx, v, &this->str.len);
		if (this->str.ptr != QSE_NULL) return 0;
	}
	else if (v->type == QSE_AWK_VAL_MAP)
	{
		this->inum = 0;
		this->rnum = 0.0;
		this->str.ptr = QSE_NULL;
		this->str.len = 0;
		return 0;
	}

	// an error has occurred
	qse_awk_rtx_refdownval (this->run->rtx, v);
	this->val = QSE_NULL;
	return -1;
}

int Awk::Argument::init (const char_t* str, size_t len)
{
	QSE_ASSERT (this->val == QSE_NULL);

	this->str.ptr = (char_t*)str;
	this->str.len = len;

	if (qse_awk_rtx_strtonum (
		this->run->rtx, 0, 
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

	if (this->val != QSE_NULL && 
	    this->val->type == QSE_AWK_VAL_MAP)
	{
		*len = 0;
		return QSE_T("");
	}
	else if (this->str.ptr == QSE_NULL)
	{
		*len = 0;
		return QSE_T("");
	}
	else
	{
		*len = this->str.len;
		return this->str.ptr;
	}
}

bool Awk::Argument::isIndexed () const
{
	if (this->val == QSE_NULL) return false;
	return this->val->type == QSE_AWK_VAL_MAP;
}

int Awk::Argument::getIndexed (const char_t* idxptr, Awk::Argument& val) const
{
	return getIndexed (idxptr, qse_strlen(idxptr), val);
}

int Awk::Argument::getIndexed (
	const char_t* idxptr, size_t idxlen, Awk::Argument& val) const
{
	val.clear ();

	// not initialized yet. val is just nil. not an error
	if (this->val == QSE_NULL) return 0;
	// not a map. val is just nil. not an error 
	if (this->val->type != QSE_AWK_VAL_MAP) return 0;

	// get the value from the map.
	qse_awk_val_map_t* m = (qse_awk_val_map_t*)this->val;
	pair_t* pair = qse_map_search (m->map, idxptr, idxlen);

	// the key is not found. it is not an error. val is just nil 
	if (pair == QSE_NULL) return 0; 

	// if val.init fails, it should return an error 
	return val.init ((val_t*)QSE_MAP_VPTR(pair));
}

int Awk::Argument::getIndexed (long_t idx, Argument& val) const
{
	val.clear ();

	// not initialized yet. val is just nil. not an error
	if (this->val == QSE_NULL) return 0;

	// not a map. val is just nil. not an error 
	if (this->val->type != QSE_AWK_VAL_MAP) return 0;

	char_t ri[128];

	int rl = Awk::sprintf (
		(awk_t*)this->run->awk, ri, QSE_COUNTOF(ri), 
	#if QSE_SIZEOF_LONG_LONG > 0
		QSE_T("%lld"), (long long)idx
	#elif QSE_SIZEOF___INT64 > 0
		QSE_T("%I64d"), (__int64)idx
	#elif QSE_SIZEOF_LONG > 0
		QSE_T("%ld"), (long)idx
	#elif QSE_SIZEOF_INT > 0
		QSE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		run->setError (ERR_INTERN, 0, QSE_NULL, 0);
		return -1;
	}

	// get the value from the map.
	qse_awk_val_map_t* m = (qse_awk_val_map_t*)this->val;
	pair_t* pair = qse_map_search (m->map, ri, rl);

	// the key is not found. it is not an error. val is just nil 
	if (pair == QSE_NULL) return 0; 

	// if val.init fails, it should return an error 
	return val.init ((val_t*)QSE_MAP_VPTR(pair));
}

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

//////////////////////////////////////////////////////////////////
// Awk::Return
//////////////////////////////////////////////////////////////////

Awk::Return::Return (Run& run): run(&run), val(qse_awk_val_nil)
{
}

Awk::Return::Return (Run* run): run(run), val(qse_awk_val_nil)
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
	if (this->run == QSE_NULL) return -1;

	val_t* x = qse_awk_rtx_makeintval (this->run->rtx, v);
	if (x == QSE_NULL) return -1;

	qse_awk_rtx_refdownval (this->run->rtx, this->val);
	this->val = x;
	qse_awk_rtx_refupval (this->run->rtx, this->val);

	return 0;
}

int Awk::Return::set (real_t v)
{
	if (this->run == QSE_NULL) return -1;

	val_t* x = qse_awk_rtx_makerealval (this->run->rtx, v);
	if (x == QSE_NULL) return -1;

	qse_awk_rtx_refdownval (this->run->rtx, this->val);
	this->val = x;
	qse_awk_rtx_refupval (this->run->rtx, this->val);

	return 0;
}

int Awk::Return::set (const char_t* ptr, size_t len)
{
	if (this->run == QSE_NULL) return -1;

	val_t* x = qse_awk_rtx_makestrval (this->run->rtx, ptr, len);
	if (x == QSE_NULL) return -1;

	qse_awk_rtx_refdownval (this->run->rtx, this->val);
	this->val = x;
	qse_awk_rtx_refupval (this->run->rtx, this->val);
	return 0;
}

bool Awk::Return::isIndexed () const
{
	if (this->val == QSE_NULL) return false;
	return this->val->type == QSE_AWK_VAL_MAP;
}

int Awk::Return::setIndexed (const char_t* idx, size_t iln, long_t v)
{
	if (this->run == QSE_NULL) return -1;

	int opt = this->run->awk->getOption();
	if ((opt & OPT_MAPTOVAR) == 0)
	{
		/* refer to run_return in run.c */
		this->run->setError (ERR_MAPNOTALLOWED, 0, QSE_NULL, 0);
		return -1;
	}

	if (this->val->type != QSE_AWK_VAL_MAP)
	{
		val_t* x = qse_awk_rtx_makemapval (this->run->rtx);
		if (x == QSE_NULL) return -1;

		qse_awk_rtx_refupval (this->run->rtx, x);

		val_t* x2 = qse_awk_rtx_makeintval (this->run->rtx, v);
		if (x2 == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x);
			return -1;
		}

		qse_awk_rtx_refupval (this->run->rtx, x2);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)x)->map,
			(char_t*)idx, iln, x2, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x2);
			qse_awk_rtx_refdownval (this->run->rtx, x);
			this->run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
			return -1;
		}

		qse_awk_rtx_refdownval (this->run->rtx, this->val);
		this->val = x;
	}
	else
	{
		val_t* x2 = qse_awk_rtx_makeintval (this->run->rtx, v);
		if (x2 == QSE_NULL) return -1;

		qse_awk_rtx_refupval (this->run->rtx, x2);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)this->val)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x2);
			this->run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
			return -1;
		}
	}

	return 0;
}

int Awk::Return::setIndexed (const char_t* idx, size_t iln, real_t v)
{
	if (this->run == QSE_NULL) return -1;

	int opt = this->run->awk->getOption();
	if ((opt & OPT_MAPTOVAR) == 0)
	{
		/* refer to run_return in run.c */
		this->run->setError (ERR_MAPNOTALLOWED, 0, QSE_NULL, 0);
		return -1;
	}

	if (this->val->type != QSE_AWK_VAL_MAP)
	{
		val_t* x = qse_awk_rtx_makemapval (this->run->rtx);
		if (x == QSE_NULL) return -1;

		qse_awk_rtx_refupval (this->run->rtx, x);

		val_t* x2 = qse_awk_rtx_makerealval (this->run->rtx, v);
		if (x2 == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x);
			return -1;
		}

		qse_awk_rtx_refupval (this->run->rtx, x2);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)x)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x2);
			qse_awk_rtx_refdownval (this->run->rtx, x);
			this->run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
			return -1;
		}

		qse_awk_rtx_refdownval (this->run->rtx, this->val);
		this->val = x;
	}
	else
	{
		val_t* x2 = qse_awk_rtx_makerealval (this->run->rtx, v);
		if (x2 == QSE_NULL) return -1;

		qse_awk_rtx_refupval (this->run->rtx, x2);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)this->val)->map,
			(char_t*)idx, iln, x2, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x2);
			this->run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
			return -1;
		}
	}

	return 0;
}

int Awk::Return::setIndexed (const char_t* idx, size_t iln, const char_t* str, size_t sln)
{
	if (this->run == QSE_NULL) return -1;

	int opt = this->run->awk->getOption();
	if ((opt & OPT_MAPTOVAR) == 0)
	{
		/* refer to run_return in run.c */
		this->run->setError (ERR_MAPNOTALLOWED, 0, QSE_NULL, 0);
		return -1;
	}

	if (this->val->type != QSE_AWK_VAL_MAP)
	{
		val_t* x = qse_awk_rtx_makemapval (this->run->rtx);
		if (x == QSE_NULL) return -1;

		qse_awk_rtx_refupval (this->run->rtx, x);

		val_t* x2 = qse_awk_rtx_makestrval (this->run->rtx, str, sln);
		if (x2 == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x);
			return -1;
		}

		qse_awk_rtx_refupval (this->run->rtx, x2);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)x)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x2);
			qse_awk_rtx_refdownval (this->run->rtx, x);
			this->run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
			return -1;
		}

		qse_awk_rtx_refdownval (this->run->rtx, this->val);
		this->val = x;
	}
	else
	{
		val_t* x2 = qse_awk_rtx_makestrval (this->run->rtx, str, sln);
		if (x2 == QSE_NULL) return -1;

		qse_awk_rtx_refupval (this->run->rtx, x2);

		pair_t* pair = qse_map_upsert (
			((qse_awk_val_map_t*)this->val)->map, 
			(char_t*)idx, iln, x2, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refdownval (this->run->rtx, x2);
			this->run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
			return -1;
		}
	}

	return 0;
}

int Awk::Return::setIndexed (long_t idx, long_t v)
{
	if (this->run == QSE_NULL) return -1;

	char_t ri[128];

	int rl = Awk::sprintf (
		(awk_t*)this->run->awk, ri, QSE_COUNTOF(ri), 
	#if QSE_SIZEOF_LONG_LONG > 0
		QSE_T("%lld"), (long long)idx
	#elif QSE_SIZEOF___INT64 > 0
		QSE_T("%I64d"), (__int64)idx
	#elif QSE_SIZEOF_LONG > 0
		QSE_T("%ld"), (long)idx
	#elif QSE_SIZEOF_INT > 0
		QSE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		this->run->setError (ERR_INTERN, 0, QSE_NULL, 0);
		return -1;
	}

	return setIndexed (ri, rl, v);
}

int Awk::Return::setIndexed (long_t idx, real_t v)
{
	if (this->run == QSE_NULL) return -1;

	char_t ri[128];

	int rl = Awk::sprintf (
		(awk_t*)this->run->awk, ri, QSE_COUNTOF(ri), 
	#if QSE_SIZEOF_LONG_LONG > 0
		QSE_T("%lld"), (long long)idx
	#elif QSE_SIZEOF___INT64 > 0
		QSE_T("%I64d"), (__int64)idx
	#elif QSE_SIZEOF_LONG > 0
		QSE_T("%ld"), (long)idx
	#elif QSE_SIZEOF_INT > 0
		QSE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		this->run->setError (ERR_INTERN, 0, QSE_NULL, 0);
		return -1;
	}

	return setIndexed (ri, rl, v);
}

int Awk::Return::setIndexed (long_t idx, const char_t* str, size_t sln)
{
	if (this->run == QSE_NULL) return -1;

	char_t ri[128];

	int rl = Awk::sprintf (
		(awk_t*)this->run->awk, ri, QSE_COUNTOF(ri), 
	#if QSE_SIZEOF_LONG_LONG > 0
		QSE_T("%lld"), (long long)idx
	#elif QSE_SIZEOF___INT64 > 0
		QSE_T("%I64d"), (__int64)idx
	#elif QSE_SIZEOF_LONG > 0
		QSE_T("%ld"), (long)idx
	#elif QSE_SIZEOF_INT > 0
		QSE_T("%d"), (int)idx
	#else
		#error unsupported size	
	#endif
		);

	if (rl < 0)
	{
		this->run->setError (ERR_INTERN, 0, QSE_NULL, 0);
		return -1;
	}

	return setIndexed (ri, rl, str, sln);
}

void Awk::Return::clear ()
{
	qse_awk_rtx_refdownval (this->run->rtx, this->val);
	this->val = qse_awk_val_nil;
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

void Awk::Run::setError (ErrorNumber code)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_awk_rtx_seterror (this->rtx, (errnum_t)code, 0, QSE_NULL);
}

void Awk::Run::setError (ErrorNumber code, size_t line)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_awk_rtx_seterror (this->rtx, (errnum_t)code, line, QSE_NULL);
}

void Awk::Run::setError (ErrorNumber code, size_t line, const char_t* arg)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_cstr_t x = { arg, qse_strlen(arg) };
	qse_awk_rtx_seterror (this->rtx, (errnum_t)code, line, &x);
}

void Awk::Run::setError (
	ErrorNumber code, size_t line, const char_t* arg, size_t len)
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_cstr_t x = { arg, len };
	qse_awk_rtx_seterror (this->rtx, (errnum_t)code, line, &x);
}

void Awk::Run::setErrorWithMessage (
	ErrorNumber code, size_t line, const char_t* msg)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	qse_awk_errinf_t errinf;

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

int Awk::Run::setGlobal (int id, const Return& gbl)
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	return qse_awk_rtx_setgbl (this->rtx, id, gbl.toVal());
}

int Awk::Run::getGlobal (int id, Argument& gbl) const
{
	QSE_ASSERT (this->rtx != QSE_NULL);

	gbl.clear ();
	return gbl.init (qse_awk_rtx_getgbl (this->rtx, id));
}

//////////////////////////////////////////////////////////////////
// Awk
//////////////////////////////////////////////////////////////////

Awk::Awk () throw (): awk (QSE_NULL), functionMap (QSE_NULL),
	sourceIn (this, Source::READ), sourceOut (this, Source::WRITE),
	errnum (ERR_NOERR), errlin (0), runCallback (false), 
	runctx (this)

{
	this->errmsg[0] = QSE_T('\0');
}

Awk::operator Awk::awk_t* () const
{
	return this->awk;
}

Awk::ErrorNumber Awk::getErrorNumber () const
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

void Awk::setError (ErrorNumber code)
{
	setError (code, 0, QSE_NULL, 0);
}

void Awk::setError (ErrorNumber code, size_t line)
{
	setError (code, line, QSE_NULL, 0);
}

void Awk::setError (ErrorNumber code, size_t line, const char_t* arg)
{
	setError (code, line, arg, qse_strlen(arg));
}

void Awk::setError (ErrorNumber code, size_t line, const char_t* arg, size_t len)
{
	if (awk != QSE_NULL)
	{
		qse_cstr_t x = { arg, len };
		qse_awk_seterror (awk, (errnum_t)code, line, &x);
		retrieveError ();
	}
	else
	{
		this->errnum = code;
		this->errlin = line;
		qse_strxcpy (this->errmsg, QSE_COUNTOF(this->errmsg), 
			QSE_T("not ready to set an error message"));
	}
}

void Awk::setErrorWithMessage (ErrorNumber code, size_t line, const char_t* msg)
{
	if (awk != QSE_NULL)
	{
		qse_awk_errinf_t errinf;

		errinf.num = (errnum_t)code;
		errinf.lin = line;
		qse_strxcpy (errinf.msg, QSE_COUNTOF(errinf.msg), msg);

		qse_awk_seterrinf (awk, &errinf);
		retrieveError ();
	}
	else
	{
		this->errnum = code;
		this->errlin = line;
		qse_strxcpy (this->errmsg, QSE_COUNTOF(this->errmsg), msg);
	}
}

void Awk::clearError ()
{
	this->errnum = ERR_NOERR;
	this->errlin = 0;
	this->errmsg[0] = QSE_T('\0');
}

void Awk::retrieveError ()
{
	if (this->awk == QSE_NULL) 
	{
		clearError ();
	}
	else
	{
		errnum_t num;
		const char_t* msg;

		qse_awk_geterror (this->awk, &num, &this->errlin, &msg);
		this->errnum = (ErrorNumber)num;
		qse_strxcpy (this->errmsg, QSE_COUNTOF(this->errmsg), msg);
	}
}

void Awk::retrieveError (rtx_t* rtx)
{
	errnum_t num;
	const char_t* msg;

	qse_awk_rtx_geterror (rtx, &num, &this->errlin, &msg);
	this->errnum = (ErrorNumber)num;
	qse_strxcpy (this->errmsg, QSE_COUNTOF(this->errmsg), msg);
}

int Awk::open ()
{
	QSE_ASSERT (awk == QSE_NULL && functionMap == QSE_NULL);

	qse_awk_prm_t prm;
	prm.pow     = pow;
	prm.sprintf = sprintf;

	awk = qse_awk_open (this, QSE_SIZEOF(xtn_t), &prm);
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

	//functionMap = qse_map_open (
	//	this, 512, 70, freeFunctionMapValue, QSE_NULL, 
	//	qse_awk_getmmgr(awk));
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
	qse_map_setfreeer (functionMap, QSE_MAP_VAL, freeFunctionMapValue);
	qse_map_setscale (functionMap, QSE_MAP_KEY, QSE_SIZEOF(qse_char_t));

	runCallback = false;
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
	runCallback = false;
}

void Awk::setOption (int opt)
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_setoption (awk, opt);
}

int Awk::getOption () const
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_getoption (awk);
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

const Awk::char_t* Awk::getErrorString (ErrorNumber num) const
{
	QSE_ASSERT (dflerrstr != QSE_NULL);
	return dflerrstr (awk, (errnum_t)num);
}

const Awk::char_t* Awk::xerrstr (awk_t* a, errnum_t num) throw ()
{
	Awk* awk = *(Awk**)QSE_XTN(a);
	try
	{
		return awk->getErrorString ((ErrorNumber)num);
	}
	catch (...)
	{
		return awk->dflerrstr (a, num);
	}
}

int Awk::getWord (
	const char_t* ow, qse_size_t owl,
	const char_t** nw, qse_size_t* nwl)
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_getword (awk, ow, owl, nw, nwl);
}

int Awk::setWord (const char_t* ow, const char_t* nw)
{
	return setWord (ow, qse_strlen(ow), nw, qse_strlen(nw));
}

int Awk::setWord (
	const char_t* ow, qse_size_t owl,
	const char_t* nw, qse_size_t nwl)
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_setword (awk, ow, owl, nw, nwl);
}

int Awk::unsetWord (const char_t* ow)
{
	return unsetWord (ow, qse_strlen(ow));
}

int Awk::unsetWord (const char_t* ow, qse_size_t owl)
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_setword (awk, ow, owl, QSE_NULL, 0);
}

int Awk::unsetAllWords ()
{
	QSE_ASSERT (awk != QSE_NULL);
	return qse_awk_setword (awk, QSE_NULL, 0, QSE_NULL, 0);
}

Awk::Run* Awk::parse (Source* in, Source* out)
{
	QSE_ASSERT (awk != QSE_NULL);

	fini_runctx ();

	sourceReader = in;
	sourceWriter = out;

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

int Awk::init_runctx ()
{
	if (runctx.rtx != QSE_NULL) return 0;

	qse_awk_rio_t rio;
	qse_awk_rcb_t rcb;

	rio.pipe    = pipeHandler;
	rio.file    = fileHandler;
	rio.console = consoleHandler;

	if (runCallback)
	{
		QSE_MEMSET (&rcb, 0, QSE_SIZEOF(rcb));
		rcb.on_loop_enter = onLoopEnter;
		rcb.on_loop_exit  = onLoopExit;
		rcb.on_statement  = onStatement;
		rcb.udd           = &runctx;
	}

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

	if (runCallback) qse_awk_rtx_setrcb (rtx, &rcb);
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

int Awk::loop ()
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	int n = qse_awk_rtx_loop (runctx.rtx);
	if (n <= -1) retrieveError (runctx.rtx);
	return n;
}

int Awk::call (const char_t* name, const Return* args, size_t nargs)
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	val_t** ptr = QSE_NULL;

	if (args != QSE_NULL)
	{
		ptr = (val_t**) qse_awk_alloc (awk, QSE_SIZEOF(val_t*) * nargs);
		if (ptr == QSE_NULL)
		{
			runctx.setError (ERR_NOMEM);
			return -1;
		}

		for (size_t i = 0; i < nargs; i++) ptr[i] = args[i].val;
	}

	val_t* ret = qse_awk_rtx_call (runctx.rtx, name, ptr, nargs);

	if (ptr != QSE_NULL) qse_awk_free (awk, ptr);

	if (ret == QSE_NULL) 
	{
		retrieveError (runctx.rtx);
		return -1;
	}

// TODO: how can i store it???
	qse_awk_rtx_refdownval (runctx.rtx, ret);
	return 0;
}

void Awk::stop ()
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_stopall (awk);
}

int Awk::dispatchFunction (Run* run, const char_t* name, size_t len)
{
	pair_t* pair;
	awk_t* awk = run->awk->awk;

	pair = qse_map_search (functionMap, name, len);
	if (pair == QSE_NULL) 
	{
		run->setError (ERR_FUNNF, 0, name, len);
		return -1;
	}

	FunctionHandler handler;
       	handler = *(FunctionHandler*)QSE_MAP_VPTR(pair);	

	size_t i, nargs = qse_awk_rtx_getnargs(run->rtx);

	//Argument* args = QSE_NULL;
	//try { args = new Argument [nargs]; } catch (...)  {}
	Argument* args = new(awk) Argument[nargs];
	if (args == QSE_NULL) 
	{
		run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
		return -1;
	}

	for (i = 0; i < nargs; i++)
	{
		args[i].run = run; // dirty late initialization 
		                   // due to c++ array creation limitation.

		val_t* v = qse_awk_rtx_getarg (run->rtx, i);
		if (args[i].init (v) == -1)
		{
			run->setError (ERR_NOMEM, 0, QSE_NULL, 0);
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

	qse_awk_rtx_setretval (run->rtx, ret);
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
	if (n == -1) retrieveError ();
	return n;
}

int Awk::deleteGlobal (const char_t* name)
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_delgbl (awk, name, qse_strlen(name));
	if (n == -1) retrieveError ();
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

	void* p = qse_awk_addfnc (awk, name, nameLen,
	                          0, minArgs, maxArgs, QSE_NULL, 
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

void Awk::enableRunCallback ()
{
	runCallback = true;
}

void Awk::disableRunCallback ()
{
	runCallback = false;
}

bool Awk::onLoopEnter (Run& run)
{
	return true;
}

void Awk::onLoopExit (Run& run, const Argument& ret)
{
}

void Awk::onStatement (Run& run, size_t line)
{
}

Awk::ssize_t Awk::readSource (
	awk_t* awk, qse_awk_sio_cmd_t cmd, char_t* data, size_t count)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);

	switch (cmd)
	{
		case QSE_AWK_SIO_OPEN:
			return xtn->awk->sourceReader->open (xtn->awk->sourceIn);
		case QSE_AWK_SIO_CLOSE:
			return xtn->awk->sourceReader->close (xtn->awk->sourceIn);
		case QSE_AWK_SIO_READ:
			return xtn->awk->sourceReader->read (xtn->awk->sourceIn, data, count);
		default:
			return -1;
	}
}

Awk::ssize_t Awk::writeSource (
	awk_t* awk, qse_awk_sio_cmd_t cmd, char_t* data, size_t count)
{
	xtn_t* xtn = (xtn_t*) QSE_XTN (awk);

	switch (cmd)
	{
		case QSE_AWK_SIO_OPEN:
			return xtn->awk->sourceWriter->open (xtn->awk->sourceOut);
		case QSE_AWK_SIO_CLOSE:
			return xtn->awk->sourceWriter->close (xtn->awk->sourceOut);
		case QSE_AWK_SIO_WRITE:
			return xtn->awk->sourceWriter->write (xtn->awk->sourceOut, data, count);
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

		case QSE_AWK_RIO_NEXT:
			return -1;
	}

	return -1;
}

Awk::ssize_t Awk::fileHandler (
	rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
	char_t* data, size_t count)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	Awk* awk = rxtn->run->awk;

	QSE_ASSERT ((riod->type & 0xFF) == QSE_AWK_RIO_FILE);

	File file (rxtn->run, riod);

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

		case QSE_AWK_RIO_NEXT:
			return -1;
	}

	return -1;
}

Awk::ssize_t Awk::consoleHandler (
	rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod,
	char_t* data, size_t count)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	Awk* awk = rxtn->run->awk;

	QSE_ASSERT ((riod->type & 0xFF) == QSE_AWK_RIO_CONSOLE);

	Console console (rxtn->run, riod);

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
	}

	return -1;
}

int Awk::functionHandler (rtx_t* rtx, const char_t* name, size_t len)
{
	rxtn_t* rxtn = (rxtn_t*) QSE_XTN (rtx);
	return rxtn->run->awk->dispatchFunction (rxtn->run, name, len);
}	

void Awk::freeFunctionMapValue (map_t* map, void* dptr, size_t dlen)
{
	Awk* awk = *(Awk**) QSE_XTN (map);
	qse_awk_free (awk->awk, dptr);
}

int Awk::onLoopEnter (rtx_t* rtx, void* data)
{
	Run* run = (Run*)data;
	return run->awk->onLoopEnter(*run)? 0: -1;
}

void Awk::onLoopExit (rtx_t* rtx, val_t* ret, void* data)
{
	Run* run = (Run*)data;

	Argument x (run);
	if (x.init (ret) == -1) 
		qse_awk_rtx_seterrnum (run->rtx, (errnum_t)ERR_NOMEM);
	else run->awk->onLoopExit (*run, x);
}

void Awk::onStatement (rtx_t* rtx, size_t line, void* data)
{
	Run* run = (Run*)data;
	run->awk->onStatement (*run, line);
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

