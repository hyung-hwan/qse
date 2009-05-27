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

#include <qse/sed/StdSed.hpp>
#include <qse/cmn/fio.h>
#include <qse/cmn/sio.h>
#include <stdlib.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

void* StdSed::allocMem (qse_size_t n) throw ()
{ 
	return ::malloc (n); 
}

void* StdSed::reallocMem (void* ptr, qse_size_t n) throw ()
{ 
	return ::realloc (ptr, n); 
}

void StdSed::freeMem (void* ptr) throw ()
{ 
	::free (ptr); 
}

int StdSed::openInput (IO& io)
{
	int flags;
	const qse_char_t* path = io.getPath ();

	if (path == QSE_NULL) io.setHandle (qse_sio_in);
	else
	{
		qse_fio_t* fio;
		fio = qse_fio_open (
			this, 0, path,
			QSE_FIO_READ | QSE_FIO_TEXT,
			QSE_FIO_RUSR | QSE_FIO_WUSR |
			QSE_FIO_RGRP | QSE_FIO_ROTH
		);	
		if (fio == NULL) return -1;

		io.setHandle (fio);
	}

	return 1;
}

int StdSed::closeInput (IO& io)
{
	if (io.getPath() != QSE_NULL) 
		qse_fio_close ((qse_fio_t*)io.getHandle());
	return 0;
}

ssize_t StdSed::readInput (IO& io, char_t* buf, size_t len)
{
	if (io.getPath() == QSE_NULL)
		return qse_sio_getsn ((qse_sio_t*)io.getHandle(), buf, len);
	else
		return qse_fio_read ((qse_fio_t*)io.getHandle(), buf, len);
}

int StdSed::openOutput (IO& io) 
{
	int flags;
	const qse_char_t* path = io.getPath ();

	if (path == QSE_NULL) io.setHandle (qse_sio_out);
	else
	{
		qse_fio_t* fio;
		fio = qse_fio_open (
			this, 0, path,
			QSE_FIO_WRITE | QSE_FIO_CREATE |
			QSE_FIO_TRUNCATE | QSE_FIO_TEXT,
			QSE_FIO_RUSR | QSE_FIO_WUSR |
			QSE_FIO_RGRP | QSE_FIO_ROTH
		);	
		if (fio == NULL) return -1;

		io.setHandle (fio);
	}

	return 1;
}

int StdSed::closeOutput (IO& io) 
{
	if (io.getPath() != QSE_NULL)
		qse_fio_close ((qse_fio_t*)io.getHandle());
	return 0;
}

ssize_t StdSed::writeOutput (IO& io, const char_t* data, size_t len) 
{
	if (io.getPath() == QSE_NULL)
		return qse_sio_putsn ((qse_sio_t*)io.getHandle(), data, len);
	else
		return qse_fio_write ((qse_fio_t*)io.getHandle(), data, len);
}

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////
