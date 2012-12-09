/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include <qse/cmn/fio.h>
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/alg.h>
#include <qse/cmn/time.h>
#include <qse/cmn/mbwc.h>
#include "mem.h"

#if defined(_WIN32)
#	include <windows.h>
/*#	include <psapi.h>*/ /* for GetMappedFileName(). but dynamically loaded */
#	include <tchar.h>
#elif defined(__OS2__)
#	define INCL_DOSFILEMGR
#	define INCL_DOSERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <io.h>
#	include <fcntl.h>
#	include <errno.h>
#elif defined(vms) || defined(__vms)
#	define __NEW_STARLET 1
#	include <starlet.h>
#	include <rms.h>
#else
#	include "syscall.h"
#endif

/* internal status codes */
enum
{
	STATUS_APPEND  = (1 << 0),
	STATUS_NOCLOSE = (1 << 1)	
};

#if defined(_WIN32)
static qse_fio_errnum_t syserr_to_errnum (DWORD e)
{

	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return QSE_FIO_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_FIO_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_FIO_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_FIO_ENOENT;

		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return QSE_FIO_EEXIST;

		default:
			return QSE_FIO_ESYSERR;
	}
}
#elif defined(__OS2__)
static qse_fio_errnum_t syserr_to_errnum (APIRET e)
{
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
			return QSE_FIO_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_FIO_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_FIO_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_FIO_ENOENT;

		case ERROR_ALREADY_EXISTS:
			return QSE_FIO_EEXIST;

		default:
			return QSE_FIO_ESYSERR;
	}
}
#elif defined(__DOS__)
static qse_fio_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_FIO_ENOMEM;

		case EINVAL:
			return QSE_FIO_EINVAL;

		case EACCES:
			return QSE_FIO_EACCES;

		case ENOENT:
			return QSE_FIO_ENOENT;

		case EEXIST:
			return QSE_FIO_EEXIST;
	
		default:
			return QSE_FIO_ESYSERR;
	}
}
#elif defined(vms) || defined(__vms)
static qse_fio_errnum_t syserr_to_errnum (unsigned long e)
{
	switch (e)
	{
		case RMS$_NORMAL:
			return QSE_FIO_ENOERR;
		
		/* TODO: add more */

		default:
			return QSE_FIO_ESYSERR;
	}
}
#else
static qse_fio_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_FIO_ENOMEM;

		case EINVAL:
			return QSE_FIO_EINVAL;

		case ENOENT:
			return QSE_FIO_ENOENT;

		case EACCES:
			return QSE_FIO_EACCES;

		case EEXIST:
			return QSE_FIO_EEXIST;
	
		case EINTR:
			return QSE_FIO_EINTR;

		default:
			return QSE_FIO_ESYSERR;
	}
}
#endif

qse_fio_t* qse_fio_open (
	qse_mmgr_t* mmgr, qse_size_t ext,
	const qse_char_t* path, int flags, int mode)
{
	qse_fio_t* fio;

	fio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_fio_t) + ext);
	if (fio == QSE_NULL) return QSE_NULL;

	if (qse_fio_init (fio, mmgr, path, flags, mode) <= -1)
	{
		QSE_MMGR_FREE (mmgr, fio);
		return QSE_NULL;
	}

	return fio;
}

void qse_fio_close (qse_fio_t* fio)
{
	qse_fio_fini (fio);
	QSE_MMGR_FREE (fio->mmgr, fio);
}

