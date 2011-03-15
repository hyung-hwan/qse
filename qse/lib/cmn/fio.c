/*
 * $Id: fio.c 397 2011-03-15 03:40:39Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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
#include "mem.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <psapi.h>
#	include <tchar.h>
#elif defined(__OS2__)
#	define INCL_DOSFILEMGR
#	define INCL_DOSERRORS
#	include <os2.h>
#else
#	include "syscall.h"
#	include <sys/types.h>
#	include <fcntl.h>
#	include <limits.h>
#	ifndef PATH_MAX
#		define PATH_MAX 2048
#	endif
#endif

QSE_IMPLEMENT_COMMON_FUNCTIONS (fio)

static qse_ssize_t fio_input (int cmd, void* arg, void* buf, qse_size_t size);
static qse_ssize_t fio_output (int cmd, void* arg, void* buf, qse_size_t size);

qse_fio_t* qse_fio_open (
	qse_mmgr_t* mmgr, qse_size_t ext,
	const qse_char_t* path, int flags, int mode)
{
	qse_fio_t* fio;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	fio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_fio_t) + ext);
	if (fio == QSE_NULL) return QSE_NULL;

	if (qse_fio_init (fio, mmgr, path, flags, mode) == QSE_NULL)
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

qse_fio_t* qse_fio_init (
	qse_fio_t* fio, qse_mmgr_t* mmgr,
	const qse_char_t* path, int flags, int mode)
{
	qse_fio_hnd_t handle;

	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (fio, 0, QSE_SIZEOF(*fio));
	fio->mmgr = mmgr;

#if defined(_WIN32)
	if (flags & QSE_FIO_HANDLE)
	{
		handle = *(qse_fio_hnd_t*)path;
	}
	else
	{
		DWORD desired_access = 0;
		DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		DWORD creation_disposition = 0;
		DWORD flag_and_attr = FILE_ATTRIBUTE_NORMAL;

		if (flags & QSE_FIO_APPEND)
		{
			/* this is not officially documented for CreateFile.
			 * ZwCreateFile (kernel) seems to document it */
			desired_access |= FILE_APPEND_DATA;
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

		if (flags & QSE_FIO_NOSHRD)
			share_mode &= ~FILE_SHARE_READ;
		if (flags & QSE_FIO_NOSHWR)
			share_mode &= ~FILE_SHARE_WRITE;

		if (!(mode & QSE_FIO_WUSR)) 
			flag_and_attr = FILE_ATTRIBUTE_READONLY;
		if (flags & QSE_FIO_SYNC) 
			flag_and_attr |= FILE_FLAG_WRITE_THROUGH;

		/* these two are just hints to OS */
		if (flags & QSE_FIO_RANDOM) 
			flag_and_attr |= FILE_FLAG_RANDOM_ACCESS;
		if (flags & QSE_FIO_SEQUENTIAL) 
			flag_and_attr |= FILE_FLAG_SEQUENTIAL_SCAN;

		handle = CreateFile (path,
			desired_access, share_mode, QSE_NULL,
			creation_disposition, flag_and_attr, 0);
	}

	if (handle == INVALID_HANDLE_VALUE) return QSE_NULL;

	{
		DWORD file_type = GetFileType(handle);
		if (file_type == FILE_TYPE_UNKNOWN)
		{
			CloseHandle (handle);
			return QSE_NULL;
		}
	}

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
		ULONG open_action, open_mode;
		LONGLONG zero;

		zero.ulLo = 0;
		zero.ulHi = 0;

		if (flags & QSE_FIO_CREATE)
		{
     		if (flags & QSE_FIO_EXCLUSIVE)
			{
      			open_action = OPEN_ACTION_FAIL_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
      		}
			else if (flags & QSE_FIO_TRUNCATE)
			{
      			open_action = OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
      		}
			else
			{
      			open_action = OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS;
      		}
		}
		else if (flags & QSE_FIO_TRUNCATE)
		{
			open_action = OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;
		}
		else 
		{
			open_action = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;
		}

		open_mode = OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE;
		if ((flags & QSE_FIO_READ) &&
		    (flags & QSE_FIO_WRITE)) open_mode |= OPEN_ACCESS_READWRITE;
		else if (flags & QSE_FIO_READ) open_mode |= OPEN_ACCESS_READONLY;
		else if (flags & QSE_FIO_WRITE) open_mode |= OPEN_ACCESS_WRITEONLY;
		
		ret = DosOpenL (
			path,          /* file name */
			&handle,       /* file handle */
			&action_taken, /* store action taken */
			zero,          /* size */
			FILE_NORMAL,   /* attribute */
			open_action,   /* action if it exists */
			open_mode,     /* open mode */
			0L                            
		);

		if (ret != NO_ERROR) return QSE_NULL;
	}
