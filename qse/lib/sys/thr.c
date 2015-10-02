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


#include "thr.h"
#include "../cmn/mem.h"
#include <qse/cmn/time.h>
#include <stdarg.h>

#if (!defined(__unix__) && !defined(__unix)) || defined(HAVE_PTHREAD)

qse_thr_t* qse_thr_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_thr_routine_t routine)
{
	qse_thr_t* thr;

	thr = (qse_thr_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_thr_t) + xtnsize);
	if (thr)
	{
		if (qse_thr_init (thr, mmgr, routine) <= -1)
		{
			QSE_MMGR_FREE (mmgr, thr);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(thr), 0, xtnsize);
	}

	return thr;
}

void qse_thr_close (qse_thr_t* thr)
{
	qse_thr_fini (thr);
	QSE_MMGR_FREE (thr->mmgr, thr);
}

int qse_thr_init (qse_thr_t* thr, qse_mmgr_t* mmgr, qse_thr_routine_t routine)
{
	QSE_MEMSET (thr, 0, QSE_SIZEOF(*thr));

	thr->mmgr = mmgr;
	thr->__handle = QSE_THR_HND_INVALID;
	thr->__state = QSE_THR_INCUBATING;
	thr->__return_code = 0;
	thr->__main_routine = routine;
	thr->__joinable = 1;
	thr->__stacksize = 0;

	return 0;
}

void qse_thr_fini (qse_thr_t* thr)
{
#if defined(_WIN32)
	if (thr->__handle != QSE_THR_HND_INVALID) CloseHandle (thr->__handle);
#endif
	thr->__handle = QSE_THR_HND_INVALID;
}

qse_mmgr_t* qse_thr_getmmgr (qse_thr_t* thr)
{
	return thr->mmgr;
}

void* qse_thr_getxtn (qse_thr_t* thr)
{
	return QSE_XTN (thr);
}

qse_size_t qse_thr_getstacksize (qse_thr_t* thr)
{
	return thr->__stacksize;
}

void qse_thr_setstacksize (qse_thr_t* thr, qse_size_t num)
{
	thr->__stacksize = num;
}

#if defined(__OS2__)
static void __thread_main (void* arg)
#elif defined(__BEOS__)
static int32 __thread_main (void* arg)
#else
static void* __thread_main (void* arg)
#endif
{
	qse_thr_t* thr = (qse_thr_t*)arg;

	while (thr->__state != QSE_THR_RUNNING) 
	{
#if defined(_WIN32)
		Sleep (0);
#elif defined(__OS2__)
		DosSleep (0);
#elif defined(HAVE_NANOSLEEP)
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
		nanosleep (&ts, &ts);
#else
		sleep (0);
#endif
	}

#if defined(HAVE_PTHREAD)
	/* 
	 * the asynchronous cancel-type is used to better emulate
	 * the bad effect of WIN32's TerminateThread using pthread_cancel 
	 */
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, QSE_NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, QSE_NULL);
#endif

	thr->__return_code = thr->__temp_routine? thr->__temp_routine(thr): thr->__main_routine(thr);
	thr->__state = QSE_THR_TERMINATED;

#if defined(_WIN32)
	_endthreadex (thr->__return_code);
	return QSE_NULL;

#elif defined(__OS2__)
	_endthread ();
	/* no return statement */

#elif defined(__DOS__)
	/* not implemented */
	return QSE_NULL;

#elif defined(__BEOS__)
	exit_thread (thr->__return_code);
	return 0;

#else
	pthread_exit (&thr->__return_code);
	return QSE_NULL;
#endif

}

