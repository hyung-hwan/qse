/*
 * $Id: sio.h 318 2009-12-18 12:34:42Z hyunghwan.chung $
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
 
#ifndef _QSE_CMN_SIO_H_
#define _QSE_CMN_SIO_H_

/** @file
 * This file defines a simple stream I/O interface.
 */

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

typedef qse_fio_off_t qse_sio_pos_t;
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

#define QSE_SIO_IN  qse_sio_in
#define QSE_SIO_OUT qse_sio_out
#define QSE_SIO_ERR qse_sio_err

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
	qse_sio_t*  sio,
	qse_char_t* c
);

qse_ssize_t qse_sio_gets (
	qse_sio_t* sio,
	qse_char_t* buf,
	qse_size_t size
);

qse_ssize_t qse_sio_getsn (
	qse_sio_t*  sio,
	qse_char_t* buf,
	qse_size_t  size
);

qse_ssize_t qse_sio_putc (
	qse_sio_t* sio, 
	qse_char_t c
);

qse_ssize_t qse_sio_puts (
	qse_sio_t*        sio,
	const qse_char_t* str
);

qse_ssize_t qse_sio_putsn (
	qse_sio_t*        sio, 
	const qse_char_t* str,
	qse_size_t        size
);

/****f* Common/qse_sio_getpos
 * NAME
 *  qse_sio_getpos - get the stream position
 *
 * WARNING
 *  The getpos() function may not return the desired postion because of 
 *  buffering.
 *
 * SYNOPSIS
 */
int qse_sio_getpos (
	qse_sio_t*     sio, 
	qse_sio_pos_t* pos
);
/******/

int qse_sio_setpos (
	qse_sio_t*    sio, 
	qse_sio_pos_t pos
);

#if 0
int qse_sio_rewind (qse_sio_t* sio);
int qse_sio_movetoend (qse_sio_t* sio);
#endif


#ifdef __cplusplus
}
#endif

#endif
