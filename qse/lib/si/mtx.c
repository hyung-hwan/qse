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

#include <qse/si/mtx.h>
#include "../cmn/mem-prv.h"

#if (!defined(__unix__) && !defined(__unix)) || defined(HAVE_PTHREAD)

#if defined(_WIN32)
#	include <windows.h>
#	include <process.h>

#elif defined(__OS2__)
#	define INCL_DOSSEMAPHORES
#	define INCL_DOSERRORS
#	include <os2.h>

#elif defined(__DOS__)
	/* implement this */

#elif defined(__BEOS__)
#	include <be/kernel/OS.h>

#else
#	if defined(AIX) && defined(__GNUC__)
		typedef int crid_t;
		typedef unsigned int class_id_t;
#	endif
#	include <pthread.h>
#endif

qse_mtx_t* qse_mtx_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_mtx_t* mtx;

	mtx = (qse_mtx_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_mtx_t) + xtnsize);
	if (mtx)
	{
		if (qse_mtx_init (mtx, mmgr) <= -1)
		{
			QSE_MMGR_FREE (mmgr, mtx);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(mtx), 0, xtnsize);
	}

	return mtx;
}

void qse_mtx_close (qse_mtx_t* mtx)
{
	qse_mtx_fini (mtx);
	QSE_MMGR_FREE (mtx->mmgr, mtx);
}

int qse_mtx_init (qse_mtx_t* mtx, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (mtx, 0, QSE_SIZEOF(*mtx));
	mtx->mmgr = mmgr;

#if defined(_WIN32)
	mtx->hnd = CreateMutex (QSE_NULL, FALSE, QSE_NULL);
	if (mtx->hnd == QSE_NULL) return -1;

#elif defined(__OS2__)

	{
		APIRET rc;
		HMTX m;

		rc = DosCreateMutexSem (QSE_NULL, &m, DC_SEM_SHARED, FALSE);
		if (rc != NO_ERROR) return -1;

		mtx->hnd = m;
	}

#elif defined(__DOS__)
#	error not implemented

#elif defined(__BEOS__)
	mtx->hnd = create_sem (1, QSE_NULL);
	if (mtx->hnd < B_OK)  return -1;

#else
	/*
	qse_ensure (pthread_mutexattr_init (&attr) == 0);
	if (pthread_mutexattr_settype (&attr, type) != 0) 
	{
		int num = qse_geterrno();
		pthread_mutexattr_destroy (&attr);
		if (mtx->__dynamic) qse_free (mtx);
		qse_seterrno (num);
		return QSE_NULL;
	}
	qse_ensure (pthread_mutex_init (&mtx->hnd, &attr) == 0);
	qse_ensure (pthread_mutexattr_destroy (&attr) == 0);
	*/
	if (pthread_mutex_init ((pthread_mutex_t*)&mtx->hnd, QSE_NULL) != 0) return -1;
#endif

	return 0;
}

void qse_mtx_fini (qse_mtx_t* mtx)
{
#if defined(_WIN32)
	CloseHandle (mtx->hnd);

#elif defined(__OS2__)
	DosCloseMutexSem (mtx->hnd);

#elif defined(__DOS__)
#	error not implemented

#elif defined(__BEOS__)
	/*if (delete_sem(mtx->hnd) != B_NO_ERROR) return -1;*/
	delete_sem(mtx->hnd);

#else
	pthread_mutex_destroy((pthread_mutex_t*)&mtx->hnd);
#endif
}

qse_mmgr_t* qse_mtx_getmmgr (qse_mtx_t* mtx)
{
	return mtx->mmgr;
}

void* qse_mtx_getxtn (qse_mtx_t* mtx)
{
	return QSE_XTN (mtx);
}

int qse_mtx_lock (qse_mtx_t* mtx, const qse_ntime_t* waiting_time)
{
#if defined(_WIN32)
	/* 
	 * MSDN
	 *   WAIT_ABANDONED The specified object is a mutex object that was 
	 *                  not released by the thread that owned the mutex 
	 *                  object before the owning thread terminated. 
	 *                  Ownership of the mutex object is granted to the 
	 *                  calling thread, and the mutex is set to nonsignaled.
	 *   WAIT_OBJECT_0  The state of the specified object is signaled. 
	 *   WAIT_TIMEOUT   The time-out interval elapsed, and the object's 
	 *                  state is nonsignaled. 
	 *   WAIT_FAILED    An error occurred
	 */
	if (waiting_time)
	{
		DWORD msec;
		msec = QSE_SECNSEC_TO_MSEC (waiting_time->sec, waiting_time->nsec);
		if (WaitForSingleObject(mtx->hnd, msec) != WAIT_OBJECT_0)  return -1;
	}
	else
	{
		if (WaitForSingleObject (mtx->hnd, INFINITE) == WAIT_FAILED) return -1;
	}

#elif defined(__OS2__)
	if (waiting_time)
	{
		ULONG msec;
		msec = QSE_SECNSEC_TO_MSEC (waiting_time->sec, waiting_time->nsec);
		if (DosRequestMutexSem (mtx->hnd, msec) != NO_ERROR) return -1;
	}
	else
	{
		if (DosRequestMutexSem (mtx->hnd, SEM_INDEFINITE_WAIT) != NO_ERROR) return -1;
	}

#elif defined(__DOS__)

	/* nothing to do */

#elif defined(__BEOS__)
	if (waiting_time)
	{
		/* TODO: check for B_WOULD_BLOCK */
		/*if (acquire_sem_etc(mtx->hnd, 1, B_ABSOLUTE_TIMEOUT, 0) != B_NO_ERROR) return -1;*/
		bigtime_t usec;

		usec = QSE_SECNSEC_TO_USEC (waiting_time->sec, waiting_time->nsec);
		if (acquire_sem_etc(mtx->hnd, 1, B_TIMEOUT, 0) != B_NO_ERROR) return -1;
	}
	else
	{
		if (acquire_sem(mtx->hnd) != B_NO_ERROR) return -1;
	}

#else

	/* if pthread_mutex_timedlock() isn't available, don't honor the waiting time. */
	#if defined(HAVE_PTHREAD_MUTEX_TIMEDLOCK)
	if (waiting_time)
	{
		qse_ntime_t t;
		struct timespec ts;

		qse_gettime (&t);
		qse_addtime (&t, waiting_time, &t);

		ts.tv_sec = t.sec;
		ts.tv_nsec = t.nsec;
		if (pthread_mutex_timedlock ((pthread_mutex_t*)&mtx->hnd, &ts) != 0) return -1;
	}
	else
	{
	#endif
		if (pthread_mutex_lock ((pthread_mutex_t*)&mtx->hnd) != 0) return -1;

	#if defined(HAVE_PTHREAD_MUTEX_TIMEDLOCK)
	}
	#endif
#endif

	return 0;
}

int qse_mtx_unlock (qse_mtx_t* mtx)
{
#if defined(_WIN32)
	if (ReleaseMutex (mtx->hnd) == FALSE) return -1;

#elif defined(__OS2__)
	if (DosReleaseMutexSem (mtx->hnd) != NO_ERROR) return -1;

#elif defined(__DOS__)

	/* nothing to do */

#elif defined(__BEOS__)
	if (release_sem(mtx->hnd) != B_NO_ERROR) return -1;

#else
	if (pthread_mutex_unlock ((pthread_mutex_t*)&mtx->hnd) != 0) return -1;
#endif
	return 0;
}

#endif
