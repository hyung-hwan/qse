/*
 * $Id: StdSed.cpp 235 2009-07-15 10:43:31Z hyunghwan.chung $
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
