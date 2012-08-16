/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include <qse/cut/Cut.hpp>
#include <qse/cmn/sio.h>
#include "../cmn/mem.h"
#include "cut.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

int Cut::open ()
{
	cut = qse_cut_open (this->mmgr, QSE_SIZEOF(Cut*));
	if (cut == QSE_NULL) return -1;
	*(Cut**)QSE_XTN(cut) = this;

	dflerrstr = qse_cut_geterrstr (cut);
	qse_cut_seterrstr (cut, xerrstr);

	return 0;
}

void Cut::close ()
{
	if (cut != QSE_NULL)
	{
		qse_cut_close (cut);
		cut = QSE_NULL;	
	}
}

int Cut::compile (const char_t* sptr)
{
	QSE_ASSERT (cut != QSE_NULL);
	return qse_cut_comp (cut, sptr, qse_strlen(sptr));
}

int Cut::compile (const char_t* sptr, size_t slen)
{
	QSE_ASSERT (cut != QSE_NULL);
	return qse_cut_comp (cut, sptr, slen);
}

int Cut::execute (Stream& iostream)
{
	QSE_ASSERT (cut != QSE_NULL);

	this->iostream = &iostream;
	return qse_cut_exec (cut, xin, xout);
}

int Cut::getOption() const
{
	QSE_ASSERT (cut != QSE_NULL);
	return qse_cut_getoption (cut);
}

void Cut::setOption (int opt)
{
	QSE_ASSERT (cut != QSE_NULL);
	qse_cut_setoption (cut, opt);
}

const Cut::char_t* Cut::getErrorMessage () const
{
	return (cut == QSE_NULL)? QSE_T(""): qse_cut_geterrmsg (cut);
}

Cut::errnum_t Cut::getErrorNumber () const
{
	return (cut == QSE_NULL)? QSE_CUT_ENOERR: qse_cut_geterrnum (cut);
}

void Cut::setError (errnum_t err, const cstr_t* args)
{
	QSE_ASSERT (cut != QSE_NULL);
	qse_cut_seterror (cut, err, args);
}

Cut::ssize_t Cut::xin (
	cut_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len)
{
	Cut* cut = *(Cut**)QSE_XTN(s);

	Stream::Data iodata (cut, Stream::READ, arg);

	try
	{
		switch (cmd)
		{
			case QSE_CUT_IO_OPEN:
				return cut->iostream->open (iodata);
			case QSE_CUT_IO_CLOSE:
				return cut->iostream->close (iodata);
			case QSE_CUT_IO_READ:
				return cut->iostream->read (iodata, buf, len);
			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

Cut::ssize_t Cut::xout (
	cut_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* dat, size_t len)
{
	Cut* cut = *(Cut**)QSE_XTN(s);

	Stream::Data iodata (cut, Stream::WRITE, arg);

	try
	{
		switch (cmd)
		{
			case QSE_CUT_IO_OPEN:
				return cut->iostream->open (iodata);
			case QSE_CUT_IO_CLOSE:
				return cut->iostream->close (iodata);
			case QSE_CUT_IO_WRITE:
				return cut->iostream->write (iodata, dat, len);
			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

const Cut::char_t* Cut::getErrorString (errnum_t num) const
{
	QSE_ASSERT (dflerrstr != QSE_NULL);
	return dflerrstr (cut, num);
}

const Cut::char_t* Cut::xerrstr (cut_t* s, errnum_t num)
{
	Cut* cut = *(Cut**)QSE_XTN(s);
	try
	{
		return cut->getErrorString (num);
	}
	catch (...)
	{
		return cut->dflerrstr (s, num);
	}
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

