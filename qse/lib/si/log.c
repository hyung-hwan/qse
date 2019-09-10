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

#include <qse/si/log.h>
#include <qse/cmn/str.h>
#include <qse/cmn/time.h>
#include <qse/cmn/mbwc.h>
#include "../cmn/mem-prv.h"
#include "../cmn/va_copy.h"

#if defined(_WIN32)
	/* TODO: windows event log */
#else
	#include <syslog.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/un.h>
	#include <sys/utsname.h>
	#include "../cmn/syscall.h"
#endif

static const qse_mchar_t* __syslog_month_names[] =
{
	QSE_MT("Jan"),
	QSE_MT("Feb"),
	QSE_MT("Mar"),
	QSE_MT("Apr"),
	QSE_MT("May"),
	QSE_MT("Jun"),
	QSE_MT("Jul"),
	QSE_MT("Aug"),
	QSE_MT("Sep"),
	QSE_MT("Oct"),
	QSE_MT("Nov"),
	QSE_MT("Dec"),
};

#ifndef LOG_EMERG
#	define LOG_EMERG  0 
#endif
#ifndef LOG_ALERT
#	define LOG_ALERT  1  
#endif
#ifndef LOG_CRIT
#	define LOG_CRIT   2
#endif
#ifndef LOG_ERR
#	define LOG_ERR         3
#endif
#ifndef LOG_WARNING
#	define LOG_WARNING     4
#endif
#ifndef LOG_NOTICE
#	define LOG_NOTICE      5
#endif
#ifndef LOG_INFO
#	define LOG_INFO        6
#endif
#ifndef LOG_DEBUG
#	define LOG_DEBUG       7
#endif

/* use a simple look-up table for mapping a priority to a syslog value.
 * i assume it's faster than getting the position of the first lowest bit set
 * and use a smaller and dense table without gap-filling 0s. */
static int __syslog_priority[256] =
{
	0,
	LOG_EMERG,      /* 1 */
	LOG_ALERT,      /* 2 */
	0,
	LOG_CRIT,       /* 4 */
	0, 0, 0,
	LOG_ERR,        /* 8 */
	0, 0, 0, 0, 0, 0, 0,
	LOG_WARNING,    /* 16 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	LOG_NOTICE,     /* 32 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	LOG_INFO,       /* 64 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0,
	LOG_DEBUG,      /* 128 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0
};

#if defined(_WIN32)
/* TODO: windows event logging */
#else

struct syslog_fac_info_t
{
	const qse_char_t* name;
	qse_log_facility_t code;
};

static struct syslog_fac_info_t __syslog_fac_info[] =
{
	{ QSE_T("kern"),     QSE_LOG_KERN },
	{ QSE_T("user"),     QSE_LOG_USER },
	{ QSE_T("mail"),     QSE_LOG_MAIL },
	{ QSE_T("daemon"),   QSE_LOG_DAEMON },
	{ QSE_T("auth"),     QSE_LOG_AUTH },
	{ QSE_T("syslogd"),  QSE_LOG_SYSLOGD },
	{ QSE_T("lpr"),      QSE_LOG_LPR },
	{ QSE_T("news"),     QSE_LOG_NEWS },
	{ QSE_T("uucp"),     QSE_LOG_UUCP },
	{ QSE_T("cron"),     QSE_LOG_CRON },
	{ QSE_T("authpriv"), QSE_LOG_AUTHPRIV },
	{ QSE_T("ftp"),      QSE_LOG_FTP },
	{ QSE_T("local0"),   QSE_LOG_LOCAL0 },
	{ QSE_T("local1"),   QSE_LOG_LOCAL1 },
	{ QSE_T("local2"),   QSE_LOG_LOCAL2 },
	{ QSE_T("local3"),   QSE_LOG_LOCAL3 },
	{ QSE_T("local4"),   QSE_LOG_LOCAL4 },
	{ QSE_T("local5"),   QSE_LOG_LOCAL5 },
	{ QSE_T("local6"),   QSE_LOG_LOCAL6 },
	{ QSE_T("local7"),   QSE_LOG_LOCAL7 }
};
#endif

static QSE_INLINE int get_active_priority_bits (int flags)
{
	int priority_bits = 0;

	if (flags & QSE_LOG_MASKED_PRIORITY)
	{
		priority_bits = flags & QSE_LOG_MASK_PRIORITY;
	}
	else
	{
		int pri = flags & QSE_LOG_MASK_PRIORITY;
		switch (pri)
		{
			case QSE_LOG_DEBUG:
				priority_bits |= QSE_LOG_DEBUG;
			case QSE_LOG_INFO:
				priority_bits |= QSE_LOG_INFO;
			case QSE_LOG_NOTICE:
				priority_bits |= QSE_LOG_NOTICE;
			case QSE_LOG_WARNING:
				priority_bits |= QSE_LOG_WARNING;
			case QSE_LOG_ERROR:
				priority_bits |= QSE_LOG_ERROR;
			case QSE_LOG_CRITICAL:
				priority_bits |= QSE_LOG_CRITICAL;
			case QSE_LOG_ALERT:
				priority_bits |= QSE_LOG_ALERT;
			case QSE_LOG_PANIC:
				priority_bits |= QSE_LOG_PANIC;
		}
	}

	return priority_bits;
}

qse_log_t* qse_log_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_char_t* ident, int potflags, const qse_log_target_t* target)
{
	qse_log_t* log;

	log = (qse_log_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_log_t) + xtnsize);
	if (log)
	{
		 if (qse_log_init(log, mmgr, ident, potflags, target) <= -1)
		 {
			QSE_MMGR_FREE (mmgr, log);
			return QSE_NULL;
		 }
		 else QSE_MEMSET (QSE_XTN(log), 0, xtnsize);
	}

	return log;
}

