/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
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

QSE_EXPORT void qse_tmr_remove (
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
