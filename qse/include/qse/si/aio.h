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

#ifndef _QSE_SI_AIO_H_
#define _QSE_SI_AIO_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/time.h>

#if defined(_WIN32)
	typedef qse_uintptr_t qse_aio_syshnd_t;
	#define QSE_AIO_SYSHND_INVALID (~(qse_uintptr_t)0)
#else
	typedef int qse_aio_syshnd_t;
	#define QSE_AIO_SYSHND_INVALID (-1)
#endif

typedef struct qse_aio_devaddr_t qse_aio_devaddr_t;
struct qse_aio_devaddr_t
{
	int len;
	void* ptr;
};

/* ========================================================================= */

typedef struct qse_aio_t qse_aio_t;
typedef struct qse_aio_dev_t qse_aio_dev_t;
typedef struct qse_aio_dev_mth_t qse_aio_dev_mth_t;
typedef struct qse_aio_dev_evcb_t qse_aio_dev_evcb_t;

typedef struct qse_aio_wq_t qse_aio_wq_t;
typedef qse_intptr_t qse_aio_iolen_t; /* NOTE: this is a signed type */

enum qse_aio_errnum_t
{
	QSE_AIO_ENOERR,
	QSE_AIO_ENOIMPL,
	QSE_AIO_ESYSERR,
	QSE_AIO_EINTERN,

	QSE_AIO_ENOMEM,
	QSE_AIO_EINVAL,
	QSE_AIO_EEXIST,
	QSE_AIO_ENOENT,
	QSE_AIO_ENOSUP,     /* not supported */
	QSE_AIO_EMFILE,     /* too many open files */
	QSE_AIO_ENFILE,
	QSE_AIO_EAGAIN,
	QSE_AIO_ECONRF,     /* connection refused */
	QSE_AIO_ECONRS,     /* connection reset */
	QSE_AIO_ENOCAPA,    /* no capability */
	QSE_AIO_ETMOUT,     /* timed out */
	QSE_AIO_EPERM,      /* operation not permitted */

	QSE_AIO_EDEVMAKE,
	QSE_AIO_EDEVERR,
	QSE_AIO_EDEVHUP
};

typedef enum qse_aio_errnum_t qse_aio_errnum_t;

enum qse_aio_stopreq_t
{
	QSE_AIO_STOPREQ_NONE = 0,
	QSE_AIO_STOPREQ_TERMINATION,
	QSE_AIO_STOPREQ_WATCHER_ERROR
};
typedef enum qse_aio_stopreq_t qse_aio_stopreq_t;

/* ========================================================================= */

#define QSE_AIO_TMRIDX_INVALID ((qse_aio_tmridx_t)-1)

typedef qse_size_t qse_aio_tmridx_t;

typedef struct qse_aio_tmrjob_t qse_aio_tmrjob_t;

typedef void (*qse_aio_tmrjob_handler_t) (
	qse_aio_t*           aio,
	const qse_ntime_t*   now, 
	qse_aio_tmrjob_t*    tmrjob
);

struct qse_aio_tmrjob_t
{
	void*                     ctx;
	qse_ntime_t               when;
	qse_aio_tmrjob_handler_t  handler;
	qse_aio_tmridx_t*         idxptr; /* pointer to the index holder */
};

/* ========================================================================= */

struct qse_aio_dev_mth_t
{
	/* ------------------------------------------------------------------ */
	/* mandatory. called in qse_aio_makedev() */
	int           (*make)         (qse_aio_dev_t* dev, void* ctx); 

	/* ------------------------------------------------------------------ */
	/* mandatory. called in qse_aio_killdev(). also called in qse_aio_makedev() upon
	 * failure after make() success.
	 * 
	 * when 'force' is 0, the return value of -1 causes the device to be a
	 * zombie. the kill method is called periodically on a zombie device
	 * until the method returns 0.
	 *
	 * when 'force' is 1, the called should not return -1. If it does, the
	 * method is called once more only with the 'force' value of 2.
	 * 
	 * when 'force' is 2, the device is destroyed regardless of the return value.
	 */
	int           (*kill)         (qse_aio_dev_t* dev, int force); 

