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

#ifndef _QSE_CMN_TIME_H_
#define _QSE_CMN_TIME_H_

/** \file
 * This file provides time manipulation functions.
 */

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_EPOCH_YEAR  (1970)
#define QSE_EPOCH_MON   (1)
#define QSE_EPOCH_DAY   (1)
#define QSE_EPOCH_WDAY  (4)

/* windows specific epoch time */
#define QSE_EPOCH_YEAR_WIN   (1601)
#define QSE_EPOCH_MON_WIN    (1)
#define QSE_EPOCH_DAY_WIN    (1)

#define QSE_BTIME_YEAR_BASE (1900)

#define QSE_DAYS_PER_NORMYEAR  (365)
#define QSE_DAYS_PER_LEAPYEAR  (366)

#define QSE_DAYS_PER_WEEK  (7)
#define QSE_MONS_PER_YEAR  (12)
#define QSE_HOURS_PER_DAY  (24)
#define QSE_MINS_PER_HOUR  (60)
#define QSE_MINS_PER_DAY   (QSE_MINS_PER_HOUR*QSE_HOURS_PER_DAY)
#define QSE_SECS_PER_MIN   (60)
#define QSE_SECS_PER_HOUR  (QSE_SECS_PER_MIN*QSE_MINS_PER_HOUR)
#define QSE_SECS_PER_DAY   (QSE_SECS_PER_MIN*QSE_MINS_PER_DAY)
#define QSE_MSECS_PER_SEC  (1000)
#define QSE_MSECS_PER_MIN  (QSE_MSECS_PER_SEC*QSE_SECS_PER_MIN)
#define QSE_MSECS_PER_HOUR (QSE_MSECS_PER_SEC*QSE_SECS_PER_HOUR)
#define QSE_MSECS_PER_DAY  (QSE_MSECS_PER_SEC*QSE_SECS_PER_DAY)

#define QSE_USECS_PER_MSEC (1000)
#define QSE_NSECS_PER_USEC (1000)
#define QSE_NSECS_PER_MSEC (QSE_NSECS_PER_USEC*QSE_USECS_PER_MSEC)
#define QSE_USECS_PER_SEC  (QSE_USECS_PER_MSEC*QSE_MSECS_PER_SEC)
#define QSE_NSECS_PER_SEC  (QSE_NSECS_PER_USEC*QSE_USECS_PER_MSEC*QSE_MSECS_PER_SEC)

#define QSE_IS_LEAPYEAR(year) ((!((year)%4) && ((year)%100)) || !((year)%400))
#define QSE_DAYS_PER_YEAR(year) \
	(QSE_IS_LEAPYEAR(year)? QSE_DAYS_PER_LEAPYEAR: QSE_DAYS_PER_NORMYEAR)

#define QSE_SECNSEC_TO_MSEC(sec,nsec) \
	(((qse_ntime_sec_t)(sec) * QSE_MSECS_PER_SEC) + ((qse_ntime_sec_t)(nsec) / QSE_NSECS_PER_MSEC))

#define QSE_SECNSEC_TO_USEC(sec,nsec) \
	(((qse_ntime_sec_t)(sec) * QSE_USECS_PER_SEC) + ((qse_ntime_sec_t)(nsec) / QSE_NSECS_PER_USEC))

#define QSE_SEC_TO_MSEC(sec) ((sec) * QSE_MSECS_PER_SEC)
#define QSE_MSEC_TO_SEC(sec) ((sec) / QSE_MSECS_PER_SEC)

#define QSE_USEC_TO_NSEC(usec) ((usec) * QSE_NSECS_PER_USEC)
#define QSE_NSEC_TO_USEC(nsec) ((nsec) / QSE_NSECS_PER_USEC)

#define QSE_MSEC_TO_NSEC(msec) ((msec) * QSE_NSECS_PER_MSEC)
#define QSE_NSEC_TO_MSEC(nsec) ((nsec) / QSE_NSECS_PER_MSEC)

#define QSE_SEC_TO_NSEC(sec) ((sec) * QSE_NSECS_PER_SEC)
#define QSE_NSEC_TO_SEC(nsec) ((nsec) / QSE_NSECS_PER_SEC)

#define QSE_SEC_TO_USEC(sec) ((sec) * QSE_USECS_PER_SEC)
#define QSE_USEC_TO_SEC(usec) ((usec) / QSE_USECS_PER_SEC)

#if defined(QSE_SIZEOF_INT64_T) && (QSE_SIZEOF_INT64_T > 0)
typedef qse_int64_t qse_ntime_sec_t;
#else
typedef qse_int32_t qse_ntime_sec_t;
#endif
typedef qse_int32_t qse_ntime_nsec_t;

/**
 * The qse_ntime_t type defines a numeric time type expressed in the 
 *  number of milliseconds since the Epoch (00:00:00 UTC, Jan 1, 1970).
 */
typedef struct qse_ntime_t qse_ntime_t;
struct qse_ntime_t
{
	qse_ntime_sec_t  sec;
	qse_ntime_nsec_t nsec; /* nanoseconds */
};

