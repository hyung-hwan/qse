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
	QSE_DIR_MBSPATH    = (1 << 0),
	QSE_DIR_SORT       = (1 << 1),
	QSE_DIR_SKIPSPCDIR = (1 << 2)   /**< limited to normal entries excluding . and .. */
};
typedef enum qse_dir_flag_t qse_dir_flag_t;

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

/**
 * The qse_dir_read() function reads a directory entry and
 * stores it in memory pointed to by \a ent.
 * \return -1 on failure, 0 upon no more entry, 1 on success
 */
QSE_EXPORT int qse_dir_read (
	qse_dir_t*     dir,
	qse_dir_ent_t* ent
);

#if defined(__cplusplus)
}
#endif

#endif
