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

#ifndef _QSE_SYS_RWL_H_
#define _QSE_SYS_RWL_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/sys/mtx.h>
#include <qse/sys/cnd.h>

enum qse_rwl_flag_t
{
	QSE_RWL_PREFER_WRITER = (1 << 0)
};
typedef enum qse_rwl_flag_t qse_rwl_flag_t;

struct qse_rwl_t
{
	qse_mmgr_t* mmgr;
	int flags;

	qse_mtx_t mtx;
	qse_cnd_t rcnd;
	qse_cnd_t wcnd;

	qse_size_t rwait_count;
	qse_size_t wwait_count;
	qse_size_t ractive_count;
	qse_size_t wactive_count;
};

typedef struct qse_rwl_t qse_rwl_t;

#ifdef __cplusplus
extern "C" {
#endif

qse_rwl_t* qse_rwl_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize,
	int         flags
);

void qse_rwl_close (
	qse_rwl_t* rwl
);

int qse_rwl_init (
	qse_rwl_t*   rwl,
	qse_mmgr_t*  mmgr,
	int          flags
);

void qse_rwl_fini (
	qse_rwl_t* rwl
);

qse_mmgr_t* qse_rwl_getmmgr (
	qse_rwl_t* rwl
);

void* qse_rwl_getxtn (
	qse_rwl_t* rwl
);

int qse_rwl_lockr (
	qse_rwl_t*   rwl,
	qse_ntime_t* waiting_time
);

int qse_rwl_unlockr (
	qse_rwl_t* rwl
);

int qse_rwl_lockw (
	qse_rwl_t*   rwl,
	qse_ntime_t* waiting_time
);

int qse_rwl_unlockw (
	qse_rwl_t* rwl
);

#ifdef __cplusplus
}
#endif

#endif