#else

	if (flags & QSE_FIO_HANDLE)
	{
		handle = *(qse_fio_hnd_t*)path;
	}
	else
	{
		int desired_access = 0;

	#ifdef QSE_CHAR_IS_MCHAR
		const qse_mchar_t* path_mb = path;
	#else
		qse_mchar_t path_mb[PATH_MAX + 1];
		if (qse_wcstombs_strict (path,
			path_mb, QSE_COUNTOF(path_mb)) == -1) return QSE_NULL;
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

	#if defined(O_LARGEFILE)
		desired_access |= O_LARGEFILE;
	#endif

		handle = QSE_OPEN (path_mb, desired_access, mode);
	}

	if (handle == -1) return QSE_NULL;

#endif

	if (flags & QSE_FIO_TEXT)
	{
		qse_tio_t* tio;

		tio = qse_tio_open (fio->mmgr, 0);
		if (tio == QSE_NULL) QSE_THROW_ERR (tio);

		if (qse_tio_attachin (tio, fio_input, fio) == -1 ||
		    qse_tio_attachout (tio, fio_output, fio) == -1)
		{
			qse_tio_close (tio);
			QSE_THROW_ERR (tio);
		}

		QSE_CATCH_ERR (tio) 
		{
		#if defined(_WIN32)
			CloseHandle (handle);
		#elif defined(__OS2__)
			DosClose (handle);
		#else
			QSE_CLOSE (handle);     
		#endif
			return QSE_NULL;
		}

		fio->tio = tio;
	}

	fio->handle = handle;

	return fio;
}

void qse_fio_fini (qse_fio_t* fio)
{
	if (fio->tio != QSE_NULL) qse_tio_close (fio->tio);
#if defined(_WIN32)
	CloseHandle (fio->handle);
#elif defined(__OS2__)
	DosClose (fio->handle);
#else
	QSE_CLOSE (fio->handle);
#endif
}

qse_fio_hnd_t qse_fio_gethandle (qse_fio_t* fio)
{
	return fio->handle;
}

void qse_fio_sethandle (qse_fio_t* fio, qse_fio_hnd_t handle)
{
	fio->handle = handle;
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
	LARGE_INTEGER x, y;

	QSE_ASSERT (QSE_SIZEOF(offset) <= QSE_SIZEOF(x.QuadPart));

	x.QuadPart = offset;
	if (SetFilePointerEx (fio->handle, x, &y, seek_map[origin]) == FALSE)
	{
		return (qse_fio_off_t)-1;
	}

	return (qse_fio_off_t)y.QuadPart;

	/*
	x.QuadPart = offset;
	x.LowPart = SetFilePointer (
		fio->handle, x.LowPart, &x.HighPart, seek_map[origin]);
	if (x.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		return (qse_fio_off_t)-1;
	}

	return (qse_fio_off_t)x.QuadPart;
	*/
#elif defined(__OS2__)
	static int seek_map[] =
	{
		FILE_BEGIN,
		FILE_CURRENT,
		FILE_END
	};

	LONGLONG pos, newpos;
	APIRET ret;

	QSE_ASSERT (QSE_SIZEOF(offset) >= QSE_SIZEOF(pos));

	pos.ulLo = (ULONG)(offset&0xFFFFFFFFlu);
	pos.ulHi = (ULONG)(offset>>32);

	ret = DosSetFilePtrL (fio->handle, pos, seek_map[origin], &newpos);
	if (ret != NO_ERROR) return (qse_fio_off_t)-1;

	return ((qse_fio_off_t)pos.ulHi << 32) | pos.ulLo;
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
		return (qse_fio_off_t)-1;
	}

	return (qse_fio_off_t)tmp;

