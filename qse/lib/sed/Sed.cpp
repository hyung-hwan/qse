/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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

#include <qse/sed/Sed.hpp>
#include <qse/cmn/sio.h>
#include "../cmn/mem.h"
#include "sed.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

int Sed::open ()
{
	sed = qse_sed_open (this->mmgr, QSE_SIZEOF(Sed*));
	if (!sed) return -1;
	*(Sed**)QSE_XTN(sed) = this;

	dflerrstr = qse_sed_geterrstr (sed);
	qse_sed_seterrstr (sed, xerrstr);

	return 0;
}

void Sed::close ()
{
	if (sed)
	{
		qse_sed_close (sed);
		sed = QSE_NULL;
	}
}

int Sed::compile (Stream& sstream)
{
	QSE_ASSERT (sed != QSE_NULL);

	this->sstream = &sstream;
	return qse_sed_comp (sed, sin);
}

int Sed::execute (Stream& iostream)
{
	QSE_ASSERT (sed != QSE_NULL);

	this->iostream = &iostream;
	return qse_sed_exec (sed, xin, xout);
}

void Sed::stop ()
{
	QSE_ASSERT (sed != QSE_NULL);
	qse_sed_stop (sed);
}

bool Sed::isStop () const
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_isstop (sed);
}

int Sed::getTrait () const
{
	QSE_ASSERT (sed != QSE_NULL);
	int val;
	qse_sed_getopt (sed, QSE_SED_TRAIT, &val);
	return val;
}

void Sed::setTrait (int trait)
{
	QSE_ASSERT (sed != QSE_NULL);
	qse_sed_setopt (sed, QSE_SED_TRAIT, &trait);
}

const Sed::char_t* Sed::getErrorMessage () const
{
	return (sed == QSE_NULL)? QSE_T(""): qse_sed_geterrmsg (sed);
}

Sed::loc_t Sed::getErrorLocation () const
{
	if (sed == QSE_NULL) 
	{
		loc_t loc;
		loc.line = 0; 
		loc.colm = 0;
		return loc;
	}
	return *qse_sed_geterrloc (sed);
}

Sed::errnum_t Sed::getErrorNumber () const
{
	return (sed == QSE_NULL)? QSE_SED_ENOERR: qse_sed_geterrnum (sed);
}

void Sed::setError (errnum_t err, const cstr_t* args, const loc_t* loc)
{
	QSE_ASSERT (sed != QSE_NULL);
	qse_sed_seterror (sed, err, args, loc);
}

const Sed::char_t* Sed::getCompileId () const
{
	return qse_sed_getcompid (this->sed);
}

const Sed::char_t* Sed::setCompileId (const char_t* id)
{
	return qse_sed_setcompid (this->sed, id);
}

Sed::size_t Sed::getConsoleLine ()
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_getlinenum (this->sed);
}

void Sed::setConsoleLine (size_t num)
{
	QSE_ASSERT (sed != QSE_NULL);
	qse_sed_setlinenum (this->sed, num);
}

Sed::ssize_t Sed::sin (
	sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len)
{
	Sed* sed = *(Sed**)QSE_XTN(s);

	Stream::Data iodata (sed, Stream::READ, arg);

	try
	{
		switch (cmd)
		{
			case QSE_SED_IO_OPEN:
				return sed->sstream->open (iodata);
			case QSE_SED_IO_CLOSE:
				return sed->sstream->close (iodata);
			case QSE_SED_IO_READ:
				return sed->sstream->read (iodata, buf, len);
			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

Sed::ssize_t Sed::xin (
	sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len)
{
	Sed* sed = *(Sed**)QSE_XTN(s);

	Stream::Data iodata (sed, Stream::READ, arg);

	try
	{
		switch (cmd)
		{
			case QSE_SED_IO_OPEN:
				return sed->iostream->open (iodata);
			case QSE_SED_IO_CLOSE:
				return sed->iostream->close (iodata);
			case QSE_SED_IO_READ:
				return sed->iostream->read (iodata, buf, len);
			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

Sed::ssize_t Sed::xout (
	sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* dat, size_t len)
{
	Sed* sed = *(Sed**)QSE_XTN(s);

	Stream::Data iodata (sed, Stream::WRITE, arg);

	try
	{
		switch (cmd)
		{
			case QSE_SED_IO_OPEN:
				return sed->iostream->open (iodata);
			case QSE_SED_IO_CLOSE:
				return sed->iostream->close (iodata);
			case QSE_SED_IO_WRITE:
				return sed->iostream->write (iodata, dat, len);
			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

const Sed::char_t* Sed::getErrorString (errnum_t num) const
{
	QSE_ASSERT (dflerrstr != QSE_NULL);
	return dflerrstr (sed, num);
}

const Sed::char_t* Sed::xerrstr (const sed_t* s, errnum_t num)
{
	Sed* sed = *(Sed**)QSE_XTN(s);
	try
	{
		return sed->getErrorString (num);
	}
	catch (...)
	{
		return sed->dflerrstr (s, num);
	}
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

