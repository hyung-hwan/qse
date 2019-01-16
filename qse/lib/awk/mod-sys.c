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

#include "mod-sys.h"
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/time.h>
#include <qse/cmn/mbwc.h>
#include <qse/si/nwad.h>
#include <qse/si/nwif.h>
#include "../cmn/mem-prv.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <process.h>
#	include <tchar.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include "../cmn/syscall.h"
#	if defined(HAVE_SYS_SYSCALL_H)
#		include <sys/syscall.h>
#	endif

#	define ENABLE_SYSLOG
#	include <syslog.h>
#endif

#include <stdlib.h> /* getenv, system */
#include <time.h>

struct mod_ctx_t
{
	char* log_ident;
};
typedef struct mod_ctx_t mod_ctx_t;

static int fnc_fork (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	pid = fork ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wait (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;
	int rx;

/* TODO: handle more parameters */

	rx = qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 0), &pid);
	if (rx >= 0)
	{
#if defined(_WIN32)
		/* TOOD: implement this*/
		rx = -1;
#elif defined(__OS2__)
		/* TOOD: implement this*/
		rx = -1;
#elif defined(__DOS__)
		/* TOOD: implement this*/
		rx = -1;
#else
		rx = waitpid (pid, QSE_NULL, 0);
#endif
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_kill (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid, sig;
	qse_awk_val_t* retv;
	int rx;

	if (qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 0), &pid) <= -1 ||
	    qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 1), &sig) <= -1)
	{
		rx = -1;
	}
	else
	{
#if defined(_WIN32)
		/* TOOD: implement this*/
		rx = -1;
#elif defined(__OS2__)
		/* TOOD: implement this*/
		rx = -1;
#elif defined(__DOS__)
		/* TOOD: implement this*/
		rx = -1;
#else
		rx = kill (pid, sig);
#endif
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getpgid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	/* TODO: support specifing calling process id other than 0 */
	#if defined(HAVE_GETPGID)
	pid = getpgid (0);
	#elif defined(HAVE_GETPGRP)
	pid = getpgrp ();
	#else
	pid = -1;
	#endif
#endif

	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getpid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	pid = GetCurrentProcessId();
	
#elif defined(__OS2__)
	PTIB tib;
	PPIB pib;

	pid = (DosGetInfoBlocks (&tib, &pib) == NO_ERROR)?
		pib->pib_ulpid: -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	pid = getpid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_gettid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_intptr_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	pid = GetCurrentThreadId();
	
#elif defined(__OS2__)
	PTIB tib;
	PPIB pib;

	pid = (DosGetInfoBlocks (&tib, &pib) == NO_ERROR && tib->tib_ptib2)?
		 tib->tib_ptib2->tib2_ultid: -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	#if defined(SYS_gettid) && defined(QSE_SYSCALL0)
	QSE_SYSCALL0 (pid, SYS_gettid);
	#elif defined(SYS_gettid)
	pid = syscall (SYS_gettid);
	#else
	pid = -1;
	#endif
#endif

	retv = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getppid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	pid = getppid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getuid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t uid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	uid = -1;

#else
	uid = getuid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, uid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getgid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t gid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	gid = -1;

#else
	gid = getgid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, gid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_geteuid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t uid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	uid = -1;

#else
	uid = geteuid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, uid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getegid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t gid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	gid = -1;

#else
	gid = getegid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, gid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_sleep (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t lv;
	qse_awk_flt_t fv;
	qse_awk_val_t* retv;
	int rx;

	rx = qse_awk_rtx_valtonum (rtx, qse_awk_rtx_getarg (rtx, 0), &lv, &fv);
	if (rx == 0)
	{
#if defined(_WIN32)
		Sleep ((DWORD)QSE_SEC_TO_MSEC(lv));
		rx = 0;
#elif defined(__OS2__)
		DosSleep ((ULONG)QSE_SEC_TO_MSEC(lv));
		rx = 0;
#elif defined(__DOS__)
		#if (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
			sleep (lv);	
			rx = 0;
		#else
			rx = sleep (lv);	
		#endif
#elif defined(HAVE_NANOSLEEP)
		struct timespec req;
		req.tv_sec = lv;
		req.tv_nsec = 0;
		rx = nanosleep (&req, QSE_NULL);
#else
		rx = sleep (lv);	
#endif
	}
	else if (rx >= 1)
	{
#if defined(_WIN32)
		Sleep ((DWORD)QSE_SEC_TO_MSEC(fv));
		rx = 0;
#elif defined(__OS2__)
		DosSleep ((ULONG)QSE_SEC_TO_MSEC(fv));
		rx = 0;
#elif defined(__DOS__)
		/* no high-resolution sleep() is available */
		#if (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
			sleep ((qse_awk_int_t)fv);	
			rx = 0;
		#else
			rx = sleep ((qse_awk_int_t)fv);	
		#endif;
#elif defined(HAVE_NANOSLEEP)
		struct timespec req;
		req.tv_sec = (qse_awk_int_t)fv;
		req.tv_nsec = QSE_SEC_TO_NSEC(fv - req.tv_sec);
		rx = nanosleep (&req, QSE_NULL);
#elif defined(HAVE_SELECT)
		struct timeval req;
		req.tv_sec = (qse_awk_int_t)fv;
		req.tv_usec = QSE_SEC_TO_USEC(fv - req.tv_sec);
		rx = select (0, QSE_NULL, QSE_NULL, QSE_NULL, &req);
#else
		/* no high-resolution sleep() is available */
		rx = sleep ((qse_awk_int_t)fv);	
#endif
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_gettime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_ntime_t now;

	if (qse_gettime (&now) <= -1) now.sec = 0;

	retv = qse_awk_rtx_makeintval (rtx, now.sec);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_settime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_ntime_t now;
	qse_awk_int_t tmp;
	int rx;

	now.nsec = 0;

	if (qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 0), &tmp) <= -1) rx = -1;
	else
	{
		now.sec = tmp;
		if (qse_settime (&now) <= -1) rx = -1;
		else rx = 0;
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_mktime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_ntime_t nt;
	qse_size_t nargs;
	qse_awk_val_t* retv;

	nargs = qse_awk_rtx_getnargs (rtx);
	if (nargs >= 1)
	{
		int sign;
		qse_char_t* str, * p, * end;
		qse_size_t len;
		qse_awk_val_t* a0;
		qse_btime_t bt;

		a0 = qse_awk_rtx_getarg (rtx, 0);
		str = qse_awk_rtx_getvalstr (rtx, a0, &len);
		if (str == QSE_NULL) return -1;

		/* the string must be of the format  YYYY MM DD HH MM SS[ DST] */
		p = str;
		end = str + len;
		QSE_MEMSET (&bt, 0, QSE_SIZEOF(bt));

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.year = bt.year * 10 + (*p++ - QSE_T('0'));
		bt.year *= sign;
		bt.year -= 1900;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.mon = bt.mon * 10 + (*p++ - QSE_T('0'));
		bt.mon *= sign;
		bt.mon -= 1;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.mday = bt.mday * 10 + (*p++ - QSE_T('0'));
		bt.mday *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.hour = bt.hour * 10 + (*p++ - QSE_T('0'));
		bt.hour *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.min = bt.min * 10 + (*p++ - QSE_T('0'));
		bt.min *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.sec = bt.sec * 10 + (*p++ - QSE_T('0'));
		bt.sec *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.isdst = bt.isdst * 10 + (*p++ - QSE_T('0'));
		bt.isdst *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		qse_awk_rtx_freevalstr (rtx, a0, str);
		qse_timelocal (&bt, &nt);
	}
	else
	{
		/* get the current time when no argument is given */
		qse_gettime (&nt);
	}

	retv = qse_awk_rtx_makeintval (rtx, nt.sec);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}


static int fnc_strftime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_mchar_t* fmt;
	qse_size_t len;
	qse_awk_val_t* retv;

	fmt = qse_awk_rtx_valtombsdup (rtx, qse_awk_rtx_getarg (rtx, 0), &len);
	if (fmt) 
	{
		qse_ntime_t nt;
		qse_btime_t bt;
		qse_awk_int_t tmpsec;

		nt.nsec = 0;
		if (qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 1), &tmpsec) <= -1) 
		{
			nt.sec = 0;
		}
		else
		{
			nt.sec = tmpsec;
		}

		if (qse_localtime (&nt, &bt) >= 0)
		{
			qse_mchar_t tmpbuf[64], * tmpptr;
			struct tm tm;
			qse_size_t sl;

			QSE_MEMSET (&tm, 0, QSE_SIZEOF(tm));
			tm.tm_year = bt.year;
			tm.tm_mon = bt.mon;
			tm.tm_mday = bt.mday;
			tm.tm_hour = bt.hour;
			tm.tm_min = bt.min;
			tm.tm_sec = bt.sec;
			tm.tm_isdst = bt.isdst;

			sl = strftime (tmpbuf, QSE_COUNTOF(tmpbuf), fmt, &tm);
			if (sl <= 0 || sl >= QSE_COUNTOF(tmpbuf))
			{
				/* buffer too small */
				qse_mchar_t* tmp;
				qse_size_t tmpcapa, i, count = 0;

/*
man strftime >>>

RETURN VALUE
       The strftime() function returns the number of bytes placed in the array s, not including the  terminating  null  byte,  provided  the
       string,  including  the  terminating  null  byte,  fits.  Otherwise, it returns 0, and the contents of the array is undefined.  (This
       behavior applies since at least libc 4.4.4; very old versions of libc, such as libc 4.4.1, would return max  if  the  array  was  too
       small.)

       Note that the return value 0 does not necessarily indicate an error; for example, in many locales %p yields an empty string.
 
--------------------------------------------------------------------------------------
* 
I use 'count' to limit the maximum number of retries when 0 is returned.
*/

				for (i = 0; i < len;)
				{
					if (fmt[i] == QSE_MT('%')) 
					{
						count++; /* the nubmer of % specifier */
						i++;
						if (i < len) i++;
					}
					else i++;
				}

				tmpptr = QSE_NULL;
				tmpcapa = QSE_COUNTOF(tmpbuf);
				if (tmpcapa < len) tmpcapa = len;

				do
				{
					if (count <= 0) 
					{
						if (tmpptr) qse_awk_rtx_freemem (rtx, tmpptr);
						tmpbuf[0] = QSE_MT('\0');
						tmpptr = tmpbuf;
						break;
					}
					count--;

					tmpcapa *= 2;
					tmp = qse_awk_rtx_reallocmem (rtx, tmpptr, tmpcapa * QSE_SIZEOF(*tmpptr));
					if (!tmp) 
					{
						if (tmpptr) qse_awk_rtx_freemem (rtx, tmpptr);
						tmpbuf[0] = QSE_MT('\0');
						tmpptr = tmpbuf;
						break;
					}

					tmpptr = tmp;
					sl = strftime (tmpptr, tmpcapa, fmt, &tm);
				}
				while (sl <= 0 || sl >= tmpcapa);
			}
			else
			{
				tmpptr = tmpbuf;
			}
			qse_awk_rtx_freemem (rtx, fmt);

			retv = qse_awk_rtx_makestrvalwithmbs (rtx, tmpptr);
			if (tmpptr && tmpptr != tmpbuf) qse_awk_rtx_freemem (rtx, tmpptr);
			if (retv == QSE_NULL) return -1;

			qse_awk_rtx_setretval (rtx, retv);
		}
		else
		{
			qse_awk_rtx_freemem (rtx, fmt);
		}
	}
	return 0;
}

