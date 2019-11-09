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
    OF MERCHANTABILITY AND FITNESS FOR A PARTICAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_SI_LOG_H_
#define _QSE_SI_LOG_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/si/mtx.h>
#include <qse/si/sio.h>
#include <qse/si/nwad.h>
#include <stdarg.h>

/* TODO: improve this and complete win32 portion */
#define QSE_LOG_IDENT_MAX 32

#define QSE_LOG_PRIORITY_LEN_MAX 64

#define QSE_LOG_MASK_PRIORITY 0x000000FF /* bit 0 to 7 */
                                           /* bit 8 to 11 unused */
#define QSE_LOG_MASK_OPTION   0x000FF000 /* bit 12 to 19 */
#define QSE_LOG_MASK_TARGET   0xFFF00000 /* bit 20 to 31 */


/* priority */

enum qse_log_priority_flag_t /* bit 0 to 7. potentially to be extended beyond */
{
	QSE_LOG_PANIC           = (1 << 0),
	QSE_LOG_ALERT           = (1 << 1),
	QSE_LOG_CRITICAL        = (1 << 2),
	QSE_LOG_ERROR           = (1 << 3),
	QSE_LOG_WARNING         = (1 << 4),
	QSE_LOG_NOTICE          = (1 << 5),
	QSE_LOG_INFO            = (1 << 6),
	QSE_LOG_DEBUG           = (1 << 7),
	QSE_LOG_ALL_PRIORITIES  = (QSE_LOG_PANIC | QSE_LOG_ALERT | QSE_LOG_CRITICAL | QSE_LOG_ERROR | QSE_LOG_WARNING | QSE_LOG_NOTICE | QSE_LOG_INFO | QSE_LOG_DEBUG)
};

enum qse_log_option_flag_t /* bit 12 to 19 */
{
	QSE_LOG_KEEP_FILE_OPEN        = (1 << 12),
	QSE_LOG_MASKED_PRIORITY       = (1 << 13),
	QSE_LOG_INCLUDE_PID           = (1 << 14),
	QSE_LOG_HOST_IN_REMOTE_SYSLOG = (1 << 15),
};

enum qse_log_target_flag_t /* bit 20 to 31 */
{
	QSE_LOG_CONSOLE       =  (1 << 20),
	QSE_LOG_FILE          =  (1 << 21),
	QSE_LOG_SYSLOG        =  (1 << 22),
	QSE_LOG_SYSLOG_REMOTE =  (1 << 23)
};

/* facility */
enum qse_log_facility_t
{
	QSE_LOG_KERN     = (0 << 3),  /* kernel messages */
	QSE_LOG_USER     = (1 << 3),  /* random user-level messages */
	QSE_LOG_MAIL     = (2 << 3),  /* mail system */
	QSE_LOG_DAEMON   = (3 << 3),  /* system daemons */
	QSE_LOG_AUTH     = (4 << 3),  /* security/authorization messages */
	QSE_LOG_SYSLOGD  = (5 << 3),  /* messages from syslogd */
	QSE_LOG_LPR      = (6 << 3),  /* line printer subsystem */
	QSE_LOG_NEWS     = (7 << 3),  /* network news subsystem */
	QSE_LOG_UUCP     = (8 << 3),  /* UUCP subsystem */
	QSE_LOG_CRON     = (9 << 3),  /* clock daemon */
	QSE_LOG_AUTHPRIV = (10 << 3), /* authorization messages (private) */
	QSE_LOG_FTP      = (11 << 3), /* ftp daemon */
	QSE_LOG_LOCAL0   = (16 << 3), /* reserved for local use */
	QSE_LOG_LOCAL1   = (17 << 3), /* reserved for local use */
	QSE_LOG_LOCAL2   = (18 << 3), /* reserved for local use */
	QSE_LOG_LOCAL3   = (19 << 3), /* reserved for local use */
	QSE_LOG_LOCAL4   = (20 << 3), /* reserved for local use */
	QSE_LOG_LOCAL5   = (21 << 3), /* reserved for local use */
	QSE_LOG_LOCAL6   = (22 << 3), /* reserved for local use */
	QSE_LOG_LOCAL7   = (23 << 3)  /* reserved for local use */
};
typedef enum qse_log_facility_t qse_log_facility_t;

