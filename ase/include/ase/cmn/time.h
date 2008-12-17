/*
 * $Id$
 */

#ifndef _ASE_CMN_TIME_H_
#define _ASE_CMN_TIME_H_

#include <ase/types.h>
#include <ase/macros.h>

#define ASE_EPOCH_YEAR  ((ase_time_t)1970)
#define ASE_EPOCH_MON   ((ase_time_t)1)
#define ASE_EPOCH_DAY   ((ase_time_t)1)
#define ASE_EPOCH_WDAY  ((ase_time_t)4)

#define ASE_DAY_IN_WEEK  ((ase_time_t)7)
#define ASE_MON_IN_YEAR  ((ase_time_t)12)
#define ASE_HOUR_IN_DAY  ((ase_time_t)24)
#define ASE_MIN_IN_HOUR  ((ase_time_t)60)
#define ASE_MIN_IN_DAY   (ASE_MIN_IN_HOUR * ASE_HOUR_IN_DAY)
#define ASE_SEC_IN_MIN   ((ase_time_t)60)
#define ASE_SEC_IN_HOUR  (ASE_SEC_IN_MIN * ASE_MIN_IN_HOUR)
#define ASE_SEC_IN_DAY   (ASE_SEC_IN_MIN * ASE_MIN_IN_DAY)
#define ASE_MSEC_IN_SEC  ((ase_time_t)1000)
#define ASE_MSEC_IN_MIN  (ASE_MSEC_IN_SEC * ASE_SEC_IN_MIN)
#define ASE_MSEC_IN_HOUR (ASE_MSEC_IN_SEC * ASE_SEC_IN_HOUR)
#define ASE_MSEC_IN_DAY  (ASE_MSEC_IN_SEC * ASE_SEC_IN_DAY)

#define ASE_USEC_IN_MSEC ((ase_time_t)1000)
#define ASE_NSEC_IN_USEC ((ase_time_t)1000)
#define ASE_USEC_IN_SEC  ((ase_time_t)ASE_USEC_IN_MSEC * ASE_MSEC_IN_SEC)

/* number of milliseconds since the Epoch (00:00:00 UTC, Jan 1, 1970) */
typedef ase_long_t ase_time_t;

#ifdef __cplusplus
extern "C" {
#endif

int ase_gettime (ase_time_t* t);
int ase_settime (ase_time_t t);

#ifdef __cplusplus
}
#endif

#endif