static int fnc_getenv (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_mchar_t* var;
	qse_size_t len;
	qse_awk_val_t* retv;

	var = qse_awk_rtx_valtombsdup (
		rtx, qse_awk_rtx_getarg (rtx, 0), &len);
	if (var)
	{
		qse_mchar_t* val;

		val = getenv (var);	
		if (val) 
		{
			retv = qse_awk_rtx_makestrvalwithmbs (rtx, val);
			if (retv == QSE_NULL) return -1;

			qse_awk_rtx_setretval (rtx, retv);
		}

		qse_awk_rtx_freemem (rtx, var);
	}

	return 0;
}

static int fnc_getnwifcfg (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_nwifcfg_t cfg;
	qse_awk_rtx_valtostr_out_t out;
	int ret = -1;

	out.type = QSE_AWK_RTX_VALTOSTR_CPLCPY;
	out.u.cplcpy.ptr = cfg.name;
	out.u.cplcpy.len = QSE_COUNTOF(cfg.name);
	if (qse_awk_rtx_valtostr(rtx, qse_awk_rtx_getarg(rtx, 0), &out) >= 0)
	{
		qse_awk_int_t type;
		int rx;

		rx = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &type);
		if (rx >= 0)
		{
			cfg.type = type;

			if (qse_getnwifcfg(&cfg) >= 0)
			{
				/* make a map value containg configuration */
				qse_awk_int_t index, mtu;
				qse_char_t addr[128];
				qse_char_t mask[128];
				qse_char_t ethw[32];
				qse_awk_val_map_data_t md[7];
				qse_awk_val_t* tmp;

				QSE_MEMSET (md, 0, QSE_SIZEOF(md));

				md[0].key.ptr = QSE_T("index");
				md[0].key.len = 5;
				md[0].type = QSE_AWK_VAL_MAP_DATA_INT;
				index = cfg.index;
				md[0].vptr = &index;

				md[1].key.ptr = QSE_T("mtu");
				md[1].key.len = 3;
				md[1].type = QSE_AWK_VAL_MAP_DATA_INT;
				mtu = cfg.mtu;
				md[1].vptr = &mtu;
		
				md[2].key.ptr = QSE_T("addr");
				md[2].key.len = 4;
				md[2].type = QSE_AWK_VAL_MAP_DATA_STR;
				qse_nwadtostr (&cfg.addr, addr, QSE_COUNTOF(addr), QSE_NWADTOSTR_ADDR);
				md[2].vptr = addr;

				md[3].key.ptr = QSE_T("mask");
				md[3].key.len = 4;
				md[3].type = QSE_AWK_VAL_MAP_DATA_STR;
				qse_nwadtostr (&cfg.mask, mask, QSE_COUNTOF(mask), QSE_NWADTOSTR_ADDR);
				md[3].vptr = mask;

				md[4].key.ptr = QSE_T("ethw");
				md[4].key.len = 4;
				md[4].type = QSE_AWK_VAL_MAP_DATA_STR;
				qse_strxfmt (ethw, QSE_COUNTOF(ethw), QSE_T("%02X:%02X:%02X:%02X:%02X:%02X"), 
					cfg.ethw[0], cfg.ethw[1], cfg.ethw[2], cfg.ethw[3], cfg.ethw[4], cfg.ethw[5]);
				md[4].vptr = ethw;

				if (cfg.flags & (QSE_NWIFCFG_LINKUP | QSE_NWIFCFG_LINKDOWN))
				{
					md[5].key.ptr = QSE_T("link");
					md[5].key.len = 4;
					md[5].type = QSE_AWK_VAL_MAP_DATA_STR;
					md[5].vptr = (cfg.flags & QSE_NWIFCFG_LINKUP)? QSE_T("up"): QSE_T("down");
				}

				tmp = qse_awk_rtx_makemapvalwithdata (rtx, md);
				if (tmp)
				{
					int x;
					qse_awk_rtx_refupval (rtx, tmp);
					x = qse_awk_rtx_setrefval (rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg (rtx, 2), tmp);
					qse_awk_rtx_refdownval (rtx, tmp);
					if (x <= -1) return -1;
					ret = 0;
				}
			}
		}
	}

	/* no error check for qse_awk_rtx_makeintval() since ret is 0 or -1 */
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval (rtx, ret));
	return 0;
}

