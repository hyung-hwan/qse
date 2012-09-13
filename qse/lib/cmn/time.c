/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include <qse/cmn/time.h>
#include "mem.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <time.h>
#elif defined(__OS2__)
#	define INCL_DOSDATETIME
#	define INCL_DOSERRORS
#	include <os2.h>
#	include <time.h>
#elif defined(__DOS__)
#	include <dos.h>
#	include <time.h>
#else
#	include "syscall.h"
#	if defined(HAVE_SYS_TIME_H)
#		include <sys/time.h>
#	endif
#	if defined(HAVE_TIME_H)
#		include <time.h>
#	endif
#endif

#if defined(_WIN32)
	#define WIN_EPOCH_YEAR   (1601)
	#define WIN_EPOCH_MON    (1)
	#define WIN_EPOCH_DAY    (1)

	#define EPOCH_DIFF_YEARS (QSE_EPOCH_YEAR-WIN_EPOCH_YEAR)
	#define EPOCH_DIFF_DAYS  ((qse_ntime_t)EPOCH_DIFF_YEARS*365+EPOCH_DIFF_YEARS/4-3)
	#define EPOCH_DIFF_SECS  ((qse_ntime_t)EPOCH_DIFF_DAYS*24*60*60)
	#define EPOCH_DIFF_MSECS ((qse_ntime_t)EPOCH_DIFF_SECS*QSE_MSECS_PER_SEC)
#endif

static const int mdays[2][QSE_MONS_PER_YEAR] = 
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

/* number of days from the beginning of the year to the end of of
 * a previous month. adjust for leap years in the code. */
static const int mdays_tot[2][QSE_MONS_PER_YEAR] =
{
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};

/* number of days from beginning of a previous month to the end of 
 * the year. adjust for leap years in the code. */
static const int mdays_rtot[2][QSE_MONS_PER_YEAR] =
{
	{ 334, 306, 275, 245, 214, 184, 153, 122, 92, 61, 31, 0 },
	{ 335, 306, 275, 245, 214, 184, 153, 122, 92, 61, 31, 0 }
};

/* get number of extra days for leap years between fy and ty inclusive */
static int get_leap_days (int fy, int ty)
{
	fy--; ty--;
	return (ty / 4 - fy / 4) - 
	       (ty / 100 - fy / 100) +
	       (ty / 400 - fy / 400);
}

int qse_gettime (qse_ntime_t* t)
{
#if defined(_WIN32)
	SYSTEMTIME st;
	FILETIME ft;

	/* 
	 * MSDN: The FILETIME structure is a 64-bit value representing the 
	 *       number of 100-nanosecond intervals since January 1, 1601 (UTC).
	 */

	GetSystemTime (&st);
	if (SystemTimeToFileTime (&st, &ft) == FALSE) return -1;
	*t = ((qse_ntime_t)(*((qse_int64_t*)&ft)) / (10 * 1000));
	*t -= EPOCH_DIFF_MSECS;
	return 0;

#elif defined(__OS2__)

	APIRET rc;
	DATETIME dt;
	qse_btime_t bt;

	/* Can I use DosQuerySysInfo(QSV_TIME_LOW) and 
	 * DosQuerySysInfo(QSV_TIME_HIGH) for this instead? 
	 * Maybe, resolution too low as it returns values 
	 * in seconds. */

	rc = DosGetDateTime (&dt);
	if (rc != NO_ERROR) return -1;

	bt.year = dt.year - QSE_BTIME_YEAR_BASE;
	bt.mon = dt.month - 1;
	bt.mday = dt.day;
	bt.hour = dt.hours;
	bt.min = dt.minutes;
	bt.sec = dt.seconds;
	bt.msec = dt.hundredths * 10;
	bt.isdst = -1; /* determine dst for me */

	if (qse_timelocal (&bt, t) <= -1) return -1;
	return 0;

#elif defined(__DOS__)

	struct dostime_t dt;
	struct dosdate_t dd;
	qse_btime_t bt;

	_dos_gettime (&dt);
	_dos_getdate (&dd);

	bt.year = dd.year - QSE_BTIME_YEAR_BASE;
	bt.mon = dd.month - 1;
	bt.mday = dd.day;
	bt.hour = dt.hour;
	bt.min = dt.minute;
	bt.sec = dt.second;
	bt.msec = dt.hsecond * 10;
	bt.isdst = -1; /* determine dst for me */

	if (qse_timelocal (&bt, t) <= -1) return -1;
	return 0;

#else
	struct timeval tv;
	int n;

	n = QSE_GETTIMEOFDAY (&tv, QSE_NULL);
	if (n == -1) return -1;

	*t = (qse_ntime_t)tv.tv_sec * QSE_MSECS_PER_SEC + 
	     (qse_ntime_t)tv.tv_usec / QSE_USECS_PER_MSEC;
	return 0;
#endif
}

