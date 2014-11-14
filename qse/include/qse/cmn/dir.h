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

#ifndef _QSE_DIR_H_
#define _QSE_DIR_H_

/** @file
 * This file provides functions and data types for I/O multiplexing.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/time.h>

typedef struct qse_dir_t qse_dir_t;
typedef struct qse_dir_ent_t qse_dir_ent_t;

enum qse_dir_errnum_t
{
	QSE_DIR_ENOERR = 0, /**< no error */
	QSE_DIR_EOTHER,     /**< other error */
	QSE_DIR_ENOIMPL,    /**< not implemented */
	QSE_DIR_ESYSERR,    /**< subsystem(system call) error */
	QSE_DIR_EINTERN,    /**< internal error */

	QSE_DIR_ENOMEM,     /**< out of memory */
	QSE_DIR_EINVAL,     /**< invalid parameter */
	QSE_DIR_EACCES,     /**< access denied */
	QSE_DIR_ENOENT,     /**< no such file */
	QSE_DIR_EEXIST,     /**< already exist */
	QSE_DIR_EINTR,      /**< interrupted */
	QSE_DIR_EPIPE,      /**< broken pipe */
	QSE_DIR_EAGAIN      /**< resource not available temporarily */
};
typedef enum qse_dir_errnum_t qse_dir_errnum_t;

enum qse_dir_flag_t
{
	QSE_DIR_MBSPATH = (1 << 0),
	QSE_DIR_SORT    = (1 << 1)
};

struct qse_dir_ent_t
{
	const qse_char_t* name;
};

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_dir_t* qse_dir_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        xtnsize,
	const qse_char_t* path, 
	int               flags,
	qse_dir_errnum_t* errnum /** error number */
);

QSE_EXPORT void qse_dir_close (
	qse_dir_t* dir
);

QSE_EXPORT qse_mmgr_t* qse_dir_getmmgr (
	qse_dir_t* dir
);

QSE_EXPORT void* qse_dir_getxtn (
	qse_dir_t* dir
);

QSE_EXPORT qse_dir_errnum_t qse_dir_geterrnum (
	qse_dir_t* dir
);

QSE_EXPORT int qse_dir_reset (
	qse_dir_t*        dir,
	const qse_char_t* path
);

QSE_EXPORT int qse_dir_read (
	qse_dir_t*     dir,
	qse_dir_ent_t* ent
);

#if defined(__cplusplus)
}
#endif

#endif
