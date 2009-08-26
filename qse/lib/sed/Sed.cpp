/*
 * $Id: Sed.cpp 269 2009-08-26 03:03:51Z hyunghwan.chung $
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

Sed::ssize_t Sed::xin (sed_t* s, io_cmd_t cmd, io_arg_t* arg)
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
					return sed->readConsole (
						io, arg->u.r.buf, arg->u.r.len);
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
					return sed->readFile (
						io, arg->u.r.buf, arg->u.r.len);
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

Sed::ssize_t Sed::xout (sed_t* s, io_cmd_t cmd, io_arg_t* arg)
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
					return sed->writeConsole (
						io, arg->u.w.data, arg->u.w.len);
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
					return sed->writeFile (
						io, arg->u.w.data, arg->u.w.len);
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

