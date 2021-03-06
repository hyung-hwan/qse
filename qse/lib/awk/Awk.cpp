/*
 * $Id$
 * 
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/awk/Awk.hpp>
#include <qse/cmn/str.h>
#include "../cmn/mem-prv.h"
#include "awk-prv.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

//////////////////////////////////////////////////////////////////
// Awk::Source
//////////////////////////////////////////////////////////////////

struct xtn_t
{
	Awk* awk;
	qse_awk_ecb_t ecb;
};

struct rxtn_t
{
	Awk::Run* run;
};

Awk::NoSource Awk::Source::NONE;

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE xtn_t* GET_XTN(qse_awk_t* awk) { return (xtn_t*)((qse_uint8_t*)qse_awk_getxtn(awk) - QSE_SIZEOF(xtn_t)); }
static QSE_INLINE rxtn_t* GET_RXTN(qse_awk_rtx_t* rtx) { return (rxtn_t*)((qse_uint8_t*)qse_awk_rtx_getxtn(rtx) - QSE_SIZEOF(rxtn_t)); }
#else
#define GET_XTN(awk) ((xtn_t*)((qse_uint8_t*)qse_awk_getxtn(awk) - QSE_SIZEOF(xtn_t)))
#define GET_RXTN(rtx) ((rxtn_t*)((qse_uint8_t*)qse_awk_rtx_getxtn(rtx) - QSE_SIZEOF(rxtn_t)))
#endif

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
const qse_mchar_t* Awk::Value::getEmptyMbs()
{
	static const qse_mchar_t* EMPTY_STRING = QSE_MT("");
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

#if defined(QSE_AWK_VALUE_USE_IN_CLASS_PLACEMENT_NEW)
void* Awk::Value::operator new (size_t n, Run* run) throw ()
{
	void* ptr = qse_awk_rtx_allocmem (run->rtx, QSE_SIZEOF(run) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(Run**)ptr = run;
	return (char*)ptr + QSE_SIZEOF(run);
}

void* Awk::Value::operator new[] (size_t n, Run* run) throw () 
{
	void* ptr = qse_awk_rtx_allocmem (run->rtx, QSE_SIZEOF(run) + n);
	if (ptr == QSE_NULL) return QSE_NULL;

	*(Run**)ptr = run;
	return (char*)ptr + QSE_SIZEOF(run);
}

#if !defined(__BORLANDC__) && !defined(__WATCOMC__)
void Awk::Value::operator delete (void* ptr, Run* run) 
{
	// this placement delete is to be called when the constructor 
	// throws an exception and it's caught by the caller.
	qse_awk_rtx_freemem (run->rtx, (char*)ptr - QSE_SIZEOF(run));
}

void Awk::Value::operator delete[] (void* ptr, Run* run) 
{
	// this placement delete is to be called when the constructor 
	// throws an exception and it's caught by the caller.
	qse_awk_rtx_freemem (run->rtx, (char*)ptr - QSE_SIZEOF(run));
}
#endif

void Awk::Value::operator delete (void* ptr) 
{
	// this delete is to be called for normal destruction.
	void* p = (char*)ptr - QSE_SIZEOF(Run*);
	qse_awk_rtx_freemem ((*(Run**)p)->rtx, p);
}

void Awk::Value::operator delete[] (void* ptr) 
{
	// this delete is to be called for normal destruction.
	void* p = (char*)ptr - QSE_SIZEOF(Run*);
	qse_awk_rtx_freemem ((*(Run**)p)->rtx, p);
}
#endif

Awk::Value::Value (): run (QSE_NULL), val (qse_get_awk_nil_val()) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
	cached.mbs.ptr = QSE_NULL;
	cached.mbs.len = 0;
}

Awk::Value::Value (Run& run): run (&run), val (qse_get_awk_nil_val()) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
	cached.mbs.ptr = QSE_NULL;
	cached.mbs.len = 0;
}

Awk::Value::Value (Run* run): run (run), val (qse_get_awk_nil_val()) 
{
	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
	cached.mbs.ptr = QSE_NULL;
	cached.mbs.len = 0;
}

Awk::Value::Value (const Value& v): run(v.run), val(v.val)
{
	if (run) qse_awk_rtx_refupval (run->rtx, val);

	cached.str.ptr = QSE_NULL;
	cached.str.len = 0;
	cached.mbs.ptr = QSE_NULL;
	cached.mbs.len = 0;
}

Awk::Value::~Value ()
{
	if (run != QSE_NULL)
	{
		qse_awk_rtx_refdownval (run->rtx, val);
		if (cached.str.ptr) qse_awk_rtx_freemem (run->rtx, cached.str.ptr);
		if (cached.mbs.ptr) qse_awk_rtx_freemem (run->rtx, cached.mbs.ptr);
	}
}

Awk::Value& Awk::Value::operator= (const Value& v)
{
	if (this == &v) return *this;

	if (run)
	{
		qse_awk_rtx_refdownval (run->rtx, val);
		if (cached.str.ptr)
		{
			qse_awk_rtx_freemem (run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}
		if (cached.mbs.ptr)
		{
			qse_awk_rtx_freemem (run->rtx, cached.mbs.ptr);
			cached.mbs.ptr = QSE_NULL;
			cached.mbs.len = 0;
		}
	}

	run = v.run;
	val = v.val;

	if (run) qse_awk_rtx_refupval (run->rtx, val);

	return *this;
}

void Awk::Value::clear ()
{
	if (run)
	{
		qse_awk_rtx_refdownval (run->rtx, val);

		if (cached.str.ptr)
		{
			qse_awk_rtx_freemem (run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}
		if (cached.mbs.ptr)
		{
			qse_awk_rtx_freemem (run->rtx, cached.mbs.ptr);
			cached.mbs.ptr = QSE_NULL;
			cached.mbs.len = 0;
		}

		run = QSE_NULL;
		val = qse_get_awk_nil_val();
	}
}

Awk::Value::operator Awk::int_t () const
{
	int_t v;
	if (this->getInt(&v) <= -1) v = 0;
	return v;
}

Awk::Value::operator Awk::flt_t () const
{
	flt_t v;
	if (this->getFlt(&v) <= -1) v = 0.0;
	return v;
}

Awk::Value::operator const Awk::char_t* () const
{
	const Awk::char_t* ptr;
	size_t len;
	if (Awk::Value::getStr(&ptr, &len) <= -1) ptr = getEmptyStr();
	return ptr;
}

#if defined(QSE_CHAR_IS_WCHAR)
Awk::Value::operator const qse_mchar_t* () const
{
	const qse_mchar_t* ptr;
	size_t len;
	if (Awk::Value::getMbs(&ptr, &len) <= -1) ptr = getEmptyMbs();
	return ptr;
}
#endif

int Awk::Value::getInt (int_t* v) const
{
	int_t lv = 0;

	QSE_ASSERT (this->val != QSE_NULL);

	if (this->run)
	{
		int n = qse_awk_rtx_valtoint(this->run->rtx, this->val, &lv);
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
		int n = qse_awk_rtx_valtoflt(this->run->rtx, this->val, &rv);
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

	if (this->run)
	{
		int n = qse_awk_rtx_valtonum(this->run->rtx, this->val, lv, fv);
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

	if (this->run)
	{
		if (QSE_AWK_RTX_GETVALTYPE(this->run->rtx, this->val) == QSE_AWK_VAL_STR)
		{
			p = ((qse_awk_val_str_t*)this->val)->val.ptr;
			l = ((qse_awk_val_str_t*)this->val)->val.len;
		}
		else
		{
			if (!cached.str.ptr)
			{
				qse_awk_rtx_valtostr_out_t out;
				out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
				if (qse_awk_rtx_valtostr(this->run->rtx, this->val, &out) <= -1)
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

int Awk::Value::getMbs (const qse_mchar_t** str, size_t* len) const
{
	const qse_mchar_t* p = getEmptyMbs();
	size_t l = 0;

	QSE_ASSERT (this->val != QSE_NULL);

	if (this->run)
	{
		if (QSE_AWK_RTX_GETVALTYPE(this->run->rtx, this->val) == QSE_AWK_VAL_MBS)
		{
			p = ((qse_awk_val_mbs_t*)this->val)->val.ptr;
			l = ((qse_awk_val_mbs_t*)this->val)->val.len;
		}
		else
		{
			if (!cached.mbs.ptr)
			{
				p = qse_awk_rtx_valtombsdup(this->run->rtx, this->val, &l);
				if (!p)
				{
					run->awk->retrieveError (this->run);
					return -1;
				}

				cached.mbs.ptr = (qse_mchar_t*)p;
				cached.mbs.len = l;
			}
			else
			{
				p = cached.mbs.ptr;
				l = cached.mbs.len;
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
	return this->setVal(this->run, v);
}

int Awk::Value::setVal (Run* r, val_t* v)
{
	if (this->run != QSE_NULL)
	{
		qse_awk_rtx_refdownval (this->run->rtx, val);
		if (cached.str.ptr)
		{
			qse_awk_rtx_freemem (this->run->rtx, cached.str.ptr);
			cached.str.ptr = QSE_NULL;
			cached.str.len = 0;
		}
		if (cached.mbs.ptr)
		{
			qse_awk_rtx_freemem (this->run->rtx, cached.mbs.ptr);
			cached.mbs.ptr = QSE_NULL;
			cached.mbs.len = 0;
		}
	}

	QSE_ASSERT (cached.str.ptr == QSE_NULL);
	QSE_ASSERT (cached.mbs.ptr == QSE_NULL);
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
	return this->setInt(this->run, v);
}

int Awk::Value::setInt (Run* r, int_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makeintval(r->rtx, v);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = this->setVal(r, tmp);
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
	return this->setFlt(this->run, v);
}

int Awk::Value::setFlt (Run* r, flt_t v)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makefltval(r->rtx, v);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = this->setVal(r, tmp);
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
	return this->setStr(this->run, str, len, numeric);
}

int Awk::Value::setStr (Run* r, const char_t* str, size_t len, bool numeric)
{
	val_t* tmp;

	cstr_t cstr;
	cstr.ptr = (char_t*)str;
	cstr.len = len;

	tmp = numeric? qse_awk_rtx_makenstrvalwithcstr(r->rtx, &cstr):
	               qse_awk_rtx_makestrvalwithcstr(r->rtx, &cstr);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = this->setVal(r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setStr (const char_t* str, bool numeric)
{
	if (this->run == QSE_NULL) return -1;
	return this->setStr(this->run, str, numeric);
}

int Awk::Value::setStr (Run* r, const char_t* str, bool numeric)
{
	val_t* tmp;
	tmp = numeric? qse_awk_rtx_makenstrvalwithstr(r->rtx, str):
	               qse_awk_rtx_makestrvalwithstr(r->rtx, str);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = this->setVal(r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}


int Awk::Value::setMbs (const qse_mchar_t* str, size_t len)
{
	if (this->run == QSE_NULL) 
	{
		/* no runtime context assoicated. unfortunately, i can't 
		 * set an error number for the same reason */
		return -1; 
	}
	return this->setMbs(this->run, str, len);
}

