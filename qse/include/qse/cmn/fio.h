/*
 * $Id: fio.h 565 2011-09-11 02:48:21Z hyunghwan.chung $
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

#ifndef _QSE_CMN_FIO_H_
#define _QSE_CMN_FIO_H_

/** @file
 * This file defines a simple file I/O interface.
 */

#include <qse/types.h>
#include <qse/macros.h>

#include <qse/cmn/tio.h>

enum qse_fio_open_flag_t
{
	/* request qse_char_io based IO */
	QSE_FIO_TEXT          = (1 << 0),
	QSE_FIO_IGNOREMBWCERR = (1 << 1),

	/* treat the file name pointer as a handle pointer */
	QSE_FIO_HANDLE        = (1 << 3),

	QSE_FIO_READ       = (1 << 8),
	QSE_FIO_WRITE      = (1 << 9),
	QSE_FIO_APPEND     = (1 << 10),

	QSE_FIO_CREATE     = (1 << 11),
	QSE_FIO_TRUNCATE   = (1 << 12),
	QSE_FIO_EXCLUSIVE  = (1 << 13),
	QSE_FIO_SYNC       = (1 << 14),

	/* for WIN32 only. harmless(no effect) when used on other platforms */
	QSE_FIO_NOSHRD     = (1 << 24),
	QSE_FIO_NOSHWR     = (1 << 25),
	QSE_FIO_RANDOM     = (1 << 26), /* hint that access be random */
	QSE_FIO_SEQUENTIAL = (1 << 27)  /* hint that access is sequential */
};

/* seek origin */
enum qse_fio_ori_t
{
	QSE_FIO_BEGIN   = 0,
	QSE_FIO_CURRENT = 1,
	QSE_FIO_END     = 2
};

enum qse_fio_mode_t
{
	QSE_FIO_SUID = 04000, /* set UID */
	QSE_FIO_SGID = 02000, /* set GID */
	QSE_FIO_SVTX = 01000, /* sticky bit */
	QSE_FIO_RUSR = 00400, /* can be read by owner */
	QSE_FIO_WUSR = 00200, /* can be written by owner */
	QSE_FIO_XUSR = 00100, /* can be executed by owner */
	QSE_FIO_RGRP = 00040, /* can be read by group */
	QSE_FIO_WGRP = 00020, /* can be written by group */
	QSE_FIO_XGRP = 00010, /* can be executed by group */
	QSE_FIO_ROTH = 00004, /* can be read by others */
	QSE_FIO_WOTH = 00002, /* can be written by others */
	QSE_FIO_XOTH = 00001  /* can be executed by others */
};

#if defined(_WIN32)
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_fio_hnd_t;
#elif defined(__OS2__)
	/* <os2def.h> => typedef LHANDLE HFILE;
	                 typedef unsigned long LHANDLE; */
	typedef unsigned long qse_fio_hnd_t;
#elif defined(__DOS__)
	typedef int qse_fio_hnd_t;
#else
	typedef int qse_fio_hnd_t;
#endif

/* file offset */
typedef qse_foff_t qse_fio_off_t;

/* file origin for seek */
typedef enum qse_fio_ori_t qse_fio_ori_t;

typedef struct qse_fio_t qse_fio_t;
typedef struct qse_fio_lck_t qse_fio_lck_t;

struct qse_fio_t
{
	/* note that qse_fio_t is instantiated statically 
	 * in sio.c. make sure that you update the static instantiation
	 * when you change the structure of qse_fio_t */
	QSE_DEFINE_COMMON_FIELDS (fio)
	int           errnum;
	qse_fio_hnd_t handle;
	int           flags; /* extra flags */
	qse_tio_t*    tio;
};

struct qse_fio_lck_t
{
	int            type;   /* READ, WRITE */
	qse_fio_off_t  offset; /* starting offset */
	qse_fio_off_t  length; /* length */
	qse_fio_ori_t  origin; /* origin */
};

#define QSE_FIO_ERRNUM(fio) ((fio)->errnum)
#define QSE_FIO_HANDLE(fio) ((fio)->handle)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (fio)

/**
 * The qse_fio_open() function opens a file.
 * To open a file, you should set the flags with at least one of
 * QSE_FIO_READ, QSE_FIO_WRITE, QSE_FIO_APPEND.
 */
qse_fio_t* qse_fio_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_char_t* path,
	int               flags,
	int               mode
);

/***
 * The qse_fio_close() function closes a file.
 */
void qse_fio_close (
	qse_fio_t* fio
);

/***
 * The qse_fio_close() function opens a file into @a fio.
 */
int qse_fio_init (
	qse_fio_t*        fio,
	qse_mmgr_t*       mmgr,
	const qse_char_t* path,
	int               flags,
	int               mode
);

/***
 * The qse_fio_close() function finalizes a file by closing the handle 
 * stored in @a fio.
 */
void qse_fio_fini (
	qse_fio_t* fio
);

/**
 * The qse_fio_gethandle() function returns the native file handle.
 */
qse_fio_hnd_t qse_fio_gethandle (
	qse_fio_t* fio
);

/**
 * The qse_fio_sethandle() function sets the file handle
 * Avoid using this function if you don't know what you are doing.
 * You may have to retrieve the previous handle using qse_fio_gethandle()
 * to take relevant actions before resetting it with qse_fio_sethandle().
 */
void qse_fio_sethandle (
	qse_fio_t* fio,
	qse_fio_hnd_t handle
);

/**
 * The qse_fio_seek() function changes the current file position.
 */
qse_fio_off_t qse_fio_seek (
	qse_fio_t*    fio,
	qse_fio_off_t offset,
	qse_fio_ori_t origin
);

/**
 * The qse_fio_truncate() function truncates a file to @a size.
 */
int qse_fio_truncate (
	qse_fio_t*    fio,
	qse_fio_off_t size
);

/**
 * The qse_fio_read() function reads data.
 */
qse_ssize_t qse_fio_read (
	qse_fio_t*  fio,
	void*       buf,
	qse_size_t  size
);

/**
 * The qse_fio_write() function writes data.
 * If QSE_FIO_TEXT is used and the size parameter is (qse_size_t)-1,
 * the function treats the data parameter as a pointer to a null-terminated
 * string.
 */
qse_ssize_t qse_fio_write (
	qse_fio_t*  fio,
	const void* data,
	qse_size_t  size
);


/**
 * The qse_fio_flush() function flushes data. It is useful if #QSE_FIO_TEXT is 
 * set for the file handle @a fio.
 */
qse_ssize_t qse_fio_flush (
        qse_fio_t*    fio
);

/**
 * The qse_fio_chmod() function changes the file mode.
 *
 * @note
 * On _WIN32, this function is implemented on the best-effort basis and 
 * returns an error on the following conditions:
 * - The file size is 0.
 * - The file is opened without #QSE_FIO_READ.
 */
int qse_fio_chmod (
	qse_fio_t* fio,
	int        mode
);

/**
 * The qse_fio_sync() function synchronizes file contents into storage media
 * It is useful in determining the media error, without which qse_fio_close() 
 * may succeed despite such an error.
 */
int qse_fio_sync (
	qse_fio_t* fio
);


/* TODO: qse_fio_lock, qse_fio_unlock */
int qse_fio_lock ( 
	qse_fio_t*     fio, 
	qse_fio_lck_t* lck,
	int            flags
);

int qse_fio_unlock (
	qse_fio_t*     fio,
	qse_fio_lck_t* lck,
	int            flags
);

#ifdef __cplusplus
}
#endif

#endif
