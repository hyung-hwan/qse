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

#ifndef _QSE_CMN_TMR_H_
#define _QSE_CMN_TMR_H_

#include <qse/cmn/time.h>

typedef struct qse_tmr_t qse_tmr_t;
typedef struct qse_tmr_event_t qse_tmr_event_t;
typedef qse_size_t qse_tmr_index_t;

typedef void (*qse_tmr_handler_t) (
	qse_tmr_t*         tmr,
	const qse_ntime_t* now, 
	qse_tmr_event_t*   evt
);

typedef void (*qse_tmr_updater_t) (
	qse_tmr_t*        tmr,
	qse_tmr_index_t   old_index,
	qse_tmr_index_t   new_index,
	qse_tmr_event_t*  evt
);

struct qse_tmr_t
{
	qse_mmgr_t*      mmgr;
	qse_size_t       capa;
	qse_size_t       size;
	qse_tmr_event_t* event;
};

struct qse_tmr_event_t
{
	void*              ctx;    /* primary context pointer */
	void*              ctx2;   /* secondary context pointer */
	void*              ctx3;   /* tertiary context pointer */

	qse_ntime_t        when;
	qse_tmr_handler_t  handler;
	qse_tmr_updater_t  updater;
};

#define QSE_TMR_INVALID_INDEX ((qse_size_t)-1)

#define QSE_TMR_SIZE(tmr) ((tmr)->size)
#define QSE_TMR_CAPA(tmr) ((tmr)->capa);

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_tmr_t* qse_tmr_open (
	qse_mmgr_t* mmgr, 
	qse_size_t  xtnsize,
	qse_size_t  capa
);

QSE_EXPORT void qse_tmr_close (
	qse_tmr_t* tmr
);

QSE_EXPORT int qse_tmr_init (
	qse_tmr_t*  tmr, 
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

QSE_EXPORT void qse_tmr_fini (
	qse_tmr_t* tmr
);

QSE_EXPORT qse_mmgr_t* qse_tmr_getmmgr (
	qse_tmr_t* tmr
);

QSE_EXPORT void* qse_tmr_getxtn (
	qse_tmr_t* tmr
);

QSE_EXPORT void qse_tmr_clear (
	qse_tmr_t* tmr
);

/**
 * The qse_tmr_insert() function schedules a new event.
 *
 * \return #QSE_TMR_INVALID_INDEX on failure, valid index on success.
 */

QSE_EXPORT qse_tmr_index_t qse_tmr_insert (
	qse_tmr_t*             tmr,
	const qse_tmr_event_t* event
);

QSE_EXPORT qse_size_t qse_tmr_update (
	qse_tmr_t*             tmr,
	qse_tmr_index_t        index,
	const qse_tmr_event_t* event
);

QSE_EXPORT void qse_tmr_delete (
	qse_tmr_t*      tmr,
	qse_tmr_index_t index
);

QSE_EXPORT qse_size_t qse_tmr_fire (
	qse_tmr_t*         tmr,
	const qse_ntime_t* tm
);

QSE_EXPORT int qse_tmr_gettmout (
	qse_tmr_t*         tmr,
	const qse_ntime_t* tm,
	qse_ntime_t*       tmout
);

/**
 * The qse_tmr_getevent() function returns the
 * pointer to the registered event at the given index.
 */
QSE_EXPORT qse_tmr_event_t* qse_tmr_getevent (
	qse_tmr_t*        tmr,
	qse_tmr_index_t   index
);

#if defined(__cplusplus)
}
#endif

#endif