typedef struct qse_btime_t qse_btime_t;
struct qse_btime_t
{
	int sec;  /* 0-61 */
	int min;  /* 0-59 */
	int hour; /* 0-23 */
	int mday; /* 1-31 */
	int mon;  /* 0(jan)-11(dec) */
	int year; /* the number of years since QSE_BTIME_YEAR_BASE */
	int wday; /* 0(sun)-6(sat) */
	int yday; /* 0(jan 1) to 365 */
	int isdst; /* -1(unknown), 0(not in effect), 1 (in effect) */
	int gmtoff;
};

/* number of milliseconds since the Epoch (00:00:00 UTC, Jan 1, 1970) */
typedef qse_long_t qse_mtime_t;

#if defined(QSE_HAVE_INLINE)
	static QSE_INLINE void qse_init_ntime(qse_ntime_t* x, qse_ntime_sec_t s, qse_ntime_nsec_t nsec)
	{
		x->sec = s;
		x->nsec = nsec;
	}
	static QSE_INLINE void qse_clear_ntime(qse_ntime_t* x) { qse_init_ntime (x, 0, 0); }
	/*static QSE_INLINE int qse_cmp_ntime(const qse_ntime_t* x, const qse_ntime_t* y)
	{
		// TODO: fix the type and value range issue and replace the macro below.
		return (x->sec == y->sec)? (x->nsec - y->nsec): (x->sec - y->sec);
	}*/
#	define qse_cmp_ntime(x,y) (((x)->sec == (y)->sec)? ((x)->nsec - (y)->nsec): ((x)->sec - (y)->sec))

	static QSE_INLINE int qse_is_neg_ntime(qse_ntime_t* x) { return x->sec < 0; }
	static QSE_INLINE int qse_is_pos_ntime(qse_ntime_t* x) { return x->sec > 0 || (x->sec == 0 && x->nsec > 0); }
	static QSE_INLINE int qse_is_zero_ntime(qse_ntime_t* x) { return x->sec == 0 && x->nsec == 0; }
#else
#	define qse_init_ntime(x,s,ns) (((x)->sec = (s)), ((x)->nsec = (ns)))
#	define qse_clear_ntime(x) qse_init_ntime(x,0,0)
#	define qse_cmp_ntime(x,y) (((x)->sec == (y)->sec)? ((x)->nsec - (y)->nsec): ((x)->sec - (y)->sec))

/* if time has been normalized properly, nsec must be equal to or
 * greater than 0. */
#	define qse_is_neg_ntime(x) ((x)->sec < 0)
#	define qse_is_pos_ntime(x) ((x)->sec > 0 || ((x)->sec == 0 && (x)->nsec > 0))
#	define qse_is_zero_ntime(x) ((x)->sec == 0 && (x)->nsec == 0)
#endif


#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_gettime() function gets the current time.
 */
QSE_EXPORT int qse_gettime (
	qse_ntime_t* nt
);

/**
 * The qse_getmtime() function gets the current time in milliseconds.
 */
QSE_EXPORT int qse_getmtime (
	qse_mtime_t* mt
);

/**
 * The qse_settime() function sets the current time.
 */
QSE_EXPORT int qse_settime (
	const qse_ntime_t* nt
);

/**
 * The qse_gmtime() function converts numeric time to broken-down time.
 */
QSE_EXPORT int qse_gmtime (
	const qse_ntime_t*  nt, 
	qse_btime_t*        bt
);

/**
 * The qse_localtime() function converts numeric time to broken-down time 
 */
QSE_EXPORT int qse_localtime (
	const qse_ntime_t*  nt, 
	qse_btime_t*        bt
); 

/**
 * The qse_timegm() function converts broken-down time to numeric time. It is 
 * the inverse of qse_gmtime(). It is useful if the broken-down time is in UTC
 * and the local environment is not.
 */
QSE_EXPORT int qse_timegm (
	const qse_btime_t* bt,
	qse_ntime_t*       nt
);

/**
 * The qse_timelocal() converts broken-down time to numeric time. It is the
 * inverse of qse_localtime();
 */
QSE_EXPORT int qse_timelocal (
	const qse_btime_t* bt,
	qse_ntime_t*       nt
);

/**
 * The qse_add_time() function adds x and y and stores the result in z 
 */
QSE_EXPORT void qse_add_ntime (
	qse_ntime_t*       z,
	const qse_ntime_t* x,
	const qse_ntime_t* y
);

/**
 * The qse_sub_time() function subtract y from x and stores the result in z.
 */
QSE_EXPORT void qse_sub_ntime (
	qse_ntime_t*       z,
	const qse_ntime_t* x,
	const qse_ntime_t* y
);


/**
 * The qse_mbs_to_ntime() function converts a numeric text to the numeric time.
 *  seconds.nanoseconds
 *  10.231
 */
QSE_EXPORT int qse_mbs_to_ntime (
	const qse_mchar_t* text,
	qse_ntime_t*       ntime
);

QSE_EXPORT int qse_wcs_to_ntime (
	const qse_wchar_t* text,
	qse_ntime_t*       ntime
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_str_to_ntime(text,ntime) qse_mbs_to_ntime(text,ntime)
#else
#	define qse_str_to_ntime(text,ntime) qse_wcs_to_ntime(text,ntime)
#endif


#if defined(__cplusplus)
}
#endif

#endif
