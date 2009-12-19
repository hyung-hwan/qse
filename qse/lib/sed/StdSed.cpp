/*
 * $Id: StdSed.cpp 318 2009-12-18 12:34:42Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

int StdSed::StdStream::open (Data& io)
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

int StdSed::StdStream::close (Data& io)
{
	qse_sio_t* sio = (qse_sio_t*)io.getHandle();

	qse_sio_flush (sio);
	if (sio != qse_sio_in && sio != qse_sio_out && sio != qse_sio_err)
		qse_sio_close (sio);

	return 0;
}

StdSed::ssize_t StdSed::StdStream::read (Data& io, char_t* buf, size_t len)
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

StdSed::ssize_t StdSed::StdStream::write (Data& io, const char_t* buf, size_t len)
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

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