#define QSE_LOG_ENABLED(log,pri) ((pri) & (log)->active_priority_bits) 

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define QSE_LOG(log,ident,pri,...) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, __VA_ARGS__); \
	} while (0)
#endif

#define QSE_LOG0(log,ident,pri,fmt) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt); \
	} while (0)

#define QSE_LOG1(log,ident,pri,fmt,m1) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1); \
	} while (0)
#define QSE_LOG2(log,ident,pri,fmt,m1,m2) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2); \
	} while (0)
#define QSE_LOG3(log,ident,pri,fmt,m1,m2,m3) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3); \
	} while (0)
#define QSE_LOG4(log,ident,pri,fmt,m1,m2,m3,m4) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4); \
	} while (0)
#define QSE_LOG5(log,ident,pri,fmt,m1,m2,m3,m4,m5) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5); \
	} while (0)

#define QSE_LOG6(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6); \
	} while (0)

#define QSE_LOG7(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7); \
	} while (0)

#define QSE_LOG8(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8); \
	} while (0)

#define QSE_LOG9(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8,m9) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8, m9); \
	} while (0)

#define QSE_LOG10(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10); \
	} while (0)

#define QSE_LOG11(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11); \
	} while (0)

#define QSE_LOG12(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11 ,m12); \
	} while (0)
#define QSE_LOG13(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13); \
	} while (0)
#define QSE_LOG14(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14); \
	} while (0)
#define QSE_LOG15(log,ident,pri,fmt,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15) \
	do { \
		if (QSE_LOG_ENABLED(log,pri)) \
			qse_log_report (log, ident, pri, fmt, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15); \
	} while (0)


typedef struct qse_log_t qse_log_t;
struct qse_log_t
{
	qse_mmgr_t* mmgr;
	int flags; 

	struct 
	{
		struct
		{
			qse_sio_t* sio;
		} console;

		struct
		{
			qse_char_t pathbuf[128]; /* static path buffer */
			qse_char_t* path;
			qse_sio_t* sio;
		} file;

		struct
		{
			int opened;
		} syslog;

		struct
		{
			qse_skad_t addr;
			int        sock;
		} syslog_remote;
	} t;

	qse_char_t ident[QSE_LOG_IDENT_MAX + 1];
	qse_mbs_t* dmsgbuf;
#if defined(QSE_CHAR_IS_WCHAR)
	qse_wcs_t* wmsgbuf;
	qse_mchar_t mident[QSE_LOG_IDENT_MAX * 2 +1];
#endif
	qse_mtx_t mtx;

	int active_priority_bits;

#if !defined(_WIN32)
	qse_log_facility_t syslog_facility;
#endif
};


struct qse_log_target_data_t
{
	const qse_char_t* file;
	qse_skad_t        syslog_remote;
};
typedef struct qse_log_target_data_t qse_log_target_data_t;

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_log_t* qse_log_open (
	qse_mmgr_t*                  mmgr,
	qse_size_t                   xtnsize,
	const qse_char_t*            ident,
	int                          potflags, /* priority + option + target bits */
	const qse_log_target_data_t* target_data
);

QSE_EXPORT void qse_log_close (
	qse_log_t*        log
);

QSE_EXPORT int qse_log_init (
	qse_log_t*              log,
	qse_mmgr_t*             mmgr,
	const qse_char_t*       ident,
	int                     potflags,
	const qse_log_target_data_t* target_data
);