static int fnc_system (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* v, * a0;
	qse_char_t* str;
	qse_size_t len;
	int n = 0;

	a0 = qse_awk_rtx_getarg (rtx, 0);
	str = qse_awk_rtx_getvalstr (rtx, a0, &len);
	if (str == QSE_NULL) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr (str, len, QSE_T('\0')))
	{
		n = -1;
		goto skip_system;
	}

#if defined(_WIN32)
	n = _tsystem (str);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = system (str);
#else

	{
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdup (str, QSE_NULL, qse_awk_rtx_getmmgr(rtx));
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_system;
		}
		n = system (mbs);
		qse_awk_rtx_freemem (rtx, mbs);
	}

#endif

skip_system:
	qse_awk_rtx_freevalstr (rtx, a0, str);

	v = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

static int fnc_openlog (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx = -1;
	qse_awk_int_t opt, fac;
	qse_awk_val_t* retv;
	qse_char_t* ident = QSE_NULL;
	qse_size_t ident_len;
	mod_ctx_t* mctx = fi->mod->ctx;

#if defined(ENABLE_SYSLOG)
	ident = qse_awk_rtx_getvalstr(rtx, qse_awk_rtx_getarg(rtx, 0), &ident_len);
	if (!ident) goto done;

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr(ident, ident_len, QSE_T('\0'))) goto done;

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &opt) <= -1) goto done;
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 2), &fac) <= -1) goto done;

	#if defined(QSE_CHAR_IS_MCHAR)
	openlog(ident, opt, fac);
	#else
	{

		qse_mchar_t* mbs;
		mbs = qse_wcstombsdup(ident, QSE_NULL, qse_awk_rtx_getmmgr(rtx));
		if (!mbs) goto done;

		if (mctx->log_ident) qse_awk_rtx_freemem (rtx, mctx->log_ident);
		mctx->log_ident = mbs;

		openlog (mbs, opt, fac);
	}
	#endif

	rx = 0;
