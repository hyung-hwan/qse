#include <qse/cmn/time.h>
#include <qse/utl/stdio.h>
#include <locale.h>

#include <sys/time.h>
#include <time.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

void print_time (qse_ntime_t nt, const qse_btime_t* bt)
{
	qse_printf (QSE_T("TIME: %lld\n"), (long long)nt);
	qse_printf (QSE_T("year: %d\n"), bt->year + QSE_BTIME_YEAR_BASE);
	qse_printf (QSE_T("mon: %d\n"), bt->mon + 1);
	qse_printf (QSE_T("mday: %d\n"), bt->mday);
	qse_printf (QSE_T("wday: %d\n"), bt->wday);
	qse_printf (QSE_T("hour: %d\n"), bt->hour);
	qse_printf (QSE_T("min: %d\n"), bt->min);
	qse_printf (QSE_T("sec: %d\n"), bt->sec);
	qse_printf (QSE_T("msec: %d\n"), bt->msec);
}

static int test1 (void)
{
	qse_ntime_t nt;
	qse_btime_t bt;

	if (qse_gettime (&nt) == -1)
	{
		qse_printf (QSE_T("cannot get the current time\n"));
		return -1;
	}

	qse_gmtime (nt, &bt);
	print_time (nt, &bt);
	qse_printf (QSE_T("-------------------------------\n"));

	nt = 9999999;
	if (qse_timegm (&bt, &nt) == -1)
	{
		qse_printf (QSE_T("cannot covert back to ntime\n"));
		qse_printf (QSE_T("-------------------------------\n"));
	}
	else
	{
		qse_printf (QSE_T("back to ntime: %lld\n"), (long long)nt);
		qse_gmtime (nt, &bt);
		print_time (nt, &bt);
		qse_printf (QSE_T("-------------------------------\n"));
	}
	
	nt *= -1;
	qse_gmtime (nt, &bt);
	print_time (nt, &bt);
	qse_printf (QSE_T("-------------------------------\n"));


	for (nt = (qse_ntime_t)QSE_TYPE_MIN(int); 
	     nt <= (qse_ntime_t)QSE_TYPE_MAX(int); nt += QSE_SECS_PER_DAY)
	{
		time_t t = (time_t)nt;
		struct tm* tm;

		tm = gmtime (&t);
		qse_gmtime (nt * 1000, &bt);

		qse_printf (QSE_T(">>> time %lld: "), (long long)nt*1000);

		if (tm->tm_year != bt.year ||
		    tm->tm_mon != bt.mon ||
		    tm->tm_mday != bt.mday ||
		    tm->tm_wday != bt.wday ||
		    tm->tm_hour != bt.hour ||
		    tm->tm_min != bt.min ||
		    tm->tm_sec != bt.sec) 
		{
			qse_printf (QSE_T("[ERROR]\n"));
			print_time (nt, &bt);
			qse_printf (QSE_T("-------------------------------\n"));
		}	
		else
		{
			qse_printf (QSE_T("[OK]\n"));
		}
	}

	return 0;
}

int main ()
{
	setlocale (LC_ALL, "");

	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);

	return 0;
}
