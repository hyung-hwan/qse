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

#include <qse/sed/StdSed.hpp>
#include <qse/si/fio.h>
#include <qse/si/sio.h>
#include "sed-prv.h"
#include "../cmn/mem-prv.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

static qse_sio_t* open_sio (StdSed::Stream::Data& io, const qse_char_t* file, int flags)
{
	qse_sio_t* sio;

	sio = qse_sio_open(qse_sed_getmmgr((StdSed::sed_t*)io), 0, file, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = (StdSed::char_t*)file;
		ea.len = qse_strlen (file);
		((Sed*)io)->setError (QSE_SED_EIOFIL, &ea);
	}
	return sio;
}

static qse_sio_t* open_sio_std (StdSed::Stream::Data& io, qse_sio_std_t std, int flags)
{
	qse_sio_t* sio;
	static const StdSed::char_t* std_names[] =
	{
		QSE_T("stdin"),
		QSE_T("stdout"),
		QSE_T("stderr"),
	};

	sio = qse_sio_openstd(qse_sed_getmmgr((StdSed::sed_t*)io), 0, std, flags);
	if (sio == QSE_NULL)
	{
		qse_cstr_t ea;
		ea.ptr = (StdSed::char_t*)std_names[std];
		ea.len = qse_strlen (std_names[std]);
		((Sed*)io)->setError (QSE_SED_EIOFIL, &ea);
	}
	return sio;
}

int StdSed::FileStream::open (Data& io)
{
	qse_sio_t* sio;
	const char_t* ioname = io.getName();
	int oflags;

	if (io.getMode() == READ)
		oflags = QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR;
	else
		oflags = QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE | QSE_SIO_IGNOREMBWCERR | QSE_SIO_LINEBREAK;

	if (ioname == QSE_NULL)
	{
		//
		// a normal console is indicated by a null name or a dash
		//
		if (io.getMode() == READ)
		{
			sio = (infile == QSE_NULL || (infile[0] == QSE_T('-') && infile[1] == QSE_T('\0')))?
				open_sio_std (io, QSE_SIO_STDIN, oflags):
				open_sio (io, infile, oflags);
		}
		else
		{
			sio = (outfile == QSE_NULL || (outfile[0] == QSE_T('-') && outfile[1] == QSE_T('\0')))?
				open_sio_std (io, QSE_SIO_STDOUT, oflags):
				open_sio (io, outfile, oflags);
		}
	}
	else
	{
		//
		// if ioname is not empty, it is a file name
		//
		sio = open_sio (io, ioname, oflags);
	}
	if (sio == QSE_NULL) return -1;

	if (this->cmgr) qse_sio_setcmgr (sio, this->cmgr);
	io.setHandle (sio);
	return 1;
}

int StdSed::FileStream::close (Data& io)
{
	qse_sio_close ((qse_sio_t*)io.getHandle());
	return 0;
}

StdSed::ssize_t StdSed::FileStream::read (Data& io, char_t* buf, size_t len)
{
	ssize_t n = qse_sio_getstrn ((qse_sio_t*)io.getHandle(), buf, len);

	if (n == -1)
	{
		if (io.getName() == QSE_NULL && infile != QSE_NULL) 
		{
			// if writing to outfile, set the error message
			// explicitly. other cases are handled by 
			// the caller in sed.c.
			qse_cstr_t ea;
			ea.ptr = (char_t*)infile;
			ea.len = qse_strlen (infile);
			((Sed*)io)->setError (QSE_SED_EIOFIL, &ea);
		}
	}

	return n;
}

StdSed::ssize_t StdSed::FileStream::write (Data& io, const char_t* buf, size_t len)
{
	ssize_t n = qse_sio_putstrn ((qse_sio_t*)io.getHandle(), buf, len);

	if (n == -1)
	{
		if (io.getName() == QSE_NULL && outfile != QSE_NULL) 
		{
			// if writing to outfile, set the error message
			// explicitly. other cases are handled by 
			// the caller in sed.c.
			qse_cstr_t ea;
			ea.ptr = (char_t*)outfile;
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
		return qse_sio_getstrn ((qse_sio_t*)handle, buf, len);
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
		return qse_sio_putstrn ((qse_sio_t*)handle, data, len);
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
