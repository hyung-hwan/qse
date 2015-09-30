/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following cnditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of cnditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of cnditions and the following disclaimer in the
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

#include <qse/sys/cnd.h>
#include "../cmn/mem.h"

#if (!defined(__unix__) && !defined(__unix)) || defined(HAVE_PTHREAD)

#if defined(_WIN32)
	#include <windows.h>
	#include <process.h>
#elif defined(__OS2__)
	/* implement this */
#elif defined(__DOS__)
	/* implement this */
#else
	#if defined(AIX) && defined(__GNUC__)
		typedef int crid_t;
		typedef unsigned int class_id_t;
	#endif
	#include <pthread.h>
#endif

qse_cnd_t* qse_cnd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_cnd_t* cnd;

	cnd = (qse_cnd_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_cnd_t) + xtnsize);
	if (cnd)
	{
		if (qse_cnd_init (cnd, mmgr) <= -1)
		{
			QSE_MMGR_FREE (mmgr, cnd);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(cnd), 0, xtnsize);
	}

	return cnd;
}

void qse_cnd_close (qse_cnd_t* cnd)
{
	qse_cnd_fini (cnd);
	QSE_MMGR_FREE (cnd->mmgr, cnd);
}


int qse_cnd_init (qse_cnd_t* cnd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (cnd, 0, QSE_SIZEOF(*cnd));
	cnd->mmgr = mmgr;

#if defined(_WIN32)
	cnd->gone    = 0;
	cnd->blocked = 0;
	cnd->waiting = 0;

	cnd->gate = CreateSemaphore (0, 1, 1, 0);
	cnd->queue = CreateSemaphore (0, 0, QSE_TYPE_MAX(long), 0);
	cnd->mutex = CreateMutex (0, 0, 0);

	if (cnd->gate == QSE_NULL || 
	    cnd->queue == QSE_NULL || 
	    cnd->mutex == QSE_NULL) 
	{
		if (cnd->gate) CloseHandle (cnd->gate);
		if (cnd->queue) CloseHandle (cnd->queue);
		if (cnd->mutex) CloseHandle (cnd->mutex);

		return -1;
	}
#elif defined(__OS2__)
#	error not implemented
#elif defined(__DOS__)
#	error not implemented
#else
	if (pthread_cond_init ((pthread_cond_t*)&cnd->hnd, QSE_NULL) != 0)  return -1;
#endif

	return 0;
}

void qse_cnd_fini (qse_cnd_t* cnd)
{
#if defined(_WIN32)
	CloseHandle (cnd->gate);
	CloseHandle (cnd->queue);
	CloseHandle (cnd->mutex);
#elif defined(__OS2__)
#	error not implemented
#elif defined(__DOS__)
#	error not implemented
#else
	pthread_cond_destroy ((pthread_cond_t*)&cnd->hnd);
#endif
}

void qse_cnd_signal (qse_cnd_t* cnd)
{
#if defined(_WIN32)
	unsigned int signals = 0;

	WaitForSingleObject ((HANDLE)cnd->mutex, INFINITE);
	if (cnd->waiting != 0)
	{
		if (cnd->blocked == 0) 
		{
			ReleaseMutex ((HANDLE)cnd->mutex);
			return;
		}

		++cnd->waiting;
		--cnd->blocked;
		signals = 1;
	}
	else
	{
		WaitForSingleObject ((HANDLE)cnd->gate, INFINITE);
		if (cnd->blocked > cnd->gone) 
		{
			if (cnd->gone != 0)
			{
				cnd->blocked -= cnd->gone;
				cnd->gone = 0;
			}
			signals = cnd->waiting = 1;
			--cnd->blocked;
		}
		else 
		{
			ReleaseSemaphore ((HANDLE)cnd->gate, 1, QSE_NULL);
		}
	}

	ReleaseMutex ((HANDLE)cnd->mutex);
	if (signals) ReleaseSemaphore ((HANDLE)cnd->queue, signals, QSE_NULL);
#else
	pthread_cond_signal ((pthread_cond_t*)&cnd->hnd);
#endif
}