#elif defined(QSE_LSEEK64)
	return QSE_LSEEK64 (fio->handle, offset, seek_map[origin]);
#else
	return QSE_LSEEK (fio->handle, offset, seek_map[origin]);
#endif

#endif
}

int qse_fio_truncate (qse_fio_t* fio, qse_fio_off_t size)
{
#ifdef _WIN32
	LARGE_INTEGER x;
	x.QuadPart = size;

	if (SetFilePointerEx(fio->handle,x,NULL,FILE_BEGIN) == FALSE ||
	    SetEndOfFile(fio->handle) == FALSE) return -1;

	return 0;
#elif defined(__OS2__)
	APIRET ret;
	LONGLONG sz;
	/* the file must have the write access for it to succeed */

	sz.ulLo = (ULONG)(size&0xFFFFFFFFlu);
	sz.ulHi = (ULONG)(size>>32);

	ret = DosSetFileSizeL (fio->handle, sz);
	return (ret == NO_ERROR)? 0: -1;
#else
	return QSE_FTRUNCATE (fio->handle, size);
#endif
}

static qse_ssize_t fio_read (qse_fio_t* fio, void* buf, qse_size_t size)
{
#if defined(_WIN32)
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (ReadFile(fio->handle, buf, (DWORD)size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#elif defined(__OS2__)
	ULONG count;
	if (size > QSE_TYPE_MAX(ULONG)) size = QSE_TYPE_MAX(ULONG);
	if (DosRead (fio->handle, buf, (ULONG)size, &count) != NO_ERROR) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	return QSE_READ (fio->handle, buf, size);
#endif
}

qse_ssize_t qse_fio_read (qse_fio_t* fio, void* buf, qse_size_t size)
{
	if (fio->tio == QSE_NULL) 
		return fio_read (fio, buf, size);
	else
		return qse_tio_read (fio->tio, buf, size);
}

static qse_ssize_t fio_write (qse_fio_t* fio, const void* data, qse_size_t size)
{
#if defined(_WIN32)
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (WriteFile(fio->handle, data, (DWORD)size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#elif defined(__OS2__)
	ULONG count;
	if (size > QSE_TYPE_MAX(ULONG)) size = QSE_TYPE_MAX(ULONG);
	if (DosWrite(fio->handle, (PVOID)data, (ULONG)size, &count) != NO_ERROR) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	return QSE_WRITE (fio->handle, data, size);
#endif
}

qse_ssize_t qse_fio_write (qse_fio_t* fio, const void* data, qse_size_t size)
{
	if (fio->tio == QSE_NULL)
		return fio_write (fio, data, size);
	else
		return qse_tio_write (fio->tio, data, size);
}

qse_ssize_t qse_fio_flush (qse_fio_t* fio)
{
	if (fio->tio == QSE_NULL) return 0;
	return qse_tio_flush (fio->tio);
}


#ifdef _WIN32

static int get_devname_from_handle (
	HANDLE handle, qse_char_t* buf, qse_size_t len) 
{
	HANDLE map = NULL;
	void* mem = NULL;
	DWORD olen;

	/* create a file mapping object */
	map = CreateFileMapping (
		handle, 
		NULL, 
		PAGE_READONLY,
		0, 
		1,
		NULL
	);
	if (map == NULL) return -1;

	/* create a file mapping to get the file name. */
	mem = MapViewOfFile (map, FILE_MAP_READ, 0, 0, 1);
	if (mem == NULL)
	{
		CloseHandle (map);
		return -1;
	}

	olen = GetMappedFileName (GetCurrentProcess(), mem, buf, len); 
	if (olen == 0)
	{
		UnmapViewOfFile (mem);
		CloseHandle (map);
		return -1;
	}

	UnmapViewOfFile (mem);
	CloseHandle (map);
	return 0;
}

static int get_volname_from_handle (
	HANDLE handle, qse_char_t* buf, qse_size_t len) 
{
	if (get_devname_from_handle (handle, buf, len) == -1) return -1;

	if (_tcsnicmp(QSE_T("\\Device\\LanmanRedirector\\"), buf, 25) == 0)
	{
		buf[0] = QSE_T('\\');
		_tcscpy (&buf[1], &buf[24]);
	}
	else
	{
		DWORD n;
		qse_char_t drives[128];

		n = GetLogicalDriveStrings (QSE_COUNTOF(drives), drives);

		if (n == 0 /* error */ || 
		    n > QSE_COUNTOF(drives) /* buffer small */) 
		{
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
				qse_size_t pl = _tcslen(path);
				qse_size_t bl = _tcslen(buf);
				if (bl > pl && buf[pl] == QSE_T('\\') &&
				    _tcsnicmp(path, buf, pl) == 0)
				{
					buf[0] = drv[0];
					buf[1] = QSE_T(':');
					_tcscpy (&buf[2], &buf[pl]);
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
	 */
	if (get_volname_from_handle (
		fio->handle, name, QSE_COUNTOF(name)) == -1) return -1;

	if (!(mode & QSE_FIO_WUSR)) flags = FILE_ATTRIBUTE_READONLY;
	return (SetFileAttributes (name, flags) == FALSE)? -1: 0;
#elif defined(__OS2__)
	int flags = FILE_NORMAL;
	FILESTATUS3L stat;
	ULONG size = QSE_SIZEOF(stat);

	if (DosQueryFileInfo (fio->handle, FIL_STANDARDL, &stat, size) != NO_ERROR) return -1;

	if (!(mode & QSE_FIO_WUSR)) flags = FILE_READONLY;
	
	stat.attrFile = flags;
	return (DosSetFileInfo (fio->handle, FIL_STANDARDL, &stat, size) != NO_ERROR)? -1: 0;
#else
	return QSE_FCHMOD (fio->handle, mode);
#endif
}

int qse_fio_sync (qse_fio_t* fio)
{
#if defined(_WIN32)
	return (FlushFileBuffers (fio->handle) == FALSE)? -1: 0;
#elif defined(__OS2__)
	return (DosResetBuffer (fio->handle) == NO_ERROR)? 0: -1;
#else
	return QSE_FSYNC (fio->handle);
#endif
}

int qse_fio_lock (qse_fio_t* fio, qse_fio_lck_t* lck, int flags)
{
	/* TODO: qse_fio_lock 
	 * struct flock fl;
	 * fl.l_type = F_RDLCK, F_WRLCK;
	 * QSE_FCNTL (fio->handle, F_SETLK, &fl);
	 */
	return -1;
}

int qse_fio_unlock (qse_fio_t* fio, qse_fio_lck_t* lck, int flags)
{
	/* TODO: qse_fio_unlock 
	 * struct flock fl;
	 * fl.l_type = F_UNLCK;
	 * QSE_FCNTL (fio->handle, F_SETLK, &fl);
	 */
	return -1;
}

static qse_ssize_t fio_input (int cmd, void* arg, void* buf, qse_size_t size)
{
	qse_fio_t* fio = (qse_fio_t*)arg;
	QSE_ASSERT (fio != QSE_NULL);
	if (cmd == QSE_TIO_IO_DATA) 
	{
		return fio_read (fio, buf, size);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by fio */
	return 0;
}

static qse_ssize_t fio_output (int cmd, void* arg, void* buf, qse_size_t size)
{
	qse_fio_t* fio = (qse_fio_t*)arg;
	QSE_ASSERT (fio != QSE_NULL);
	if (cmd == QSE_TIO_IO_DATA) 
	{
		return fio_write (fio, buf, size);
	}

	/* take no actions for OPEN and CLOSE as they are handled
	 * by fio */
	return 0;
}
