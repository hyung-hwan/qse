/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundatmrn, either version 3 of
    the License, or (at your optmrn) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/tmr.h>
#include "mem.h"

#define HEAP_PARENT(x) (((x) - 1) / 2)
#define HEAP_LEFT(x)   ((x) * 2 + 1)
#define HEAP_RIGHT(x)  ((x) * 2 + 2)

#define YOUNGER_THAN(x,y) (qse_cmptime(&(x)->when, &(y)->when) < 0)

qse_tmr_t* qse_tmr_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t capa)
{
	qse_tmr_t* tmr;

	tmr = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_tmr_t) + xtnsize);
	if (tmr)
	{
		if (qse_tmr_init (tmr, mmgr, capa) <= -1)
		{
			QSE_MMGR_FREE (mmgr, tmr);
			return QSE_NULL;
		}

		else QSE_MEMSET (QSE_XTN(tmr), 0, xtnsize);
	}

	return tmr;
}


void qse_tmr_close (qse_tmr_t* tmr)
{
	qse_tmr_fini (tmr);
	QSE_MMGR_FREE (tmr->mmgr, tmr);
}

int qse_tmr_init (qse_tmr_t* tmr, qse_mmgr_t* mmgr, qse_size_t capa)
{
	QSE_MEMSET (tmr, 0, QSE_SIZEOF(*tmr));

	if (capa <= 0) capa = 1;

	tmr->event = QSE_MMGR_ALLOC (mmgr, capa * QSE_SIZEOF(*tmr->event));
	if (tmr->event == QSE_NULL) return -1;

	tmr->mmgr = mmgr;
	tmr->capa = capa;
	return 0;
}

void qse_tmr_fini (qse_tmr_t* tmr)
{
	if (tmr->event) QSE_MMGR_FREE (tmr->mmgr, tmr->event);
}

qse_mmgr_t* qse_tmr_getmmgr (qse_tmr_t* tmr)
{
	return tmr->mmgr;
}

void* qse_tmr_getxtn (qse_tmr_t* tmr)
{
	return QSE_XTN (tmr);
}

void qse_tmr_clear (qse_tmr_t* tmr)
{
	tmr->size = 0;
}

static qse_size_t sift_up (qse_tmr_t* tmr, qse_size_t index)
{
	qse_size_t parent;

	parent = HEAP_PARENT(index);
	if (index > 0 && YOUNGER_THAN(&tmr->event[index], &tmr->event[parent]))
	{
		qse_tmr_event_t item = tmr->event[index]; 
		qse_size_t old_index = index;

		do
		{
			/* move down the parent to my current position */
			tmr->event[index] = tmr->event[parent];
			tmr->event[index].updater (tmr, parent, index, tmr->event[index].ctx);

			/* traverse up */
			index = parent;
			parent = HEAP_PARENT(parent);
		}
		while (index > 0 && YOUNGER_THAN(&item, &tmr->event[parent]));

		tmr->event[index] = item;
		if (index != old_index)
			tmr->event[index].updater (tmr, old_index, index, tmr->event[index].ctx);
	}

	return index;
}

static qse_size_t sift_down (qse_tmr_t* tmr, qse_size_t index)
{
	qse_size_t base = tmr->size / 2;

	if (index < base) /* at least 1 child is under the 'index' positmrn */
	{
		qse_tmr_event_t item = tmr->event[index];
		qse_size_t old_index = index;

		do
		{
			qse_size_t left, right, younger;

			left= HEAP_LEFT(index);
			right = HEAP_RIGHT(index);

			if (right < tmr->size && YOUNGER_THAN(&tmr->event[right], &tmr->event[left]))
			{
				younger = right;
			}
			else
			{
				younger = left;
			}

			if (YOUNGER_THAN(&item, &tmr->event[younger])) break;

			tmr->event[index] = tmr->event[younger];
			tmr->event[index].updater (tmr, younger, index, tmr->event[index].ctx);

			index = younger;
		}
		while (index < base);
		
		tmr->event[index] = item;
		if (index != old_index)
			tmr->event[index].updater (tmr, old_index, index, tmr->event[index].ctx);
	}

	return index;
}

void qse_tmr_remove (qse_tmr_t* tmr, qse_size_t index)
{
	qse_tmr_event_t item;

	QSE_ASSERT (index < tmr->size);

	item = tmr->event[index];
	tmr->event[index].updater (tmr, index, QSE_TMR_INVALID, tmr->event[index].ctx);
	tmr->size = tmr->size - 1;
	if (tmr->size > 0)
	{
		tmr->event[index] = tmr->event[tmr->size];
		tmr->event[index].updater (tmr, tmr->size, index, tmr->event[index].ctx);
		YOUNGER_THAN(&tmr->event[index], &item)? sift_up(tmr, index): sift_down(tmr, index);
	}
}


qse_size_t qse_tmr_insert (qse_tmr_t* tmr, const qse_tmr_event_t* event)
{
	qse_size_t index = tmr->size;

	if (index >= tmr->capa)
	{
		qse_tmr_event_t* tmp;
		qse_size_t new_capa;

		new_capa = tmr->capa * 2;
		tmp = QSE_MMGR_REALLOC (tmr->mmgr, tmr->event, new_capa);
		if (tmp == QSE_NULL) return QSE_TMR_INVALID;

		tmr->event = tmp;
		tmr->capa = new_capa;
	}

	tmr->size = tmr->size + 1;
	tmr->event[index] = *event;
	return sift_up (tmr, index);
}

qse_size_t qse_tmr_update (qse_tmr_t* tmr, qse_size_t index, const qse_tmr_event_t* event)
{
	qse_tmr_event_t item;

	item = tmr->event[index];
	tmr->event[index] = *event;
	return YOUNGER_THAN(event, &item)? sift_up (tmr, index): sift_down (tmr, index);
}

qse_size_t qse_tmr_fire (qse_tmr_t* tmr, const qse_ntime_t* tm)
{
	qse_ntime_t now;
	qse_tmr_event_t event;
	qse_size_t fire_count = 0;

	/* if the current time is not specified, get it from the system */
	if (tm) now = *tm;
	else if (qse_gettime (&now) <= -1) return -1;

	while (tmr->size > 0)
	{
		if (qse_cmptime(&tmr->event[0].when, &now) > 0) break;

		event = tmr->event[0];
		qse_tmr_remove (tmr, 0);

		fire_count++;
		event.handler (tmr, &now, event.ctx);
	}

	return fire_count;
}

int qse_tmr_gettmout (qse_tmr_t* tmr, const qse_ntime_t* tm, qse_ntime_t* tmout)
{
	qse_ntime_t now;

	/* time-out can't be calculated when there's no event scheduled */
	if (tmr->size <= 0) return -1;

	/* if the current time is not specified, get it from the system */
	if (tm) now = *tm;
	else if (qse_gettime (&now) <= -1) return -1;

	qse_subtime (&tmr->event[0].when, &now, tmout);
	if (tmout->sec < 0) qse_cleartime (tmout);
}

