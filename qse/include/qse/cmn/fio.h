/*
 * $Id$
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

#ifndef _QSE_CMN_FIO_H_
#define _QSE_CMN_FIO_H_

#include <qse/types.h>
#include <qse/macros.h>

#include <qse/cmn/tio.h>

enum qse_fio_open_flag_t
{
	/* request qse_char_io based IO */
	QSE_FIO_TEXT       = (1 << 0),

	/* treat the file name pointer as a handle pointer */
	QSE_FIO_HANDLE     = (1 << 1),

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
enum qse_fio_seek_origin_t
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

#ifdef _WIN32
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_fio_hnd_t;
#else
	typedef int qse_fio_hnd_t;
#endif

/* file offset */
typedef qse_int64_t qse_fio_off_t;
typedef enum qse_fio_seek_origin_t qse_fio_ori_t;

typedef struct qse_fio_t qse_fio_t;
typedef struct qse_fio_lck_t qse_fio_lck_t;

struct qse_fio_t
{
	QSE_DEFINE_STD_FIELDS (fio)
	qse_fio_hnd_t handle;
	qse_tio_t* tio;
};

struct qse_fio_lck_t
{
	int            type;   /* READ, WRITE */
	qse_fio_off_t  offset; /* starting offset */
	qse_fio_off_t  length; /* length */
	qse_fio_ori_t  origin; /* origin */
};

#define QSE_FIO_MMGR(fio)   ((fio)->mmgr)
#define QSE_FIO_HANDLE(fio) ((fio)->handle)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_STD_FUNCTIONS (fio)

/****f* qse.fio/qse_fio_open
 * NAME
 *  qse_fio_open - open a file
 *
 * DESCRIPTION
 *  To open a file, you should set the flags with at least one of
 *  QSE_FIO_READ, QSE_FIO_WRITE, QSE_FIO_APPEND.
 *
 * SYNOPSIS
 */
qse_fio_t* qse_fio_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_char_t* path,
	int               flags,
	int               mode
);
/******/

/****f* qse.fio/qse_fio_close
 * NAME
 *  qse_fio_close - close a file
 *
 * SYNOPSIS
 */
void qse_fio_close (
	qse_fio_t* fio
);
/******/

qse_fio_t* qse_fio_init (
	qse_fio_t* fio,
	qse_mmgr_t* mmgr,
	const qse_char_t* path,
	int flags,
	int mode
);

void qse_fio_fini (
	qse_fio_t* fio
);

/****f* qse.cmn.fio/qse_fio_gethandle
 * NAME
 *  qse_fio_gethandle - get the native file handle
 * SYNOPSIS
 */
qse_fio_hnd_t qse_fio_gethandle (
	qse_fio_t* fio
);
/******/

/****f* qse.cmn.fio/qse_fio_sethandle
 * NAME
 *  qse_fio_sethandle - set the file handle
 * WARNING
 *  Avoid using this function if you don't know what you are doing.
 *  You may have to retrieve the previous handle using qse_fio_gethandle()
 *  to take relevant actions before resetting it with qse_fio_sethandle().
 * SYNOPSIS
 */
void qse_fio_sethandle (
	qse_fio_t* fio,
	qse_fio_hnd_t handle
);
/******/

qse_fio_off_t qse_fio_seek (
	qse_fio_t*    fio,
	qse_fio_off_t offset,
	qse_fio_ori_t origin
);

int qse_fio_truncate (
	qse_fio_t*    fio,
	qse_fio_off_t size
);

/****f* qse.cmn.fio/qse_fio_read
 * NAME
 *  qse_fio_read - read data
 * SYNOPSIS
 */
qse_ssize_t qse_fio_read (
	qse_fio_t*  fio,
	void*       buf,
	qse_size_t  size
);
/******/

/****f* qse.cmn.fio/qse_fio_write
 * NAME
 *  qse_fio_write - write data
 * SYNOPSIS
 */
qse_ssize_t qse_fio_write (
	qse_fio_t*  fio,
	const void* data,
	qse_size_t  size
);
/******/


/****f* qse.cmn.fio/qse_fio_flush
 * NAME
 *  qse_fio_flush - flush data
 *
 * DESCRIPTION
 *  The qse_fio_flush() function is useful if QSE_FIO_TEXT is used in 
 *  qse_fio_open ().
 *
 * SYNOPSIS
 */
qse_ssize_t qse_fio_flush (
        qse_fio_t*    fio
);
/******/

/****f* qse.cmn.fio/qse_fio_chmod
 * NAME
 *  qse_fio_chmod - change the file mode
 * SYNOPSIS
 */
int qse_fio_chmod (
	qse_fio_t* fio,
	int mode
);
/******/

/****f* qse.cmn.fio/qse_fio_sync
 * NAME
 *  qse_fio_sync - synchronize file contents into storage media
 * DESCRIPTION
 *  The qse_fio_sync() function is useful in determining the media error,
 *  without which qse_fio_close() may succeed despite such an error.
 * SYNOPSIS
 */
int qse_fio_sync (
	qse_fio_t* fio
);
/******/


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