#endif

done:
	if (ident) qse_awk_rtx_freevalstr(rtx, qse_awk_rtx_getarg(rtx, 0), ident);

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_closelog (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx = -1;
	qse_awk_val_t* retv;
	mod_ctx_t* mctx = fi->mod->ctx;

#if defined(ENABLE_SYSLOG)
	if (mctx->log_ident)
	{
		qse_awk_rtx_freemem (rtx, mctx->log_ident);
		mctx->log_ident = QSE_NULL;
	}
	closelog ();
	rx = 0;
#endif

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_writelog (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx = -1;
	qse_awk_val_t* retv;
	qse_awk_int_t pri;
	qse_char_t* msg = QSE_NULL;
	qse_size_t msglen;

#if defined(ENABLE_SYSLOG)
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &pri) <= -1) goto done;

	msg = qse_awk_rtx_getvalstr(rtx, qse_awk_rtx_getarg(rtx, 1), &msglen);
	if (!msg) goto done;

	if (qse_strxchr(msg, msglen, QSE_T('\0'))) goto done;
	#if defined(QSE_CHAR_IS_MCHAR)
	syslog(pri, "%s", msg);
	#else
	{
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdup(msg, QSE_NULL, qse_awk_rtx_getmmgr(rtx));
		if (!mbs) goto done;
		syslog(pri, "%s", mbs);
		qse_awk_rtx_freemem (rtx, mbs);
	}
	#endif
	rx = 0;
