/*
 * $Id$
 */

#include <ase/cmn/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#if defined(ASE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>
#endif

int ase_gettime (ase_time_t* t)
{
#ifdef _WIN32
	SYSTEMTIME st;
	FILETIME ft;

	/* 
	 * MSDN: The FILETIME structure is a 64-bit value representing the 
	 *       number of 100-nanosecond intervals since January 1, 1601 (UTC).
	 */

	GetSystemTime (&st);
	if (SystemTimeToFileTime (&st, &ft) == FALSE) return -1;
	*t = ((ase_time_t)(*((ase_int64_t*)&ft)) / (10 * 1000));
	*t -= EPOCH_DIFF_MSECS;
	return 0;
#else
	struct timeval tv;
	int n;

#ifdef SYS_gettimeofday
	n = syscall (SYS_gettimeofday, &tv, ASE_NULL);
#else
	n = gettimeofday (&tv, ASE_NULL);
#endif
	if (n == -1) return -1;

	*t = tv.tv_sec * ASE_MSEC_IN_SEC + tv.tv_usec / ASE_USEC_IN_MSEC;
	return 0;
#endif
}

int ase_settime (ase_time_t t)
{
#ifdef _WIN32
	FILETIME ft;
	SYSTEMTIME st;

	*((ase_int64_t*)&ft) = ((value + EPOCH_DIFF_MSECS) * (10 * 1000));
	if (FileTimeToSystemTime (&ft, &st) == FALSE) return -1;
	if (SetSystemTime(&st) == FALSE) return -1;
	return 0;
#else
	struct timeval tv;
	int n;

	tv.tv_sec = t / ASE_MSEC_IN_SEC;
	tv.tv_usec = (t % ASE_MSEC_IN_SEC) * ASE_USEC_IN_MSEC;

#ifdef SYS_settimeofday
	n = syscall (SYS_settimeofday, &tv, ASE_NULL);
#else
	n = settimeofday (&tv, ASE_NULL);
#endif
	if (n == -1) return -1;
	return 0;
#endif
}