	/* ------------------------------------------------------------------ */
	qse_aio_syshnd_t (*getsyshnd)    (qse_aio_dev_t* dev); /* mandatory. called in qse_aio_makedev() after successful make() */


	/* ------------------------------------------------------------------ */
	/* return -1 on failure, 0 if no data is availble, 1 otherwise.
	 * when returning 1, *len must be sent to the length of data read.
	 * if *len is set to 0, it's treated as EOF. */
	int           (*read)         (qse_aio_dev_t* dev, void* data, qse_aio_iolen_t* len, qse_aio_devaddr_t* srcaddr);

	/* ------------------------------------------------------------------ */
	int           (*write)        (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t* len, const qse_aio_devaddr_t* dstaddr);

	/* ------------------------------------------------------------------ */
	int           (*ioctl)        (qse_aio_dev_t* dev, int cmd, void* arg);

};

struct qse_aio_dev_evcb_t
{
	/* return -1 on failure. 0 or 1 on success.
	 * when 0 is returned, it doesn't attempt to perform actual I/O.
	 * when 1 is returned, it attempts to perform actual I/O. */
	int           (*ready)        (qse_aio_dev_t* dev, int events);

	/* return -1 on failure, 0 or 1 on success.
	 * when 0 is returned, the main loop stops the attempt to read more data.
	 * when 1 is returned, the main loop attempts to read more data without*/
	int           (*on_read)      (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t len, const qse_aio_devaddr_t* srcaddr);

	/* return -1 on failure, 0 on success. 
	 * wrlen is the length of data written. it is the length of the originally
	 * posted writing request for a stream device. For a non stream device, it
	 * may be shorter than the originally posted length. */
	int           (*on_write)     (qse_aio_dev_t* dev, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_devaddr_t* dstaddr);
};

struct qse_aio_wq_t
{
	qse_aio_wq_t*       next;
	qse_aio_wq_t*       prev;

	qse_aio_iolen_t     olen; /* original data length */
	qse_uint8_t*    ptr;  /* pointer to data */
	qse_aio_iolen_t     len;  /* remaining data length */
	void*            ctx;
	qse_aio_dev_t*      dev; /* back-pointer to the device */

	qse_aio_tmridx_t    tmridx;
	qse_aio_devaddr_t   dstaddr;
};

#define QSE_AIO_WQ_INIT(wq) ((wq)->next = (wq)->prev = (wq))
#define QSE_AIO_WQ_TAIL(wq) ((wq)->prev)
#define QSE_AIO_WQ_HEAD(wq) ((wq)->next)
#define QSE_AIO_WQ_ISEMPTY(wq) (QSE_AIO_WQ_HEAD(wq) == (wq))
#define QSE_AIO_WQ_ISNODE(wq,x) ((wq) != (x))
#define QSE_AIO_WQ_ISHEAD(wq,x) (QSE_AIO_WQ_HEAD(wq) == (x))
#define QSE_AIO_WQ_ISTAIL(wq,x) (QSE_AIO_WQ_TAIL(wq) == (x))

#define QSE_AIO_WQ_NEXT(x) ((x)->next)
#define QSE_AIO_WQ_PREV(x) ((x)->prev)

#define QSE_AIO_WQ_LINK(p,x,n) do { \
	qse_aio_wq_t* pp = (p), * nn = (n); \
	(x)->prev = (p); \
	(x)->next = (n); \
	nn->prev = (x); \
	pp->next = (x); \
} while (0)

#define QSE_AIO_WQ_UNLINK(x) do { \
	qse_aio_wq_t* pp = (x)->prev, * nn = (x)->next; \
	nn->prev = pp; pp->next = nn; \
} while (0)

#define QSE_AIO_WQ_REPL(o,n) do { \
	qse_aio_wq_t* oo = (o), * nn = (n); \
	nn->next = oo->next; \
	nn->next->prev = nn; \
	nn->prev = oo->prev; \
	nn->prev->next = nn; \
} while (0)

