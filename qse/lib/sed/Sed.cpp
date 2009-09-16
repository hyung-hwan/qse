/*
 * $Id: Sed.cpp 287 2009-09-15 10:01:02Z hyunghwan.chung $
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

#include <qse/sed/Sed.hpp>
#include "sed.h"

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

int Sed::open ()
{
	sed = qse_sed_open (this, QSE_SIZEOF(Sed*));
	if (sed == QSE_NULL) return -1;
	*(Sed**)QSE_XTN(sed) = this;

	dflerrstr = qse_sed_geterrstr (sed);
	qse_sed_seterrstr (sed, xerrstr);

	return 0;
}

void Sed::close ()
{
	if (sed != QSE_NULL)
	{
		qse_sed_close (sed);
		sed = QSE_NULL;	
	}
}

int Sed::compile (const char_t* sptr)
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_comp (sed, sptr, qse_strlen(sptr));
}

int Sed::compile (const char_t* sptr, size_t slen)
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_comp (sed, sptr, slen);
}

int Sed::execute ()
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_exec (sed, xin, xout);
}

int Sed::getOption() const
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_getoption (sed);
}

void Sed::setOption (int opt)
{
	QSE_ASSERT (sed != QSE_NULL);
	qse_sed_setoption (sed, opt);
}

Sed::size_t Sed::getMaxDepth (depth_t id) const
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_getmaxdepth (sed, id);
}

void Sed::setMaxDepth (int ids, size_t depth)
{
	QSE_ASSERT (sed != QSE_NULL);
	qse_sed_setmaxdepth (sed, ids, depth);
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
		loc.lin = 0; loc.col = 0;
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

Sed::size_t Sed::getConsoleLine ()
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_getlinnum (sed);
}

void Sed::setConsoleLine (size_t num)
{
	QSE_ASSERT (sed != QSE_NULL);
	qse_sed_setlinnum (sed, num);
}

Sed::ssize_t Sed::xin (
	sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* buf, size_t len)
{
	Sed* sed = *(Sed**)QSE_XTN(s);

	if (arg->path == QSE_NULL)
	{
		Console io (arg, Console::READ);

		try
		{
			switch (cmd)	
			{
				case QSE_SED_IO_OPEN:
					return sed->openConsole (io);
				case QSE_SED_IO_CLOSE:
					return sed->closeConsole (io);
				case QSE_SED_IO_READ:
					return sed->readConsole (io, buf, len);
				default:
					return -1;
			}
		}
		catch (...)
		{
			return -1;
		}
	}
	else
	{
		File io (arg, File::READ);

		try
		{
			switch (cmd)
			{
				case QSE_SED_IO_OPEN:
					return sed->openFile (io);
				case QSE_SED_IO_CLOSE:
					return sed->closeFile (io);
				case QSE_SED_IO_READ:
					return sed->readFile (io, buf, len);
				default:
					return -1;
			}
		}
		catch (...)
		{
			return -1;
		}
	}
}

Sed::ssize_t Sed::xout (
	sed_t* s, io_cmd_t cmd, io_arg_t* arg, char_t* dat, size_t len)
{
	Sed* sed = *(Sed**)QSE_XTN(s);

	if (arg->path == QSE_NULL)
	{
		Console io (arg, Console::WRITE);

		try
		{
			switch (cmd)	
			{
				case QSE_SED_IO_OPEN:
					return sed->openConsole (io);
				case QSE_SED_IO_CLOSE:
					return sed->closeConsole (io);
				case QSE_SED_IO_WRITE:
					return sed->writeConsole (io, dat, len);
				default:
					return -1;
			}
		}
		catch (...)
		{
			return -1;
		}
	}
	else
	{
		File io (arg, File::WRITE);

		try
		{
			switch (cmd)
			{
				case QSE_SED_IO_OPEN:
					return sed->openFile (io);
				case QSE_SED_IO_CLOSE:
					return sed->closeFile (io);
				case QSE_SED_IO_WRITE:
					return sed->writeFile (io, dat, len);
				default:
					return -1;
			}
		}
		catch (...)
		{
			return -1;
		}
	}
}

const Sed::char_t* Sed::getErrorString (errnum_t num) const
{
	QSE_ASSERT (dflerrstr != QSE_NULL);
	return dflerrstr (sed, num);
}

const Sed::char_t* Sed::xerrstr (sed_t* s, errnum_t num)
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