int qse_settime (qse_ntime_t t)
{
#if defined(_WIN32)
	FILETIME ft;
	SYSTEMTIME st;

	*((qse_int64_t*)&ft) = ((t + EPOCH_DIFF_MSECS) * (10 * 1000));
	if (FileTimeToSystemTime (&ft, &st) == FALSE) return -1;
	if (SetSystemTime(&st) == FALSE) return -1;
	return 0;

#elif defined(__OS2__)

	APIRET rc;
	DATETIME dt;
	qse_btime_t bt;

	if (qse_localtime (t, &bt) <= -1) return -1;

	QSE_MEMSET (&dt, 0, QSE_SIZEOF(dt));
	dt.year = bt.year + QSE_BTIME_YEAR_BASE;
	dt.month = bt.mon + 1;
	dt.day = bt.mday;
	dt.hours = bt.hour;
	dt.minutes = bt.min;
	dt.seconds = bt.sec;
	dt.hundredths = bt.msec / 10;

	rc = DosSetDateTime (&dt);
	return (rc != NO_ERROR)? -1: 0;

#elif defined(__DOS__)

	struct dostime_t dt;
	struct dosdate_t dd;
	qse_btime_t bt;

	if (qse_localtime (t, &bt) <= -1) return -1;

	dd.year = bt.year + QSE_BTIME_YEAR_BASE;
	dd.month = bt.mon + 1;
	dd.day = bt.mday;
	dt.hour = bt.hour;
	dt.minute = bt.min;
	dt.second = bt.sec;
	dt.hsecond = bt.msec / 10;

	if (_dos_settime (&dt) != 0) return -1;
	if (_dos_setdate (&dd) != 0) return -1;

	return 0;

#else
	struct timeval tv;
	int n;

	tv.tv_sec = t / QSE_MSECS_PER_SEC;
	tv.tv_usec = (t % QSE_MSECS_PER_SEC) * QSE_USECS_PER_MSEC;

/*
#if defined CLOCK_REALTIME && HAVE_CLOCK_SETTIME
	{
		int r = clock_settime (CLOCK_REALTIME, ts);
		if (r == 0 || errno == EPERM)
		return r;
	}
#elif HAVE_STIME
	return stime (&ts->tv_sec);
#else
*/
	n = QSE_SETTIMEOFDAY (&tv, QSE_NULL);
	if (n == -1) return -1;
	return 0;
#endif
}

