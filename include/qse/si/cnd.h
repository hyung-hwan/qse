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

#ifndef _QSE_SI_CND_H_
#define _QSE_SI_CND_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/time.h>
#include <qse/si/mtx.h>

typedef struct qse_cnd_t qse_cnd_t;


#if defined(_WIN32)
	/* define nothing */

#elif defined(__OS2__)
	/* typdef unsigned long ULONG;
	 * typedef ULONG HEV; */
	typedef unsigned long qse_cnd_hnd_t;

#elif defined(__DOS__)
#	error not implemented

#else

#	if (QSE_SIZEOF_PTHREAD_COND_T == 0)
#		error unsupported

#	elif (QSE_SIZEOF_PTHREAD_COND_T == QSE_SIZEOF_INT)
#		if defined(QSE_PTHREAD_COND_T_IS_SIGNED)
			typedef int qse_cnd_hnd_t;
#		else
			typedef unsigned int qse_cnd_hnd_t;
#		endif
#	elif (QSE_SIZEOF_PTHREAD_COND_T == QSE_SIZEOF_LONG)
#		if defined(QSE_PTHREAD_COND_T_IS_SIGNED)
			typedef long qse_cnd_hnd_t;
#		else
			typedef unsigned long qse_cnd_hnd_t;
#		endif
#	else
#		include <qse/pack1.h>
		struct qse_cnd_hnd_t
		{
			qse_uint8_t b[QSE_SIZEOF_PTHREAD_COND_T];
		};
		typedef struct qse_cnd_hnd_t qse_cnd_hnd_t;
#		include <qse/unpack.h>
#	endif

#endif

struct qse_cnd_t
{
	qse_mmgr_t* mmgr;

#if defined(_WIN32)
	void* gate;
	void* queue;
	void* mutex;
	unsigned int  gone;
	unsigned long blocked;
	unsigned int  waiting;
#elif defined(__OS2__)
	qse_size_t wait_count;
	qse_cnd_hnd_t hnd;


#else
	qse_cnd_hnd_t hnd;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_cnd_t* qse_cnd_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize
);

QSE_EXPORT void qse_cnd_close (
	qse_cnd_t* cnd
);

QSE_EXPORT int qse_cnd_init (
	qse_cnd_t*  cnd,
	qse_mmgr_t* mmgr
);

QSE_EXPORT void qse_cnd_fini (
	qse_cnd_t* cnd
);

QSE_EXPORT qse_mmgr_t* qse_cnd_getmmgr (
	qse_cnd_t* cnd
);

QSE_EXPORT void* qse_cnd_getxtn (
	qse_cnd_t* cnd
);

QSE_EXPORT void qse_cnd_signal (
	qse_cnd_t* cond
);

QSE_EXPORT void qse_cnd_broadcast (
	qse_cnd_t* cond
);

/**
 * The qse_cnd_wait() function blocks the calling thread until the condition
 * variable is signaled. The caller must lock the mutex before calling this
 * function and unlock the mutex after this function finishes.
 */
QSE_EXPORT void qse_cnd_wait (
	qse_cnd_t*         cond, 
	qse_mtx_t*         mutex,
	const qse_ntime_t* waiting_time
);

#ifdef __cplusplus
}
#endif

#endif
