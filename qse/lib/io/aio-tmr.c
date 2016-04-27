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


#include "aio-prv.h"

#define HEAP_PARENT(x) (((x) - 1) / 2)
#define HEAP_LEFT(x)   ((x) * 2 + 1)
#define HEAP_RIGHT(x)  ((x) * 2 + 2)

#define YOUNGER_THAN(x,y) (qse_cmptime(&(x)->when, &(y)->when) < 0)

void qse_aio_cleartmrjobs (qse_aio_t* aio)
{
	while (aio->tmr.size > 0) qse_aio_deltmrjob (aio, 0);
}

static qse_aio_tmridx_t sift_up (qse_aio_t* aio, qse_aio_tmridx_t index, int notify)
{
	qse_aio_tmridx_t parent;

	parent = HEAP_PARENT(index);
	if (index > 0 && YOUNGER_THAN(&aio->tmr.jobs[index], &aio->tmr.jobs[parent]))
	{
		qse_aio_tmrjob_t item;

		item = aio->tmr.jobs[index]; 

		do
		{
			/* move down the parent to my current position */
			aio->tmr.jobs[index] = aio->tmr.jobs[parent];
			if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = index;

			/* traverse up */
			index = parent;
			parent = HEAP_PARENT(parent);
		}
		while (index > 0 && YOUNGER_THAN(&item, &aio->tmr.jobs[parent]));

		aio->tmr.jobs[index] = item;
		if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = index;
	}

	return index;
}

static qse_aio_tmridx_t sift_down (qse_aio_t* aio, qse_aio_tmridx_t index, int notify)
{
	qse_size_t base = aio->tmr.size / 2;

	if (index < base) /* at least 1 child is under the 'index' position */
	{
		qse_aio_tmrjob_t item;

		item = aio->tmr.jobs[index];

		do
		{
			qse_aio_tmridx_t left, right, younger;

			left = HEAP_LEFT(index);
			right = HEAP_RIGHT(index);

			if (right < aio->tmr.size && YOUNGER_THAN(&aio->tmr.jobs[right], &aio->tmr.jobs[left]))
			{
				younger = right;
			}
			else
			{
				younger = left;
			}

			if (YOUNGER_THAN(&item, &aio->tmr.jobs[younger])) break;

			aio->tmr.jobs[index] = aio->tmr.jobs[younger];
			if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = index;

			index = younger;
		}
		while (index < base);
		
		aio->tmr.jobs[index] = item;
		if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = index;
	}

	return index;
}

void qse_aio_deltmrjob (qse_aio_t* aio, qse_aio_tmridx_t index)
{
	qse_aio_tmrjob_t item;

	QSE_ASSERT (index < aio->tmr.size);

	item = aio->tmr.jobs[index];
	if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = QSE_AIO_TMRIDX_INVALID;

	aio->tmr.size = aio->tmr.size - 1;
	if (aio->tmr.size > 0 && index != aio->tmr.size)
	{
		aio->tmr.jobs[index] = aio->tmr.jobs[aio->tmr.size];
		if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = index;
		YOUNGER_THAN(&aio->tmr.jobs[index], &item)? sift_up(aio, index, 1): sift_down(aio, index, 1);
	}
}

qse_aio_tmridx_t qse_aio_instmrjob (qse_aio_t* aio, const qse_aio_tmrjob_t* job)
{
	qse_aio_tmridx_t index = aio->tmr.size;

	if (index >= aio->tmr.capa)
	{
		qse_aio_tmrjob_t* tmp;
		qse_size_t new_capa;

		QSE_ASSERT (aio->tmr.capa >= 1);
		new_capa = aio->tmr.capa * 2;
		tmp = (qse_aio_tmrjob_t*)QSE_MMGR_REALLOC (aio->mmgr, aio->tmr.jobs, new_capa * QSE_SIZEOF(*tmp));
		if (tmp == QSE_NULL) 
		{
			aio->errnum = QSE_AIO_ENOMEM;
			return QSE_AIO_TMRIDX_INVALID;
		}

		aio->tmr.jobs = tmp;
		aio->tmr.capa = new_capa;
	}

	aio->tmr.size = aio->tmr.size + 1;
	aio->tmr.jobs[index] = *job;
	if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = index;
	return sift_up (aio, index, 0);
}

qse_aio_tmridx_t qse_aio_updtmrjob (qse_aio_t* aio, qse_aio_tmridx_t index, const qse_aio_tmrjob_t* job)
{
	qse_aio_tmrjob_t item;
	item = aio->tmr.jobs[index];
	aio->tmr.jobs[index] = *job;
	if (aio->tmr.jobs[index].idxptr) *aio->tmr.jobs[index].idxptr = index;
	return YOUNGER_THAN(job, &item)? sift_up (aio, index, 0): sift_down (aio, index, 0);
}

void qse_aio_firetmrjobs (qse_aio_t* aio, const qse_ntime_t* tm, qse_size_t* firecnt)
{
	qse_ntime_t now;
	qse_aio_tmrjob_t tmrjob;
	qse_size_t count = 0;

	/* if the current time is not specified, get it from the system */
	if (tm) now = *tm;
	else qse_gettime (&now);

	while (aio->tmr.size > 0)
	{
		if (qse_cmptime(&aio->tmr.jobs[0].when, &now) > 0) break;

		tmrjob = aio->tmr.jobs[0]; /* copy the scheduled job */
		qse_aio_deltmrjob (aio, 0); /* deschedule the job */

		count++;
		tmrjob.handler (aio, &now, &tmrjob); /* then fire the job */
	}

	if (firecnt) *firecnt = count;
}

int qse_aio_gettmrtmout (qse_aio_t* aio, const qse_ntime_t* tm, qse_ntime_t* tmout)
{
	qse_ntime_t now;

	/* time-out can't be calculated when there's no job scheduled */
	if (aio->tmr.size <= 0) 
	{
		aio->errnum = QSE_AIO_ENOENT;
		return -1;
	}

	/* if the current time is not specified, get it from the system */
	if (tm) now = *tm;
	else qse_gettime (&now);

	qse_subtime (&aio->tmr.jobs[0].when, &now, tmout);
	if (tmout->sec < 0) qse_cleartime (tmout);

	return 0;
}

qse_aio_tmrjob_t* qse_aio_gettmrjob (qse_aio_t* aio, qse_aio_tmridx_t index)
{
	if (index < 0 || index >= aio->tmr.size)
	{
		aio->errnum = QSE_AIO_ENOENT;
		return QSE_NULL;
	}

	return &aio->tmr.jobs[index];
}

int qse_aio_gettmrjobdeadline (qse_aio_t* aio, qse_aio_tmridx_t index, qse_ntime_t* deadline)
{
	if (index < 0 || index >= aio->tmr.size)
	{
		aio->errnum = QSE_AIO_ENOENT;
		return -1;
	}

	*deadline = aio->tmr.jobs[index].when;
	return 0;
}
