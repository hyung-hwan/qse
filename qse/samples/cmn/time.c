/*
 * NOTE  Targets without a 64-bit or bigger integer will suffer 
 * since milliseconds could be too large for a 32-bit integer.
 */

#include <qse/cmn/time.h>
#include <qse/cmn/sio.h>
#include <locale.h>

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

void print_time (const qse_ntime_t* nt, const qse_btime_t* bt)
{
#if defined(_WIN32)
	qse_printf (QSE_T("TIME: %I64d.%I64d\n"), (__int64)nt->sec, (__int64)nt->nsec);
#elif (QSE_SIZEOF_LONG_LONG > 0)
	qse_printf (QSE_T("TIME: %lld.%lld\n"), (long long)nt->sec, (long long)nt->nsec);
#else
	qse_printf (QSE_T("TIME: %ld.%ld\n"), (long)nt->sec, (long)nt->nsec);
#endif
	qse_printf (QSE_T("year: %d\n"), bt->year + QSE_BTIME_YEAR_BASE);
	qse_printf (QSE_T("mon: %d\n"), bt->mon + 1);
	qse_printf (QSE_T("mday: %d\n"), bt->mday);
	qse_printf (QSE_T("wday: %d\n"), bt->wday);
	qse_printf (QSE_T("hour: %d\n"), bt->hour);
	qse_printf (QSE_T("min: %d\n"), bt->min);
	qse_printf (QSE_T("sec: %d\n"), bt->sec);
	/*qse_printf (QSE_T("msec: %d\n"), bt->msec);*/
}

static int test1 (void)
{
	qse_ntime_t nt;
	qse_btime_t bt;
	int count = 0;

	if (qse_gettime (&nt) == -1)
	{
		qse_printf (QSE_T("cannot get the current time\n"));
		return -1;
	}

	do
	{
		qse_gmtime (&nt, &bt);
		print_time (&nt, &bt);
		qse_printf (QSE_T("-------------------------------\n"));

		if (qse_timegm (&bt, &nt) == -1)
		{
			qse_printf (QSE_T("cannot covert back to ntime\n"));
			qse_printf (QSE_T("===================================\n"));
		}
		else
		{
			/* note that nt.nsec is set to 0 after qse_timegm()
			 * since qse_btime_t doesn't have the nsec field. */
#ifdef _WIN32
			qse_printf (QSE_T("back to ntime: %I64d.%I64d\n"), (__int64)nt.sec, (__int64)nt.nsec);
#elif (QSE_SIZEOF_LONG_LONG > 0)
			qse_printf (QSE_T("back to ntime: %lld.%lld\n"), (long long)nt.sec, (long long)nt.nsec);
#else
			qse_printf (QSE_T("back to ntime: %ld\.%ldn"), (long)nt.sec, (long)nt.nsec);
#endif
			qse_gmtime (&nt, &bt);
			print_time (&nt, &bt);
			qse_printf (QSE_T("===================================\n"));
		}
	
		if (count > 0) break;

		nt.sec *= -1;
		count++;
	}
	while (1);


	nt.nsec = 0;
#if (QSE_SIZEOF_LONG_LONG > 0)
	for (nt.sec = (qse_long_t)QSE_TYPE_MIN(int); nt.sec <= (qse_long_t)QSE_TYPE_MAX(int); nt.sec += QSE_SECS_PER_DAY)
#else
	for (nt.sec = QSE_TYPE_MIN(int); nt.sec < (QSE_TYPE_MAX(int) - QSE_SECS_PER_DAY * 2); nt.sec += QSE_SECS_PER_DAY) 
#endif
	{
#ifdef _WIN32
		__time64_t t = (__time64_t)nt.sec;
#else
		time_t t = (time_t)nt.sec;
#endif
		qse_ntime_t qnt;
		struct tm* tm;
		qse_ntime_t xx;
		
		qnt = nt;
#if 0
		if (qnt.sec >= 0) qnt.sec += rand() % 1000;
		else qnt.sec -= rand() % 1000;
#endif

#ifdef _WIN32
		tm = _gmtime64 (&t);
#else
		tm = gmtime (&t);
#endif
		qse_gmtime (&qnt, &bt);

#ifdef _WIN32
		qse_printf (QSE_T(">>> time %I64d.%I64d: "), (__int64)qnt.sec, (__int64)qnt.nsec);
#elif (QSE_SIZEOF_LONG_LONG > 0)
		qse_printf (QSE_T(">>> time %lld.%lld: "), (long long)qnt.sec, (long long)qnt.nsec);
#else
		qse_printf (QSE_T(">>> time %ld.%ld: "), (long)qnt.sec, (long)qnt.nsec);
#endif

		if (tm == QSE_NULL ||
		    tm->tm_year != bt.year ||
		    tm->tm_mon != bt.mon ||
		    tm->tm_mday != bt.mday ||
		    tm->tm_wday != bt.wday ||
		    tm->tm_hour != bt.hour ||
		    tm->tm_min != bt.min ||
		    tm->tm_sec != bt.sec) 
		{
#ifdef _WIN32
			qse_printf (QSE_T("[GMTIME ERROR %I64d]\n"), (__int64)t);
#elif (QSE_SIZEOF_LONG_LONG > 0)
			qse_printf (QSE_T("[GMTIME ERROR %lld]\n"), (long long)t);
#else
			qse_printf (QSE_T("[GMTIME ERROR %ld]\n"), (long)t);
#endif
			if (tm == QSE_NULL) qse_printf (QSE_T(">> GMTIME RETURNED NULL\n"));
			print_time (&qnt, &bt);
			qse_printf (QSE_T("-------------------------------\n"));
		}	
		else
		{
			qse_printf (QSE_T("[GMTIME OK]"));
		}

		if (qse_timegm (&bt, &xx) == -1)
		{
			qse_printf (QSE_T("[TIMEGM FAIL]\n"));
		}
		else
		{
			if (xx.sec == qnt.sec && xx.nsec == qnt.nsec) 
			{
				qse_printf (QSE_T("[TIMEGM OK %d/%d/%d %d:%d:%d]\n"), bt.year + QSE_BTIME_YEAR_BASE, bt.mon + 1, bt.mday, bt.hour, bt.min, bt.sec);
			}
			else 
			{
#ifdef _WIN32
				qse_printf (QSE_T("[TIMEGM ERROR %I64d.%I64d, %d/%d/%d %d:%d:%d]\n"), (__int64)xx.sec, (__int64)xx.nsec, bt.year + QSE_BTIME_YEAR_BASE, bt.mon + 1, bt.mday, bt.hour, bt.min, bt.sec);
#elif (QSE_SIZEOF_LONG_LONG > 0)
				qse_printf (QSE_T("[TIMEGM ERROR %lld.%lld, %d/%d/%d %d:%d:%d]\n"), (long long)xx.sec, (long long)xx.nsec, bt.year + QSE_BTIME_YEAR_BASE, bt.mon + 1, bt.mday, bt.hour, bt.min, bt.sec);
#else
				qse_printf (QSE_T("[TIMEGM ERROR %ld.%ld, %d/%d/%d %d:%d:%d]\n"), (long)xx.sec, (long)xx.nsec, bt.year + QSE_BTIME_YEAR_BASE, bt.mon + 1, bt.mday, bt.hour, bt.min, bt.sec);
#endif
			}
		}
	}

	return 0;
}

int main ()
{
	setlocale (LC_ALL, "");

	qse_openstdsios ();
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);
	qse_closestdsios ();

	return 0;
}