int qse_fio_init (
	qse_fio_t* fio, qse_mmgr_t* mmgr,
	const qse_char_t* path, int flags, int mode)
{
	qse_fio_hnd_t handle;

	qse_uint32_t temp_no;
	qse_char_t* temp_ptr;
	qse_size_t temp_tries;

#if defined(_WIN32)
	int fellback = 0;
#endif

	QSE_MEMSET (fio, 0, QSE_SIZEOF(*fio));
	fio->mmgr = mmgr;

	if (!(flags & (QSE_FIO_READ | QSE_FIO_WRITE | QSE_FIO_APPEND | QSE_FIO_HANDLE)))
	{
		/* one of QSE_FIO_READ, QSE_FIO_WRITE, QSE_FIO_APPEND, 
		 * and QSE_FIO_HANDLE must be specified */
		fio->errnum = QSE_FIO_EINVAL;
		return -1;
	}

	/* Store some flags for later use */
	if (flags & QSE_FIO_NOCLOSE) 
		fio->status |= STATUS_NOCLOSE;

	if (flags & QSE_FIO_TEMPORARY)
	{
		qse_ntime_t now;

		if (flags & (QSE_FIO_HANDLE | QSE_FIO_MBSPATH))
		{
			/* QSE_FIO_TEMPORARY and QSE_FIO_HANDLE/QSE_FIO_MBSPATH 
			 * are mutually exclusive */
			fio->errnum = QSE_FIO_EINVAL;
			return -1;
		}

		temp_no = 0;
		/* if QSE_FIO_TEMPORARY is used, the path name must 
		 * be writable. */
		for (temp_ptr = (qse_char_t*)path; *temp_ptr; temp_ptr++) 
			temp_no += *temp_ptr;

		/* The path name template must be at least 4 characters long
		 * excluding the terminating null. this function fails if not */
		if (temp_ptr - path < 4) 
		{
			fio->errnum = QSE_FIO_EINVAL;
			return -1; 
		}

		qse_gettime (&now);
		temp_no += (now.sec & 0xFFFFFFFFlu);

		temp_tries = 0;
		temp_ptr -= 4;

	retry_temporary:
		temp_tries++;

		/* Fails after 5000 tries. 5000 randomly chosen */
		if (temp_tries > 5000) 
		{
			fio->errnum = QSE_FIO_EINVAL;
			return -1; 
		}

		/* Generate the next random number to use to make a 
		 * new path name */
		temp_no = qse_rand31 (temp_no);

		/* 
		 * You must not pass a constant string for a path name
		 * when QSE_FIO_TEMPORARY is set, because it changes
		 * the path name with a random number generated
		 */
		qse_fmtuintmax (
			temp_ptr,
			4, 
			temp_no % 0x10000, 
			16 | QSE_FMTUINTMAX_NOTRUNC | QSE_FMTUINTMAX_NONULL,
			4,
			QSE_T('\0'),
			QSE_NULL
		);
	}

#if defined(_WIN32)
	if (flags & QSE_FIO_HANDLE)
	{
		handle = *(qse_fio_hnd_t*)path;
		QSE_ASSERTX (
			handle != INVALID_HANDLE_VALUE,
			"Do not specify an invalid handle value"
		);
	}
	else
	{
		DWORD desired_access = 0;
		DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		DWORD creation_disposition = 0;
		DWORD flag_and_attr = FILE_ATTRIBUTE_NORMAL;

		if (fellback) share_mode &= ~FILE_SHARE_DELETE;

		if (flags & QSE_FIO_APPEND)
		{
			if (fellback)
			{
				desired_access |= GENERIC_WRITE;
			}
			else
			{
				/* this is not officially documented for CreateFile.
				 * ZwCreateFile (kernel) seems to document it */
				fio->status &= ~STATUS_APPEND;
				desired_access |= FILE_APPEND_DATA;
			}
		}
		else if (flags & QSE_FIO_WRITE)
		{
			/* In WIN32, FILE_APPEND_DATA and GENERIC_WRITE can't
			 * be used together */
			desired_access |= GENERIC_WRITE;
		}
		if (flags & QSE_FIO_READ) desired_access |= GENERIC_READ;

		if (flags & QSE_FIO_CREATE)
		{
			creation_disposition =
				(flags & QSE_FIO_EXCLUSIVE)? CREATE_NEW:
				(flags & QSE_FIO_TRUNCATE)? CREATE_ALWAYS: OPEN_ALWAYS;
		}
		else if (flags & QSE_FIO_TRUNCATE)
		{
			creation_disposition = TRUNCATE_EXISTING;
		}
		else creation_disposition = OPEN_EXISTING;

		if (flags & QSE_FIO_NOSHREAD)
			share_mode &= ~FILE_SHARE_READ;
		if (flags & QSE_FIO_NOSHWRITE)
			share_mode &= ~FILE_SHARE_WRITE;
		if (flags & QSE_FIO_NOSHDELETE)
			share_mode &= ~FILE_SHARE_DELETE;

		if (!(mode & QSE_FIO_WUSR)) 
			flag_and_attr = FILE_ATTRIBUTE_READONLY;
		if (flags & QSE_FIO_SYNC) 
			flag_and_attr |= FILE_FLAG_WRITE_THROUGH;

		if (flags & QSE_FIO_NOFOLLOW)
			flag_and_attr |= FILE_FLAG_OPEN_REPARSE_POINT;

		/* these two are just hints to OS */
		if (flags & QSE_FIO_RANDOM) 
			flag_and_attr |= FILE_FLAG_RANDOM_ACCESS;
		if (flags & QSE_FIO_SEQUENTIAL) 
			flag_and_attr |= FILE_FLAG_SEQUENTIAL_SCAN;

		if (flags & QSE_FIO_MBSPATH)
		{
			handle = CreateFileA (
				(const qse_mchar_t*)path, desired_access, share_mode, 
				QSE_NULL, /* set noinherit by setting no secattr */
				creation_disposition, flag_and_attr, 0
			);
		}
		else
		{
			handle = CreateFile (
				path, desired_access, share_mode, 
				QSE_NULL, /* set noinherit by setting no secattr */
				creation_disposition, flag_and_attr, 0
			);
		}
		if (handle == INVALID_HANDLE_VALUE) 
		{
			DWORD e = GetLastError();	
			if (!fellback && e == ERROR_INVALID_PARAMETER && 
			    ((share_mode & FILE_SHARE_DELETE) || (flags & QSE_FIO_APPEND)))
			{
				/* old windows fails with ERROR_INVALID_PARAMETER
				 * when some advanced flags are used. so try again
				 * with fallback flags */
				fellback = 1;

				share_mode &= ~FILE_SHARE_DELETE;
				if (flags & QSE_FIO_APPEND)
				{
					fio->status |= STATUS_APPEND;
					desired_access &= ~FILE_APPEND_DATA;
					desired_access |= GENERIC_WRITE;
				}
			
				if (flags & QSE_FIO_MBSPATH)
				{
					handle = CreateFileA (
						(const qse_mchar_t*)path, desired_access, share_mode, 
						QSE_NULL, /* set noinherit by setting no secattr */
						creation_disposition, flag_and_attr, 0
					);
				}
				else
				{
					handle = CreateFile (
						path, desired_access, share_mode, 
						QSE_NULL, /* set noinherit by setting no secattr */
						creation_disposition, flag_and_attr, 0
					);
				}
				if (handle == INVALID_HANDLE_VALUE) 
				{
					if (flags & QSE_FIO_TEMPORARY) goto retry_temporary;
					fio->errnum = syserr_to_errnum(GetLastError());
					return -1;
				}
			}
			else
			{
				if (flags & QSE_FIO_TEMPORARY) goto retry_temporary;
				fio->errnum = syserr_to_errnum(e);
				return -1;
			}
		}
	}

	/* some special check */
#if 0
	if (GetFileType(handle) == FILE_TYPE_UNKNOWN)
	{
		fio->errnum = syserr_to_errnum(GetLastError());
		CloseHandle (handle);
		return -1;
	}
#endif

	/* TODO: support more features on WIN32 - TEMPORARY, DELETE_ON_CLOSE */

#elif defined(__OS2__)

	if (flags & QSE_FIO_HANDLE)
	{
		handle = *(qse_fio_hnd_t*)path;
	}
	else
	{
		APIRET ret;
		ULONG action_taken = 0;
		ULONG open_action, open_mode, open_attr;
	#if defined(FIL_STANDARDL)
		LONGLONG zero;
	#else
		ULONG zero;	
	#endif

	#if defined(QSE_CHAR_IS_MCHAR)
		const qse_mchar_t* path_mb = path;
	#else
		qse_mchar_t path_mb_buf[CCHMAXPATH];
		qse_mchar_t* path_mb;
		qse_size_t wl, ml;
		int px;

		if (flags & QSE_FIO_MBSPATH)
		{
			path_mb = (qse_mchar_t*)path;
		}
		else
		{
			path_mb = path_mb_buf;
			ml = QSE_COUNTOF(path_mb_buf);
			px = qse_wcstombs (path, &wl, path_mb, &ml);
			if (px == -2)
			{
				/* the static buffer is too small.
				 * dynamically allocate a buffer */
				path_mb = qse_wcstombsdup (path, QSE_NULL, mmgr);
				if (path_mb == QSE_NULL) 
				{
					fio->errnum = QSE_FIO_ENOMEM;
					return -1;
				}
			}
			else if (px <= -1) 
			{
				fio->errnum = QSE_FIO_EINVAL;
				return -1;
			}
		}
	#endif

	#if defined(FIL_STANDARDL)
		zero.ulLo = 0;
		zero.ulHi = 0;
	#else
		zero = 0;
	#endif

		if (flags & QSE_FIO_APPEND) 
			fio->status |= STATUS_APPEND;

		if (flags & QSE_FIO_CREATE)
		{
			if (flags & QSE_FIO_EXCLUSIVE)
			{
				open_action = OPEN_ACTION_FAIL_IF_EXISTS | 
				              OPEN_ACTION_CREATE_IF_NEW;
			}
			else if (flags & QSE_FIO_TRUNCATE)
			{
				open_action = OPEN_ACTION_REPLACE_IF_EXISTS |
					      OPEN_ACTION_CREATE_IF_NEW;
			}
			else
			{
				open_action = OPEN_ACTION_CREATE_IF_NEW | 
				              OPEN_ACTION_OPEN_IF_EXISTS;
			}
		}
		else if (flags & QSE_FIO_TRUNCATE)
		{
			open_action = OPEN_ACTION_REPLACE_IF_EXISTS | 
			              OPEN_ACTION_FAIL_IF_NEW;
		}
		else 
		{
			open_action = OPEN_ACTION_OPEN_IF_EXISTS |
			              OPEN_ACTION_FAIL_IF_NEW;
		}

		open_mode = OPEN_FLAGS_NOINHERIT;

		if (flags & QSE_FIO_SYNC) 
			open_mode |= OPEN_FLAGS_WRITE_THROUGH;

		if ((flags & QSE_FIO_NOSHREAD) && (flags & QSE_FIO_NOSHWRITE))
			open_mode |= OPEN_SHARE_DENYREADWRITE;
		else if (flags & QSE_FIO_NOSHREAD)
			open_mode |= OPEN_SHARE_DENYREAD;
		else if (flags & QSE_FIO_NOSHWRITE)
			open_mode |= OPEN_SHARE_DENYWRITE;
		else
			open_mode |= OPEN_SHARE_DENYNONE;

		if ((flags & QSE_FIO_READ) &&
		    (flags & QSE_FIO_WRITE)) open_mode |= OPEN_ACCESS_READWRITE;
		else if (flags & QSE_FIO_READ) open_mode |= OPEN_ACCESS_READONLY;
		else if (flags & QSE_FIO_WRITE) open_mode |= OPEN_ACCESS_WRITEONLY;

		open_attr = (mode & QSE_FIO_WUSR)? FILE_NORMAL: FILE_READONLY;
		
	#if defined(FIL_STANDARDL)
		ret = DosOpenL (
	#else
		ret = DosOpen (
	#endif
			path_mb,       /* file name */
			&handle,       /* file handle */
			&action_taken, /* store action taken */
			zero,          /* size */
			open_attr,     /* attribute */
			open_action,   /* action if it exists */
			open_mode,     /* open mode */
			0L                            
		);

	#if defined(QSE_CHAR_IS_MCHAR)
		/* nothing to do */
	#else
		if (path_mb != path_mb_buf) QSE_MMGR_FREE (mmgr, path_mb);
	#endif

		if (ret != NO_ERROR) 
		{
			if (flags & QSE_FIO_TEMPORARY) goto retry_temporary;
			fio->errnum = syserr_to_errnum(ret);
			return -1;
		}
	}

#elif defined(__DOS__)

	if (flags & QSE_FIO_HANDLE)
	{
		handle = *(qse_fio_hnd_t*)path;
		QSE_ASSERTX (
			handle >= 0,
			"Do not specify an invalid handle value"
		);
	}
	else
	{
		int oflags = 0;
		int permission = 0;

	#if defined(QSE_CHAR_IS_MCHAR)
		const qse_mchar_t* path_mb = path;
	#else
		qse_mchar_t path_mb_buf[_MAX_PATH];
		qse_mchar_t* path_mb;
		qse_size_t wl, ml;
		int px;

		path_mb = path_mb_buf;
		ml = QSE_COUNTOF(path_mb_buf);
		px = qse_wcstombs (path, &wl, path_mb, &ml);
		if (px == -2)
		{
			path_mb = qse_wcstombsdup (path, QSE_NULL, mmgr);
			if (path_mb == QSE_NULL) 
			{
				fio->errnum = QSE_FIO_ENOMEM;
				return -1;
			}
		}
		else if (px <= -1) 
		{
			fio->errnum = QSE_FIO_EINVAL;
			return -1;
		}
	#endif

		if (flags & QSE_FIO_APPEND)
		{
			if ((flags & QSE_FIO_READ)) oflags |= O_RDWR;
			else oflags |= O_WRONLY;
			oflags |= O_APPEND;
		}
		else
		{
			if ((flags & QSE_FIO_READ) &&
			    (flags & QSE_FIO_WRITE)) oflags |= O_RDWR;
			else if (flags & QSE_FIO_READ) oflags |= O_RDONLY;
			else if (flags & QSE_FIO_WRITE) oflags |= O_WRONLY;
		}

		if (flags & QSE_FIO_CREATE) oflags |= O_CREAT;
		if (flags & QSE_FIO_TRUNCATE) oflags |= O_TRUNC;
		if (flags & QSE_FIO_EXCLUSIVE) oflags |= O_EXCL;

		oflags |= O_BINARY | O_NOINHERIT;
		
		if (mode & QSE_FIO_RUSR) permission |= S_IREAD;
		if (mode & QSE_FIO_WUSR) permission |= S_IWRITE;

		handle = open (
			path_mb,
			oflags,
			permission
		);

	#if defined(QSE_CHAR_IS_MCHAR)
		/* nothing to do */
	#else
		if (path_mb != path_mb_buf) QSE_MMGR_FREE (mmgr, path_mb);
	#endif

		if (handle <= -1) 
		{
			if (flags & QSE_FIO_TEMPORARY) goto retry_temporary;
			fio->errnum = syserr_to_errnum (errno);
			return -1;
		}
	}

#elif defined(vms) || defined(__vms)

	if (flags & QSE_FIO_HANDLE)
	{
		/* TODO: implement this */
		fio->errnum = QSE_FIO_ENOIMPL;
		return -1;
	}
	else
	{
		struct FAB* fab;
		struct RAB* rab;
		unsigned long r0;
		
	#if defined(QSE_CHAR_IS_MCHAR)
		const qse_mchar_t* path_mb = path;
	#else
		qse_mchar_t path_mb_buf[1024];
		qse_mchar_t* path_mb;
		qse_size_t wl, ml;
		int px;

		if (flags & QSE_FIO_MBSPATH)
		{
			path_mb = (qse_mchar_t*)path;
		}
		else
		{
			path_mb = path_mb_buf;
			ml = QSE_COUNTOF(path_mb_buf);
			px = qse_wcstombs (path, &wl, path_mb, &ml);
			if (px == -2)
			{
				/* the static buffer is too small.
				 * allocate a buffer */
				path_mb = qse_wcstombsdup (path, mmgr);
				if (path_mb == QSE_NULL) 
				{
					fio->errnum = QSE_FIO_ENOMEM;
					return -1;
				}
			}
			else if (px <= -1) 
			{
				fio->errnum = QSE_FIO_EINVAL;
				return -1;
			}
		}
	#endif

		rab = (struct RAB*)QSE_MMGR_ALLOC (
			mmgr, QSE_SIZEOF(*rab) + QSE_SIZEOF(*fab));
		if (rab == QSE_NULL)
		{
	#if defined(QSE_CHAR_IS_MCHAR)
			/* nothing to do */
	#else
			if (path_mb != path_mb_buf) QSE_MMGR_FREE (mmgr, path_mb);
	#endif
			fio->errnum = QSE_FIO_ENOMEM;
			return -1;			
		}
	
		fab = (struct FAB*)(rab + 1);
		*rab = cc$rms_rab;
		rab->rab$l_fab = fab;

		*fab = cc$rms_fab;
		fab->fab$l_fna = path_mb;
		fab->fab$b_fns = strlen(path_mb);
		fab->fab$b_org = FAB$C_SEQ;
		fab->fab$b_rfm = FAB$C_VAR; /* FAB$C_STM, FAB$C_STMLF, FAB$C_VAR, etc... */
		fab->fab$b_fac = FAB$M_GET | FAB$M_PUT;

		fab->fab$b_fac = FAB$M_NIL;
		if (flags & QSE_FIO_READ) fab->fab$b_fac |= FAB$M_GET;
		if (flags & (QSE_FIO_WRITE | QSE_FIO_APPEND)) fab->fab$b_fac |= FAB$M_PUT | FAB$M_TRN; /* put, truncate */
		
		fab->fab$b_shr |= FAB$M_SHRPUT | FAB$M_SHRGET; /* FAB$M_NIL */
		if (flags & QSE_FIO_NOSHREAD) fab->fab$b_shr &= ~FAB$M_SHRGET;
		if (flags & QSE_FIO_NOSHWRITE) fab->fab$b_shr &= ~FAB$M_SHRPUT;

		if (flags & QSE_FIO_APPEND) rab->rab$l_rop |= RAB$M_EOF;

		if (flags & QSE_FIO_CREATE) 
		{
			if (flags & QSE_FIO_EXCLUSIVE) 
				fab->fab$l_fop &= ~FAB$M_CIF;
			else
				fab->fab$l_fop |= FAB$M_CIF;

			r0 = sys$create (&fab, 0, 0);
		}
		else
		{
			r0 = sys$open (&fab, 0, 0);
		}

		if (r0 != RMS$_NORMAL && r0 != RMS$_CREATED)
		{
	#if defined(QSE_CHAR_IS_MCHAR)
			/* nothing to do */
	#else
			if (path_mb != path_mb_buf) QSE_MMGR_FREE (mmgr, path_mb);
	#endif
			fio->errnum = syserr_to_errnum (r0);
			return -1;
		}


		r0 = sys$connect (&rab, 0, 0);
		if (r0 != RMS$_NORMAL)
		{
	#if defined(QSE_CHAR_IS_MCHAR)
			/* nothing to do */
	#else
			if (path_mb != path_mb_buf) QSE_MMGR_FREE (mmgr, path_mb);
	#endif
			fio->errnum = syserr_to_errnum (r0);
			return -1;
		}

	#if defined(QSE_CHAR_IS_MCHAR)
		/* nothing to do */
	#else
		if (path_mb != path_mb_buf) QSE_MMGR_FREE (mmgr, path_mb);
	#endif

		handle = rab;
	}

#else

	if (flags & QSE_FIO_HANDLE)
	{
		handle = *(qse_fio_hnd_t*)path;
		QSE_ASSERTX (
			handle >= 0,
			"Do not specify an invalid handle value"
		);
	}
	else
	{
		int desired_access = 0;

	#if defined(QSE_CHAR_IS_MCHAR)
		const qse_mchar_t* path_mb = path;
	#else
		qse_mchar_t path_mb_buf[1024];  /* PATH_MAX instead? */
		qse_mchar_t* path_mb;
		qse_size_t wl, ml;
		int px;

		if (flags & QSE_FIO_MBSPATH)
		{
			path_mb = (qse_mchar_t*)path;
		}
		else
		{
			path_mb = path_mb_buf;
			ml = QSE_COUNTOF(path_mb_buf);
			px = qse_wcstombs (path, &wl, path_mb, &ml);
			if (px == -2)
			{
				/* the static buffer is too small.
				 * allocate a buffer */
				path_mb = qse_wcstombsdup (path, QSE_NULL, mmgr);
				if (path_mb == QSE_NULL) 
				{
					fio->errnum = QSE_FIO_ENOMEM;
					return -1;
				}
			}
			else if (px <= -1) 
			{
				fio->errnum = QSE_FIO_EINVAL;
				return -1;
			}
		}
	#endif
		/*
		 * rwa -> RDWR   | APPEND
		 * ra  -> RDWR   | APPEND
		 * wa  -> WRONLY | APPEND
		 * a   -> WRONLY | APPEND
		 */
		if (flags & QSE_FIO_APPEND)
		{
			if ((flags & QSE_FIO_READ)) desired_access |= O_RDWR;
			else desired_access |= O_WRONLY;
			desired_access |= O_APPEND;
		}
		else
		{
			if ((flags & QSE_FIO_READ) &&
			    (flags & QSE_FIO_WRITE)) desired_access |= O_RDWR;
			else if (flags & QSE_FIO_READ) desired_access |= O_RDONLY;
			else if (flags & QSE_FIO_WRITE) desired_access |= O_WRONLY;
		}

		if (flags & QSE_FIO_CREATE) desired_access |= O_CREAT;
		if (flags & QSE_FIO_TRUNCATE) desired_access |= O_TRUNC;
		if (flags & QSE_FIO_EXCLUSIVE) desired_access |= O_EXCL;
		if (flags & QSE_FIO_SYNC) desired_access |= O_SYNC;

	#if defined(O_NOFOLLOW)
		if (flags & QSE_FIO_NOFOLLOW) desired_access |= O_NOFOLLOW;
	#endif

	#if defined(O_LARGEFILE)
		desired_access |= O_LARGEFILE;
	#endif
	#if defined(O_CLOEXEC)
		desired_access |= O_CLOEXEC; /* no inherit */
	#endif

		handle = QSE_OPEN (path_mb, desired_access, mode);

	#if defined(QSE_CHAR_IS_MCHAR)
		/* nothing to do */
	#else
		if (path_mb != path_mb_buf && path_mb != path) 
		{
			QSE_MMGR_FREE (mmgr, path_mb);
		}
	#endif
		if (handle == -1) 
		{
			if (flags & QSE_FIO_TEMPORARY) goto retry_temporary;
			fio->errnum = syserr_to_errnum (errno);
			return -1;
		}

		/* set some file access hints */
	#if defined(POSIX_FADV_RANDOM)
		if (flags & QSE_FIO_RANDOM) 
			posix_fadvise (handle, 0, 0, POSIX_FADV_RANDOM);
	#endif
	#if defined(POSIX_FADV_SEQUENTIAL)
		if (flags & QSE_FIO_SEQUENTIAL) 
			posix_fadvise (handle, 0, 0, POSIX_FADV_SEQUENTIAL);
	#endif
	}
#endif

	fio->handle = handle;
	return 0;
}

void qse_fio_fini (qse_fio_t* fio)
{
	if (!(fio->status & STATUS_NOCLOSE))
	{
#if defined(_WIN32)
		CloseHandle (fio->handle);

#elif defined(__OS2__)
		DosClose (fio->handle);

#elif defined(__DOS__)
		close (fio->handle);

#elif defined(vms) || defined(__vms)
		struct RAB* rab = (struct RAB*)fio->handle;
		sys$disconnect (rab, 0, 0);
		sys$close ((struct FAB*)(rab + 1), 0, 0);
		QSE_MMGR_FREE (fio->mmgr, fio->handle);

#else
		QSE_CLOSE (fio->handle);
#endif
	}
}

qse_mmgr_t* qse_fio_getmmgr (qse_fio_t* fio)
{
	return fio->mmgr;
}

void* qse_fio_getxtn (qse_fio_t* fio)
{
	return QSE_XTN (fio);
}

qse_fio_errnum_t qse_fio_geterrnum (const qse_fio_t* fio)
{
	return fio->errnum;
}

qse_fio_hnd_t qse_fio_gethandle (const qse_fio_t* fio)
{
	return fio->handle;
}

qse_ubi_t qse_fio_gethandleasubi (const qse_fio_t* fio)
{
	qse_ubi_t handle;

#if defined(_WIN32)
	handle.ptr = fio->handle;
#elif defined(__OS2__)
	handle.ul = fio->handle;
#elif defined(__DOS__)
	handle.i = fio->handle;
#elif defined(vms) || defined(__vms)
	handle.ptr = fio->handle;
#else
	handle.i = fio->handle;
#endif

	return handle;
}

qse_fio_off_t qse_fio_seek (
	qse_fio_t* fio, qse_fio_off_t offset, qse_fio_ori_t origin)
{
#if defined(_WIN32)
	static int seek_map[] =
	{
		FILE_BEGIN,
		FILE_CURRENT,
		FILE_END
	};
	LARGE_INTEGER x;
	#if defined(_WIN64)
	LARGE_INTEGER y;
	#endif

	QSE_ASSERT (QSE_SIZEOF(offset) <= QSE_SIZEOF(x.QuadPart));

	#if defined(_WIN64)
	x.QuadPart = offset;
	if (SetFilePointerEx (fio->handle, x, &y, seek_map[origin]) == FALSE)
	{
		return (qse_fio_off_t)-1;
	}
	return (qse_fio_off_t)y.QuadPart;
	#else

	/* SetFilePointerEx is not available on Windows NT 4.
	 * So let's use SetFilePointer */
	x.QuadPart = offset;
	x.LowPart = SetFilePointer (
		fio->handle, x.LowPart, &x.HighPart, seek_map[origin]);
	if (x.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		return (qse_fio_off_t)-1;
	}
	return (qse_fio_off_t)x.QuadPart;
	#endif

#elif defined(__OS2__)
	static int seek_map[] =
	{
		FILE_BEGIN,
		FILE_CURRENT,
		FILE_END
	};

	#if defined(FIL_STANDARDL)

	LONGLONG pos, newpos;
	APIRET ret;

	QSE_ASSERT (QSE_SIZEOF(offset) >= QSE_SIZEOF(pos));

	pos.ulLo = (ULONG)(offset&0xFFFFFFFFlu);
	pos.ulHi = (ULONG)(offset>>32);

	ret = DosSetFilePtrL (fio->handle, pos, seek_map[origin], &newpos);
	if (ret != NO_ERROR) 
	{
		fio->errnum = syserr_to_errnum (ret);
		return (qse_fio_off_t)-1;
	}

	return ((qse_fio_off_t)newpos.ulHi << 32) | newpos.ulLo;

	#else
	ULONG newpos;
	APIRET ret;

	ret = DosSetFilePtr (fio->handle, offset, seek_map[origin], &newpos);
	if (ret != NO_ERROR) 
	{
		fio->errnum = syserr_to_errnum (ret);
		return (qse_fio_off_t)-1;
	}

	return newpos;
	#endif

#elif defined(__DOS__)
	static int seek_map[] =
	{
		SEEK_SET,                    
		SEEK_CUR,
		SEEK_END
	};

	return lseek (fio->handle, offset, seek_map[origin]);
#elif defined(vms) || defined(__vms)

	/* TODO: */
	fio->errnum = QSE_FIO_ENOIMPL;
	return (qse_fio_off_t)-1;
#else
	static int seek_map[] =
	{
		SEEK_SET,                    
		SEEK_CUR,
		SEEK_END
	};

#if defined(QSE_LLSEEK)
	loff_t tmp;

	if (QSE_LLSEEK (fio->handle,
		(unsigned long)(offset>>32),
		(unsigned long)(offset&0xFFFFFFFFlu),
		&tmp,
		seek_map[origin]) == -1)
	{
		fio->errnum = syserr_to_errnum (errno);
		return (qse_fio_off_t)-1;
	}

	return (qse_fio_off_t)tmp;
#else
	return QSE_LSEEK (fio->handle, offset, seek_map[origin]);
#endif

#endif
}

int qse_fio_truncate (qse_fio_t* fio, qse_fio_off_t size)
{
#if defined(_WIN32)
	if (qse_fio_seek (fio, size, QSE_FIO_BEGIN) == (qse_fio_off_t)-1) return -1;
	if (SetEndOfFile(fio->handle) == FALSE) 
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		return -1;
	}
	return 0;
#elif defined(__OS2__)

	APIRET ret;

	#if defined(FIL_STANDARDL)
	LONGLONG sz;
	/* the file must have the write access for it to succeed */

	sz.ulLo = (ULONG)(size&0xFFFFFFFFlu);
	sz.ulHi = (ULONG)(size>>32);

	ret = DosSetFileSizeL (fio->handle, sz);
	#else
	ret = DosSetFileSize (fio->handle, size);
	#endif

	if (ret != NO_ERROR)
	{
		fio->errnum = syserr_to_errnum (ret);
		return -1;
	}
	return 0;

#elif defined(__DOS__)

	int n;
	n = chsize (fio->handle, size);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;

#elif defined(vms) || defined(__vms)

	unsigned long r0;
	struct RAB* rab = (struct RAB*)fio->handle;
	
	if ((r0 = sys$rewind (rab, 0, 0)) != RMS$_NORMAL ||
	    (r0 = sys$truncate (rab, 0, 0)) != RMS$_NORMAL)
	{
		fio->errnum = syserr_to_errnum (r0);
		return -1;
	}

	return 0;
#else
	int n;
	n = QSE_FTRUNCATE (fio->handle, size);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;
#endif
}

