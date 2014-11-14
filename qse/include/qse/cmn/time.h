/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_CMN_TIME_H_
#define _QSE_CMN_TIME_H_

/** @file
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
	(((qse_long_t)(sec) * QSE_MSECS_PER_SEC) + ((qse_long_t)(nsec) / QSE_NSECS_PER_MSEC))

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

/**
 * The qse_ntime_t type defines a numeric time type expressed in the 
 *  number of milliseconds since the Epoch (00:00:00 UTC, Jan 1, 1970).
 */
typedef struct qse_ntime_t qse_ntime_t;
struct qse_ntime_t
{
	qse_long_t  sec;
	qse_int32_t nsec; /* nanoseconds */
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
	/*int offset;*/
};

#define qse_cleartime(x) ((x)->sec = (x)->nsec = 0);
#define qse_cmptime(x,y) \
	(((x)->sec == (y)->sec)? ((x)->nsec - (y)->nsec): \
	                         ((x)->sec -  (y)->sec))

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
 * The qse_addtime() function adds x and y and stores the result in z 
 */
QSE_EXPORT void qse_addtime (
	const qse_ntime_t* x,
	const qse_ntime_t* y,
	qse_ntime_t*       z
);

/**
 * The qse_subtime()  function subtract y from x and stores the result in z.
 */
QSE_EXPORT void qse_subtime (
	const qse_ntime_t* x,
	const qse_ntime_t* y,
	qse_ntime_t*       z
);

#if defined(__cplusplus)
}
#endif

#endif
