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

#include <qse/si/rwl.h>
#include "../cmn/mem-prv.h"

qse_rwl_t* qse_rwl_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, int flags)
{
	qse_rwl_t* rwl;

	rwl = (qse_rwl_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_rwl_t) + xtnsize);
	if (rwl)
	{
		if (qse_rwl_init (rwl, mmgr, flags) <= -1)
		{
			QSE_MMGR_FREE (mmgr, rwl);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(rwl), 0, xtnsize);
	}

	return rwl;
}

void qse_rwl_close (qse_rwl_t* rwl)
{
	qse_rwl_fini (rwl);
	QSE_MMGR_FREE (rwl->mmgr, rwl);
}

int qse_rwl_init (qse_rwl_t* rwl, qse_mmgr_t* mmgr, int flags)
{
	QSE_MEMSET (rwl, 0, QSE_SIZEOF(*rwl));
	rwl->mmgr = mmgr;
	rwl->flags = flags;

	if (qse_mtx_init (&rwl->mtx, mmgr) <= -1)
	{
		return -1;
	}

	if (qse_cnd_init (&rwl->rcnd, mmgr) <= -1)
	{
		qse_mtx_fini (&rwl->mtx);
		return -1;
	}

	if (qse_cnd_init (&rwl->wcnd, mmgr) <= -1)
	{
		qse_cnd_fini (&rwl->rcnd);
		qse_mtx_fini (&rwl->mtx);
		return -1;
	}

	rwl->rwait_count = 0;
	rwl->wwait_count = 0;
	rwl->ractive_count = 0;
	rwl->wactive_count = 0;

	return 0;
}

void qse_rwl_fini (qse_rwl_t* rwl)
{
	qse_cnd_fini (&rwl->wcnd);
	qse_cnd_fini (&rwl->rcnd);
	qse_mtx_fini (&rwl->mtx);
}

qse_mmgr_t* qse_rwl_getmmgr (qse_rwl_t* rwl)
{
	return rwl->mmgr;
}

void* qse_rwl_getxtn (qse_rwl_t* rwl)
{
	return QSE_XTN (rwl);
}

int qse_rwl_lockr (qse_rwl_t* rwl, const qse_ntime_t* waiting_time)
{
	qse_ntime_t dead_line, now, rem, zero;

	if (waiting_time)
	{
		qse_cleartime (&zero);
		qse_gettime (&now);
		qse_addtime (&now, waiting_time, &dead_line);
	}
	if (qse_mtx_lock (&rwl->mtx, waiting_time) <= -1) return -1;

	if (rwl->wactive_count > 0 || ((rwl->flags & QSE_RWL_PREFER_WRITER) && rwl->wwait_count > 0)) 
	{
		rwl->rwait_count++;
		while (rwl->wactive_count > 0 || ((rwl->flags & QSE_RWL_PREFER_WRITER) && rwl->wwait_count > 0)) 
		{
			if (waiting_time)
			{
				qse_gettime (&now);
				qse_subtime (&dead_line, &now, &rem);
				if (qse_cmptime(&rem, &zero) <= 0)
				{
					/* timed out */
					rwl->rwait_count--;
					qse_mtx_unlock (&rwl->mtx);
					return -1;
				}
				qse_cnd_wait (&rwl->rcnd, &rwl->mtx, &rem);
			}
			else
			{
				qse_cnd_wait (&rwl->rcnd, &rwl->mtx, QSE_NULL);
			}
		}
		rwl->rwait_count--;
	}

	rwl->ractive_count++;
	qse_mtx_unlock (&rwl->mtx);

	return 0;
}

int qse_rwl_unlockr (qse_rwl_t* rwl)
{
	if (qse_mtx_lock (&rwl->mtx, QSE_NULL) <= -1) return -1;

	if (rwl->ractive_count <= 0) 
	{
		qse_mtx_unlock (&rwl->mtx);
		return -1;
	}

	rwl->ractive_count--;
	if (rwl->ractive_count == 0 && rwl->wwait_count > 0) 
	{
		qse_cnd_signal (&rwl->wcnd);
	}

	qse_mtx_unlock (&rwl->mtx);
	return 0;
}

int qse_rwl_lockw (qse_rwl_t* rwl, const qse_ntime_t* waiting_time)
{
	qse_ntime_t dead_line, now, rem, zero;

	if (waiting_time)
	{
		qse_cleartime (&zero);
		qse_gettime (&now);
		qse_addtime (&now, waiting_time, &dead_line);
	}
	if (qse_mtx_lock (&rwl->mtx, waiting_time) <= -1) return -1;

	if (rwl->wactive_count > 0 || rwl->ractive_count > 0) 
	{
		rwl->wwait_count++;
		while (rwl->wactive_count > 0 || rwl->ractive_count > 0) 
		{
			if (waiting_time)
			{
				qse_gettime (&now);
				qse_subtime (&dead_line, &now, &rem);
				if (qse_cmptime(&rem, &zero) <= 0)
				{
					/* timed out */
					rwl->wwait_count--;
					qse_mtx_unlock (&rwl->mtx);
					return -1;
				}
				qse_cnd_wait (&rwl->wcnd, &rwl->mtx, &rem);
			}
			else 
			{
				qse_cnd_wait (&rwl->wcnd, &rwl->mtx, QSE_NULL);
			}
		}
		rwl->wwait_count--;
	}

	rwl->wactive_count++;
	qse_mtx_unlock (&rwl->mtx);
	return 0;
}

int qse_rwl_unlockw (qse_rwl_t* rwl)
{
	if (qse_mtx_lock (&rwl->mtx, QSE_NULL) <= -1) return -1;


	if (rwl->wactive_count <= 0) 
	{
		qse_mtx_unlock (&rwl->mtx);
		return -1;
	}

	rwl->wactive_count--;

#if 0
	if (rwl->flags & QSE_RWL_PREFER_WRITER)
	{
		if (rwl->wwait_count > 0) 
		{
			qse_cnd_signal (&rwl->wcnd);
		}
		else if (rwl->rwait_count > 0) 
		{
			qse_cnd_broadcast (&rwl->rcnd);
		}
	}
	else
	{
#endif
		if (rwl->rwait_count > 0) 
		{
			qse_cnd_broadcast (&rwl->rcnd);
		}
		/*else*/ if (rwl->wwait_count > 0) 
		{
			qse_cnd_signal (&rwl->wcnd);
		}
#if 0
	}
#endif
	qse_mtx_unlock (&rwl->mtx);
	return 0;
}