int Awk::Value::setMbs (Run* r, const qse_mchar_t* str, size_t len)
{
	val_t* tmp;

	qse_mcstr_t cstr;
	cstr.ptr = (qse_mchar_t*)str;
	cstr.len = len;

	tmp = qse_awk_rtx_makembsvalwithmcstr(r->rtx, &cstr);
	if (!tmp)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = this->setVal(r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setMbs (const qse_mchar_t* str)
{
	if (this->run == QSE_NULL) return -1;
	return this->setMbs(this->run, str);
}

int Awk::Value::setMbs (Run* r, const qse_mchar_t* str)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makembsval(r->rtx, str, qse_mbslen(str));
	if (!tmp) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	int n = this->setVal(r, tmp);
	QSE_ASSERT (n == 0);
	return n;
}

int Awk::Value::setIndexedVal (const Index& idx, val_t* v)
{
	if (this->run == QSE_NULL) return -1;
	return this->setIndexedVal (this->run, idx, v);
}

int Awk::Value::setIndexedVal (Run* r, const Index& idx, val_t* v)
{
	QSE_ASSERT (r != QSE_NULL);

	if (QSE_AWK_RTX_GETVALTYPE (r->rtx, this->val) != QSE_AWK_VAL_MAP)
	{
		// the previous value is not a map. 
		// a new map value needs to be created first.
		val_t* map = qse_awk_rtx_makemapval(r->rtx);
		if (map == QSE_NULL) 
		{
			r->awk->retrieveError (r);
			return -1;
		}

		qse_awk_rtx_refupval (r->rtx, map);

		// update the map with a given value 
		if (qse_awk_rtx_setmapvalfld(r->rtx, map, idx.ptr, idx.len, v) == QSE_NULL)
		{
			qse_awk_rtx_refdownval (r->rtx, map);
			r->awk->retrieveError (r);
			return -1;
		}

		// free the previous value
		if (this->run) 
		{
			// if val is not nil, this->run can't be NULL
			qse_awk_rtx_refdownval (this->run->rtx, this->val);
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
		if (qse_awk_rtx_setmapvalfld(r->rtx, val, idx.ptr, idx.len, v) == QSE_NULL)
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
	return this->setIndexedInt (run, idx, v);
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
	int n = this->setIndexedVal(r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedFlt (const Index& idx, flt_t v)
{
	if (run == QSE_NULL) return -1;
	return this->setIndexedFlt(run, idx, v);
}

int Awk::Value::setIndexedFlt (Run* r, const Index& idx, flt_t v)
{
	val_t* tmp = qse_awk_rtx_makefltval(r->rtx, v);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = this->setIndexedVal(r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedStr (const Index& idx, const char_t* str, size_t len, bool numeric)
{
	if (run == QSE_NULL) return -1;
	return this->setIndexedStr(run, idx, str, len, numeric);
}

int Awk::Value::setIndexedStr (
	Run* r, const Index& idx, const char_t* str, size_t len, bool numeric)
{
	val_t* tmp;

	cstr_t cstr;
	cstr.ptr = (char_t*)str;
	cstr.len = len;

	tmp = numeric? qse_awk_rtx_makenstrvalwithcstr(r->rtx, &cstr):
	               qse_awk_rtx_makestrvalwithcstr(r->rtx, &cstr);
	if (tmp == QSE_NULL) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = this->setIndexedVal(r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedStr (const Index& idx, const char_t* str, bool numeric)
{
	if (run == QSE_NULL) return -1;
	return this->setIndexedStr(run, idx, str, numeric);
}

int Awk::Value::setIndexedStr (Run* r, const Index& idx, const char_t* str, bool numeric)
{
	val_t* tmp;
	tmp = numeric? qse_awk_rtx_makenstrvalwithstr(r->rtx, str):
	               qse_awk_rtx_makestrvalwithstr(r->rtx, str);
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = this->setIndexedVal(r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedMbs (const Index& idx, const qse_mchar_t* str, size_t len)
{
	if (run == QSE_NULL) return -1;
	return this->setIndexedMbs(run, idx, str, len);
}

int Awk::Value::setIndexedMbs (Run* r, const Index& idx, const qse_mchar_t* str, size_t len)
{
	val_t* tmp;

	qse_mcstr_t cstr;
	cstr.ptr = (qse_mchar_t*)str;
	cstr.len = len;

	tmp = qse_awk_rtx_makembsvalwithmcstr(r->rtx, &cstr);
	if (!tmp) 
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = this->setIndexedVal(r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}

int Awk::Value::setIndexedMbs (const Index& idx, const qse_mchar_t* str)
{
	if (run == QSE_NULL) return -1;
	return this->setIndexedMbs(run, idx, str);
}

int Awk::Value::setIndexedMbs (Run* r, const Index& idx, const qse_mchar_t* str)
{
	val_t* tmp;
	tmp = qse_awk_rtx_makembsval(r->rtx, str, qse_mbslen(str));
	if (tmp == QSE_NULL)
	{
		r->awk->retrieveError (r);
		return -1;
	}

	qse_awk_rtx_refupval (r->rtx, tmp);
	int n = this->setIndexedVal(r, idx, tmp);
	qse_awk_rtx_refdownval (r->rtx, tmp);

	return n;
}


bool Awk::Value::isIndexed () const
{
	QSE_ASSERT (val != QSE_NULL);
	return QSE_AWK_RTX_GETVALTYPE(this->run->rtx, val) == QSE_AWK_VAL_MAP;
}

int Awk::Value::getIndexed (const Index& idx, Value* v) const
{
	QSE_ASSERT (val != QSE_NULL);

	// not a map. v is just nil. not an error 
	if (QSE_AWK_RTX_GETVALTYPE(this->run->rtx, val) != QSE_AWK_VAL_MAP) 
	{
		v->clear ();
		return 0;
	}

	// get the value from the map.
	val_t* fv = qse_awk_rtx_getmapvalfld(this->run->rtx, val, (char_t*)idx.ptr, idx.len);

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

	if (QSE_AWK_RTX_GETVALTYPE (this->run->rtx, this->val) != QSE_AWK_VAL_MAP) return IndexIterator::END;

	QSE_ASSERT (this->run != QSE_NULL);

	Awk::Value::IndexIterator itr;
	qse_awk_val_map_itr_t* iptr;

	iptr = qse_awk_rtx_getfirstmapvalitr(this->run->rtx, this->val, &itr);
	if (iptr == QSE_NULL) return IndexIterator::END; // no more key

	idx->set (QSE_AWK_VAL_MAP_ITR_KEY(iptr));

	return itr;
}

Awk::Value::IndexIterator Awk::Value::getNextIndex (
	Index* idx, const IndexIterator& curitr) const
{
	QSE_ASSERT (val != QSE_NULL);

	if (QSE_AWK_RTX_GETVALTYPE (this->run->rtx, val) != QSE_AWK_VAL_MAP) return IndexIterator::END;

	QSE_ASSERT (this->run != QSE_NULL);

	Awk::Value::IndexIterator itr (curitr);
	qse_awk_val_map_itr_t* iptr;

	iptr = qse_awk_rtx_getnextmapvalitr(this->run->rtx, this->val, &itr);
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

void Awk::Run::halt () const 
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	qse_awk_rtx_halt (this->rtx);
}

bool Awk::Run::isHalt () const
{
	QSE_ASSERT (this->rtx != QSE_NULL);
	return qse_awk_rtx_ishalt (this->rtx)? true: false;
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
	Mmged (mmgr), awk (QSE_NULL), 
#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
	functionMap (QSE_NULL), 
#else
	functionMap (this), 
#endif
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

const Awk::char_t* Awk::xerrstr (awk_t* a, errnum_t num) 
{
	Awk* awk = *(Awk**)GET_XTN(a);
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
		this->retrieveError ();
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

static void fini_xtn (qse_awk_t* awk)
{
	xtn_t* xtn = GET_XTN(awk);
	xtn->awk->uponClosing ();
}

static void clear_xtn (qse_awk_t* awk)
{
	xtn_t* xtn = GET_XTN(awk);
	xtn->awk->uponClearing ();
}

int Awk::open () 
{
	QSE_ASSERT (this->awk == QSE_NULL);
#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
	QSE_ASSERT (this->functionMap == QSE_NULL);
#endif

	qse_awk_prm_t prm;

	QSE_MEMSET (&prm, 0, QSE_SIZEOF(prm));
	prm.math.pow = pow;
	prm.math.mod = mod;
	prm.modopen  = modopen;
	prm.modclose = modclose;
	prm.modsym   = modsym;

	qse_awk_errnum_t errnum;
	this->awk = qse_awk_open(this->getMmgr(), QSE_SIZEOF(xtn_t), &prm, &errnum);
	if (!this->awk)
	{
		this->setError (errnum);
		return -1;
	}

	this->awk->_instsize += QSE_SIZEOF(xtn_t);

	// associate this Awk object with the underlying awk object
	xtn_t* xtn = (xtn_t*)GET_XTN(this->awk);
	xtn->awk = this;
	xtn->ecb.close = fini_xtn;
	xtn->ecb.clear = clear_xtn;

	dflerrstr = qse_awk_geterrstr (this->awk);
	qse_awk_seterrstr (this->awk, xerrstr);

#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
	this->functionMap = qse_htb_open (
		qse_awk_getmmgr(this->awk), QSE_SIZEOF(this), 512, 70,
		QSE_SIZEOF(qse_char_t), 1
	);
	if (this->functionMap == QSE_NULL)
	{
		qse_awk_close (this->awk);
		this->awk = QSE_NULL;

		this->setError (QSE_AWK_ENOMEM);
		return -1;
	}

	*(Awk**)QSE_XTN(this->functionMap) = this;

	static qse_htb_style_t style =
	{
		{
			QSE_HTB_COPIER_DEFAULT, // keep the key pointer only
			QSE_HTB_COPIER_INLINE   // copy the value into the pair
		},
		{
			QSE_HTB_FREEER_DEFAULT, // free nothing 
			QSE_HTB_FREEER_DEFAULT  // free nothing 
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};
	qse_htb_setstyle (this->functionMap, &style);
	
#endif

	// push the call back after everything else is ok.
	qse_awk_pushecb (this->awk, &xtn->ecb);
	return 0;
}

void Awk::close () 
{
	this->fini_runctx ();
	this->clearArguments ();

#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
	if (this->functionMap)
	{
		qse_htb_close (this->functionMap);
		this->functionMap = QSE_NULL;
	}
#else
	this->functionMap.clear ();
#endif

	if (this->awk) 
	{
		qse_awk_close (this->awk);
		this->awk = QSE_NULL;
	}

	this->clearError ();
}

qse_cmgr_t* Awk::getCmgr () const
{
	if (!this->awk) return QSE_NULL;
	return qse_awk_getcmgr(this->awk);
}

void Awk::uponClosing ()
{
	// nothing to do
}

void Awk::uponClearing ()
{
	// nothing to do
}

Awk::Run* Awk::parse (Source& in, Source& out) 
{
	QSE_ASSERT (awk != QSE_NULL);

	if (&in == &Source::NONE) 
	{
		this->setError (QSE_AWK_EINVAL);
		return QSE_NULL;
	}

	this->fini_runctx ();

	source_reader = &in;
	source_writer = (&out == &Source::NONE)? QSE_NULL: &out;

	qse_awk_sio_t sio;
	sio.in = readSource;
	sio.out = (source_writer == QSE_NULL)? QSE_NULL: writeSource;

	int n = qse_awk_parse(awk, &sio);
	if (n <= -1) 
	{
		this->retrieveError ();
		return QSE_NULL;
	}

	if (this->init_runctx() <= -1) return QSE_NULL;
	return &this->runctx;
}

Awk::Run* Awk::resetRunContext ()
{
	if (this->runctx.rtx)
	{
		this->fini_runctx ();
		if (this->init_runctx() <= -1) return QSE_NULL;
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
		this->retrieveError (&this->runctx);
		return -1;
	}

	ret->setVal (&this->runctx, rv);
	qse_awk_rtx_refdownval (this->runctx.rtx, rv);
	
	return 0;
}

int Awk::call (const qse_mchar_t* name, Value* ret, const Value* args, size_t nargs)
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
			ptr = (val_t**)qse_awk_allocmem(awk, QSE_SIZEOF(val_t*) * nargs);
			if (ptr == QSE_NULL)
			{
				this->runctx.setError (QSE_AWK_ENOMEM);
				this->retrieveError (&this->runctx);
				return -1;
			}
		}

		for (size_t i = 0; i < nargs; i++) ptr[i] = (val_t*)args[i];
	}

	val_t* rv = qse_awk_rtx_callwithmbs(this->runctx.rtx, name, ptr, nargs);

	if (ptr != QSE_NULL && ptr != buf) qse_awk_freemem (awk, ptr);

	if (rv == QSE_NULL) 
	{
		this->retrieveError (&this->runctx);
		return -1;
	}

	ret->setVal (&this->runctx, rv);

	qse_awk_rtx_refdownval (this->runctx.rtx, rv);
	return 0;
}

int Awk::call (const qse_wchar_t* name, Value* ret, const Value* args, size_t nargs)
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
			ptr = (val_t**)qse_awk_allocmem(awk, QSE_SIZEOF(val_t*) * nargs);
			if (ptr == QSE_NULL)
			{
				this->runctx.setError (QSE_AWK_ENOMEM);
				this->retrieveError (&this->runctx);
				return -1;
			}
		}

		for (size_t i = 0; i < nargs; i++) ptr[i] = (val_t*)args[i];
	}

	val_t* rv = qse_awk_rtx_callwithwcs(this->runctx.rtx, name, ptr, nargs);

	if (ptr != QSE_NULL && ptr != buf) qse_awk_freemem (awk, ptr);

	if (rv == QSE_NULL) 
	{
		this->retrieveError (&this->runctx);
		return -1;
	}

	ret->setVal (&this->runctx, rv);

	qse_awk_rtx_refdownval (this->runctx.rtx, rv);
	return 0;
}

void Awk::halt () 
{
	QSE_ASSERT (awk != QSE_NULL);
	qse_awk_haltall (awk);
}

int Awk::init_runctx () 
{
	if (this->runctx.rtx) return 0;

	qse_awk_rio_t rio;

	rio.pipe    = pipeHandler;
	rio.file    = fileHandler;
	rio.console = consoleHandler;

	rtx_t* rtx = qse_awk_rtx_open(awk, QSE_SIZEOF(rxtn_t), &rio);
	if (rtx == QSE_NULL) 
	{
		this->retrieveError();
		return -1;
	}

	rtx->_instsize += QSE_SIZEOF(rxtn_t);
	runctx.rtx = rtx;

	rxtn_t* rxtn = GET_RXTN(rtx);
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
	bool has_ref_arg = false;

#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
	qse_htb_pair_t* pair;
	pair = qse_htb_search (this->functionMap, fi->name.ptr, fi->name.len);
	if (pair == QSE_NULL) 
	{
		run->setError (QSE_AWK_EFUNNF, &fi->name);
		return -1;
	}
	
	FunctionHandler handler;
	handler = *(FunctionHandler*)QSE_HTB_VPTR(pair);
#else
	FunctionMap::Pair* pair = this->functionMap.search (Cstr(fi->name.ptr, fi->name.len));
	if (pair == QSE_NULL)
	{
		run->setError (QSE_AWK_EFUNNF, &fi->name);
		return -1;
	}

	FunctionHandler handler;
	handler = pair->value;
#endif

	size_t i, nargs = qse_awk_rtx_getnargs(run->rtx);

	Value buf[16];
	Value* args;
	if (nargs <= QSE_COUNTOF(buf)) args = buf;
	else
	{
	#if defined(AWK_VALUE_USE_IN_CLASS_PLACEMENT_NEW)
		args = new(run) Value[nargs];
		if (args == QSE_NULL) 
		{
			run->setError (QSE_AWK_ENOMEM);
			return -1;
		}
	#else
		try 
		{ 
			//args = (Value*)::operator new (QSE_SIZEOF(Value) * nargs, this->getMmgr()); 
			args = (Value*)this->getMmgr()->allocate(QSE_SIZEOF(Value) * nargs);
		}
		catch (...) { args = QSE_NULL; }
		if (args == QSE_NULL) 
		{
			run->setError (QSE_AWK_ENOMEM);
			return -1;
		}
		for (i = 0; i < nargs; i++)
		{
			// call the default constructor on the space allocated above.
			// no exception handling is implemented here as i know 
			// that Value::Value() doesn't throw an exception
			new((QSE::Mmgr*)QSE_NULL, (void*)&args[i]) Value ();
		}
	#endif
	}

	for (i = 0; i < nargs; i++)
	{
		int xx;
		val_t* v = qse_awk_rtx_getarg (run->rtx, i);

		if (QSE_AWK_RTX_GETVALTYPE (run->rtx, v) == QSE_AWK_VAL_REF)
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
					qse_awk_val_t* val = (qse_awk_val_t*)RTX_STACK_GBL(run->rtx, idx);
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

			val_t* v = qse_awk_rtx_getarg(run->rtx, i);
			if (QSE_AWK_RTX_GETVALTYPE(run->rtx, v) == QSE_AWK_VAL_REF)
			{
				if (qse_awk_rtx_setrefval(run->rtx, (qse_awk_val_ref_t*)v, args[i].toVal()) <= -1)
				{
					n = -1;
					break;
				}
			}
		}
	}

	if (args != buf) 
	{
	#if defined(AWK_VALUE_USE_IN_CLASS_PLACEMENT_NEW)
		delete[] args;
	#else
		for (i = nargs; i > 0; )
		{
			--i;
			args[i].Value::~Value ();
		}

		//::operator delete (args, this->getMmgr());
		this->getMmgr()->dispose (args);
	#endif
	}

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
		qse_cstr_t* ptr;
		size_t capa = this->capa;

		capa += 64;
		ptr = (qse_cstr_t*)qse_awk_reallocmem(awk, this->ptr, QSE_SIZEOF(qse_cstr_t)*(capa+1));
		if (ptr == QSE_NULL) return -1;

		this->ptr = ptr;
		this->capa = capa;
	}

	this->ptr[this->len].len = len;
	this->ptr[this->len].ptr = qse_awk_strxdup(awk, arg, len);
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
	int n = runarg.add(awk, arg, len);
	if (n <= -1) this->setError (QSE_AWK_ENOMEM);
	return n;
}

int Awk::addArgument (const char_t* arg) 
{
	return addArgument(arg, qse_strlen(arg));
}

void Awk::clearArguments () 
{
	runarg.clear (awk);
}

int Awk::addGlobal(const qse_mchar_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_addgblwithmbs(awk, name);
	if (n <= -1) this->retrieveError ();
	return n;
}

int Awk::addGlobal(const qse_wchar_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_addgblwithwcs(awk, name);
	if (n <= -1) this->retrieveError ();
	return n;
}

int Awk::deleteGlobal (const qse_mchar_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_delgblwithmbs(awk, name);
	if (n <= -1) this->retrieveError ();
	return n;
}

int Awk::deleteGlobal (const qse_wchar_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_delgblwithwcs(awk, name);
	if (n <= -1) this->retrieveError ();
	return n;
}


int Awk::findGlobal (const qse_mchar_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_findgblwithmbs(awk, name);
	if (n <= -1) this->retrieveError ();
	return n;
}

int Awk::findGlobal (const qse_wchar_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);
	int n = qse_awk_findgblwithwcs(awk, name);
	if (n <= -1) this->retrieveError ();
	return n;
}

int Awk::setGlobal (int id, const Value& v) 
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	if (v.run != &runctx) 
	{
		this->setError (QSE_AWK_EINVAL);
		return -1;
	}

	int n = runctx.setGlobal (id, v);
	if (n <= -1) this->retrieveError ();
	return n;
}

int Awk::getGlobal (int id, Value& v) 
{
	QSE_ASSERT (awk != QSE_NULL);
	QSE_ASSERT (runctx.rtx != QSE_NULL);

	int n = runctx.getGlobal (id, v);
	if (n <= -1) this->retrieveError ();
	return n;
}

int Awk::addFunction (
	const qse_mchar_t* name, size_t minArgs, size_t maxArgs, 
	const qse_mchar_t* argSpec, FunctionHandler handler, int validOpts)
{
	QSE_ASSERT (awk != QSE_NULL);

	qse_awk_fnc_mspec_t spec;

	QSE_MEMSET (&spec, 0, QSE_SIZEOF(spec));
	spec.arg.min = minArgs;
	spec.arg.max = maxArgs;
	spec.arg.spec = argSpec;
	spec.impl = this->functionHandler;
	spec.trait = validOpts;

	qse_awk_fnc_t* fnc = qse_awk_addfncwithmbs(awk, name, &spec);
	if (fnc == QSE_NULL) 
	{
		this->retrieveError ();
		return -1;
	}

#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
	// handler is a pointer to a member function. 
	// sizeof(handler) is likely to be greater than sizeof(void*)
	// copy the handler pointer into the table.
	//
	// the function name exists in the underlying function table.
	// use the pointer to the name to maintain the hash table.
	qse_htb_pair_t* pair = qse_htb_upsert(this->functionMap, (char_t*)fnc->name.ptr, fnc->name.len, &handler, QSE_SIZEOF(handler));
#else
	FunctionMap::Pair* pair;
	try { pair = this->functionMap.upsert(Cstr(fnc->name.ptr, fnc->name.len), handler); }
	catch (...) { pair = QSE_NULL; }
#endif

	if (pair == QSE_NULL)
	{
		qse_awk_delfncwithmbs (awk, name);
		this->setError (QSE_AWK_ENOMEM);
		return -1;
	}

	return 0;
}

int Awk::addFunction (
	const qse_wchar_t* name, size_t minArgs, size_t maxArgs, 
	const qse_wchar_t* argSpec, FunctionHandler handler, int validOpts)
{
	QSE_ASSERT (awk != QSE_NULL);

	qse_awk_fnc_wspec_t spec;

	QSE_MEMSET (&spec, 0, QSE_SIZEOF(spec));
	spec.arg.min = minArgs;
	spec.arg.max = maxArgs;
	spec.arg.spec = argSpec;
	spec.impl = this->functionHandler;
	spec.trait = validOpts;

	qse_awk_fnc_t* fnc = qse_awk_addfncwithwcs(awk, name, &spec);
	if (fnc == QSE_NULL) 
	{
		this->retrieveError ();
		return -1;
	}

#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
	// handler is a pointer to a member function. 
	// sizeof(handler) is likely to be greater than sizeof(void*)
	// copy the handler pointer into the table.
	//
	// the function name exists in the underlying function table.
	// use the pointer to the name to maintain the hash table.
	qse_htb_pair_t* pair = qse_htb_upsert(this->functionMap, (char_t*)fnc->name.ptr, fnc->name.len, &handler, QSE_SIZEOF(handler));
#else
	FunctionMap::Pair* pair;
	try { pair = this->functionMap.upsert(Cstr(fnc->name.ptr, fnc->name.len), handler); }
	catch (...) { pair = QSE_NULL; }
#endif

	if (pair == QSE_NULL)
	{
		qse_awk_delfncwithwcs (awk, name);
		this->setError (QSE_AWK_ENOMEM);
		return -1;
	}

	return 0;
}

int Awk::deleteFunction (const char_t* name) 
{
	QSE_ASSERT (awk != QSE_NULL);

	int n = qse_awk_delfnc(awk, name);
	if (n == 0) 
	{
#if defined(QSE_AWK_USE_HTB_FOR_FUNCTION_MAP)
		qse_htb_delete (this->functionMap, name, qse_strlen(name));
#else
		this->functionMap.remove (Cstr(name));
#endif
	}
	else this->retrieveError ();

	return n;
}

Awk::ssize_t Awk::readSource (
	awk_t* awk, sio_cmd_t cmd, sio_arg_t* arg,
	char_t* data, size_t count)
{
	xtn_t* xtn = GET_XTN(awk);
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
	xtn_t* xtn = GET_XTN(awk);
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

Awk::ssize_t Awk::pipeHandler (rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod, void* data, size_t count)
{
	rxtn_t* rxtn = GET_RXTN(rtx);
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
					return awk->pipe_handler->open(pipe);
				case QSE_AWK_RIO_CLOSE:
					return awk->pipe_handler->close(pipe);

				case QSE_AWK_RIO_READ:
					return awk->pipe_handler->read(pipe, (qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->pipe_handler->write(pipe, (const qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE_BYTES:
					return awk->pipe_handler->writeBytes(pipe, (const qse_mchar_t*)data, count);

				case QSE_AWK_RIO_FLUSH:
					return awk->pipe_handler->flush(pipe);
	
				default:
					return -1;
			}
		}
		else
		{
			switch (cmd)
			{
				case QSE_AWK_RIO_OPEN:
					return awk->openPipe(pipe);
				case QSE_AWK_RIO_CLOSE:
					return awk->closePipe(pipe);

				case QSE_AWK_RIO_READ:
					return awk->readPipe(pipe, (qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->writePipe(pipe, (const qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE_BYTES:
					return awk->writePipeBytes(pipe, (const qse_mchar_t*)data, count);
	
				case QSE_AWK_RIO_FLUSH:
					return awk->flushPipe(pipe);
	
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

Awk::ssize_t Awk::fileHandler (rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod, void* data, size_t count)
{
	rxtn_t* rxtn = GET_RXTN(rtx);
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
					return awk->file_handler->open(file);
				case QSE_AWK_RIO_CLOSE:
					return awk->file_handler->close(file);

				case QSE_AWK_RIO_READ:
					return awk->file_handler->read(file, (qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->file_handler->write(file, (const qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE_BYTES:
					return awk->file_handler->writeBytes(file, (const qse_mchar_t*)data, count);

				case QSE_AWK_RIO_FLUSH:
					return awk->file_handler->flush(file);
	
				default:
					return -1;
			}
		}
		else
		{
			switch (cmd)
			{
				case QSE_AWK_RIO_OPEN:
					return awk->openFile(file);
				case QSE_AWK_RIO_CLOSE:
					return awk->closeFile(file);

				case QSE_AWK_RIO_READ:
					return awk->readFile(file, (qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->writeFile(file, (const qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE_BYTES:
					return awk->writeFileBytes(file, (const qse_mchar_t*)data, count);

				case QSE_AWK_RIO_FLUSH:
					return awk->flushFile(file);

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

Awk::ssize_t Awk::consoleHandler (rtx_t* rtx, rio_cmd_t cmd, rio_arg_t* riod, void* data, size_t count)
{
	rxtn_t* rxtn = GET_RXTN(rtx);
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
					return awk->console_handler->open(console);
				case QSE_AWK_RIO_CLOSE:
					return awk->console_handler->close(console);

				case QSE_AWK_RIO_READ:
					return awk->console_handler->read(console, (qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->console_handler->write(console, (const qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE_BYTES:
					return awk->console_handler->writeBytes(console, (const qse_mchar_t*)data, count);

				case QSE_AWK_RIO_FLUSH:
					return awk->console_handler->flush(console);
				case QSE_AWK_RIO_NEXT:
					return awk->console_handler->next(console);

				default:
					return -1;
			}
		}
		else
		{
			switch (cmd)
			{
				case QSE_AWK_RIO_OPEN:
					return awk->openConsole(console);
				case QSE_AWK_RIO_CLOSE:
					return awk->closeConsole(console);

				case QSE_AWK_RIO_READ:
					return awk->readConsole(console, (qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE:
					return awk->writeConsole(console, (const qse_char_t*)data, count);
				case QSE_AWK_RIO_WRITE_BYTES:
					return awk->writeConsoleBytes(console, (const qse_mchar_t*)data, count);

				case QSE_AWK_RIO_FLUSH:
					return awk->flushConsole(console);
				case QSE_AWK_RIO_NEXT:
					return awk->nextConsole(console);

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

int Awk::openPipe (Pipe& io)
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

Awk::ssize_t Awk::writePipeBytes (Pipe& io, const qse_mchar_t* buf, size_t len)
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

Awk::ssize_t Awk::writeFileBytes (File& io, const qse_mchar_t* buf, size_t len)
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

Awk::ssize_t Awk::writeConsoleBytes (Console& io, const qse_mchar_t* buf, size_t len)
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
	rxtn_t* rxtn = GET_RXTN(rtx);
	return rxtn->run->awk->dispatch_function (rxtn->run, fi);
}	
	
Awk::flt_t Awk::pow (awk_t* awk, flt_t x, flt_t y)
{
	xtn_t* xtn = GET_XTN(awk);
	return xtn->awk->pow (x, y);
}

Awk::flt_t Awk::mod (awk_t* awk, flt_t x, flt_t y)
{
	xtn_t* xtn = GET_XTN(awk);
	return xtn->awk->mod (x, y);
}

void* Awk::modopen (awk_t* awk, const mod_spec_t* spec)
{
	xtn_t* xtn = GET_XTN(awk);
	return xtn->awk->modopen (spec);
}

void Awk::modclose (awk_t* awk, void* handle)
{
	xtn_t* xtn = GET_XTN(awk);
	xtn->awk->modclose (handle);
}

void* Awk::modsym (awk_t* awk, void* handle, const char_t* name)
{
	xtn_t* xtn = GET_XTN(awk);
	return xtn->awk->modsym (handle, name);
}
/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
