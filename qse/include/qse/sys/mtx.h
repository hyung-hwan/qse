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


#ifndef _QSE_SYS_MTX_H_
#define _QSE_SYS_MTX_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/time.h>

typedef struct qse_mtx_t qse_mtx_t;

#if defined(_WIN32)
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_mtx_hnd_t;

#elif defined(__OS2__)

	/* typdef unsigned long ULONG;
	 * typedef ULONG HMTX; */
	typedef unsigned long qse_mtx_hnd_t;

#elif defined(__DOS__)
	/* not implemented */
#	error not implemented

#elif defined(__BEOS__)
	/* typedef int32 sem_id; 
	 * typedef sem_id qse_mtx_hnd_t; */
	typdef qse_int32_t qse_mtx_hnd_t;

#else

#	if (QSE_SIZEOF_PTHREAD_MUTEX_T == 0)
#		error unsupported

#	elif (QSE_SIZEOF_PTHREAD_MUTEX_T == QSE_SIZEOF_INT)
#		if defined(QSE_PTHREAD_MUTEX_T_IS_SIGNED)
			typedef int qse_mtx_hnd_t;
#		else
			typedef unsigned int qse_mtx_hnd_t;
#		endif
#	elif (QSE_SIZEOF_PTHREAD_MUTEX_T == QSE_SIZEOF_LONG)
#		if defined(QSE_PTHREAD_MUTEX_T_IS_SIGNED)
			typedef long qse_mtx_hnd_t;
#		else
			typedef unsigned long qse_mtx_hnd_t;
#		endif
#	else
#		include <qse/pack1.h>
		struct qse_mtx_hnd_t
		{
			qse_uint8_t b[QSE_SIZEOF_PTHREAD_MUTEX_T];
		};
		typedef struct qse_mtx_hnd_t qse_mtx_hnd_t;
#		include <qse/unpack.h>
#	endif

#endif

struct qse_mtx_t
{
	qse_mmgr_t* mmgr;
	qse_mtx_hnd_t hnd;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_mtx_t* qse_mtx_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        xtnsize
);

QSE_EXPORT void qse_mtx_close (
	qse_mtx_t* mtx
);

QSE_EXPORT int qse_mtx_init (
	qse_mtx_t*        mtx,
	qse_mmgr_t*       mmgr
);

QSE_EXPORT void qse_mtx_fini (
	qse_mtx_t* mtx
);

QSE_EXPORT qse_mmgr_t* qse_mtx_getmmgr (
	qse_mtx_t* mtx
);

QSE_EXPORT void* qse_mtx_getxtn (
	qse_mtx_t* mtx
);

QSE_EXPORT int qse_mtx_lock (
	qse_mtx_t*         mtx,
	const qse_ntime_t* waiting_time
);

QSE_EXPORT int qse_mtx_unlock (
	qse_mtx_t*   mtx
);


#ifdef __cplusplus
}
#endif

#endif
