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

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EQSERESS OR
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

#ifndef _QSE_SI_THR_H_
#define _QSE_SI_THR_H_

#include <qse/types.h>
#include <qse/macros.h>

/**
 * The qse_thr_t type defines a thread object.
 */
typedef struct qse_thr_t qse_thr_t;

/** 
 * The qse_thr_rtn_t type defines a thread routine that can be passed to 
 * qse_thr_open() and qse_thr_start(). When it is executed, the pointer to the
 * calling thread object is passed as its first argument. 
 */
typedef int (*qse_thr_rtn_t) (qse_thr_t* thr, void* ctx);

enum qse_thr_state_t
{
	QSE_THR_INCUBATING,
	QSE_THR_RUNNING,
	QSE_THR_TERMINATED,
	QSE_THR_ABORTED
};
typedef enum qse_thr_state_t qse_thr_state_t;

enum qse_thr_flag_t
{
	QSE_THR_DETACHED        = (1 << 0),
	QSE_THR_SIGNALS_BLOCKED = (1 << 1)
};
typedef enum qse_thr_flag_t qse_thr_flag_t;

#if defined(_WIN32)
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_thr_hnd_t;

#elif defined(__OS2__)
	/* typedef unsigned long LHANDLE
	 * typedef LHANDLE TID */
	typedef unsigned long qse_thr_hnd_t;

#elif defined(__DOS__)
	/* not implemented */
#	error not implemented

#elif defined(__BEOS__)
	/*typedef thread_id qse_thr_hnd_t;*/
	typdef qse_int32_t qse_thr_hnd_t;

#else
	#if (QSE_SIZEOF_PTHREAD_T == QSE_SIZEOF_INT)
		#if defined(QSE_PTHREAD_T_IS_SIGNED)
			typedef int qse_thr_hnd_t;
		#else
			typedef unsigned int qse_thr_hnd_t;
		#endif
	#elif (QSE_SIZEOF_PTHREAD_T == QSE_SIZEOF_LONG)
		#if defined(QSE_PTHREAD_T_IS_SIGNED)
			typedef long qse_thr_hnd_t;
		#else
			typedef unsigned long qse_thr_hnd_t;
		#endif
	#else
		typedef int qse_thr_hnd_t;
	#endif
#endif


struct qse_thr_t
{
	qse_mmgr_t*       mmgr;


	unsigned int      __flags;
	qse_size_t        __stacksize;

	qse_thr_rtn_t     __main_routine;
	void*             __ctx;

	qse_thr_hnd_t     __handle;
	qse_thr_state_t   __state;

	int               __return_code;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_thr_open() function instantiates a thread object. The function 
 * pointed to by \a routine is executed when qse_thr_start() is called.
 * 
 */
QSE_EXPORT qse_thr_t* qse_thr_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        xtnsize
);

/**
 * The qse_thr_close() function destroys a thread object. Make sure that the
 * thread routine has been terminated properly.
 */
QSE_EXPORT void qse_thr_close (
	qse_thr_t* thr
);

QSE_EXPORT int qse_thr_init (
	qse_thr_t*        thr,
	qse_mmgr_t*       mmgr
);

QSE_EXPORT void qse_thr_fini (
	qse_thr_t* thr
);

QSE_EXPORT qse_mmgr_t* qse_thr_getmmgr (
	qse_thr_t* thr
);

QSE_EXPORT void* qse_thr_getxtn (
	qse_thr_t* thr
);

QSE_EXPORT qse_size_t qse_thr_getstacksize (
	qse_thr_t* thr
);

/**
 * The qse_thr_setstacksize() function sets the stack size of a thread.
 * It must be called before a thread routine gets started.
 */
QSE_EXPORT void qse_thr_setstacksize (
	qse_thr_t* thr,
	qse_size_t num
);

/** 
 * The qse_thr_start() executes a thread routine in a new thread of control.

 * QSE_THR_DETACHED, when set in \a flags, puts the thread in a detached state.
 * Otherwise, the thread is joinable. 
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_thr_start (
	qse_thr_t*    thr,
	qse_thr_rtn_t func,
	void*         ctx,
	int           flags /**< 0 or bitwise-or of the #qse_thr_flag_t enumerators  */
);

/**
 * The qse_thr_stop() function aborts a thread.
 * \return  0 on success, -1 on failure
 */
QSE_EXPORT int qse_thr_stop (qse_thr_t* thr);

/**
 * The qse_thr_join() function waits for thread termination.
 * \return  0 on success, -1 on failure
 */
QSE_EXPORT int qse_thr_join (qse_thr_t* thr);

/**
 * The qse_thr_detach() function detaches a thread.
 * \return  0 on success, -1 on failure
 */
QSE_EXPORT int qse_thr_detach (qse_thr_t* thr);

/**
 * The qse_thr_kill() function sends a signal to a thread.
 */
QSE_EXPORT int qse_thr_kill (qse_thr_t* thr, int sig);

/**
 * The qse_thr_blocksig() function causes a therad to block the signal \a sig.
 */
QSE_EXPORT int qse_thr_blocksig (qse_thr_t* thr, int sig);

/**
 * The qse_thr_unblocksig() function causes a therad to unblock the signal \a sig.
 */
QSE_EXPORT int qse_thr_unblocksig (qse_thr_t* thr, int sig);

/**
 * The qse_thr_blockallsigs() function causes a therad to block all signals.
 */
QSE_EXPORT int qse_thr_blockallsigs (qse_thr_t* thr);

/**
 * The qse_thr_unblockallsigs() function causes a therad to unblock all signals.
 */
QSE_EXPORT int qse_thr_unblockallsigs (qse_thr_t* thr);


/**
 * The qse_thr_gethnd() function returns the native thread handle.
 */
QSE_EXPORT qse_thr_hnd_t qse_thr_gethnd (
	qse_thr_t* thr
);

/** 
 * The qse_thr_getretcode() returns the return code a thread rountine
 * that has been terminated. If no thread routine has been started and
 * terminated, 0 is returned.
 */
QSE_EXPORT int qse_thr_getretcode (
	qse_thr_t* thr
);

/**
 * The qse_thr_state() function returns the current state.
 */
QSE_EXPORT qse_thr_state_t qse_thr_getstate (
	qse_thr_t* thr
);

/**
 * The qse_get_thr_hnd() function returns the native handle to the 
 * calling thread.
 */
QSE_EXPORT qse_thr_hnd_t qse_get_thr_hnd (void);

#ifdef __cplusplus
}
#endif

#endif