static void breakdown_time (qse_ntime_t nt, qse_btime_t* bt, qse_ntoff_t offset)
{
	int midx;
	qse_ntime_t days; /* total days */
	qse_ntime_t secs; /* the remaining seconds */
	qse_ntime_t year = QSE_EPOCH_YEAR;
	
	nt += offset;

	bt->msec = nt % QSE_MSECS_PER_SEC;
	if (bt->msec < 0) bt->msec = QSE_MSECS_PER_SEC + bt->msec;

	secs = nt / QSE_MSECS_PER_SEC;
	days = secs / QSE_SECS_PER_DAY;
	secs %= QSE_SECS_PER_DAY;

	while (secs < 0)
	{
		secs += QSE_SECS_PER_DAY;
		--days;
	}

	while (secs >= QSE_SECS_PER_DAY)
	{
		secs -= QSE_SECS_PER_DAY;
		++days;
	}

	bt->hour = secs / QSE_SECS_PER_HOUR;
	secs %= QSE_SECS_PER_HOUR;
	bt->min = secs / QSE_SECS_PER_MIN;
	bt->sec = secs % QSE_SECS_PER_MIN;

	bt->wday = (days + QSE_EPOCH_WDAY) % QSE_DAYS_PER_WEEK;  
	if (bt->wday < 0) bt->wday += QSE_DAYS_PER_WEEK;

	if (days >= 0)
	{
		while (days >= QSE_DAYS_PER_YEAR(year))
		{
    			days -= QSE_DAYS_PER_YEAR(year);
    			year++;
    		}
	}
    	else 
	{
		do 
		{
    			year--;
   			days += QSE_DAYS_PER_YEAR(year);
    		} 
		while (days < 0);
	}

	bt->year = year - QSE_BTIME_YEAR_BASE;
	bt->yday = days;

	midx = QSE_IS_LEAPYEAR(year)? 1: 0;
	for (bt->mon = 0; days >= mdays[midx][bt->mon]; bt->mon++) 
	{
		days -= mdays[midx][bt->mon];
	}

	bt->mday = days + 1;
	bt->isdst = 0; /* TODO: this may vary depeding on offset and time */
	/*bt->offset = offset;*/
}

int qse_gmtime (qse_ntime_t nt, qse_btime_t* bt)
{
	breakdown_time (nt, bt, 0);
	return 0;
}

int qse_localtime (qse_ntime_t nt, qse_btime_t* bt)
{
	struct tm* tm;
	time_t t = (time_t)(nt / QSE_MSECS_PER_SEC);
	qse_ntime_t rem = nt % QSE_MSECS_PER_SEC;

	/* TODO: remove dependency on localtime/localtime_r */
#if defined(_WIN32)
	tm = localtime (&t);
#elif defined(__OS2__)
#	if defined(__WATCOMC__)
		struct tm btm;
		tm = _localtime (&t, &btm);
#	else
#		error Please support other compilers 
#	endif
#elif defined(__DOS__)
#	if defined(__WATCOMC__)
		struct tm btm;
		tm = _localtime (&t, &btm);
#	else
#		error Please support other compilers
#	endif
#else
	struct tm btm;
	tm = localtime_r (&t, &btm);
#endif
	if (tm == QSE_NULL) return -1;
	
	QSE_MEMSET (bt, 0, QSE_SIZEOF(*bt));

	bt->msec = (rem >= 0)? rem: (QSE_MSECS_PER_SEC + rem);
	bt->sec = tm->tm_sec;
	bt->min = tm->tm_min;
	bt->hour = tm->tm_hour;
	bt->mday = tm->tm_mday;
	bt->mon = tm->tm_mon;
	bt->year = tm->tm_year;
	bt->wday = tm->tm_wday;
	bt->yday = tm->tm_yday;
	bt->isdst = tm->tm_isdst;
	/*bt->offset = tm->tm_offset;*/

	return 0;
}