#endif

done:
	if (msg) qse_awk_rtx_freevalstr(rtx, qse_awk_rtx_getarg(rtx, 1), msg);

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

typedef struct inttab_t inttab_t;
struct inttab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_int_t info;
};

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */

	{ QSE_T("closelog"),   { { 0, 0, QSE_NULL     }, fnc_closelog,   0  } },
	{ QSE_T("fork"),       { { 0, 0, QSE_NULL     }, fnc_fork,       0  } },
	{ QSE_T("getegid"),    { { 0, 0, QSE_NULL     }, fnc_getegid,    0  } },
	{ QSE_T("getenv"),     { { 1, 1, QSE_NULL     }, fnc_getenv,     0  } },
	{ QSE_T("geteuid"),    { { 0, 0, QSE_NULL     }, fnc_geteuid,    0  } },
	{ QSE_T("getgid"),     { { 0, 0, QSE_NULL     }, fnc_getgid,     0  } },
	{ QSE_T("getnwifcfg"), { { 3, 3, QSE_T("vvr") }, fnc_getnwifcfg, 0  } },
	{ QSE_T("getpgid"),    { { 0, 0, QSE_NULL     }, fnc_getpgid,    0  } },
	{ QSE_T("getpid"),     { { 0, 0, QSE_NULL     }, fnc_getpid,     0  } },
	{ QSE_T("getppid"),    { { 0, 0, QSE_NULL     }, fnc_getppid,    0  } },
	{ QSE_T("gettid"),     { { 0, 0, QSE_NULL     }, fnc_gettid,     0  } },
	{ QSE_T("gettime"),    { { 0, 0, QSE_NULL     }, fnc_gettime,    0  } },
	{ QSE_T("getuid"),     { { 0, 0, QSE_NULL     }, fnc_getuid,     0  } },
	{ QSE_T("kill"),       { { 2, 2, QSE_NULL     }, fnc_kill,       0  } },
	{ QSE_T("mktime"),     { { 0, 1, QSE_NULL     }, fnc_mktime,     0  } },
	{ QSE_T("openlog"),    { { 3, 3, QSE_NULL     }, fnc_openlog,    0  } },
	{ QSE_T("settime"),    { { 1, 1, QSE_NULL     }, fnc_settime,    0  } },
	{ QSE_T("sleep"),      { { 1, 1, QSE_NULL     }, fnc_sleep,      0  } },
	{ QSE_T("strftime"),   { { 2, 2, QSE_NULL     }, fnc_strftime,   0  } },
	{ QSE_T("system"),     { { 1, 1, QSE_NULL     }, fnc_system,     0  } },
	{ QSE_T("systime"),    { { 0, 0, QSE_NULL     }, fnc_gettime,    0  } }, /* alias to gettime() */
	{ QSE_T("wait"),       { { 1, 1, QSE_NULL     }, fnc_wait,       0  } },
	{ QSE_T("writelog"),   { { 2, 2, QSE_NULL     }, fnc_writelog,   0  } }
};

