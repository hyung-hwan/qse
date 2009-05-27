/*
 * $Id: Sed.hpp 127 2009-05-07 13:15:04Z baconevi $
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

#include <qse/utl/stdio.h>
#include <stdlib.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

int Sed::open () throw ()
{
	sed = qse_sed_open (this, QSE_SIZEOF(Sed*));
	if (sed == QSE_NULL)
	{
		// TODO: set error...
		return -1;
	}

	*(Sed**)QSE_XTN(sed) = this;
	return 0;
}

void Sed::close () throw()
{
	if (sed != QSE_NULL)
	{
		qse_sed_close (sed);
		sed = QSE_NULL;	
	}
}

int Sed::compile (const char_t* sptr) throw ()
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_comp (sed, sptr, qse_strlen(sptr));
}

int Sed::compile (const char_t* sptr, size_t slen) throw ()
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_comp (sed, sptr, slen);
}

int Sed::execute () throw ()
{
	QSE_ASSERT (sed != QSE_NULL);
	return qse_sed_exec (sed, xin, xout);
}

int Sed::xin (sed_t* s, sed_io_cmd_t cmd, sed_io_arg_t* arg)
{
	Sed* sed = *(Sed**)QSE_XTN(s);
	IO io (arg);

	try
	{
		switch (cmd)	
		{
			case QSE_SED_IO_OPEN:
				return sed->openInput (io);
			case QSE_SED_IO_CLOSE:
				return sed->closeInput (io);
			case QSE_SED_IO_READ:
				return sed->readInput (
					io, arg->u.r.buf, arg->u.w.len);
			default:
				return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

int Sed::xout (sed_t* s, sed_io_cmd_t cmd, sed_io_arg_t* arg)
{
	Sed* sed = *(Sed**)QSE_XTN(s);
	IO io (arg);

	try
	{
		switch (cmd)	
		{
			case QSE_SED_IO_OPEN:
				return sed->openOutput (io);
			case QSE_SED_IO_CLOSE:
				return sed->closeOutput (io);
			case QSE_SED_IO_READ:
				return sed->writeOutput (
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

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

