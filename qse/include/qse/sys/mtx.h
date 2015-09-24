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


#ifndef _QSE_CMN_MTX_H_
#define _QSE_CMN_MTX_H_

#include <qse/types.h>
#include <qse/macros.h>


#if defined(_WIN32)
	typedef void* qse_mtx_hnd_t;
#elif defined(__OS2__)
#	error not implemented
#elif defined(__DOS__)
#	error not implemented
#elif defined(__BEOS__)
	typedef sem_id qse_mtx_hnd_t;
#else
#	include <pthread.h>
	typedef pthread_mutex_t qse_mtx_hnd_t;
#endif

typedef struct qse_mtx_t qse_mtx_t;

struct qse_mtx_t
{
	qse_mmgr_t* mmgr;
	qse_mtx_hnd_t hnd;
};

#ifdef __cplusplus
extern "C" {
#endif

qse_mtx_t* qse_mtx_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        xtnsize
);

void qse_mtx_close (
	qse_mtx_t* mtx
);

int qse_mtx_init (
	qse_mtx_t*        mtx,
	qse_mmgr_t*       mmgr
);

void qse_mtx_fini (
	qse_mtx_t* mtx
);

qse_mmgr_t* qse_mtx_getmmgr (
	qse_mtx_t* mtx
);

void* qse_mtx_getxtn (
	qse_mtx_t* mtx
);

int qse_mtx_lock (
	qse_mtx_t* mtx
);

int qse_mtx_unlock (
	qse_mtx_t* mtx
);

int qse_mtx_trylock (
	qse_mtx_t* mtx
);

#ifdef __cplusplus
}
#endif

#endif