int qse_timegm (const qse_btime_t* bt, qse_ntime_t* nt)
{
#if 0
#if defined(_WIN32)
	/* TODO: verify qse_timegm for WIN32 */
	SYSTEMTIME st;
	FILETIME ft;

	st.wYear = bt->tm_year + QSE_EPOCH_YEAR;
	st.wMonth = bt->tm_mon + WIN_EPOCH_MON;
	st.wDayOfWeek = bt->tm_wday;
	st.wDay = bt->tm_mday;
	st.wHour = bt->tm_hour;
	st.wMinute = bt->tm_min;
	st.mSecond = bt->tm_sec;
	st.mMilliseconds = bt->tm_msec;

	if (SystemTimeToFileTime (&st, &ft) == FALSE) return -1;
	*nt = ((qse_ntime_t)(*((qse_int64_t*)&ft)) / (10 * 1000));
	*nt -= EPOCH_DIFF_MSECS;
	
	return 0;
#elif defined(__OS2__)
#	error NOT IMPLEMENTED YET
#elif defined(__DOS__)
#	error NOT IMPLEMENTED YET
#else

	/* TODO: qse_timegm - remove dependency on timegm */
	struct tm tm;

	tm.tm_sec = bt->sec;
	tm.tm_min = bt->min;
	tm.tm_hour = bt->hour;
	tm.tm_mday = bt->mday;
	tm.tm_mon = bt->mon;
	tm.tm_year = bt->year;
	tm.tm_wday = bt->wday;
	tm.tm_yday = bt->yday;
	tm.tm_isdst = bt->isdst;

#ifdef HAVE_TIMEGM
	*nt = ((qse_ntime_t)timegm(&tm)*QSE_MSECS_PER_SEC) + bt->msec;
	return 0;
#else
	#warning #### timegm() is not available on this platform ####
	return -1;
#endif

#endif
#endif

	qse_ntime_t n = 0;
	int y = bt->year + QSE_BTIME_YEAR_BASE;
	int midx = QSE_IS_LEAPYEAR(y)? 1: 0;

	QSE_ASSERT (bt->mon >= 0 && bt->mon < QSE_MONS_PER_YEAR);

	if (y < QSE_EPOCH_YEAR)
	{
		/*int x;

		for (x = y; x < QSE_EPOCH_YEAR - 1; x++)
			n += QSE_DAYS_PER_YEAR(x);*/
		n = QSE_DAYS_PER_NORMYEAR * (QSE_EPOCH_YEAR - 1 - y) + 
		    get_leap_days (y, QSE_EPOCH_YEAR - 1);

		/*for (x = bt->mon + 1; x < QSE_MONS_PER_YEAR; x++)
			n += mdays[midx][x];*/
		n += mdays_rtot[midx][bt->mon];

		n += mdays[midx][bt->mon] - bt->mday;
		if (midx == 1) n -= 1;
		n *= QSE_HOURS_PER_DAY;

		n = (n + QSE_HOURS_PER_DAY - bt->hour - 1) * QSE_MINS_PER_HOUR;
		n = (n + QSE_MINS_PER_HOUR - bt->min - 1) * QSE_SECS_PER_MIN;
		n = (n + QSE_SECS_PER_MIN - bt->sec) * QSE_MSECS_PER_SEC;

		if (bt->msec > 0) n += QSE_MSECS_PER_SEC - bt->msec;
		*nt = -n;
	} 
	else
	{
		/*int x;

		for (x = QSE_EPOCH_YEAR; x < y; x++) 
			n += QSE_DAYS_PER_YEAR(x);*/
		n = QSE_DAYS_PER_NORMYEAR * (y - QSE_EPOCH_YEAR) + 
		    get_leap_days (QSE_EPOCH_YEAR, y);

		/*for (x = 0; x < bt->mon; x++) n += mdays[midx][x];*/
		n += mdays_tot[midx][bt->mon];

		n = (n + bt->mday - 1) * QSE_HOURS_PER_DAY;
		n = (n + bt->hour) * QSE_MINS_PER_HOUR;
		n = (n + bt->min) * QSE_SECS_PER_MIN;
		n = (n + bt->sec) * QSE_MSECS_PER_SEC;

		*nt = n + bt->msec;
	}

	return 0;
}

int qse_timelocal (const qse_btime_t* bt, qse_ntime_t* nt)
{
	/* TODO: qse_timelocal - remove dependency on timelocal */
	struct tm tm;

	tm.tm_sec = bt->sec;
	tm.tm_min = bt->min;
	tm.tm_hour = bt->hour;
	tm.tm_mday = bt->mday;
	tm.tm_mon = bt->mon;
	tm.tm_year = bt->year;
	tm.tm_wday = bt->wday;
	tm.tm_yday = bt->yday;
	tm.tm_isdst = bt->isdst;

#ifdef HAVE_TIMELOCAL
	*nt = ((qse_ntime_t)timelocal(&tm)*QSE_MSECS_PER_SEC) + bt->msec;
	return 0;
#else
	*nt = ((qse_ntime_t)mktime(&tm)*QSE_MSECS_PER_SEC) + bt->msec;
	return 0;
#endif
}