void qse_log_close (qse_log_t* log)
{
	qse_log_fini (log);
	QSE_MMGR_FREE (log->mmgr, log);
}

int qse_log_init (qse_log_t* log, qse_mmgr_t* mmgr, const qse_char_t* ident, int potflags, const qse_log_target_t* target)
{
	QSE_MEMSET (log, 0, QSE_SIZEOF(*log));
	log->mmgr = mmgr;

	log->flags = potflags & (QSE_LOG_MASK_PRIORITY | QSE_LOG_MASK_OPTION);
	log->t.syslog_remote.sock = -1; 

	if (potflags & QSE_LOG_FILE) log->flags |= QSE_LOG_FILE;
	if (target && target->file)
	{
		if (qse_strlen(target->file) >= QSE_COUNTOF(log->t.file.pathbuf))
		{
			log->t.file.path = qse_strdup(target->file, log->mmgr);
			if (!log->t.file.path) return -1;
		}
		else
		{
			qse_strxcpy (log->t.file.pathbuf, QSE_COUNTOF(log->t.file.pathbuf), target->file);
			log->t.file.path = log->t.file.pathbuf;
		}
	}

	if (potflags & QSE_LOG_CONSOLE) log->flags |= QSE_LOG_CONSOLE;
	if (potflags & QSE_LOG_SYSLOG) log->flags |= QSE_LOG_SYSLOG;

	if (potflags & QSE_LOG_SYSLOG_REMOTE) log->flags |= QSE_LOG_SYSLOG_REMOTE;
	if (target && qse_skadfamily(&target->syslog_remote) >= 0) log->t.syslog_remote.addr = target->syslog_remote;

	if (ident) qse_strxcpy (log->ident, QSE_COUNTOF(log->ident), ident);
	if (qse_mtx_init(&log->mtx, mmgr) <= -1) 
	{
		if (log->t.file.path && log->t.file.path != log->t.file.pathbuf) 
		{
			QSE_MMGR_FREE (mmgr, log->t.file.path);
			log->t.file.path = QSE_NULL;
		}
		return -1;
	}

	log->active_priority_bits = get_active_priority_bits(log->flags);

#if defined(_WIN32)
	/* TODO: windows event logging */
#else
	log->syslog_facility = QSE_LOG_USER;
#endif
	
	return 0;
}

void qse_log_fini (qse_log_t* log)
{
	if (log->t.syslog_remote.sock >= 0)
	{
		QSE_CLOSE (log->t.syslog_remote.sock);
		log->t.syslog_remote.sock = -1;
	}

	if (log->t.console.sio)
	{
		qse_sio_close (log->t.console.sio);
		log->t.console.sio = QSE_NULL;
	}

	if (log->t.file.path && log->t.file.path != log->t.file.pathbuf)
	{
		QSE_MMGR_FREE (log->mmgr, log->t.file.path);
		log->t.file.path = QSE_NULL;
	}

	if (log->t.file.sio)
	{
		qse_sio_close (log->t.file.sio);
		log->t.file.sio = QSE_NULL;
	}

	if (log->t.syslog.opened) 
	{
		closelog ();
		log->t.syslog.opened = 0;
	}

	if (log->dmsgbuf) 
	{
		qse_mbs_close (log->dmsgbuf);
		log->dmsgbuf = QSE_NULL;
	}

#if defined(QSE_CHAR_IS_WCHAR)
	if (log->wmsgbuf)
	{
		qse_wcs_close (log->wmsgbuf);
		log->wmsgbuf = QSE_NULL;
	}
#endif
	qse_mtx_fini (&log->mtx);
}


