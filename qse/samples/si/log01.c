#include <qse/cmn/mem.h>
#include <qse/cmn/hwad.h>
#include <qse/si/log.h>
#include <qse/si/nwad.h>
#include <stdio.h>

static void t1_sub001 (qse_log_t* log)
{
	static struct
	{
		const qse_char_t* name;
		int pri;
	} xtab[] =
	{
		{ QSE_T(""),                          0                                             },
		{ QSE_T("   "),                       0                                             },
		{ QSE_T("|"),                         0                                             },
		{ QSE_T("debug|xinfo|panic"),         0                                             },
		{ QSE_T("debug|info|panic"),          (QSE_LOG_DEBUG | QSE_LOG_INFO | QSE_LOG_PANIC) },
		{ QSE_T("debug|notice|debug|panic"),  (QSE_LOG_DEBUG | QSE_LOG_NOTICE | QSE_LOG_PANIC) },
		{ QSE_T("notice"),                    QSE_LOG_NOTICE                                 }
	};
	qse_size_t i;
	qse_char_t buf[QSE_LOG_PRIORITY_LEN_MAX + 1];
	qse_size_t len;

	for (i = 0; i < QSE_COUNTOF(xtab); i++)
	{
		len = qse_get_log_priority_name (xtab[i].pri, buf, QSE_COUNTOF(buf));
		QSE_LOG3 (log, QSE_NULL, QSE_LOG_INFO, QSE_T("%x => %s [%d]"), xtab[i].pri, buf, len);
	}
	
	for  (i = 0; i < QSE_COUNTOF(xtab); i++)
	{
		int pri;
		if ((pri = qse_get_log_priority_by_name(xtab[i].name)) == xtab[i].pri)
		{
			QSE_LOG2 (log, QSE_NULL, QSE_LOG_INFO, QSE_T("SUCCESS: %s => %x"), xtab[i].name, pri);
		}
		else
		{
			QSE_LOG3 (log, QSE_NULL, QSE_LOG_INFO, QSE_T("FAILURE: %s => got %x expected %x"), xtab[i].name, pri, xtab[i].pri);
		}
	}
}

static void t1 (void)
{
	qse_log_t* log;
	qse_log_target_t t;
#if defined(QSE_HAVE_INT128_T)
	qse_int128_t q = 0x1234567890;
#elif defined(QSE_HAVE_INT64_T)
	qse_int64_t q = 0x1234567890;
#else
	qse_int32_t q = 0x12345678;
#endif
	int i;
	qse_nwad_t nwad;

	t.file = QSE_T("/tmp/t3.log");
	/*qse_strtonwad ("127.0.0.1:514", &nwad);*/
	/*qse_strtonwad ("@/var/run/log", &nwad);*/
	qse_strtonwad (QSE_T("@/dev/log"), &nwad);
	qse_nwadtoskad (&nwad, &t.syslog_remote);

	log = qse_log_open (QSE_MMGR_GETDFL(), 0, QSE_T("t3"), 
		QSE_LOG_INCLUDE_PID | QSE_LOG_HOST_IN_REMOTE_SYSLOG |
		QSE_LOG_DEBUG |
		QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG | QSE_LOG_SYSLOG_REMOTE, &t);

	QSE_ASSERT (qse_log_getoption (log) == (QSE_LOG_INCLUDE_PID | QSE_LOG_HOST_IN_REMOTE_SYSLOG));
	QSE_ASSERT (qse_log_gettarget (log, QSE_NULL) == (QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG | QSE_LOG_SYSLOG_REMOTE));

	t1_sub001 (log);

	for (i = 0; i < 10; i++)
	{

		if (i == 4)
		{
			qse_log_target_t t2;

			qse_log_gettarget (log, &t2);
			qse_strtonwad (QSE_T("127.0.0.1:514"), &nwad);
			qse_nwadtoskad (&nwad, &t2.syslog_remote);
			qse_log_settarget (log, QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG_REMOTE, &t2);

			qse_log_setoption (log, qse_log_getoption(log) | QSE_LOG_KEEP_FILE_OPEN);

			QSE_ASSERT (qse_log_getoption (log) == (QSE_LOG_INCLUDE_PID | QSE_LOG_HOST_IN_REMOTE_SYSLOG | QSE_LOG_KEEP_FILE_OPEN));
			QSE_ASSERT (qse_log_gettarget (log, QSE_NULL) == (QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG_REMOTE));
		}

		QSE_LOG4 (log, QSE_T("test"), QSE_LOG_DEBUG, 
#if defined(QSE_HAVE_INT128_T)
			QSE_T("MSG %d %I128x %#0128I128b %l20d >>"), 
#elif defined(QSE_HAVE_INT64_T)
			QSE_T("MSG %d %I64x %#064I64b %l20d >>"), 
#else
			QSE_T("MSG %d %I32x %#032I32b %l20d >>"), 
#endif
			10 * i , q, q, (long)45
		);
	}


#if defined(QSE_LOG)
	QSE_LOG (log, QSE_T("var"), QSE_LOG_INFO, QSE_T("variadic QSE_LOG() supported - no argument"));
	QSE_LOG (log, QSE_T("var"), QSE_LOG_ERROR, QSE_T("variadic QSE_LOG() supported %d %d"), 1, 2);
	QSE_LOG (log, QSE_T("var"), QSE_LOG_PANIC, QSE_T("variadic QSE_LOG() supported %10s"), QSE_T("panic"));
#endif
	qse_log_close (log);
}

int main (int argc, char* argv[])
{
	t1 ();
	return 0;
}