/* insert an item at the back of the queue */
/*#define QSE_AIO_WQ_ENQ(wq,x)  QSE_AIO_WQ_LINK(QSE_AIO_WQ_TAIL(wq), x, QSE_AIO_WQ_TAIL(wq)->next)*/
#define QSE_AIO_WQ_ENQ(wq,x)  QSE_AIO_WQ_LINK(QSE_AIO_WQ_TAIL(wq), x, wq)

/* remove an item in the front from the queue */
#define QSE_AIO_WQ_DEQ(wq) QSE_AIO_WQ_UNLINK(QSE_AIO_WQ_HEAD(wq))

#define QSE_AIO_DEV_HEADERS \
	qse_aio_t*          aio; \
	qse_size_t      dev_size; \
	int                 dev_capa; \
	qse_aio_dev_mth_t*  dev_mth; \
	qse_aio_dev_evcb_t* dev_evcb; \
	qse_aio_wq_t        wq; \
	qse_aio_dev_t*      dev_prev; \
	qse_aio_dev_t*      dev_next 

struct qse_aio_dev_t
{
	QSE_AIO_DEV_HEADERS;
};

enum qse_aio_dev_capa_t
{
	QSE_AIO_DEV_CAPA_VIRTUAL      = (1 << 0),
	QSE_AIO_DEV_CAPA_IN           = (1 << 1),
	QSE_AIO_DEV_CAPA_OUT          = (1 << 2),
	/* #QSE_AIO_DEV_CAPA_PRI is meaningful only if #QSE_AIO_DEV_CAPA_IN is set */
	QSE_AIO_DEV_CAPA_PRI          = (1 << 3), 
	QSE_AIO_DEV_CAPA_STREAM       = (1 << 4),
	QSE_AIO_DEV_CAPA_OUT_QUEUED   = (1 << 5),

	/* internal use only. never set this bit to the dev_capa field */
	QSE_AIO_DEV_CAPA_IN_DISABLED  = (1 << 9),
	QSE_AIO_DEV_CAPA_IN_CLOSED    = (1 << 10),
	QSE_AIO_DEV_CAPA_OUT_CLOSED   = (1 << 11),
	QSE_AIO_DEV_CAPA_IN_WATCHED   = (1 << 12),
	QSE_AIO_DEV_CAPA_OUT_WATCHED  = (1 << 13),
	QSE_AIO_DEV_CAPA_PRI_WATCHED  = (1 << 14), /**< can be set only if QSE_AIO_DEV_CAPA_IN_WATCHED is set */

	QSE_AIO_DEV_CAPA_ACTIVE       = (1 << 15),
	QSE_AIO_DEV_CAPA_HALTED       = (1 << 16),
	QSE_AIO_DEV_CAPA_ZOMBIE       = (1 << 17)
};
typedef enum qse_aio_dev_capa_t qse_aio_dev_capa_t;

enum qse_aio_dev_watch_cmd_t
{
	QSE_AIO_DEV_WATCH_START,
	QSE_AIO_DEV_WATCH_UPDATE,
	QSE_AIO_DEV_WATCH_RENEW, /* automatic update */
	QSE_AIO_DEV_WATCH_STOP
};
typedef enum qse_aio_dev_watch_cmd_t qse_aio_dev_watch_cmd_t;

enum qse_aio_dev_event_t
{
	QSE_AIO_DEV_EVENT_IN  = (1 << 0),
	QSE_AIO_DEV_EVENT_OUT = (1 << 1),

	QSE_AIO_DEV_EVENT_PRI = (1 << 2),
	QSE_AIO_DEV_EVENT_HUP = (1 << 3),
	QSE_AIO_DEV_EVENT_ERR = (1 << 4)
};
typedef enum qse_aio_dev_event_t qse_aio_dev_event_t;


/* ========================================================================= */

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_aio_t* qse_aio_open (
	qse_mmgr_t*   mmgr,
	qse_size_t    xtnsize,
	qse_size_t    tmrcapa,  /**< initial timer capacity */
	qse_aio_errnum_t* errnum
);

