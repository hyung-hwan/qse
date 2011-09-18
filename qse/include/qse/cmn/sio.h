/*
 * $Id: sio.h 568 2011-09-17 15:41:26Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
	QSE_SIO_HANDLE        = QSE_FIO_HANDLE,
	QSE_SIO_IGNOREMBWCERR = QSE_FIO_IGNOREMBWCERR,

	QSE_SIO_READ          = QSE_FIO_READ,
	QSE_SIO_WRITE         = QSE_FIO_WRITE,
	QSE_SIO_APPEND        = QSE_FIO_APPEND,

	QSE_SIO_CREATE        = QSE_FIO_CREATE,
	QSE_SIO_TRUNCATE      = QSE_FIO_TRUNCATE,
	QSE_SIO_EXCLUSIVE     = QSE_FIO_EXCLUSIVE,
	QSE_SIO_SYNC          = QSE_FIO_SYNC,

	QSE_SIO_NOSHRD        = QSE_FIO_NOSHRD,
	QSE_SIO_NOSHWR        = QSE_FIO_NOSHWR
};

typedef qse_fio_off_t qse_sio_pos_t;
typedef qse_fio_hnd_t qse_sio_hnd_t;

/**
 * The qse_sio_t type defines a simple text stream over a file. It also
 * provides predefined streams for standard input, output, and error.
 */
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

/**
 * The qse_sio_open() fucntion creates a stream object.
 */
qse_sio_t* qse_sio_open (
	qse_mmgr_t*       mmgr,    /**< memory manager */
	qse_size_t        xtnsize, /**< extension size in bytes */
	const qse_char_t* file,    /**< file name */
	int               flags    /**< number OR'ed of #qse_sio_open_flag_t */
);

/**
 * The qse_sio_close() function destroys a stream object.
 */
void qse_sio_close (
	qse_sio_t* sio  /**< stream */
);

int qse_sio_init (
	qse_sio_t*        sio,
	qse_mmgr_t*       mmgr,
	const qse_char_t* file,
	int               flags
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

/**
 * The qse_sio_getpos() gets the current position in a stream.
 * Note that it may not return the desired postion due to buffering.
 * @return 0 on success, -1 on failure
 */
int qse_sio_getpos (
	qse_sio_t*     sio,  /**< stream */
	qse_sio_pos_t* pos   /**< position */
);

/**
 * The qse_sio_setpos() changes the current position in a stream.
 * @return 0 on success, -1 on failure
 */
int qse_sio_setpos (
	qse_sio_t*    sio,   /**< stream */
	qse_sio_pos_t pos    /**< position */
);

#if 0
int qse_sio_rewind (qse_sio_t* sio);
int qse_sio_movetoend (qse_sio_t* sio);
#endif


#ifdef __cplusplus
}
#endif

#endif