#if !defined(SIGHUP)
#	define SIGHUP 1
#endif
#if !defined(SIGINT)
#	define SIGINT 2
#endif
#if !defined(SIGQUIT)
#	define SIGQUIT 3
#endif
#if !defined(SIGABRT)
#	define SIGABRT 6
#endif
#if !defined(SIGKILL)
#	define SIGKILL 9
#endif
#if !defined(SIGSEGV)
#	define SIGSEGV 11
#endif
#if !defined(SIGALRM)
#	define SIGALRM 14
#endif
#if !defined(SIGTERM)
#	define SIGTERM 15
#endif

static inttab_t inttab[] =
{
	/* keep this table sorted for binary search in query(). */

#if defined(ENABLE_SYSLOG)
	{ QSE_T("LOG_FAC_AUTH"),       { LOG_AUTH } },
	{ QSE_T("LOG_FAC_AUTHPRIV"),   { LOG_AUTHPRIV } },
	{ QSE_T("LOG_FAC_CRON"),       { LOG_CRON } },
	{ QSE_T("LOG_FAC_DAEMON"),     { LOG_DAEMON } },
	{ QSE_T("LOG_FAC_FTP"),        { LOG_FTP } },
	{ QSE_T("LOG_FAC_KERN"),       { LOG_KERN } },
	{ QSE_T("LOG_FAC_LOCAL0"),     { LOG_LOCAL0 } },
	{ QSE_T("LOG_FAC_LOCAL1"),     { LOG_LOCAL1 } },
	{ QSE_T("LOG_FAC_LOCAL2"),     { LOG_LOCAL2 } },
	{ QSE_T("LOG_FAC_LOCAL3"),     { LOG_LOCAL3 } },
	{ QSE_T("LOG_FAC_LOCAL4"),     { LOG_LOCAL4 } },
	{ QSE_T("LOG_FAC_LOCAL5"),     { LOG_LOCAL5 } },
	{ QSE_T("LOG_FAC_LOCAL6"),     { LOG_LOCAL6 } },
	{ QSE_T("LOG_FAC_LOCAL7"),     { LOG_LOCAL7 } },
	{ QSE_T("LOG_FAC_LPR"),        { LOG_LPR } },
	{ QSE_T("LOG_FAC_MAIL"),       { LOG_MAIL } },
	{ QSE_T("LOG_FAC_NEWS"),       { LOG_NEWS } },
	{ QSE_T("LOG_FAC_SYSLOG"),     { LOG_SYSLOG } },
	{ QSE_T("LOG_FAC_USER"),       { LOG_USER } },
	{ QSE_T("LOG_FAC_UUCP"),       { LOG_UUCP } },

	{ QSE_T("LOG_OPT_CONS"),       { LOG_CONS } },
	{ QSE_T("LOG_OPT_NDELAY"),     { LOG_NDELAY } },
	{ QSE_T("LOG_OPT_NOWAIT"),     { LOG_NOWAIT } },
	{ QSE_T("LOG_OPT_PID"),        { LOG_PID } },

	{ QSE_T("LOG_PRI_ALERT"),      { LOG_ALERT } },
	{ QSE_T("LOG_PRI_CRIT"),       { LOG_CRIT } },
	{ QSE_T("LOG_PRI_DEBUG"),      { LOG_DEBUG } },
	{ QSE_T("LOG_PRI_EMERG"),      { LOG_EMERG } },
	{ QSE_T("LOG_PRI_ERR"),        { LOG_ERR } },
	{ QSE_T("LOG_PRI_INFO"),       { LOG_INFO } },
	{ QSE_T("LOG_PRI_NOTICE"),     { LOG_NOTICE } },
	{ QSE_T("LOG_PRI_WARNING"),    { LOG_WARNING } },
#endif

	{ QSE_T("NWIFCFG_IN4"), { QSE_NWIFCFG_IN4 } },
	{ QSE_T("NWIFCFG_IN6"), { QSE_NWIFCFG_IN6 } },

	{ QSE_T("SIGABRT"), { SIGABRT } },
	{ QSE_T("SIGALRM"), { SIGALRM } },
	{ QSE_T("SIGHUP"),  { SIGHUP } },
	{ QSE_T("SIGINT"),  { SIGINT } },
	{ QSE_T("SIGKILL"), { SIGKILL } },
	{ QSE_T("SIGQUIT"), { SIGQUIT } },
	{ QSE_T("SIGSEGV"), { SIGSEGV } },
	{ QSE_T("SIGTERM"), { SIGTERM } }

/*
	{ QSE_T("WNOHANG"), { WNOHANG } },
*/
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int left, right, mid, n;

	left = 0; right = QSE_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (fnctab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[mid].info;
			return 0;
		}
	}

	left = 0; right = QSE_COUNTOF(inttab) - 1;
	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (inttab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[mid].info;
			return 0;
		}
	}

	ea.ptr = (qse_char_t*)name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

static int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: 
	for (each pid for rtx) kill (pid, SIGKILL);
	for (each pid for rtx) waitpid (pid, QSE_NULL, 0);
	*/
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	qse_awk_freemem (awk, mod->ctx);
}

int qse_awk_mod_sys (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;
	
	mod->ctx = qse_awk_callocmem(awk, QSE_SIZEOF(mod_ctx_t));
	if (!mod->ctx) return -1;

	return 0;
}

