/*
 * $Id: StdSed.cpp 556 2011-08-31 15:43:46Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/sed/StdSed.hpp>
#include <qse/cmn/fio.h>
#include <qse/cmn/sio.h>
#include "sed.h"
#include "../cmn/mem.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

int StdSed::FileStream::open (Data& io)
{
	qse_sio_t* sio;
	const char_t* ioname = io.getName();

	if (ioname == QSE_NULL)
	{
		//
		// a normal console is indicated by a null name 
		//

		if (io.getMode() == READ)
		{
			if (infile == QSE_NULL) sio = qse_sio_in;
			else
			{
				sio = qse_sio_open (
					((sed_t*)io)->mmgr,
					0,
					infile,
					QSE_SIO_READ
				);
				if (sio == QSE_NULL)
				{
					// set the error message explicitly
					// as the file name is different from 
					// the standard console name (NULL)
					qse_cstr_t ea;
					ea.ptr = infile;
					ea.len = qse_strlen (infile);
					((Sed*)io)->setError (
						QSE_SED_EIOFIL, &ea);
					return -1;
				}
			}
		}
		else
		{
			if (outfile == QSE_NULL) sio = qse_sio_out;
			else
			{
				sio = qse_sio_open (
					((sed_t*)io)->mmgr,
					0,
					outfile,
					QSE_SIO_WRITE |
					QSE_SIO_CREATE |
					QSE_SIO_TRUNCATE
				);
				if (sio == QSE_NULL)
				{
					// set the error message explicitly
					// as the file name is different from 
					// the standard console name (NULL)
					qse_cstr_t ea;
					ea.ptr = outfile;
					ea.len = qse_strlen (outfile);
					((Sed*)io)->setError (
						QSE_SED_EIOFIL, &ea);
					return -1;
				}
			}
		}
	}
	else
	{
		//
		// if ioname is not empty, it is a file name
		//

		sio = qse_sio_open (
			((sed_t*)io)->mmgr,
			0,
			ioname,
			(io.getMode() == READ? 
				QSE_SIO_READ: 
				(QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE))
		);

		if (sio == QSE_NULL) return -1;
	}

	io.setHandle (sio);
	return 1;
}

int StdSed::FileStream::close (Data& io)
{
	qse_sio_t* sio = (qse_sio_t*)io.getHandle();

	qse_sio_flush (sio);
	if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
		qse_sio_close (sio);

	return 0;
}

StdSed::ssize_t StdSed::FileStream::read (Data& io, char_t* buf, size_t len)
{
	ssize_t n = qse_sio_getsn ((qse_sio_t*)io.getHandle(), buf, len);

	if (n == -1)
	{
		if (io.getName() == QSE_NULL && infile != QSE_NULL) 
		{
			// if writing to outfile, set the error message
			// explicitly. other cases are handled by 
			// the caller in sed.c.
			qse_cstr_t ea;
			ea.ptr = infile;
			ea.len = qse_strlen (infile);
			((Sed*)io)->setError (QSE_SED_EIOFIL, &ea);
		}
	}

	return n;
}

StdSed::ssize_t StdSed::FileStream::write (Data& io, const char_t* buf, size_t len)
{
	ssize_t n = qse_sio_putsn ((qse_sio_t*)io.getHandle(), buf, len);

	if (n == -1)
	{
		if (io.getName() == QSE_NULL && outfile != QSE_NULL) 
		{
			// if writing to outfile, set the error message
			// explicitly. other cases are handled by 
			// the caller in sed.c.
			qse_cstr_t ea;
			ea.ptr = outfile;
			ea.len = qse_strlen (outfile);
			((Sed*)io)->setError (QSE_SED_EIOFIL, &ea);
		}
	}

	return n;
}

StdSed::StringStream::StringStream (const char_t* in)
{
	this->in.ptr = in; 
	this->in.end = in + qse_strlen(in);
	this->out.inited = false;
}

StdSed::StringStream::StringStream (const char_t* in, size_t len)
{
	this->in.ptr = in; 
	this->in.end = in + len;
	this->out.inited = false;
}

StdSed::StringStream::~StringStream ()
{
	if (out.inited) qse_str_fini (&out.buf);
}
	
int StdSed::StringStream::open (Data& io)
{
	const char_t* ioname = io.getName ();

	if (ioname == QSE_NULL)
	{
		// open a main data stream
		if (io.getMode() == READ) 
		{
			in.cur = in.ptr;
			io.setHandle ((void*)in.ptr);
		}
		else
		{
			if (!out.inited)
			{
				if (qse_str_init (&out.buf, ((Sed*)io)->getMmgr(), 256) <= -1)
				{
					((Sed*)io)->setError (QSE_SED_ENOMEM);
					return -1;
				}

				out.inited = true;
			}

			qse_str_clear (&out.buf);
			io.setHandle (&out.buf);
		}
	}
	else
	{
		// open files for a r or w command
		qse_sio_t* sio;
		int mode = (io.getMode() == READ)?
			QSE_SIO_READ: 
			(QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE);

		sio = qse_sio_open (((Sed*)io)->getMmgr(), 0, ioname, mode);
		if (sio == QSE_NULL) return -1;

		io.setHandle (sio);
	}

	return 1;
}

int StdSed::StringStream::close (Data& io)
{
	const void* handle = io.getHandle();
	if (handle != in.ptr && handle != &out.buf)
		qse_sio_close ((qse_sio_t*)handle);
	return 0;
}

StdSed::ssize_t StdSed::StringStream::read (Data& io, char_t* buf, size_t len)
{
	const void* handle = io.getHandle();

	if (len == (size_t)-1) len--; // shrink buffer if too long
	if (handle == in.ptr)
	{
		size_t n = 0;
		while (in.cur < in.end && n < len)
			buf[n++] = *in.cur++;
		return (ssize_t)n;
	}
	else
	{
		QSE_ASSERT (handle != &out.buf);
		return qse_sio_getsn ((qse_sio_t*)handle, buf, len);
	}
}

StdSed::ssize_t StdSed::StringStream::write (Data& io, const char_t* data, size_t len)
{
	const void* handle = io.getHandle();

	if (len == (qse_size_t)-1) len--; // shrink data if too long

	if (handle == &out.buf)
	{
		if (qse_str_ncat (&out.buf, data, len) == (qse_size_t)-1)
		{
			((Sed*)io)->setError (QSE_SED_ENOMEM);
			return -1;
		}

		return len;
	}
	else
	{
		QSE_ASSERT (handle != in.ptr);
		return qse_sio_putsn ((qse_sio_t*)handle, data, len);
	}
}

const StdSed::char_t* StdSed::StringStream::getInput (size_t* len) const
{
	if (len) *len = in.end - in.ptr;
	return in.ptr;
}

const StdSed::char_t* StdSed::StringStream::getOutput (size_t* len) const
{
	if (out.inited)
	{
		if (len) *len = QSE_STR_LEN(&out.buf);
		return QSE_STR_PTR(&out.buf);
	}
	else
	{
		if (len) *len = 0;
		return QSE_T("");
	}
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