static QSE_INLINE void set_ident (qse_log_t* log, const qse_char_t* ident)
{
	/* set the base identifer to use */
	if (ident) 
		qse_strxcpy (log->ident, QSE_COUNTOF(log->ident), ident);
	else
		log->ident[0] = QSE_T('\0');

	if (log->t.syslog.opened)
	{
		/* it's best to avoid using QSE_LOG_SYSLOG
		 * if the application calls syslog APIs directly.
		 * otherwise, it may conflict with openlog()/closelog()
		 * called in this library */

		closelog ();
		log->t.syslog.opened = 0;
		/* it will be opened again with the new identifier in the
		 * output function if necessary. */
	}
}


void qse_log_setidentwithmbs (qse_log_t* log, const qse_mchar_t* ident)
{
#if defined(QSE_CHAR_IS_MCHAR)
	set_ident(log, ident);
#else
	qse_wchar_t* id;
	id = qse_mbstowcsdup(ident, QSE_NULL, log->mmgr);
	if (id) set_ident(log, id); /* don't care about failure */
	QSE_MMGR_FREE (log->mmgr, id);
#endif
}

void qse_log_setidentwithwcs (qse_log_t* log, const qse_wchar_t* ident)
{
#if defined(QSE_CHAR_IS_MCHAR)
	qse_mchar_t* id;
	id = qse_wcstombsdup(ident, QSE_NULL, log->mmgr);
	if (id) set_ident(log, id); /* don't care about failure */
	QSE_MMGR_FREE (log->mmgr, id);
#else
	set_ident(log, ident);
#endif
}


int qse_log_settarget (qse_log_t* log, int flags, const qse_log_target_t* target)
{
	qse_char_t* target_file = QSE_NULL;
	if (target->file && qse_strlen(target->file) >= QSE_COUNTOF(log->t.file.pathbuf))
	{
		target_file = qse_strdup (target->file, log->mmgr);
		if (!target_file) return -1;
	}

	if (log->t.syslog_remote.sock >= 0)
	{
		QSE_CLOSE (log->t.syslog_remote.sock);
		log->t.syslog_remote.sock = -1;
	}

	if (log->t.console.sio)
	{
		qse_sio_close (log->t.console.sio);
		log->t.console.sio = QSE_NULL;
	}

	if (log->t.file.sio)
	{
		qse_sio_close (log->t.file.sio);
		log->t.file.sio = QSE_NULL;
	}

	if (log->t.syslog.opened) 
	{
		closelog ();
		log->t.syslog.opened = 0;
	}

	log->flags &= (QSE_LOG_MASK_PRIORITY | QSE_LOG_MASK_OPTION); /* preserve the priority and the options */
	if (flags & QSE_LOG_FILE) log->flags |= QSE_LOG_FILE;

	/* If you just want to set the target file path without enable QSE_LOG_FILE,
	 * just set target->file without QSE_LOG_FILE in the flags.
	 * later, you can call this function with QSE_LOG_FILE set but with target->file or QSE_NULL */
	if (target && target->file) 
	{
		if (log->t.file.path && log->t.file.path != log->t.file.pathbuf) QSE_MMGR_FREE (log->mmgr, log->t.file.path);

		if (target_file)
		{
			log->t.file.path = target_file;
		}
		else
		{
			qse_strxcpy (log->t.file.pathbuf, QSE_COUNTOF(log->t.file.pathbuf), target->file);
			log->t.file.path = log->t.file.pathbuf;
		}
	}

	if (flags & QSE_LOG_CONSOLE) log->flags |= QSE_LOG_CONSOLE;
	if (flags & QSE_LOG_SYSLOG) log->flags |= QSE_LOG_SYSLOG;

	if (flags & QSE_LOG_SYSLOG_REMOTE) log->flags |= QSE_LOG_SYSLOG_REMOTE;
	if (target && qse_skadfamily(&target->syslog_remote) >= 0) log->t.syslog_remote.addr = target->syslog_remote;

	return 0;
}

