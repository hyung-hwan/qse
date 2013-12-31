/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#ifndef _QSE_MUX_H_
#define _QSE_MUX_H_

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
	 QSE_MUX_ENOENT,     /**< no such file */
	 QSE_MUX_EEXIST,     /**< already exist */
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
	QSE_MUX_OUT = (1 << 1)
};
typedef enum qse_mux_evtmask_t qse_mux_evtmask_t;

typedef void (*qse_mux_evtfun_t) (
	qse_mux_t*           mux,
	const qse_mux_evt_t* evt
);

struct qse_mux_evt_t
{
	qse_mux_hnd_t hnd;
	int           mask;
	void*         data;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_mux_t* qse_mux_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        xtnsize,
	qse_mux_evtfun_t  evtfun,
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

#ifdef __cplusplus
}
#endif

#endif
