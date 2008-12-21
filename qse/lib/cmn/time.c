/*
 * $Id$
 */

#include <ase/cmn/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#if defined(ASE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>
#endif

#ifdef _WIN32
	#define WIN_EPOCH_YEAR   ((ase_ntime_t)1601)
	#define WIN_EPOCH_MON    ((ase_ntime_t)1)
	#define WIN_EPOCH_DAY    ((ase_ntime_t)1)

	#define EPOCH_DIFF_YEARS (ASE_EPOCH_YEAR-WIN_EPOCH_YEAR)
	#define EPOCH_DIFF_DAYS  (EPOCH_DIFF_YEARS*365+EPOCH_DIFF_YEARS/4-3)
	#define EPOCH_DIFF_SECS  (EPOCH_DIFF_DAYS*24*60*60)
	#define EPOCH_DIFF_MSECS (EPOCH_DIFF_SECS*ASE_MSEC_IN_SEC)
#endif

static int mdays[2][ASE_MON_IN_YEAR] = 
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

int ase_gettime (ase_ntime_t* t)
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
	*t = ((ase_ntime_t)(*((ase_int64_t*)&ft)) / (10 * 1000));
	*t -= EPOCH_DIFF_MSECS;
	return 0;
#else
	struct timeval tv;
	int n;

#ifdef SYS_gettimeofday
	n = syscall (SYS_gettimeofday, &tv, ASE_NULL);
#else
	n = gettimeofday (&tv, ASE_NULL);
#endif
	if (n == -1) return -1;

	*t = tv.tv_sec * ASE_MSEC_IN_SEC + tv.tv_usec / ASE_USEC_IN_MSEC;
	return 0;
#endif
}

int ase_settime (ase_ntime_t t)
{
#ifdef _WIN32
	FILETIME ft;
	SYSTEMTIME st;

	*((ase_int64_t*)&ft) = ((t + EPOCH_DIFF_MSECS) * (10 * 1000));
	if (FileTimeToSystemTime (&ft, &st) == FALSE) return -1;
	if (SetSystemTime(&st) == FALSE) return -1;
	return 0;
#else
	struct timeval tv;
	int n;

	tv.tv_sec = t / ASE_MSEC_IN_SEC;
	tv.tv_usec = (t % ASE_MSEC_IN_SEC) * ASE_USEC_IN_MSEC;

/*
#if defined CLOCK_REALTIME && HAVE_CLOCK_SETTIME
  {
    int r = clock_settime (CLOCK_REALTIME, ts);
    if (r == 0 || errno == EPERM)
      return r;
  }
#elif HAVE_STIME
  /* This fails to compile on OSF1 V5.1, due to stime requiring
     a `long int*' and tv_sec is `int'.  But that system does provide
     settimeofday.  */
  return stime (&ts->tv_sec);
#else
*/

#ifdef SYS_settimeofday
	n = syscall (SYS_settimeofday, &tv, ASE_NULL);
#else
	n = settimeofday (&tv, ASE_NULL);
#endif
	if (n == -1) return -1;
	return 0;
#endif
}

void ase_gmtime (ase_ntime_t nt, ase_btime_t* bt)
{
	/* code based on minix 2.0 src/lib/ansi/gmtime.c */

	ase_ntime_t days; /* total days */
	ase_ntime_t secs; /* number of seconds in the fractional days */ 
	ase_ntime_t time; /* total seconds */

	int year = ASE_EPOCH_YEAR;
	
	time = nt / ASE_MSEC_IN_SEC;
	days = (unsigned long)time / ASE_SEC_IN_DAY;
	secs = (unsigned long)time % ASE_SEC_IN_DAY;
	
	bt->sec = secs % ASE_SEC_IN_MIN;
	bt->min = (secs % ASE_SEC_IN_HOUR) / ASE_SEC_IN_MIN;
	bt->hour = secs / ASE_SEC_IN_HOUR;

	bt->wday = (days + 4) % ASE_DAY_IN_WEEK;  

	while (days >= ASE_DAY_IN_YEAR(year)) 
	{
		days -= ASE_DAY_IN_YEAR(year);
		year++;
	}

	bt->year = year - 1900;
	bt->yday = days;
	bt->mon = 0;

	while (days >= mdays[ASE_IS_LEAPYEAR(year)][bt->mon]) 
	{
		days -= mdays[ASE_IS_LEAPYEAR(year)][bt->mon];
		bt->mon++;
	}

	bt->mday = days + 1;
	bt->isdst = 0;
}
