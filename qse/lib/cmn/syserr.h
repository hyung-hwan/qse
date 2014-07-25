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

/* 
 * this file is intended for internal use only.
 * you can include this file and use IMPLEMENT_SYSERR_TO_ERRNUM.
 *
 *   #include "syserr.h"
 *   IMPLEMENT_SYSERR_TO_ERRNUM (pio, PIO)
 *
 * header files defining system error codes must be included
 * before this file.
 */

#define __SYSERRTYPE__(obj) qse_ ## obj ## _errnum_t
#define __SYSERRNUM__(obj,num) (QSE_ ## obj ## _ ## num)

#if defined(_WIN32)

	#define IMPLEMENT_SYSERR_TO_ERRNUM(obj1,obj2) \
	static __SYSERRTYPE__(obj1) syserr_to_errnum (DWORD e) \
	{ \
		switch (e) \
		{ \
			case ERROR_NOT_ENOUGH_MEMORY: \
			case ERROR_OUTOFMEMORY: \
				return __SYSERRNUM__ (obj2, ENOMEM); \
			case ERROR_INVALID_PARAMETER: \
			case ERROR_INVALID_HANDLE: \
			case ERROR_INVALID_NAME: \
				return __SYSERRNUM__ (obj2, EINVAL); \
			case ERROR_ACCESS_DENIED: \
			case ERROR_SHARING_VIOLATION: \
				return __SYSERRNUM__ (obj2, EACCES); \
			case ERROR_FILE_NOT_FOUND: \
			case ERROR_PATH_NOT_FOUND: \
				return __SYSERRNUM__ (obj2, ENOENT); \
			case ERROR_ALREADY_EXISTS: \
			case ERROR_FILE_EXISTS: \
				return __SYSERRNUM__ (obj2, EEXIST); \
			case ERROR_BROKEN_PIPE: \
				return __SYSERRNUM__ (obj2, EPIPE); \
			default: \
				return __SYSERRNUM__ (obj2, ESYSERR); \
		} \
	}

#elif defined(__OS2__)

	#define IMPLEMENT_SYSERR_TO_ERRNUM(obj1,obj2) \
	static __SYSERRTYPE__(obj1) syserr_to_errnum (APIRET e) \
	{ \
		switch (e) \
		{ \
			case ERROR_NOT_ENOUGH_MEMORY: \
				return __SYSERRNUM__ (obj2, ENOMEM); \
			case ERROR_INVALID_PARAMETER: \
			case ERROR_INVALID_HANDLE: \
			case ERROR_INVALID_NAME: \
				return __SYSERRNUM__ (obj2, EINVAL); \
			case ERROR_ACCESS_DENIED: \
			case ERROR_SHARING_VIOLATION:  \
				return __SYSERRNUM__ (obj2, EACCES); \
			case ERROR_FILE_NOT_FOUND: \
			case ERROR_PATH_NOT_FOUND: \
				return __SYSERRNUM__ (obj2, ENOENT); \
			case ERROR_ALREADY_EXISTS: \
				return __SYSERRNUM__ (obj2, EEXIST); \
			case ERROR_BROKEN_PIPE: \
				return __SYSERRNUM__ (obj2, EPIPE); \
			default: \
				return __SYSERRNUM__ (obj2, ESYSERR); \
		} \
	}

#elif defined(__DOS__) 

	#define IMPLEMENT_SYSERR_TO_ERRNUM(obj1,obj2) \
	static __SYSERRTYPE__(obj1) syserr_to_errnum (int e) \
	{ \
		switch (e) \
		{ \
			case ENOMEM: return __SYSERRNUM__ (obj2, ENOMEM); \
			case EINVAL: return __SYSERRNUM__ (obj2, EINVAL); \
			case EACCES: return __SYSERRNUM__ (obj2, EACCES); \
			case ENOENT: return __SYSERRNUM__ (obj2, ENOENT); \
			case EEXIST: return __SYSERRNUM__ (obj2, EEXIST); \
			default:     return __SYSERRNUM__ (obj2, ESYSERR); \
		} \
	}

#elif defined(vms) || defined(__vms)

	/* TODO: */
	#define IMPLEMENT_SYSERR_TO_ERRNUM(obj1,obj2) \
	static __SYSERRTYPE__(obj1) syserr_to_errnum (unsigned long e) \
	{ \
		switch (e) \
		{ \
			case RMS$_NORMAL: return __SYSERRNUM__ (obj2, ENOERR); \
			default:          return __SYSERRNUM__ (obj2, ESYSERR); \
		} \
	}

#else

	#if defined(EWOULDBLOCK) && defined(EAGAIN) && (EWOULDBLOCK != EAGAIN)

	#define IMPLEMENT_SYSERR_TO_ERRNUM(obj1,obj2) \
	static __SYSERRTYPE__(obj1) syserr_to_errnum (int e) \
	{ \
		switch (e) \
		{ \
			case ENOMEM: return __SYSERRNUM__ (obj2, ENOMEM); \
			case EINVAL: return __SYSERRNUM__ (obj2, EINVAL); \
			case EBUSY: \
			case EACCES: return __SYSERRNUM__ (obj2, EACCES); \
			case ENOTDIR: \
			case ENOENT: return __SYSERRNUM__ (obj2, ENOENT); \
			case EEXIST: return __SYSERRNUM__ (obj2, EEXIST); \
			case EINTR:  return __SYSERRNUM__ (obj2, EINTR); \
			case EPIPE:  return __SYSERRNUM__ (obj2, EPIPE); \
			case EWOULDBLOCK: \
			case EAGAIN: return __SYSERRNUM__ (obj2, EAGAIN); \
			default:     return __SYSERRNUM__ (obj2, ESYSERR); \
		} \
	}

	#else

	#define IMPLEMENT_SYSERR_TO_ERRNUM(obj1,obj2) \
	static __SYSERRTYPE__(obj1) syserr_to_errnum (int e) \
	{ \
		switch (e) \
		{ \
			case ENOMEM: return __SYSERRNUM__ (obj2, ENOMEM); \
			case EINVAL: return __SYSERRNUM__ (obj2, EINVAL); \
			case EBUSY: \
			case EACCES: return __SYSERRNUM__ (obj2, EACCES); \
			case ENOTDIR: \
			case ENOENT: return __SYSERRNUM__ (obj2, ENOENT); \
			case EEXIST: return __SYSERRNUM__ (obj2, EEXIST); \
			case EINTR:  return __SYSERRNUM__ (obj2, EINTR); \
			case EPIPE:  return __SYSERRNUM__ (obj2, EPIPE); \
			case EAGAIN: return __SYSERRNUM__ (obj2, EAGAIN); \
			default:     return __SYSERRNUM__ (obj2, ESYSERR); \
		} \
	}

	#endif

#endif