qse_ssize_t qse_fio_read (qse_fio_t* fio, void* buf, qse_size_t size)
{
#if defined(_WIN32)

	DWORD count;

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD);
	if (ReadFile (fio->handle, buf, (DWORD)size, &count, QSE_NULL) == FALSE)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__OS2__)

	APIRET ret;
	ULONG count;
	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG);
	ret = DosRead (fio->handle, buf, (ULONG)size, &count);
	if (ret != NO_ERROR) 
	{
		fio->errnum = syserr_to_errnum (ret);
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)

	int n;
	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);
	n = read (fio->handle, buf, size);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;

#elif defined(vms) || defined(__vms)

	unsigned long r0;
	struct RAB* rab = (struct RAB*)fio->handle;

	if (size > 32767) size = 32767;

	rab->rab$l_ubf = buf;
	rab->rab$w_usz = size;

	r0 = sys$get (rab, 0, 0);
	if (r0 != RMS$_NORMAL)
	{
		fio->errnum = syserr_to_errnum (r0);
		return -1;
	}

	return rab->rab$w_rsz;
#else

	ssize_t n;
	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);
	n = QSE_READ (fio->handle, buf, size);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;
#endif
}

qse_ssize_t qse_fio_write (qse_fio_t* fio, const void* data, qse_size_t size)
{
#if defined(_WIN32)

	DWORD count;

   	if (fio->status & STATUS_APPEND)
	{
/* TODO: only when FILE_APPEND_DATA failed???  how do i know this??? */
		/* i do this on a best-effort basis */
	#if defined(_WIN64)
		LARGE_INTEGER x;
		x.QuadPart = 0;
    		SetFilePointerEx (fio->handle, x, QSE_NULL, FILE_END);
	#else
    		SetFilePointer (fio->handle, 0, QSE_NULL, FILE_END);
	#endif
    	}

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD);
	if (WriteFile (fio->handle,
		data, (DWORD)size, &count, QSE_NULL) == FALSE) 
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__OS2__)

	APIRET ret;
	ULONG count;

   	if (fio->status & STATUS_APPEND)
	{
		/* i do this on a best-effort basis */
	#if defined(FIL_STANDARDL)
		LONGLONG pos, newpos;
		pos.ulLo = (ULONG)0;
		pos.ulHi = (ULONG)0;
    		DosSetFilePtrL (fio->handle, pos, FILE_END, &newpos);
	#else
		ULONG newpos;
    		DosSetFilePtr (fio->handle, 0, FILE_END, &newpos);
	#endif
    	}

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG);
	ret = DosWrite(fio->handle, 
		(PVOID)data, (ULONG)size, &count);
	if (ret != NO_ERROR) 
	{
		fio->errnum = syserr_to_errnum (ret);
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)

	int n;
	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);
	n = write (fio->handle, data, size);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;

