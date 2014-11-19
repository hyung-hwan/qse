/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_CMN_NWIO_H_
#define _QSE_CMN_NWIO_H_

/** @file
 * This file defines a network-based text I/O interface.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/tio.h>
#include <qse/cmn/nwad.h>
#include <qse/cmn/time.h>
#include <qse/cmn/sck.h>

enum qse_nwio_flag_t
{
	QSE_NWIO_TEXT          = (1 << 0),
	QSE_NWIO_IGNOREMBWCERR = (1 << 1),
	QSE_NWIO_NOAUTOFLUSH   = (1 << 2),

	/* normal open flags */
	QSE_NWIO_PASSIVE       = (1 << 8),
	QSE_NWIO_TCP           = (1 << 9),
	QSE_NWIO_UDP           = (1 << 10),

	QSE_NWIO_REUSEADDR     = (1 << 12),
	QSE_NWIO_KEEPALIVE     = (1 << 13),
	/** do not reread if read has been interrupted */
	QSE_NWIO_READNORETRY   = (1 << 14),
	/** do not rewrite if write has been interrupted */
	QSE_NWIO_WRITENORETRY  = (1 << 15),
};

enum qse_nwio_errnum_t
{
	QSE_NWIO_ENOERR = 0, /**< no error */
	QSE_NWIO_EOTHER,     /**< other error */
	QSE_NWIO_ENOIMPL,    /**< not implemented */
	QSE_NWIO_ESYSERR,    /**< subsystem(system call) error */
	QSE_NWIO_EINTERN,    /**< internal error */

	QSE_NWIO_ENOMEM,     /**< out of memory */
	QSE_NWIO_EINVAL,     /**< invalid parameter */
	QSE_NWIO_EACCES,     /**< access denied */
	QSE_NWIO_ENOENT,     /**< no such file */
	QSE_NWIO_EEXIST,     /**< already exist */
	QSE_NWIO_EINTR,      /**< interrupted */
	QSE_NWIO_ETMOUT,     /**< timed out */
	QSE_NWIO_EPIPE,      /**< broken pipe */
	QSE_NWIO_EAGAIN,     /**< resource not available temporarily */

	QSE_NWIO_ECONN,      /**< connection refused */
	QSE_NWIO_EILSEQ,     /**< illegal sequence */
	QSE_NWIO_EICSEQ,     /**< incomplete sequence */
	QSE_NWIO_EILCHR      /**< illegal character */
};
typedef enum qse_nwio_errnum_t qse_nwio_errnum_t;

struct qse_nwio_tmout_t 
{
	qse_ntime_t r, w, c, a;
};

typedef struct qse_nwio_tmout_t qse_nwio_tmout_t;

typedef qse_sck_hnd_t qse_nwio_hnd_t;
typedef struct qse_nwio_t qse_nwio_t;

/**
 * The qse_nwio_t type defines a structure for a network-based stream.
 */
struct qse_nwio_t
{
	qse_mmgr_t*        mmgr;
	int                flags;
	qse_nwio_errnum_t  errnum;
	qse_nwio_tmout_t   tmout;
	qse_nwio_hnd_t     handle;
	qse_tio_t*         tio;
	int                status;
};

#define QSE_NWIO_HANDLE(nwio) ((nwio)->handle)

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_nwio_open() function opens a file.
 * To open a file, you should set the flags with at least one of
 * QSE_NWIO_READ, QSE_NWIO_WRITE, QSE_NWIO_APPEND.
 *
 * If the #QSE_NWIO_HANDLE flag is set, the @a nwad parameter is interpreted
 * as a pointer to qse_nwio_hnd_t.
 */
QSE_EXPORT qse_nwio_t* qse_nwio_open (
	qse_mmgr_t*               mmgr,
	qse_size_t                ext,
	const qse_nwad_t*         nwad,
	int                       flags,
	const qse_nwio_tmout_t* tmout
);

/**
 * The qse_nwio_close() function closes a file.
 */
QSE_EXPORT void qse_nwio_close (
	qse_nwio_t* nwio
);

/**
 * The qse_nwio_close() function opens a file into @a nwio.
 */
QSE_EXPORT int qse_nwio_init (
	qse_nwio_t*             nwio,
	qse_mmgr_t*             mmgr,
	const qse_nwad_t*       nwad,
	int                     flags,
	const qse_nwio_tmout_t* tmout
);

/**
 * The qse_nwio_close() function finalizes a file by closing the handle 
 * stored in @a nwio.
 */
QSE_EXPORT void qse_nwio_fini (
	qse_nwio_t* nwio
);

QSE_EXPORT qse_mmgr_t* qse_nwio_getmmgr (
	qse_nwio_t* nwio
);

QSE_EXPORT void* qse_nwio_getxtn (
	qse_nwio_t* nwio
);

QSE_EXPORT qse_nwio_errnum_t qse_nwio_geterrnum (
	const qse_nwio_t* nwio
);

/**
 * The qse_nwio_gethandle() function returns the native file handle.
 */
QSE_EXPORT qse_nwio_hnd_t qse_nwio_gethandle (
	const qse_nwio_t* nwio
);

QSE_EXPORT qse_cmgr_t* qse_nwio_getcmgr (
	qse_nwio_t* nwio
);

QSE_EXPORT void qse_nwio_setcmgr (
	qse_nwio_t* nwio,
	qse_cmgr_t* cmgr
);


QSE_EXPORT qse_ssize_t qse_nwio_flush (
	qse_nwio_t*  nwio
);

QSE_EXPORT void qse_nwio_drain (
	qse_nwio_t*  nwio
);

/**
 * The qse_nwio_read() function reads data.
 */
QSE_EXPORT qse_ssize_t qse_nwio_read (
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
QSE_EXPORT qse_ssize_t qse_nwio_write (
	qse_nwio_t*  nwio,
	const void*  data,
	qse_size_t   size
);

#if defined(__cplusplus)
}
#endif

#endif
