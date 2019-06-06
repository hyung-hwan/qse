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

#ifndef _QSE_SI_TASK_H_
#define _QSE_SI_TASK_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_task_t qse_task_t;
typedef struct qse_task_slice_t qse_task_slice_t;

typedef qse_task_slice_t* (*qse_task_fnc_t) (
	qse_task_t*       task,
	qse_task_slice_t* slice,
	void*             ctx
);

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_task_t* qse_task_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize	
);

QSE_EXPORT void qse_task_close (
	qse_task_t* task
);

QSE_EXPORT qse_mmgr_t* qse_task_getmmgr (
	qse_task_t* task
);

QSE_EXPORT void* qse_task_getxtn (
	qse_task_t* task
);

QSE_EXPORT qse_task_slice_t* qse_task_create (
	qse_task_t*    task,
	qse_task_fnc_t fnc,
	void*          ctx,
	qse_size_t     stksize
);

QSE_EXPORT int qse_task_boot (
	qse_task_t*       task,
	qse_task_slice_t* to
);

QSE_EXPORT void qse_task_schedule (
	qse_task_t*       task,
	qse_task_slice_t* from,
	qse_task_slice_t* to
);


#if defined(__cplusplus)
}
#endif


#endif