#elif defined(vms) || defined(__vms)

	unsigned long r0;
	struct RAB* rab = (struct RAB*)fio->handle;

	if (size > 32767) size = 32767;

	rab->rab$l_rbf = (char*)data;
	rab->rab$w_rsz = size;

	r0 = sys$put (rab, 0, 0);
	if (r0 != RMS$_NORMAL)
	{
		fio->errnum = syserr_to_errnum (r0);
		return -1;
	}

	return rab->rab$w_rsz;

#else

	ssize_t n;
	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t))) 
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);
	n = QSE_WRITE (fio->handle, data, size);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;
#endif
}

#if defined(_WIN32)

static int get_devname_from_handle (
	qse_fio_t* fio, qse_char_t* buf, qse_size_t len) 
{
	HANDLE map = NULL;
	void* mem = NULL;
	DWORD olen;
	HINSTANCE psapi;

	typedef DWORD (WINAPI*getmappedfilename_t) (
		HANDLE hProcess,
		LPVOID lpv,
		LPTSTR lpFilename,
		DWORD nSize
	);
	getmappedfilename_t getmappedfilename;

	/* try to load psapi.dll dynamially for 
	 * systems without it. direct linking to the library
	 * may end up with dependency failure on such systems. 
	 * this way, the worst case is that this function simply 
	 * fails. */
	psapi = LoadLibrary (QSE_T("PSAPI.DLL"));
	if (!psapi)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		return -1;
	}

	getmappedfilename = (getmappedfilename_t) 
		GetProcAddress (psapi, QSE_MT("GetMappedFileName"));
	if (!getmappedfilename)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		FreeLibrary (psapi);
		return -1;
	}

	/* create a file mapping object */
	map = CreateFileMapping (
		fio->handle, 
		NULL,
		PAGE_READONLY,
		0, 
		1,
		NULL
	);
	if (map == NULL) 
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		FreeLibrary (psapi);
		return -1;
	}	

	/* create a file mapping to get the file name. */
	mem = MapViewOfFile (map, FILE_MAP_READ, 0, 0, 1);
	if (mem == NULL)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		CloseHandle (map);
		FreeLibrary (psapi);
		return -1;
	}

	olen = getmappedfilename (GetCurrentProcess(), mem, buf, len); 
	if (olen == 0)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		UnmapViewOfFile (mem);
		CloseHandle (map);
		FreeLibrary (psapi);
		return -1;
	}

	UnmapViewOfFile (mem);
	CloseHandle (map);
	FreeLibrary (psapi);
	return 0;
}

