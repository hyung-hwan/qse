/*
 * $Id$
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

#ifndef _QSE_CMN_NWIO_H_
#define _QSE_CMN_NWIO_H_

/** @file
 * This file defines a generic text I/O interface.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/tio.h>
#include <qse/cmn/nwad.h>

enum qse_nwio_flag_t
{
	QSE_NWIO_TEXT          = (1 << 0),
	QSE_NWIO_IGNOREMBWCERR = (1 << 1),
	QSE_NWIO_NOAUTOFLUSH   = (1 << 2),

	/* normal open flags */
	QSE_NWIO_PASSIVE       = (1 << 8),
	QSE_NWIO_TCP           = (1 << 9),
	QSE_NWIO_UDP           = (1 << 10),

	/** do not reread if read has been interrupted */
	QSE_NWIO_READNORETRY   = (1 << 14),
	/** do not rewrite if write has been interrupted */
	QSE_NWIO_WRITENORETRY  = (1 << 15),
};

enum qse_nwio_errnum_t
{
	QSE_NWIO_ENOERR = 0, /**< no error */

	QSE_NWIO_ENOMEM,     /**< out of memory */
	QSE_NWIO_EINVAL,     /**< invalid parameter */
	QSE_NWIO_EACCES,     /**< access denied */
	QSE_NWIO_ENOENT,     /**< no such file */
	QSE_NWIO_EEXIST,     /**< already exist */
	QSE_NWIO_EINTR,      /**< interrupted */
	QSE_NWIO_EPIPE,      /**< broken pipe */
	QSE_NWIO_ECONN,      /**< connection refused */
	QSE_NWIO_EILSEQ,     /**< illegal sequence */
	QSE_NWIO_EICSEQ,     /**< incomplete sequence */
	QSE_NWIO_EILCHR,     /**< illegal character */
	QSE_NWIO_ESYSERR,    /**< subsystem(system call) error */
	QSE_NWIO_ENOIMPL,    /**< not implemented */

	QSE_NWIO_EOTHER      /**< other error */
};
typedef enum qse_nwio_errnum_t qse_nwio_errnum_t;

#if defined(_WIN32)
/* TODO: */
#elif defined(__OS2__)
/* TODO: */
#elif defined(__DOS__)
/* TODO: */
#else
     typedef int qse_nwio_hnd_t; /**< defines a pipe handle type */
#    define  QSE_NWIO_HND_NIL ((qse_nwio_hnd_t)-1)
#endif

typedef struct qse_nwio_t qse_nwio_t;

struct qse_nwio_t
{
	QSE_DEFINE_COMMON_FIELDS (nwio)
	int               flags;
	qse_nwio_errnum_t errnum;
	qse_nwio_hnd_t    handle;
	qse_tio_t*        tio;
	int               status;
};

#define QSE_NWIO_HANDLE(nwio) ((nwio)->handle)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (nwio)

/**
 * The qse_nwio_open() function opens a file.
 * To open a file, you should set the flags with at least one of
 * QSE_NWIO_READ, QSE_NWIO_WRITE, QSE_NWIO_APPEND.
 *
 * If the #QSE_NWIO_HANDLE flag is set, the @a nwad parameter is interpreted
 * as a pointer to qse_nwio_hnd_t.
 */
qse_nwio_t* qse_nwio_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_nwad_t* nwad,
	int               flags
);

/**
 * The qse_nwio_close() function closes a file.
 */
void qse_nwio_close (
	qse_nwio_t* nwio
);

/**
 * The qse_nwio_close() function opens a file into @a nwio.
 */
int qse_nwio_init (
	qse_nwio_t*       nwio,
	qse_mmgr_t*       mmgr,
	const qse_nwad_t* nwad,
	int               flags
);

/**
 * The qse_nwio_close() function finalizes a file by closing the handle 
 * stored in @a nwio.
 */
void qse_nwio_fini (
	qse_nwio_t* nwio
);

qse_nwio_errnum_t qse_nwio_geterrnum (
	const qse_nwio_t* nwio
);

/**
 * The qse_nwio_gethandle() function returns the native file handle.
 */
qse_nwio_hnd_t qse_nwio_gethandle (
	const qse_nwio_t* nwio
);

qse_ubi_t qse_nwio_gethandleasubi (
	const qse_nwio_t* nwio
);


qse_cmgr_t* qse_nwio_getcmgr (
	qse_nwio_t* nwio
);

void qse_nwio_setcmgr (
	qse_nwio_t* nwio,
	qse_cmgr_t* cmgr
);


qse_ssize_t qse_nwio_flush (
	qse_nwio_t*  nwio
);

void qse_nwio_purge (
	qse_nwio_t*  nwio
);

/**
 * The qse_nwio_read() function reads data.
 */
qse_ssize_t qse_nwio_read (
	qse_nwio_t*  nwio,
	void*        buf,
	qse_size_t   size
);

/**
 * The qse_nwio_write() function writes data.
 * If QSE_NWIO_TEXT is used and the size parameter is (qse_size_t)-1,
 * the function treats the data parameter as a pointer to a null-terminated
 * string.
 */
qse_ssize_t qse_nwio_write (
	qse_nwio_t*  nwio,
	const void*  data,
	qse_size_t   size
);

#ifdef __cplusplus
}
#endif

#endif
