/*
 * $Id$
 */

#ifndef _QSE_CMN_PIO_H_
#define _QSE_CMN_PIO_H_

#include <qse/types.h>
#include <qse/macros.h>

enum qse_pio_open_flag_t
{
	QSE_PIO_READ       = (1 << 1),
	QSE_PIO_WRITE      = (1 << 2),
};

#ifdef _WIN32
/* <winnt.h> => typedef PVOID HANDLE; */
typedef void* qse_pio_hnd_t;
#else
typedef int qse_pio_hnd_t;
#endif

/* pipe offset */
typedef qse_int64_t qse_pio_off_t;
typedef enum qse_pio_seek_origin_t qse_pio_ori_t;

typedef struct qse_pio_t qse_pio_t;

struct qse_pio_t
{
	qse_mmgr_t* mmgr;
	qse_pio_hnd_t handle;
};

#define QSE_PIO_MMGR(pio)   ((pio)->mmgr)
#define QSE_PIO_HANDLE(pio) ((pio)->handle)

#ifdef __cplusplus
extern "C" {
#endif

/****f* qse.pio/qse_pio_open
 * NAME
 *  qse_pio_open - open a pipe to a child process
 *
 * DESCRIPTION
 *  To open a pipe, you should set the flags with at least one of
 *  QSE_PIO_READ, QSE_PIO_WRITE, QSE_PIO_APPEND.
 *
 * SYNOPSIS
 */
qse_pio_t* qse_pio_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_char_t* path,
	int               flags,
	int               mode
);
/******/

/****f* qse.pio/qse_pio_close
 * NAME
 *  qse_pio_close - close a pipe
 *
 * SYNOPSIS
 */
void qse_pio_close (
	qse_pio_t* pio
);
/******/

qse_pio_t* qse_pio_init (
	qse_pio_t* pio,
	qse_mmgr_t* mmgr,
	const qse_char_t* path,
	int flags,
	int mode
);

void qse_pio_fini (
	qse_pio_t* pio
);

qse_pio_hnd_t qse_pio_gethandle (
	qse_pio_t* pio
);

/****f* qse.cmn.pio/qse_pio_sethandle
 * NAME
 *  qse_pio_sethandle - set the pipe handle
 * WARNING
 *  Avoid using this function if you don't know what you are doing.
 *  You may have to retrieve the previous handle using qse_pio_gethandle()
 *  to take relevant actions before resetting it with qse_pio_sethandle().
 * SYNOPSIS
 */
void qse_pio_sethandle (
	qse_pio_t* pio,
	qse_pio_hnd_t handle
);
/******/

qse_ssize_t qse_pio_read (
	qse_pio_t* pio,
	void* buf,
	qse_size_t size
);

qse_ssize_t qse_pio_write (
	qse_pio_t* pio,
	const void* buf,
	qse_size_t size
);

#ifdef __cplusplus
}
#endif

#endif