static int get_volname_from_handle (
	qse_fio_t* fio, qse_char_t* buf, qse_size_t len) 
{
	if (get_devname_from_handle (fio, buf, len) == -1) return -1;

	if (qse_strcasebeg (buf, QSE_T("\\Device\\LanmanRedirector\\")))
	{
		/*buf[0] = QSE_T('\\');*/
		qse_strcpy (&buf[1], &buf[24]);
	}
	else
	{
		DWORD n;
		qse_char_t drives[128];

		n = GetLogicalDriveStrings (QSE_COUNTOF(drives), drives);

		if (n == 0 /* error */ || 
		    n > QSE_COUNTOF(drives) /* buffer small */) 
		{
			fio->errnum = syserr_to_errnum (GetLastError());
			return -1;	
		}

		while (n > 0)
		{
			qse_char_t drv[3];
			qse_char_t path[MAX_PATH];

			drv[0] = drives[--n];
			drv[1] = QSE_T(':');
			drv[2] = QSE_T('\0');
			if (QueryDosDevice (drv, path, QSE_COUNTOF(path)))
			{
				qse_size_t pl = qse_strlen(path);
				qse_size_t bl = qse_strlen(buf);
				if (bl > pl && buf[pl] == QSE_T('\\') &&
				    qse_strxncasecmp(buf, pl, path, pl) == 0)
				{
					buf[0] = drv[0];
					buf[1] = QSE_T(':');
					qse_strcpy (&buf[2], &buf[pl]);
					break;
				}
			}
		}
	}
	
	/* if the match is not found, the device name is returned
	 * without translation */
	return 0;
}
#endif

