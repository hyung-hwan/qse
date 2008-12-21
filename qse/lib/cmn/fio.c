/*
 * $Id: fio.c,v 1.23 2006/06/30 04:18:47 bacon Exp $
 */

#include <qse/cmn/fio.h>
#include "mem.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#endif

#if defined(QSE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>
#endif

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

	QSE_MEMSET (fio, 0, QSE_SIZEOF(*fio));
	fio->mmgr = mmgr;

#ifdef _WIN32
	if (flags & QSE_FIO_HANDLE)
	{
		handle = *(qse_fio_hnd_t*)path;
	}
	else
	{
		DWORD desired_access = 0;
		DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		DWORD creation_disposition = 0;
		DWORD attributes = FILE_ATTRIBUTE_NORMAL;

		if (flags & QSE_FIO_APPEND)  
		{
			/* this is not officialy documented for CreateFile.
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
	
		if (flags & QSE_FIO_NOSHRD) share_mode &= ~FILE_SHARE_READ;
		if (flags & QSE_FIO_NOSHWR) share_mode &= ~FILE_SHARE_WRITE;

		if (flags & QSE_FIO_SYNC) attributes |= FILE_FLAG_WRITE_THROUGH; 
		/* TODO: handle mode... set attribuets */
		handle = CreateFile (path, 
			desired_access, share_mode, QSE_NULL, 
			creation_disposition, attributes, 0);
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

	/* TODO: a lot more */
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

	#ifdef SYS_open
		handle = syscall (SYS_open, path_mb, desired_access, mode);
	#else
		handle = open (path_mb, desired_access, mode);
	#endif
	}

	if (handle == -1) return QSE_NULL;

#endif

	fio->handle = handle;
	return fio;
}

void qse_fio_fini (qse_fio_t* fio)
{
#ifdef _WIN32
	CloseHandle (fio->handle);
#else
	#if defined(SYS_close)
	syscall (SYS_close, fio->handle);
	#else
	close (fio->handle);
	#endif
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
#ifdef _WIN32
	static int seek_map[] = 
	{
		FILE_BEGIN,
		FILE_CURRENT,
		FILE_END	
	};
	LARGE_INTEGER x, y;

	QSE_ASSERT (AES_SIZEOF(offset) <= AES_SIZEOF(x.QuadPart));

	x.QuadPart = offset;
	if (SetFilePointerEx (fio->handle, x, &y, seek_map[origin]) == FALSE) 
	{
		return (qse_fio_off_t)-1;
	}

	return (qse_fio_off_t)y.QuadPart;
	
	/*
	x.QuadPart = offset;
	x.LowPart = SetFilePointer (fio->handle, x.LowPart, &x.HighPart, seek_map[origin]);
	if (x.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		return (qse_fio_off_t)-1;
	}

	return (qse_fio_off_t)x.QuadPart;
	*/

#else
	static int seek_map[] = 
	{
		SEEK_SET,
		SEEK_CUR,
		SEEK_END
	};

#if !defined(_LP64) && defined(SYS__llseek)
	loff_t tmp;

	if (syscall (SYS__llseek, fio->handle,  
		(unsigned long)(offset>>32),
		(unsigned long)(offset&0xFFFFFFFFlu), 
		&tmp,
		seek_map[origin]) == -1)
	{
		return (qse_fio_off_t)-1;
	}

	return (qse_fio_off_t)tmp;

#elif defined(SYS_lseek)
	return syscall (SYS_lseek, fio->handle, offset, seek_map[origin]);
#elif !defined(_LP64) && defined(HAVE_LSEEK64)
	return lseek64 (fio->handle, offset, seek_map[origin]);
#else
	return lseek (fio->handle, offset, seek_map[origin]);
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
#else
 
	#if !defined(_LP64) && defined(SYS_ftruncate64)
	return syscall (SYS_ftruncate64, fio->handle, size);
	#elif defined(SYS_ftruncate)
	return syscall (SYS_ftruncate, fio->handle, size);
	#elif !defined(_LP64) && defined(HAVE_FTRUNCATE64)
	return ftruncate64 (fio->handle, size);
	#else
	return ftruncate (fio->handle, size);
	#endif
#endif
}

qse_ssize_t qse_fio_read (qse_fio_t* fio, void* buf, qse_size_t size)
{
#ifdef _WIN32
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (ReadFile(fio->handle, buf, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	#ifdef SYS_read 
	return syscall (SYS_read, fio->handle, buf, size);
	#else
	return read (fio->handle, buf, size);
	#endif
#endif
}

qse_ssize_t qse_fio_write (qse_fio_t* fio, const void* data, qse_size_t size)
{
#ifdef _WIN32
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (WriteFile(fio->handle, data, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	#ifdef SYS_write
	return syscall (SYS_write, fio->handle, data, size);
	#else
	return write (fio->handle, data, size);
	#endif
#endif
}