int qse_log_gettarget (qse_log_t* log, qse_log_target_t* target)
{
	if (target)
	{
		target->file = log->t.file.path;
		target->syslog_remote = log->t.syslog_remote.addr;
	}
	return log->flags & QSE_LOG_MASK_TARGET;
}

void qse_log_setoption (qse_log_t* log, int option)
{
	log->flags = (log->flags & (QSE_LOG_MASK_TARGET | QSE_LOG_MASK_PRIORITY)) | (option & QSE_LOG_MASK_OPTION);
}

void qse_log_setpriority (qse_log_t* log, int priority)
{
	log->flags = (log->flags & (QSE_LOG_MASK_TARGET | QSE_LOG_MASK_OPTION)) | (priority & QSE_LOG_MASK_PRIORITY);
	log->active_priority_bits = get_active_priority_bits (log->flags);
}

void qse_log_setsyslogfacility (qse_log_t* log, qse_log_facility_t facility)
{
#if !defined(_WIN32)
	log->syslog_facility = facility;
#endif
}

static QSE_INLINE void __report_over_sio (qse_log_t* log, qse_sio_t* sio, const qse_mchar_t* tm, const qse_char_t* ident, const qse_char_t* fmt, va_list arg)
{
	int id_out = 0;

	qse_sio_putmbs (sio, tm);
	qse_sio_putmbs (sio, QSE_MT(" "));

	if (log->ident[0] != QSE_T('\0')) 
	{
		qse_sio_putstr (sio, log->ident);

		if (ident && ident[0] != QSE_T('\0'))
		{
			qse_sio_putmbs (sio, QSE_MT("("));
			qse_sio_putstr (sio, ident);
			qse_sio_putmbs (sio, QSE_MT(")"));
		}
		id_out = 1;
	}
	else
	{
		if (ident && ident[0] != QSE_T('\0'))
		{
			qse_sio_putstr (sio, ident);
			id_out = 1;
		}
	}

	if (log->flags & QSE_LOG_INCLUDE_PID) 
	{
		qse_sio_putmbsf (sio, QSE_MT("[%d]"), (int)QSE_GETPID());
		id_out = 1;
	}
	if (id_out) qse_sio_putmbs (sio, QSE_MT(": "));

	qse_sio_putstrvf (sio, fmt, arg);
	qse_sio_putmbs (sio, "\n");
}

void qse_log_report (qse_log_t* log, const qse_char_t* ident, int pri, const qse_char_t* fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	qse_log_reportv (log, ident, pri, fmt, ap);
	va_end (ap);
}

