/*
 * $Id: sio.h,v 1.29 2005/12/26 05:38:24 bacon Ease $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

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
 
#ifndef _QSE_CMN_SIO_H_
#define _QSE_CMN_SIO_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/fio.h>
#include <qse/cmn/tio.h>

enum qse_sio_open_flag_t
{
        QSE_SIO_HANDLE    = QSE_FIO_HANDLE,

        QSE_SIO_READ      = QSE_FIO_READ,
        QSE_SIO_WRITE     = QSE_FIO_WRITE,
	QSE_SIO_APPEND    = QSE_FIO_APPEND,

	QSE_SIO_CREATE    = QSE_FIO_CREATE,
        QSE_SIO_TRUNCATE  = QSE_FIO_TRUNCATE,
	QSE_SIO_EXCLUSIVE = QSE_FIO_EXCLUSIVE,
	QSE_SIO_SYNC      = QSE_FIO_SYNC,

        QSE_SIO_NOSHRD    = QSE_FIO_NOSHRD,
        QSE_SIO_NOSHWR    = QSE_FIO_NOSHWR
};

typedef qse_fio_off_t qse_sio_off_t;
typedef qse_fio_hnd_t qse_sio_hnd_t;

typedef struct qse_sio_t qse_sio_t;

struct qse_sio_t
{
	qse_mmgr_t* mmgr;
	qse_fio_t   fio;
	qse_tio_t   tio;
};

#ifdef __cplusplus
extern "C" {
#endif

extern qse_sio_t* qse_sio_in;
extern qse_sio_t* qse_sio_out;
extern qse_sio_t* qse_sio_err;

qse_sio_t* qse_sio_open (
        qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_char_t* file,
	int               flags
);

void qse_sio_close (
	qse_sio_t* sio
);

qse_sio_t* qse_sio_init (
        qse_sio_t*        sio,
	qse_mmgr_t*       mmgr,
	const qse_char_t* file,
	int flags
);

void qse_sio_fini (
	qse_sio_t* sio
);

qse_fio_hnd_t qse_sio_gethandle (
	qse_sio_t* sio
);

qse_ssize_t qse_sio_flush (
	qse_sio_t* sio
);

void qse_sio_purge (
	qse_sio_t* sio
);

qse_ssize_t qse_sio_getc (
	qse_sio_t* sio,
	qse_char_t* c
);

qse_ssize_t qse_sio_gets (
	qse_sio_t* sio,
	qse_char_t* buf,
	qse_size_t size
);


qse_ssize_t qse_sio_getstr (
	qse_sio_t* sio, 
	qse_str_t* buf
);

qse_ssize_t qse_sio_putc (
	qse_sio_t* sio, 
	qse_char_t c
);

qse_ssize_t qse_sio_puts (
	qse_sio_t* sio,
	const qse_char_t* str
);

qse_ssize_t qse_sio_read (
	qse_sio_t* sio,
	qse_char_t* buf,
	qse_size_t size
);

qse_ssize_t qse_sio_write (
	qse_sio_t* sio, 
	const qse_char_t* str,
	qse_size_t size
);

#if 0
qse_ssize_t qse_sio_putsn (qse_sio_t* sio, ...);
qse_ssize_t qse_sio_putsxn (qse_sio_t* sio, ...);
qse_ssize_t qse_sio_putsv (qse_sio_t* sio, qse_va_list ap);
qse_ssize_t qse_sio_putsxv (qse_sio_t* sio, qse_va_list ap);

/* WARNING:
 *   getpos may not return the desired postion because of the buffering 
 */
int qse_sio_getpos (qse_sio_t* sio, qse_sio_off_t* pos);
int qse_sio_setpos (qse_sio_t* sio, qse_sio_off_t pos);
int qse_sio_rewind (qse_sio_t* sio);
int qse_sio_movetoend (qse_sio_t* sio);
#endif


#ifdef __cplusplus
}
#endif

#endif