int qse_fio_chmod (qse_fio_t* fio, int mode)
{
#if defined(_WIN32)

	int flags = FILE_ATTRIBUTE_NORMAL;
	qse_char_t name[MAX_PATH];

	/* it is a best effort implementation. if the file size is 0,
	 * it can't even get the file name from the handle and thus fails. 
	 * if GENERIC_READ is not set in CreateFile, CreateFileMapping fails. 
	 * so if this fio is opened without QSE_FIO_READ, this function fails.
	 */
	if (get_volname_from_handle (fio, name, QSE_COUNTOF(name)) == -1) return -1;

	if (!(mode & QSE_FIO_WUSR)) flags = FILE_ATTRIBUTE_READONLY;
	if (SetFileAttributes (name, flags) == FALSE)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		return -1;
	}
	return 0;

#elif defined(__OS2__)

	APIRET n;
	int flags = FILE_NORMAL;
	#if defined(FIL_STANDARDL)
	FILESTATUS3L stat;
	#else
	FILESTATUS3 stat;
	#endif
	ULONG size = QSE_SIZEOF(stat);

	#if defined(FIL_STANDARDL)
	n = DosQueryFileInfo (fio->handle, FIL_STANDARDL, &stat, size);
	#else
	n = DosQueryFileInfo (fio->handle, FIL_STANDARD, &stat, size);
	#endif
	if (n != NO_ERROR)
	{
		fio->errnum = syserr_to_errnum (n);
		return -1;
	}

	if (!(mode & QSE_FIO_WUSR)) flags = FILE_READONLY;
	
	stat.attrFile = flags;
	#if defined(FIL_STANDARDL)
	n = DosSetFileInfo (fio->handle, FIL_STANDARDL, &stat, size);
	#else
	n = DosSetFileInfo (fio->handle, FIL_STANDARD, &stat, size);
	#endif
	if (n != NO_ERROR)
	{
		fio->errnum = syserr_to_errnum (n);
		return -1;
	}

	return 0;