QSE_EXPORT void qse_log_fini (
	qse_log_t* log
);

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE qse_mmgr_t* qse_log_getmmgr (qse_log_t* log) { return (log)->mmgr; }
#else
#define qse_log_getmmgr(log) ((log)->mmgr))
#endif

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE void* qse_log_getxtn (qse_log_t* log) { return QSE_XTN(log); }
#else
#define qse_log_getxtn(log) (QSE_XTN(log))
#endif

QSE_EXPORT void qse_log_setidentwithmbs (
	qse_log_t*         log,
	const qse_mchar_t* ident
);

QSE_EXPORT void qse_log_setidentwithwcs (
	qse_log_t*         log,
	const qse_wchar_t* ident
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_log_setident qse_log_setidentwithmbs
#else
#	define qse_log_setident qse_log_setidentwithwcs
#endif

/**
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_log_settarget (
	qse_log_t*                   log,
	int                          target_flags,
	const qse_log_target_data_t* target_data
);

/** 
 * \return target flags which is an integer bitwise-ORed of the target bits.
 */
QSE_EXPORT int qse_log_gettarget (
	qse_log_t*             log,
	qse_log_target_data_t* target_data
);

QSE_EXPORT void qse_log_setoption (
	qse_log_t* log,
	int        options
);

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE int qse_log_getoption (qse_log_t* log)
{
	return log->flags & QSE_LOG_MASK_OPTION;
}
#else
#define qse_log_getoption(log) ((log)->flags & QSE_LOG_MASK_OPTION)
#endif


QSE_EXPORT void qse_log_setpriority (
	qse_log_t* log,
	int        priority
);

QSE_EXPORT void qse_log_setsyslogfacility (
	qse_log_t*         log,
	qse_log_facility_t facility
);

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE int qse_log_getpriority (qse_log_t* log)
{
	return log->flags & QSE_LOG_MASK_PRIORITY;
}
#else
#define qse_log_getpriority(log) ((log)->flags & QSE_LOG_MASK_PRIORITY)
#endif


QSE_EXPORT void qse_log_report (
	qse_log_t*        log, 
	const qse_char_t* ident,
	int               pri,
	const qse_char_t* fmt,
	...
);

QSE_EXPORT void qse_log_reportv (
	qse_log_t*        log,
	const qse_char_t* ident,
	int               pri,
	const qse_char_t* fmt,
	va_list           ap
);

/**
 * The qse_get_log_priority_name() function returns the name of 
 * the first priority bit set.
 */
QSE_EXPORT const qse_char_t* qse_get_log_priority_name (
	int         pri
);

/**
 * The qse_make_log_priority_name() function composes a priority name
 * string representing all priority bits set */
QSE_EXPORT qse_size_t qse_make_log_priority_name (
	int               pri,
	const qse_char_t* delim,
	qse_char_t*       buf,
	qse_size_t        len /* length of the buffer, QSE_LOG_PRIORITY_LEN_MAX + 1 is enough */
);

/**
 * \return an integer bitwised-ORed of priority bits if \a name is valid. 
 *         0 if \a name is invalid or empty.
 */
QSE_EXPORT int qse_get_log_priority_by_mbsname (
	const qse_mchar_t* name,
	const qse_mchar_t* delim
);

QSE_EXPORT int qse_get_log_priority_by_wcsname (
	const qse_wchar_t* name,
	const qse_wchar_t* delim
);

QSE_EXPORT int qse_get_log_facility_by_mbsname (
	const qse_mchar_t*   name,
	qse_log_facility_t* fcode
);

QSE_EXPORT int qse_get_log_facility_by_wcsname (
	const qse_wchar_t*   name,
	qse_log_facility_t* fcode
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_get_log_priority_by_name qse_get_log_priority_by_mbsname
#	define qse_get_log_facility_by_name qse_get_log_facility_by_mbsname
#else
#	define qse_get_log_priority_by_name qse_get_log_priority_by_wcsname
#	define qse_get_log_facility_by_name qse_get_log_facility_by_wcsname
#endif

#ifdef __cplusplus
}
#endif

#endif
