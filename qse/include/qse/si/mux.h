/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _QSE_SI_MUX_H_
#define _QSE_SI_MUX_H_

/** @file
 * This file provides functions and data types for I/O multiplexing.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/time.h>

typedef struct qse_mux_t qse_mux_t;
typedef struct qse_mux_evt_t qse_mux_evt_t;

enum qse_mux_errnum_t
{
	QSE_MUX_ENOERR = 0, /**< no error */
	QSE_MUX_EOTHER,     /**< other error */
	QSE_MUX_ENOIMPL,    /**< not implemented */
	QSE_MUX_ESYSERR,    /**< subsystem(system call) error */
	QSE_MUX_EINTERN,    /**< internal error */

	QSE_MUX_ENOMEM,     /**< out of memory */
	QSE_MUX_EINVAL,     /**< invalid parameter */
	QSE_MUX_EACCES,     /**< access denied */
	QSE_MUX_EPERM,      /**< operation not permitted */
	QSE_MUX_ENOENT,     /**< no such file */
	QSE_MUX_EEXIST,     /**< already exist */
	QSE_MUX_ENOTDIR,    /**< not a directory */
	QSE_MUX_EINTR,      /**< interrupted */
	QSE_MUX_EPIPE,      /**< broken pipe */
	QSE_MUX_EAGAIN,     /**< resource not available temporarily */
};
typedef enum qse_mux_errnum_t qse_mux_errnum_t;

#if defined(_WIN32)
	/*TODO: typedef qse_uintptr_t qse_mux_hnd_t;*/
	typedef int qse_mux_hnd_t;
#elif defined(__OS2__)
	typedef int qse_mux_hnd_t;
#elif defined(__DOS__)
	typedef int qse_mux_hnd_t;
#else
	typedef int qse_mux_hnd_t;
#endif

enum qse_mux_evtmask_t
{
	QSE_MUX_IN  = (1 << 0),
	QSE_MUX_OUT = (1 << 1),

	/* output only below - available if the underlying system multiplexer returns relevant information  */
	QSE_MUX_RDHUP = (1 << 2),
	QSE_MUX_HUP = (1 << 3),
	QSE_MUX_ERR = (1 << 4)
};
typedef enum qse_mux_evtmask_t qse_mux_evtmask_t;

typedef void (*qse_mux_evtcb_t) (
	qse_mux_t*           mux,
	const qse_mux_evt_t* evt
);

struct qse_mux_evt_t
{
	qse_mux_hnd_t hnd;
	int           mask;
	void*         data;
};

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_mux_t* qse_mux_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        xtnsize,
	qse_mux_evtcb_t   evtcb,
	qse_size_t        capahint,
	qse_mux_errnum_t* errnum
);

QSE_EXPORT void qse_mux_close (
	qse_mux_t* mux
);

QSE_EXPORT qse_mmgr_t* qse_mux_getmmgr (
	qse_mux_t* mux
);

QSE_EXPORT void* qse_mux_getxtn (
	qse_mux_t* mux
);

QSE_EXPORT qse_mux_errnum_t qse_mux_geterrnum (
	qse_mux_t* mux
);

QSE_EXPORT int qse_mux_insert (
	qse_mux_t*           mux,
	const qse_mux_evt_t* evt
);

QSE_EXPORT int qse_mux_delete (
	qse_mux_t*           mux,
	const qse_mux_evt_t* evt
);

QSE_EXPORT int qse_mux_poll (
	qse_mux_t*         mux,
	const qse_ntime_t* tmout
);

QSE_EXPORT int qse_mux_setupchan (
	qse_mux_t*         mux
);

/**
 * The qse_mux_interrupt() function can be use break the blocking call  to
 * qse_mux_poll() immediately. You must call qse_mux_setupchan() once on 
 * the initialized mux before using qse_mux_interrupt().
 */
QSE_EXPORT void qse_mux_interrupt (
	qse_mux_t*         mux
);

#if defined(__cplusplus)
}
#endif

#endif
