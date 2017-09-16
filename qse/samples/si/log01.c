#include <qse/cmn/mem.h>
#include <qse/cmn/hwad.h>
#include <qse/si/log.h>
#include <qse/si/nwad.h>
#include <stdio.h>

void t1 (void)
{
	qse_log_t* log;
	qse_log_target_t t;
	qse_int128_t q = 0x1234567890;
	int i;
	qse_nwad_t nwad;

	t.file = QSE_T("/tmp/t3.log");
	/*qse_strtonwad ("127.0.0.1:514", &nwad);*/
	qse_strtonwad ("@/dev/log", &nwad);
	qse_nwadtoskad (&nwad, &t.syslog_remote);

	log = qse_log_open (QSE_MMGR_GETDFL(), 0, QSE_T("t3"), 
		QSE_LOG_INCLUDE_PID | QSE_LOG_HOST_IN_REMOTE_SYSLOG |
		QSE_LOG_DEBUG |
		QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG | QSE_LOG_SYSLOG_REMOTE, &t);

	QSE_ASSERT (qse_log_getoption (log) == (QSE_LOG_INCLUDE_PID | QSE_LOG_HOST_IN_REMOTE_SYSLOG));
	QSE_ASSERT (qse_log_gettarget (log, QSE_NULL) == (QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG | QSE_LOG_SYSLOG_REMOTE));

	for (i = 0; i < 10; i++)
	{

		if (i == 4)
		{
			qse_log_target_t t2;

			qse_log_gettarget (log, &t2);
			qse_strtonwad ("127.0.0.1:514", &nwad);
			qse_nwadtoskad (&nwad, &t2.syslog_remote);
			qse_log_settarget (log, QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG_REMOTE, &t2);

			qse_log_setoption (log, qse_log_getoption(log) | QSE_LOG_KEEP_FILE_OPEN);

			QSE_ASSERT (qse_log_getoption (log) == (QSE_LOG_INCLUDE_PID | QSE_LOG_HOST_IN_REMOTE_SYSLOG | QSE_LOG_KEEP_FILE_OPEN));
			QSE_ASSERT (qse_log_gettarget (log, QSE_NULL) == (QSE_LOG_CONSOLE | QSE_LOG_FILE | QSE_LOG_SYSLOG_REMOTE));
		}

		QSE_LOG4 (log, QSE_T("test"), QSE_LOG_DEBUG, QSE_T("XXXXXXXX %d %I128x %#0128I128b %l20d >>"), 10 * i , q, q, (long)45);
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