QSE_EXPORT void qse_aio_close (
	qse_aio_t* aio
);

QSE_EXPORT int qse_aio_init (
	qse_aio_t*  aio,
	qse_mmgr_t* mmgr,
	qse_size_t  tmrcapa
);

QSE_EXPORT void qse_aio_fini (
	qse_aio_t*      aio
);

QSE_EXPORT int qse_aio_exec (
	qse_aio_t* aio
);

QSE_EXPORT int qse_aio_loop (
	qse_aio_t* aio
);

QSE_EXPORT void qse_aio_stop (
	qse_aio_t*        aio,
	qse_aio_stopreq_t stopreq
);

QSE_EXPORT qse_aio_dev_t* qse_aio_makedev (
	qse_aio_t*          aio,
	qse_size_t      dev_size,
	qse_aio_dev_mth_t*  dev_mth,
	qse_aio_dev_evcb_t* dev_evcb,
	void*            make_ctx
);

QSE_EXPORT void qse_aio_killdev (
	qse_aio_t*     aio,
	qse_aio_dev_t* dev
);

QSE_EXPORT int qse_aio_dev_ioctl (
	qse_aio_dev_t* dev,
	int         cmd,
	void*       arg
);

QSE_EXPORT int qse_aio_dev_watch (
	qse_aio_dev_t*          dev,
	qse_aio_dev_watch_cmd_t cmd,
	/** 0 or bitwise-ORed of #QSE_AIO_DEV_EVENT_IN and #QSE_AIO_DEV_EVENT_OUT */
	int                  events
);

QSE_EXPORT int qse_aio_dev_read (
	qse_aio_dev_t*         dev,
	int                 enabled
);

/**
 * The qse_aio_dev_write() function posts a writing request. 
 * It attempts to write data immediately if there is no pending requests.
 * If writing fails, it returns -1. If writing succeeds, it calls the
 * on_write callback. If the callback fails, it returns -1. If the callback 
 * succeeds, it returns 1. If no immediate writing is possible, the request
 * is enqueued to a pending request list. If enqueing gets successful,
 * it returns 0. otherwise it returns -1.
 */ 
QSE_EXPORT int qse_aio_dev_write (
	qse_aio_dev_t*            dev,
	const void*            data,
	qse_aio_iolen_t           len,
	void*                  wrctx,
	const qse_aio_devaddr_t*  dstaddr
);


QSE_EXPORT int qse_aio_dev_timedwrite (
	qse_aio_dev_t*           dev,
	const void*           data,
	qse_aio_iolen_t          len,
	const qse_ntime_t*   tmout,
	void*                 wrctx,
	const qse_aio_devaddr_t* dstaddr
);

QSE_EXPORT void qse_aio_dev_halt (
	qse_aio_dev_t* dev
);

/**
 * The qse_aio_instmrjob() function schedules a new event.
 *
 * \return #QSE_AIO_TMRIDX_INVALID on failure, valid index on success.
 */

QSE_EXPORT qse_aio_tmridx_t qse_aio_instmrjob (
	qse_aio_t*              aio,
	const qse_aio_tmrjob_t* job
);

QSE_EXPORT qse_aio_tmridx_t qse_aio_updtmrjob (
	qse_aio_t*              aio,
	qse_aio_tmridx_t        index,
	const qse_aio_tmrjob_t* job
);

QSE_EXPORT void qse_aio_deltmrjob (
	qse_aio_t*          aio,
	qse_aio_tmridx_t    index
);

/**
 * The qse_aio_gettmrjob() function returns the
 * pointer to the registered event at the given index.
 */
QSE_EXPORT qse_aio_tmrjob_t* qse_aio_gettmrjob (
	qse_aio_t*            aio,
	qse_aio_tmridx_t      index
);

QSE_EXPORT int qse_aio_gettmrjobdeadline (
	qse_aio_t*            aio,
	qse_aio_tmridx_t      index,
	qse_ntime_t*      deadline
);

/* ========================================================================= */


#ifdef __cplusplus
}
#endif


#endif