static int __create_thread (qse_thr_t* thr)
{
#if defined(_WIN32)
	unsigned int tid;

	if (thr->__handle != QSE_THR_HND_INVALID) CloseHandle (thr->__handle);

	thr->__handle = (HANDLE)_beginthreadex (QSE_NULL, 0, (unsigned int (__stdcall*)(void*))__thread_main, thr, 0, &tid);
	if (thr->__handle == 0) return -1;

#elif defined(__OS2__)
	TID tid;

	/* default stack size to 81920(4096 * 20) */
	tid = _beginthread (__thread_main, NULL, (thr->__stacksize > 0? thr->__stacksize: 81920), thr);
	if (tid == -1) return -1;

	thr->__handle = tid;

#elif defined(__DOS__)
	/* not implemented */

#elif defined(__BEOS__)
	thread_id tid;

	tid = spawn_thread ((thread_func)__thread_main, QSE_NULL, 120, thr);
	if (tid < B_OK) return -1;

	thr->__handle = tid;
	resume_thread(thr->__handle);

#elif defined(HAVE_PTHREAD)
	pthread_attr_t attr;
	pthread_attr_init (&attr);

	if (pthread_attr_setdetachstate (&attr, (thr->__joinable? 
		PTHREAD_CREATE_JOINABLE: PTHREAD_CREATE_DETACHED)) != 0) 
	{
		pthread_attr_destroy (&attr);
		return -1;
	}

	if (thr->__stacksize > 0)
	{
		if (pthread_attr_setstacksize (&attr, thr->__stacksize) != 0)
		{
			pthread_attr_destroy (&attr);
			return -1;
		}
	}

	if (pthread_create (&thr->__handle, &attr, __thread_main, thr) != 0) 
	{
		pthread_attr_destroy (&attr);
		return -1;
	}

	pthread_attr_destroy (&attr);
#endif
	return 0;
}

static int __cancel_thread (qse_thr_t* thr)
{
	if (thr->__state != QSE_THR_RUNNING) return -1;
#if defined(_WIN32)
	if (TerminateThread (thr->__handle, 0) == 0) return -1;
#elif defined(__OS2__)
	if (DosKillThread (thr->__handle) != NO_ERROR) return -1;
#elif defined(__DOS__)
	/* not implemented */
#elif defined(__BEOS__)
	if (kill_thread (thr->__handle) < B_OK) return -1;
#elif defined(HAVE_PTHREAD)
	if (pthread_cancel (thr->__handle) != 0) return -1;
#endif
	return 0;
}

int qse_thr_start (qse_thr_t* thr, int flags, ...)
{
	if (thr->__state == QSE_THR_RUNNING) return -1;

	thr->__joinable = ((flags & QSE_THR_DETACHED) == 0);
	if (flags & QSE_THR_NEW_ROUTINE) 
	{
		va_list va;
		va_start (va, flags);
		thr->__temp_routine = va_arg (va, qse_thr_routine_t);
		va_end (va);
	}
	else thr->__temp_routine = QSE_NULL;

	if (__create_thread(thr) == -1) 
	{
		thr->__state = QSE_THR_INCUBATING;
		thr->__handle = QSE_THR_HND_INVALID;
		return -1;
	}

	thr->__state = QSE_THR_RUNNING;
	return 0;
}

int qse_thr_stop (qse_thr_t* thr)
{
	if (thr->__state == QSE_THR_RUNNING) 
	{
		if (__cancel_thread(thr) == -1) return -1;
		/* can't be sure of whether or not the thread is really terminated. */
 		thr->__state = QSE_THR_ABORTED;
		return 0;
	}

	return -1;
}

int qse_thr_join (qse_thr_t* thr)
{
	if (thr->__state == QSE_THR_INCUBATING) return -1;
	if (!thr->__joinable) return -1;

#if defined(_WIN32)
	if (thr->__state == QSE_THR_RUNNING) 
	{
		if (WaitForSingleObject (thr->__handle, INFINITE) == WAIT_FAILED) return -1;
	}

#elif defined(__OS2__)
	if (DosWaitThread (&thr->__handle, DCWW_WAIT) != NO_ERROR) return -1;

#elif defined(__DOS__)
	/* not implemented */

#elif defined(__BEOS__)
	if (wait_for_thread(thr->__handle, QSE_NULL) < B_OK) return -1;

#elif defined(HAVE_PTHREAD)
	if (pthread_join(thr->__handle, QSE_NULL) != 0) return -1;
#endif

	thr->__joinable = 0;
	return 0;
}