#elif defined(__DOS__)

	int permission = 0;

	if (mode & QSE_FIO_RUSR) permission |= S_IREAD;
	if (mode & QSE_FIO_WUSR) permission |= S_IWRITE;

	/* TODO: fchmod not available. find a way to do this
	return fchmod (fio->handle, permission); */

	fio->errnum = QSE_FIO_ENOIMPL;
	return -1;

#elif defined(vms) || defined(__vms)

	/* TODO: */
	fio->errnum = QSE_FIO_ENOIMPL;
	return (qse_fio_off_t)-1;

#else
	int n;
	n = QSE_FCHMOD (fio->handle, mode);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;
#endif
}

int qse_fio_sync (qse_fio_t* fio)
{
#if defined(_WIN32)

	if (FlushFileBuffers (fio->handle) == FALSE)
	{
		fio->errnum = syserr_to_errnum (GetLastError());
		return -1;
	}
	return 0;

#elif defined(__OS2__)

	APIRET n;
	n = DosResetBuffer (fio->handle); 
	if (n != NO_ERROR)
	{
		fio->errnum = syserr_to_errnum (n);
		return -1;
	}
	return 0;

#elif defined(__DOS__)

	int n;
	n = fsync (fio->handle);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;

#elif defined(vms) || defined(__vms)

	/* TODO: */
	fio->errnum = QSE_FIO_ENOIMPL;
	return (qse_fio_off_t)-1;

#else

	int n;
	n = QSE_FSYNC (fio->handle);
	if (n <= -1) fio->errnum = syserr_to_errnum (errno);
	return n;
#endif
}