void qse_cnd_broadcast (qse_cnd_t* cnd)
{
#if defined(_WIN32)
	unsigned int signals = 0;

	WaitForSingleObject ((HANDLE)cnd->mutex, INFINITE);

	if (cnd->waiting != 0) 
	{
		if (cnd->blocked == 0)
		{
			ReleaseMutex ((HANDLE)cnd->mutex);
			return;
		}

		cnd->waiting += (signals = cnd->blocked);
		cnd->blocked = 0;
	}
	else
	{
		WaitForSingleObject ((HANDLE)cnd->gate, INFINITE);
		if (cnd->blocked > cnd->gone)
		{
			if (cnd->gone != 0)
			{
				cnd->blocked -= cnd->gone;
				cnd->gone = 0;
			}
			signals = cnd->waiting = cnd->blocked;
			cnd->blocked = 0;
		}
		else
		{
			ReleaseSemaphore ((HANDLE)cnd->gate, 1, QSE_NULL);
		}
	}

	ReleaseMutex ((HANDLE)cnd->mutex);
	if (signals) ReleaseSemaphore ((HANDLE)cnd->queue, signals, QSE_NULL);
#else
	pthread_cond_broadcast ((pthread_cond_t*)&cnd->hnd);
#endif
}

void qse_cnd_wait (qse_cnd_t* cnd, qse_mtx_t* mutex, qse_ntime_t* waiting_time)
{
#if defined(_WIN32)
	unsigned int was_waiting, was_gone;
	int signaled;

	WaitForSingleObject ((HANDLE)cnd->gate, INFINITE);
	++cnd->blocked;
	ReleaseSemaphore ((HANDLE)cnd->gate, 1, QSE_NULL);

	qse_mtx_unlock (mutex);

	if (waiting_time)
	{
		DWORD msec;

		msec = QSE_SECNSEC_TO_MSEC (waiting_time->sec, waiting_time->nsec);
		signaled = (WaitForSingleObject((HANDLE)cnd->queue, msec) == WAIT_OBJECT_0);
	}
	else
	{
		WaitForSingleObject ((HANDLE)cnd->queue, INFINITE);
		signaled = 1;
	}

	was_waiting = 0; 
	was_gone = 0;

	WaitForSingleObject ((HANDLE)cnd->mutex, INFINITE);

	was_waiting = cnd->waiting;
	was_gone = cnd->gone;

	if (was_waiting != 0) 
	{
		if (!signaled) 
		{ 
			/* timed out */
			if (cnd->blocked != 0) --cnd->blocked;
			else ++cnd->gone;
		}

		if (--cnd->waiting == 0) 
		{
			if (cnd->blocked != 0) 
			{
				ReleaseSemaphore ((HANDLE)cnd->gate, 1, QSE_NULL);
				was_waiting = 0;
			}
			else if (cnd->gone != 0)
			{
				cnd->gone = 0;
			}
		}
	}
	else if (++cnd->gone == QSE_TYPE_MAX(unsigned int) / 2) 
	{
		WaitForSingleObject ((HANDLE)cnd->gate, INFINITE);
		cnd->blocked -= cnd->gone;
		ReleaseSemaphore ((HANDLE)cnd->gate, 1, QSE_NULL);
		cnd->gone = 0;
	}

	ReleaseMutex ((HANDLE)cnd->mutex);

	if (was_waiting == 1) 
	{
		for (;was_gone; --was_gone) 
		{
			WaitForSingleObject ((HANDLE)cnd->queue, INFINITE);
		}
		ReleaseSemaphore ((HANDLE)cnd->gate, 1, QSE_NULL);
	}

	qse_mtx_lock (mutex, QSE_NULL);
#else
	if (waiting_time)
	{
		qse_ntime_t t;
		struct timespec ts;

		qse_gettime (&t);
		qse_addtime (&t, waiting_time, &t);

		ts.tv_sec = t.sec;
		ts.tv_nsec = t.nsec;

		pthread_cond_timedwait ((pthread_cond_t*)&cnd->hnd, (pthread_mutex_t*)&mutex->hnd, &ts);
	}
	else
	{
		/* no waiting */
		pthread_cond_wait ((pthread_cond_t*)&cnd->hnd, (pthread_mutex_t*)&mutex->hnd);
	}
#endif
}

#endif
