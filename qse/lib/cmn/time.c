/*
 * $Id$
 */

#include <qse/cmn/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#if defined(QSE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>
#endif

#ifdef _WIN32
	#define WIN_EPOCH_YEAR   ((qse_ntime_t)1601)
	#define WIN_EPOCH_MON    ((qse_ntime_t)1)
	#define WIN_EPOCH_DAY    ((qse_ntime_t)1)

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

static void brkdntime (qse_ntime_t nt, qse_btime_t* bt, qse_ntime_t offset)
{
	int midx;
	qse_ntime_t days; /* total days */
	qse_ntime_t secs; /* the remaining seconds */
	qse_ntime_t year = QSE_EPOCH_YEAR;
	
	nt += offset;
	/* TODO: support bt->msecs */
	/*bt->msecs = nt % QSEC_MSECS_PER_SEC;*/

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
	bt->offset = offset;
}

void qse_gmtime (qse_ntime_t nt, qse_btime_t* bt)
{
	brkdntime (nt, bt, 0);
}