int qse_thr_detach (qse_thr_t* thr)
{
	if (thr->__state == QSE_THR_INCUBATING) return -1;
	if (!thr->__joinable) return -1;

#if defined(HAVE_PTHREAD)
	if (pthread_detach(thr->__handle) != 0) return -1;
#endif

	thr->__joinable = 0;
	return 0;
}

int qse_thr_kill (qse_thr_t* thr, int sig)
{
	/* this function is to send a signal to a thread.
	 * don't get confused by the name */
	if (thr->__state != QSE_THR_RUNNING) return -1;

#if defined(HAVE_PTHREAD)
	if (pthread_kill (thr->__handle, sig) != 0) return -1;
#endif
	return 0;
}

int qse_thr_blocksig (qse_thr_t* thr, int sig)
{
#if defined(HAVE_PTHREAD)
	sigset_t mask;
#endif

	if (thr->__state != QSE_THR_RUNNING) return -1;

#if defined(HAVE_PTHREAD)
	sigemptyset (&mask);
	sigaddset (&mask, sig);
	if (pthread_sigmask (SIG_BLOCK, &mask, QSE_NULL) != 0) return -1;
#endif
	return 0;
}

int qse_thr_unblocksig (qse_thr_t* thr, int sig)
{
#if defined(HAVE_PTHREAD)
	sigset_t mask;
#endif

	if (thr->__state != QSE_THR_RUNNING) return -1;

#if defined(HAVE_PTHREAD)
	sigemptyset (&mask);
	sigaddset (&mask, sig);
	if (pthread_sigmask (SIG_UNBLOCK, &mask, QSE_NULL) != 0) return -1;
#endif
	return 0;
}

int qse_thr_blockallsigs (qse_thr_t* thr)
{
#if defined(HAVE_PTHREAD)
	sigset_t mask;
#endif

	if (thr->__state != QSE_THR_RUNNING) return -1;

#if defined(HAVE_PTHREAD)
	sigfillset (&mask);
	if (pthread_sigmask (SIG_BLOCK, &mask, QSE_NULL) != 0) return -1;
#endif
	return 0;
}

int qse_thr_unblockallsigs (qse_thr_t* thr)
{
#if defined(HAVE_PTHREAD)
	sigset_t mask;
#endif

	if (thr->__state != QSE_THR_RUNNING) return -1;

#if defined(HAVE_PTHREAD)
	sigfillset (&mask);
	if (pthread_sigmask (SIG_UNBLOCK, &mask, QSE_NULL) != 0) return -1;
#endif
	return 0;
}

qse_thr_hnd_t qse_thr_gethnd (qse_thr_t* thr)
{
	return thr->__handle;
}

int qse_thr_getretcode (qse_thr_t* thr)
{
	return thr->__return_code;
}

qse_thr_state_t qse_thr_getstate (qse_thr_t* thr)
{
	return thr->__state;
}

qse_thr_hnd_t qse_getcurthrhnd (void)
{
#if defined(_WIN32)
	return GetCurrentThread ();
#elif defined(__OS2__)
	PTIB ptib;
	PPIB ppib;

	if (DosGetInfoBlocks (&ptib, &ppib) != NO_ERROR) return QSE_THR_HND_INVALID;
	return ptib->tib_ptib2->tib2_ultid;

#elif defined(__DOS__)
	return QSE_THR_HND_INVALID; /* TODO: implement this */
#elif defined(__BEOS__)
	return QSE_THR_HND_INVALID; /* TODO: implement this */
#else
	return pthread_self ();
#endif
}

#endif
