/*
 * $Id$
 */

#include <qse/cmn/time.h>
#include "mem.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <time.h>

#if defined(QSE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>
#endif

#ifdef _WIN32
	#define WIN_EPOCH_YEAR   (1601)
	#define WIN_EPOCH_MON    (1)
	#define WIN_EPOCH_DAY    (1)

	#define EPOCH_DIFF_YEARS (QSE_EPOCH_YEAR-WIN_EPOCH_YEAR)
	#define EPOCH_DIFF_DAYS  (EPOCH_DIFF_YEARS*365+EPOCH_DIFF_YEARS/4-3)
	#define EPOCH_DIFF_SECS  (EPOCH_DIFF_DAYS*24*60*60)
	#define EPOCH_DIFF_MSECS (EPOCH_DIFF_SECS*QSE_MSECS_PER_SEC)
#endif

static int mdays[2][QSE_MONS_PER_YEAR] = 
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

int qse_gettime (qse_ntime_t* t)
{
#ifdef _WIN32
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
#else
	struct timeval tv;
	int n;

#ifdef SYS_gettimeofday
	n = syscall (SYS_gettimeofday, &tv, QSE_NULL);
#else
	n = gettimeofday (&tv, QSE_NULL);
#endif
	if (n == -1) return -1;

	*t = (qse_ntime_t)tv.tv_sec*QSE_MSECS_PER_SEC + 
	     (qse_ntime_t)tv.tv_usec/QSE_USECS_PER_MSEC;
	return 0;
#endif
}

int qse_settime (qse_ntime_t t)
{
#ifdef _WIN32
	FILETIME ft;
	SYSTEMTIME st;

	*((qse_int64_t*)&ft) = ((t + EPOCH_DIFF_MSECS) * (10 * 1000));
	if (FileTimeToSystemTime (&ft, &st) == FALSE) return -1;
	if (SetSystemTime(&st) == FALSE) return -1;
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
  / * This fails to compile on OSF1 V5.1, due to stime requiring
     a `long int*' and tv_sec is `int'.  But that system does provide
     settimeofday.  * /
  return stime (&ts->tv_sec);
#else
*/

#ifdef SYS_settimeofday
	n = syscall (SYS_settimeofday, &tv, QSE_NULL);
#else
	n = settimeofday (&tv, QSE_NULL);
#endif
	if (n == -1) return -1;
	return 0;
#endif
}

static void breakdown_time (qse_ntime_t nt, qse_btime_t* bt, qse_ntime_t offset)
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
	bt->isdst = 0;
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
#ifdef _WIN32
	tm = localtime (&t);
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
#ifdef _WIN32
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
#else
	#warning #### timegm() is not available on this platform ####
	return -1;
#endif

#endif
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
#else
	#warning #### timelocal() is not available on this platform ####
	return -1;
#endif
}

