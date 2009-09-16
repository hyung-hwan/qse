/*
 * $Id: StdSed.cpp 287 2009-09-15 10:01:02Z hyunghwan.chung $
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
#include <stdlib.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

void* StdSed::allocMem (qse_size_t n)
{ 
	return ::malloc (n); 
}

void* StdSed::reallocMem (void* ptr, qse_size_t n)
{ 
	return ::realloc (ptr, n); 
}

void StdSed::freeMem (void* ptr)
{ 
	::free (ptr); 
}

int StdSed::openConsole (Console& io)
{
	io.setHandle ((io.getMode() == Console::READ)?  qse_sio_in: qse_sio_out);
	return 1;
}

int StdSed::closeConsole (Console& io)
{
	return 0;
}

StdSed::ssize_t StdSed::readConsole (Console& io, char_t* buf, size_t len)
{
	return qse_sio_getsn ((qse_sio_t*)io.getHandle(), buf, len);
}

StdSed::ssize_t StdSed::writeConsole (Console& io, const char_t* data, size_t len) 
{
	return qse_sio_putsn ((qse_sio_t*)io.getHandle(), data, len);
}

int StdSed::openFile (File& io) 
{
	int flags = (io.getMode() == File::READ)?
		QSE_FIO_READ: (QSE_FIO_WRITE|QSE_FIO_CREATE|QSE_FIO_TRUNCATE);

	qse_fio_t* fio = qse_fio_open (
		this, 0, io.getName(),
		flags | QSE_FIO_TEXT,
		QSE_FIO_RUSR | QSE_FIO_WUSR |
		QSE_FIO_RGRP | QSE_FIO_ROTH
	);	
	if (fio == QSE_NULL) return -1;

	io.setHandle (fio);
	return 1;
}

int StdSed::closeFile (File& io) 
{
	qse_fio_close ((qse_fio_t*)io.getHandle());
	return 0;
}

StdSed::ssize_t StdSed::readFile (File& io, char_t* buf, size_t len) 
{
	return qse_fio_read ((qse_fio_t*)io.getHandle(), buf, len);
}

StdSed::ssize_t StdSed::writeFile (File& io, const char_t* data, size_t len) 
{
	return qse_fio_write ((qse_fio_t*)io.getHandle(), data, len);
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
