/*
 * $Id$
 */

#ifndef _QSE_CMN_FIO_H_
#define _QSE_CMN_FIO_H_

#include <qse/types.h>
#include <qse/macros.h>

enum qse_fio_open_flag_t
{
	/* treat the file name pointer as a handle pointer */
	QSE_FIO_HANDLE    = (1 << 0),

	QSE_FIO_READ      = (1 << 1),
	QSE_FIO_WRITE     = (1 << 2),
	QSE_FIO_APPEND    = (1 << 3),

	QSE_FIO_CREATE    = (1 << 4),
	QSE_FIO_TRUNCATE  = (1 << 5),
	QSE_FIO_EXCLUSIVE = (1 << 6),
	QSE_FIO_SYNC      = (1 << 7),

	/* for ms windows only */
	QSE_FIO_NOSHRD    = (1 << 16),
	QSE_FIO_NOSHWR    = (1 << 17)
};

/* seek origin */
enum qse_fio_seek_origin_t
{
	QSE_FIO_BEGIN   = 0,
	QSE_FIO_CURRENT = 1,
	QSE_FIO_END     = 2
};

#ifdef _WIN32
/* <winnt.h> typedef PVOID HANDLE; */
typedef void* qse_fio_hnd_t; 
#else
typedef int qse_fio_hnd_t;
#endif

/* file offset */
typedef qse_int64_t qse_fio_off_t;
typedef enum qse_fio_seek_origin_t qse_fio_ori_t;

typedef struct qse_fio_t qse_fio_t;

struct qse_fio_t
{
	qse_mmgr_t* mmgr;
	qse_fio_hnd_t handle;
};

#define QSE_FIO_MMGR(fio)   ((fio)->mmgr)
#define QSE_FIO_HANDLE(fio) ((fio)->handle)

#ifdef __cplusplus
extern "C" {
#endif

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

qse_fio_hnd_t qse_fio_gethandle (
	qse_fio_t* fio
);

/****f* qse.cmn.fio/qse_fio_sethandle
 * SYNOPSIS
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

qse_ssize_t qse_fio_read (
	qse_fio_t* fio,
	void* buf,
	qse_size_t size
);

qse_ssize_t qse_fio_write (
	qse_fio_t* fio,
	const void* buf,
	qse_size_t size
);

#ifdef __cplusplus
}
#endif

#endif