void qse_log_reportv (qse_log_t* log, const qse_char_t* ident, int pri, const qse_char_t* fmt, va_list ap)
{
	qse_ntime_t now;
	qse_btime_t cnow;
	qse_mchar_t tm[64];

	if ((log->flags & QSE_LOG_MASK_TARGET) == 0) return; /* no target */

	if (log->flags & QSE_LOG_MASKED_PRIORITY)
	{
		if (!(pri & (log->flags & QSE_LOG_MASK_PRIORITY))) return;
	}
	else
	{
		if (pri > (log->flags & QSE_LOG_MASK_PRIORITY)) return;
	}

	if (qse_gettime(&now) || qse_localtime(&now, &cnow) <= -1) return;

	if (log->flags & (QSE_LOG_CONSOLE | QSE_LOG_FILE))
	{
		if (cnow.gmtoff == QSE_TYPE_MIN(int))
		{
			qse_mbsxfmt (tm, QSE_COUNTOF(tm),
				QSE_MT("%04.4d-%02d-%02d %02d:%02d:%02d"), 
				cnow.year + QSE_BTIME_YEAR_BASE, cnow.mon + 1, cnow.mday, 
				cnow.hour, cnow.min, cnow.sec);
		}
		else
		{
			qse_long_t gmtoff_hour, gmtoff_min;
			gmtoff_hour = cnow.gmtoff / QSE_SECS_PER_HOUR;
			gmtoff_min = (cnow.gmtoff % QSE_SECS_PER_HOUR) / QSE_SECS_PER_MIN;
			qse_mbsxfmt (tm, QSE_COUNTOF(tm),
				QSE_MT("%04.4d-%02d-%02d %02d:%02d:%02d %c%02d%02d"), 
				cnow.year + QSE_BTIME_YEAR_BASE, cnow.mon + 1, cnow.mday, 
				cnow.hour, cnow.min, cnow.sec, 
				((cnow.gmtoff > 0)? QSE_T('+'): QSE_T('-')),
				gmtoff_hour, gmtoff_min);
		}
	}

	if (qse_mtx_lock (&log->mtx, QSE_NULL) <= -1) return;

	if (log->flags & QSE_LOG_CONSOLE) 
	{
		if (!log->t.console.sio) 
			log->t.console.sio = qse_sio_openstd (log->mmgr, 0, QSE_SIO_STDERR, QSE_SIO_APPEND | QSE_SIO_IGNOREMBWCERR);
		if (log->t.console.sio)
		{
			va_list xap;
			va_copy (xap, ap);
			__report_over_sio (log, log->t.console.sio, tm, ident, fmt, xap);
		}
	}

	if (log->flags & QSE_LOG_FILE) 
	{
		if (!log->t.file.sio)
			log->t.file.sio = qse_sio_open (log->mmgr, 0, log->t.file.path, QSE_SIO_CREATE | QSE_SIO_APPEND | QSE_SIO_IGNOREMBWCERR);
		if (log->t.file.sio)
		{
			va_list xap;
			va_copy (xap, ap);
			__report_over_sio (log, log->t.file.sio, tm, ident, fmt, xap);
			if (!(log->flags & QSE_LOG_KEEP_FILE_OPEN))
			{
				qse_sio_close (log->t.file.sio);
				log->t.file.sio = QSE_NULL;
			}
		}
	}

	if (log->flags & (QSE_LOG_SYSLOG | QSE_LOG_SYSLOG_REMOTE))
	{
		va_list xap;
		qse_size_t fplen, fpdlen, fpdilen;
		int sl_pri, id_out = 0;
		const qse_mchar_t* identfmt, * identparenfmt;

		if (!log->dmsgbuf) log->dmsgbuf = qse_mbs_open (log->mmgr, 0, 0);
		if (!log->dmsgbuf) goto done;

#if defined(QSE_CHAR_IS_WCHAR)
		if (!log->wmsgbuf) log->wmsgbuf = qse_wcs_open(log->mmgr, 0, 0);
		if (!log->wmsgbuf) goto done;
#endif

		/* the priority value given must have only 1 bit set. otherwise, it will translate to wrong values */
		sl_pri = (pri < QSE_COUNTOF(__syslog_priority))? __syslog_priority[(pri & QSE_LOG_MASK_PRIORITY)]: LOG_DEBUG;

		fplen = qse_mbs_fmt(log->dmsgbuf, QSE_MT("<%d>"), (int)(log->syslog_facility | sl_pri));
		if (fplen == (qse_size_t)-1) goto done;

		fpdlen = qse_mbs_fcat (
			log->dmsgbuf, QSE_MT("%s %02d %02d:%02d:%02d "), 
			__syslog_month_names[cnow.mon], cnow.mday, 
			cnow.hour, cnow.min, cnow.sec);
		if (fpdlen == (qse_size_t)-1) goto done;

		if (log->flags & QSE_LOG_HOST_IN_REMOTE_SYSLOG)
		{
			struct utsname un;
			if (uname(&un) == 0)
			{
				fpdlen = qse_mbs_fcat (log->dmsgbuf, QSE_MT("%hs "), un.nodename);
				if (fpdlen == (qse_size_t)-1) goto done;
			}
		}

	#if defined(QSE_CHAR_IS_MCHAR)
		identfmt = QSE_MT("%hs");
		identparenfmt = QSE_MT("(%hs)");
	#else
		identfmt = QSE_MT("%ls");
		identparenfmt = QSE_MT("(%ls)");
	#endif

		if (log->ident[0])
		{
			if (qse_mbs_fcat(log->dmsgbuf, identfmt, log->ident) == (qse_size_t)-1) goto done;
			if (ident && ident[0] && 
			    qse_mbs_fcat(log->dmsgbuf, identparenfmt, ident) == (qse_size_t)-1) goto done;
			id_out = 1;
		}
		else
		{
			if (ident && ident[0])
			{
				if (qse_mbs_fcat(log->dmsgbuf, identfmt, ident) == (qse_size_t)-1) goto done;
				id_out = 1;
			}
		}

		if (log->flags & QSE_LOG_INCLUDE_PID)
		{
			fpdilen = qse_mbs_fcat(log->dmsgbuf, QSE_MT("[%d]"), (int)QSE_GETPID());
			if (fpdilen == (qse_size_t)-1) goto done;
			id_out = 1;
		}

		if (id_out)
		{
			fpdilen = qse_mbs_fcat(log->dmsgbuf, QSE_MT(": "));
			if (fpdilen == (qse_size_t)-1) goto done;
		}
		else
		{
			fpdilen = fpdlen;
		}

		va_copy (xap, ap);
	#if defined(QSE_CHAR_IS_MCHAR)
		if (qse_mbs_vfcat(log->dmsgbuf, fmt, xap) == (qse_size_t)-1) goto done;
	#else
		if (qse_wcs_vfmt(log->wmsgbuf, fmt, xap) == (qse_size_t)-1 ||
		    qse_mbs_fcat(log->dmsgbuf, QSE_MT("%.*ls"), QSE_WCS_LEN(log->wmsgbuf), QSE_WCS_PTR(log->wmsgbuf)) == (qse_size_t)-1) goto done;
	#endif

		if (log->flags & QSE_LOG_SYSLOG) 
		{
		#if defined(_WIN32)
			/* TODO: windows event log */
		#else
			int sl_opt;

			sl_opt = (log->flags & QSE_LOG_INCLUDE_PID)? LOG_PID: 0;

			if (!log->t.syslog.opened) 
			{
				/* the secondary 'ident' string is not included into syslog */
			#if defined(QSE_CHAR_IS_MCHAR)
				openlog (log->ident, sl_opt, log->syslog_facility);
			#else
				qse_mbsxfmt (log->mident, QSE_COUNTOF(log->mident), QSE_MT("%ls"), log->ident);
				openlog (log->mident, sl_opt, log->syslog_facility);
			#endif
				log->t.syslog.opened = 1;
			}

			syslog (sl_pri, "%s", QSE_MBS_CPTR(log->dmsgbuf, fpdilen));
		#endif
		}

		if (log->flags & QSE_LOG_SYSLOG_REMOTE)  
		{
		#if defined(_WIN32)
			/* TODO: windows event log */
		#else
			/* direct interface over udp to a remote syslog server. 
			 * if you specify a local unix socket address, it may interact with a local syslog server */
			if (log->t.syslog_remote.sock <= -1)
			{
			#if defined(SOCK_CLOEXECX)
				log->t.syslog_remote.sock = socket (qse_skadfamily(&log->t.syslog_remote.addr), SOCK_DGRAM | SOCK_CLOEXEC, 0);
			#else
				log->t.syslog_remote.sock = socket (qse_skadfamily(&log->t.syslog_remote.addr), SOCK_DGRAM, 0);
				#if defined(FD_CLOEXEC)
				if (log->t.syslog_remote.sock >= 0)
				{
					int flag = fcntl (log->t.syslog_remote.sock, F_GETFD);
					if (flag >= 0) fcntl (log->t.syslog_remote.sock, F_SETFD, flag | FD_CLOEXEC);
				}
				#endif
			#endif
			}
			
			if (log->t.syslog_remote.sock >= 0)
			{
				sendto (log->t.syslog_remote.sock, QSE_MBS_PTR(log->dmsgbuf), QSE_MBS_LEN(log->dmsgbuf), 0,
					(struct sockaddr*)&log->t.syslog_remote.addr, 
					qse_skadsize(&log->t.syslog_remote.addr));
			}
		#endif
		}

	}

done:
	qse_mtx_unlock (&log->mtx);
}