int qse_fio_lock (qse_fio_t* fio, qse_fio_lck_t* lck, int flags)
{
	/* TODO: qse_fio_lock 
	 * struct flock fl;
	 * fl.l_type = F_RDLCK, F_WRLCK;
	 * QSE_FCNTL (fio->handle, F_SETLK, &fl);
	 */
	fio->errnum = QSE_FIO_ENOIMPL;
	return -1;
}

int qse_fio_unlock (qse_fio_t* fio, qse_fio_lck_t* lck, int flags)
{
	/* TODO: qse_fio_unlock 
	 * struct flock fl;
	 * fl.l_type = F_UNLCK;
	 * QSE_FCNTL (fio->handle, F_SETLK, &fl);
	 */
	fio->errnum = QSE_FIO_ENOIMPL;
	return -1;
}

int qse_getstdfiohandle (qse_fio_std_t std, qse_fio_hnd_t* hnd)
{
#if defined(_WIN32)
	DWORD tab[] =
	{
		STD_INPUT_HANDLE,
		STD_OUTPUT_HANDLE,
		STD_ERROR_HANDLE
	};
#elif defined(vms) || defined(__vms)
	/* TODO */
	int tab[] = { 0, 1, 2 };
#else

	qse_fio_hnd_t tab[] =
	{
#if defined(__OS2__)
		(HFILE)0, (HFILE)1, (HFILE)2
#elif defined(__DOS__)
		0, 1, 2
#else
		0, 1, 2
#endif
	};

#endif

	if (std < 0 || std >= QSE_COUNTOF(tab)) return -1;

#if defined(_WIN32)
	{
		HANDLE tmp = GetStdHandle (tab[std]);
		if (tmp == INVALID_HANDLE_VALUE) return -1;
		*hnd = tmp;
	}
#elif defined(vms) || defined(__vms)
	/* TODO: */
	return -1;
#else
	*hnd = tab[std];
#endif
	return 0;
}
