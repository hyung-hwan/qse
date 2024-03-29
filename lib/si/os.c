
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


#include <qse/si/os.h>

#if defined(HAVE_TIME_H)
#	include <time.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#	include <sys/time.h>
#endif
#if defined(HAVE_SCHED_H)
#	include <sched.h>
#endif

void qse_sleep (const qse_ntime_t* interval)
{
#if defined(_WIN32)
	DWORD milli = QSE_SECNSEC_TO_MSEC(interval->sec, interval->nsec);
	Sleep (milli);
#elif defined(__OS2__)
	ULONG milli = QSE_SECNSEC_TO_MSEC(interval->sec, interval->nsec);
	DosSleep (milli);
#elif defined(HAVE_NANOSLEEP)
	struct timespec ts;
	ts.tv_sec = interval->sec;
	ts.tv_nsec = interval->nsec;
	nanosleep (&ts, &ts);
#else
	sleep (interval->sec);
#endif
}

void qse_msleep (qse_mtime_t interval)
{
#if defined(_WIN32)
	Sleep (interval);
#elif defined(__OS2__)
	DosSleep (interval);
#elif defined(HAVE_NANOSLEEP)
	struct timespec ts;
	ts.tv_sec = QSE_MSEC_TO_SEC(interval);
	interval -= QSE_SEC_TO_MSEC(ts.tv_sec);
	ts.tv_nsec = QSE_MSEC_TO_NSEC(interval);
	nanosleep (&ts, &ts);
#else
	sleep (QSE_MSEC_TO_SEC(interval));
#endif
}

/*
 TODO:
int qse_set_proc_name (const qse_char_t* name)
{
	::prctl(PR_SET_NAME, name, 0, 0, 0);
}*/


void qse_sched_yield (void)
{
#if defined(_WIN32)
	SwitchToThread ();
	/* or Sleep (0) */
#elif defined(__OS2__)
	DosSleep (0);
#elif defined(HAVE_SCHED_YIELD)
	sched_yield();
#elif defined(HAVE_NANOSLEEP)
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	nanosleep (&ts, &ts);
#else
	sleep (0);
#endif
}