/* --------------------------------------------------------------------------
 * HELPER FUNCTIONS
 * -------------------------------------------------------------------------- */

static const qse_char_t* __priority_names[] =
{
/* NOTE: QSE_LOG_PRIORITY_LEN_MAX must be redefined if strings here
 *       can produce a longer compositional name. e.g. panic|critical|... */
	QSE_T("panic"),
	QSE_T("alert"),
	QSE_T("critical"),
	QSE_T("error"),
	QSE_T("warning"),
	QSE_T("notice"),
	QSE_T("info"),
	QSE_T("debug")
};

const qse_char_t* qse_get_log_priority_name (int pri)
{
	if (pri & QSE_LOG_PANIC) return __priority_names[0];
	if (pri & QSE_LOG_ALERT) return __priority_names[1];
	if (pri & QSE_LOG_CRITICAL) return __priority_names[2];
	if (pri & QSE_LOG_ERROR) return __priority_names[3];
	if (pri & QSE_LOG_WARNING) return __priority_names[4];
	if (pri & QSE_LOG_NOTICE) return __priority_names[5];
	if (pri & QSE_LOG_INFO) return __priority_names[6];
	if (pri & QSE_LOG_DEBUG) return __priority_names[7];

	return QSE_NULL;
}

qse_size_t qse_make_log_priority_name (int pri, const qse_char_t* delim, qse_char_t* buf, qse_size_t len)
{
	qse_size_t tlen, xlen, rem, i;

	tlen = 0;
	rem = len;

	for (i = 0; i < QSE_COUNTOF(__priority_names); i++)
	{
		if (pri & (1UL << i))
		{
			if (rem <= 1) break;

			xlen = (tlen <= 0)?
				qse_strxcpy (&buf[tlen], rem, __priority_names[i]):
				qse_strxjoin (&buf[tlen], rem, delim, __priority_names[i], QSE_NULL);

			rem -= xlen;
			tlen += xlen;
		}
	}

	if (len >= tlen) buf[tlen] = QSE_T('\0');
	return tlen;
}

