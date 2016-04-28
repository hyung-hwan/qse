/*
 * $Id$
 *
    Copyright (c) 2006-2016 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_IO_AIO_PRO_H_
#define _QSE_IO_AIO_PRO_H_

#include <qse/io/aio.h>

enum qse_aio_dev_pro_sid_t
{
	QSE_AIO_DEV_PRO_MASTER = -1,
	QSE_AIO_DEV_PRO_IN     =  0,
	QSE_AIO_DEV_PRO_OUT    =  1,
	QSE_AIO_DEV_PRO_ERR    =  2
};
typedef enum qse_aio_dev_pro_sid_t qse_aio_dev_pro_sid_t;

typedef struct qse_aio_dev_pro_t qse_aio_dev_pro_t;
typedef struct qse_aio_dev_pro_slave_t qse_aio_dev_pro_slave_t;

typedef int (*qse_aio_dev_pro_on_read_t) (qse_aio_dev_pro_t* dev, const void* data, qse_aio_iolen_t len, qse_aio_dev_pro_sid_t sid);
typedef int (*qse_aio_dev_pro_on_write_t) (qse_aio_dev_pro_t* dev, qse_aio_iolen_t wrlen, void* wrctx);
typedef void (*qse_aio_dev_pro_on_close_t) (qse_aio_dev_pro_t* dev, qse_aio_dev_pro_sid_t sid);

struct qse_aio_dev_pro_t
{
	QSE_AIO_DEV_HEADERS;

	int flags;
	qse_intptr_t child_pid; /* defined to qse_intptr_t to hide pid_t */
	qse_aio_dev_pro_slave_t* slave[3];
	int slave_count;

	qse_aio_dev_pro_on_read_t on_read;
	qse_aio_dev_pro_on_write_t on_write;
	qse_aio_dev_pro_on_close_t on_close;

	qse_mchar_t* mcmd;
};

struct qse_aio_dev_pro_slave_t
{
	QSE_AIO_DEV_HEADERS;
	qse_aio_dev_pro_sid_t id;
	qse_aio_syshnd_t pfd;
	qse_aio_dev_pro_t* master; /* parent device */
};

enum qse_aio_dev_pro_make_flag_t
{
	QSE_AIO_DEV_PRO_WRITEIN  = (1 << 0),
	QSE_AIO_DEV_PRO_READOUT  = (1 << 1),
	QSE_AIO_DEV_PRO_READERR  = (1 << 2),

	QSE_AIO_DEV_PRO_ERRTOOUT = (1 << 3),
	QSE_AIO_DEV_PRO_OUTTOERR = (1 << 4),

	QSE_AIO_DEV_PRO_INTONUL  = (1 << 5),
	QSE_AIO_DEV_PRO_OUTTONUL = (1 << 6),
	QSE_AIO_DEV_PRO_ERRTONUL = (1 << 7),

	STUO_DEV_PRO_DROPIN   = (1 << 8),
	STUO_DEV_PRO_DROPOUT  = (1 << 9),
	STUO_DEV_PRO_DROPERR  = (1 << 10),


	QSE_AIO_DEV_PRO_SHELL                = (1 << 13),

	/* perform no waitpid() on a child process upon device destruction.
	 * you should set this flag if your application has automatic child 
	 * process reaping enabled. for instance, SIGCHLD is set to SIG_IGN
	 * on POSIX.1-2001 compliant systems */
	QSE_AIO_DEV_PRO_FORGET_CHILD         = (1 << 14),


	QSE_AIO_DEV_PRO_FORGET_DIEHARD_CHILD = (1 << 15)
};
typedef enum qse_aio_dev_pro_make_flag_t qse_aio_dev_pro_make_flag_t;

typedef struct qse_aio_dev_pro_make_t qse_aio_dev_pro_make_t;
struct qse_aio_dev_pro_make_t
{
	int flags; /**< bitwise-ORed of qse_aio_dev_pro_make_flag_t enumerators */
	const void* cmd;
	qse_aio_dev_pro_on_write_t on_write; /* mandatory */
	qse_aio_dev_pro_on_read_t on_read; /* mandatory */
	qse_aio_dev_pro_on_close_t on_close; /* optional */
};


enum qse_aio_dev_pro_ioctl_cmd_t
{
	QSE_AIO_DEV_PRO_CLOSE,
	QSE_AIO_DEV_PRO_KILL_CHILD
};
typedef enum qse_aio_dev_pro_ioctl_cmd_t qse_aio_dev_pro_ioctl_cmd_t;

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT  qse_aio_dev_pro_t* qse_aio_dev_pro_make (
	qse_aio_t*                    aio,
	qse_size_t                xtnsize,
	const qse_aio_dev_pro_make_t* data
);

QSE_EXPORT void qse_aio_dev_pro_kill (
	qse_aio_dev_pro_t* pro
);

QSE_EXPORT int qse_aio_dev_pro_write (
	qse_aio_dev_pro_t*  pro,
	const void*         data,
	qse_aio_iolen_t     len,
	void*               wrctx
);

QSE_EXPORT int qse_aio_dev_pro_timedwrite (
	qse_aio_dev_pro_t*  pro,
	const void*         data,
	qse_aio_iolen_t     len,
	const qse_ntime_t*  tmout,
	void*               wrctx
);

QSE_EXPORT int qse_aio_dev_pro_close (
	qse_aio_dev_pro_t*     pro,
	qse_aio_dev_pro_sid_t  sid
);


QSE_EXPORT int qse_aio_dev_pro_killchild (
	qse_aio_dev_pro_t*     pro
);

#ifdef __cplusplus
}
#endif

#endif
