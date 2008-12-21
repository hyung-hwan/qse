/*
 * $Id$
 */

#ifndef _ASE_CMN_TIME_H_
#define _ASE_CMN_TIME_H_

#include <ase/types.h>
#include <ase/macros.h>

#define ASE_EPOCH_YEAR  ((ase_ntime_t)1970)
#define ASE_EPOCH_MON   ((ase_ntime_t)1)
#define ASE_EPOCH_DAY   ((ase_ntime_t)1)
#define ASE_EPOCH_WDAY  ((ase_ntime_t)4)

#define ASE_DAY_IN_WEEK  ((ase_ntime_t)7)
#define ASE_MON_IN_YEAR  ((ase_ntime_t)12)
#define ASE_HOUR_IN_DAY  ((ase_ntime_t)24)
#define ASE_MIN_IN_HOUR  ((ase_ntime_t)60)
#define ASE_MIN_IN_DAY   (ASE_MIN_IN_HOUR * ASE_HOUR_IN_DAY)
#define ASE_SEC_IN_MIN   ((ase_ntime_t)60)
#define ASE_SEC_IN_HOUR  (ASE_SEC_IN_MIN * ASE_MIN_IN_HOUR)
#define ASE_SEC_IN_DAY   (ASE_SEC_IN_MIN * ASE_MIN_IN_DAY)
#define ASE_MSEC_IN_SEC  ((ase_ntime_t)1000)
#define ASE_MSEC_IN_MIN  (ASE_MSEC_IN_SEC * ASE_SEC_IN_MIN)
#define ASE_MSEC_IN_HOUR (ASE_MSEC_IN_SEC * ASE_SEC_IN_HOUR)
#define ASE_MSEC_IN_DAY  (ASE_MSEC_IN_SEC * ASE_SEC_IN_DAY)

#define ASE_USEC_IN_MSEC ((ase_ntime_t)1000)
#define ASE_NSEC_IN_USEC ((ase_ntime_t)1000)
#define ASE_USEC_IN_SEC  ((ase_ntime_t)ASE_USEC_IN_MSEC * ASE_MSEC_IN_SEC)

#define ASE_IS_LEAPYEAR(year) (!((year)%4) && (((year)%100) || !((year)%400)))
#define ASE_DAY_IN_YEAR(year) (ASE_IS_LEAPYEAR(year)? 366: 365)

/* number of milliseconds since the Epoch (00:00:00 UTC, Jan 1, 1970) */
typedef ase_long_t ase_ntime_t;
typedef struct ase_btime_t ase_btime_t;

struct ase_btime_t
{
	int sec;  /* 0-61 */
	int min;  /* 0-59 */
	int hour; /* 0-23 */
	int mday; /* 1-31 */
	int mon;  /* 0(jan)-11(dec) */
	int year; /* the number of years since 1900 */
	int wday; /* 0(sun)-6(sat) */
	int yday; /* 0(jan 1) to 365 */
	int isdst;
};

#ifdef __cplusplus
extern "C" {
#endif

/****f* ase.cmn/ase_gettime
 * NAME
 *  ase_gettime - get the current time
 *
 * SYNPOSIS
 */
int ase_gettime (
	ase_ntime_t* nt
);
/******/

/****f* ase.cmn/ase_settime
 * NAME
 *  ase_settime - set the current time
 *
 * SYNOPSIS
 */
int ase_settime (
	ase_ntime_t nt
);
/******/


/****f* ase.cmn/ase_gmtime
 * NAME
 *  ase_gmtime - convert numeric time to broken-down time 
 *
 * SYNOPSIS
 */
void ase_gmtime (
	ase_ntime_t  nt, 
	ase_btime_t* bt
);
/******/

/****f* ase.cmn/ase_localtime
 * NAME
 *  ase_localtime - convert numeric time to broken-down time 
 *
 * SYNOPSIS
 */
int ase_localtime (
	ase_ntime_t  nt, 
	ase_btime_t* bt
); 
/******/

/****f* ase.cmn/ase_mktime
 * NAME
 *  ase_mktime - convert broken-down time to numeric time
 *
 * SYNOPSIS
 */
int ase_mktime (
	const ase_btime_t* bt,
	ase_ntime_t*       nt
);
/******/

/****f* ase.cmn/ase_strftime
 * NAME
 *  ase_strftime - format time
 *
 * SYNOPSIS
 */
ase_size_t ase_strftime (
        ase_char_t*       buf, 
	ase_size_t        size, 
	const ase_char_t* fmt,
	ase_btime_t*      bt
);
/******/

#ifdef __cplusplus
}
#endif

#endif