int qse_get_log_priority_by_wcsname (const qse_wchar_t* name, const qse_wchar_t* delim)
{
	qse_size_t i;
	qse_wcstr_t tok;
	const qse_wchar_t* ptr;
	int pri = 0;

	ptr = name;
	while (ptr)
	{
		ptr = qse_wcstok(ptr, delim, &tok);
		if (tok.ptr)
		{
			for (i = 0; i < QSE_COUNTOF(__priority_names); i++)
			{
			#if defined(QSE_CHAR_IS_MCHAR)
				if (qse_wcsxmbscmp(tok.ptr, tok.len, __priority_names[i]) == 0) 
			#else
				if (qse_strxcmp(tok.ptr, tok.len, __priority_names[i]) == 0) 
			#endif
				{
					pri |= (1UL << i);
					break;
				}
			}
			if (i >= QSE_COUNTOF(__priority_names)) return 0; /* unknown name included */
		}
	}

	return pri;
}

int qse_get_log_priority_by_mbsname (const qse_mchar_t* name, const qse_mchar_t* delim)
{
	qse_size_t i;
	qse_mcstr_t tok;
	const qse_mchar_t* ptr;
	int pri = 0;

	ptr = name;
	while (ptr)
	{
		ptr = qse_mbstok(ptr, delim, &tok);
		if (tok.ptr)
		{
			for (i = 0; i < QSE_COUNTOF(__priority_names); i++)
			{
			#if defined(QSE_CHAR_IS_MCHAR)
				if (qse_strxcmp(tok.ptr, tok.len, __priority_names[i]) == 0) 
			#else
				if (qse_mbsxwcscmp(tok.ptr, tok.len, __priority_names[i]) == 0) 
			#endif
				{
					pri |= (1UL << i);
					break;
				}
			}
			if (i >= QSE_COUNTOF(__priority_names)) return 0; /* unknown name included */
		}
	}

	return pri;
}

int qse_get_log_facility_by_wcsname (const qse_wchar_t* name, qse_log_facility_t* fcode)
{
	qse_size_t i;

	for (i = 0; i < QSE_COUNTOF(__syslog_fac_info); i++)
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		if (qse_mbswcscmp(__syslog_fac_info[i].name, name) == 0)
	#else
		if (qse_wcscmp(__syslog_fac_info[i].name, name) == 0)
	#endif
		{
			*fcode = __syslog_fac_info[i].code;
			return 0;
		}
	}

	return -1;
}

int qse_get_log_facility_by_mbsname (const qse_mchar_t* name, qse_log_facility_t* fcode)
{
	qse_size_t i;

	for (i = 0; i < QSE_COUNTOF(__syslog_fac_info); i++)
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		if (qse_mbscmp(__syslog_fac_info[i].name, name) == 0)
	#else
		if (qse_wcsmbscmp(__syslog_fac_info[i].name, name) == 0)
	#endif
		{
			*fcode = __syslog_fac_info[i].code;
			return 0;
		}
	}

	return -1;
}
