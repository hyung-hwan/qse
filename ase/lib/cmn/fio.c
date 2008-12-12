/*
 * $Id: fio.c,v 1.23 2006/06/30 04:18:47 bacon Exp $
 */

#include <ase/cmn/fio.h>
#include "mem.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#endif

#if defined(ASE_USE_SYSCALL) && defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>
#endif

ase_fio_t* ase_fio_open (
	ase_mmgr_t* mmgr, ase_size_t ext, 
	const ase_char_t* path, int flags, int mode)
{
	ase_fio_t* fio;

	if (mmgr == ASE_NULL)
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;	
	}

	fio = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_fio_t) + ext);
	if (fio == ASE_NULL) return ASE_NULL;

	if (ase_fio_init (fio, mmgr, path, flags, mode) == ASE_NULL)
	{
		ASE_MMGR_FREE (mmgr, fio);
		return ASE_NULL;
	}

	return fio;
}

void ase_fio_close (ase_fio_t* fio)
{
	ase_fio_fini (fio);
	ASE_MMGR_FREE (fio->mmgr, fio);
}

ase_fio_t* ase_fio_init (
	ase_fio_t* fio, ase_mmgr_t* mmgr,
	const ase_char_t* path, int flags, int mode)
{
	ase_fio_hnd_t handle;

	ASE_MEMSET (fio, 0, ASE_SIZEOF(*fio));
	fio->mmgr = mmgr;

#ifdef _WIN32
	if (flags & ASE_FIO_HANDLE)
	{
		handle = *(ase_fio_hnd_t*)path;
	}
	else
	{
		DWORD desired_access = 0;
		DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		DWORD creation_disposition = 0;
		DWORD attributes = FILE_ATTRIBUTE_NORMAL;

		if (flags & ASE_FIO_READ) desired_access |= GENERIC_READ;
		if (flags & ASE_FIO_WRITE) desired_access |= GENERIC_WRITE;
		if (flags & ASE_FIO_APPEND)  
		{
			/* this is not officialy documented for CreateFile.
			 * ZwCreateFile (kernel) seems to document it */
			desired_access |= FILE_APPEND_DATA;
		}
	
		if (flags & ASE_FIO_CREATE) 
		{
			creation_disposition = 
				(flags & ASE_FIO_EXCLUSIVE)? CREATE_NEW:
				(flags & ASE_FIO_TRUNCATE)? CREATE_ALWAYS: OPEN_ALWAYS;
		}
		else if (flags & ASE_FIO_TRUNCATE) 
		{
			creation_disposition = TRUNCATE_EXISTING;
		}
		else creation_disposition = OPEN_EXISTING;
	
		if (flags & ASE_FIO_NOSHRD) share_mode &= ~FILE_SHARE_READ;
		if (flags & ASE_FIO_NOSHWR) share_mode &= ~FILE_SHARE_WRITE;

		if (flags & ASE_FIO_SYNC) attributes |= FILE_FLAG_WRITE_THROUGH; 
		/* TODO: handle mode... set attribuets */
		handle = CreateFile (path, 
			desired_access, share_mode, ASE_NULL, 
			creation_disposition, attributes, 0);
	}

	if (handle == INVALID_HANDLE_VALUE) return ASE_NULL;

	{
		DWORD file_type = GetFileType(handle);
		if (file_type == FILE_TYPE_UNKNOWN) 
		{
			CloseHandle (handle);
			return ASE_NULL;
		}
	}

	/* TODO: a lot more */
#else

	if (flags & ASE_FIO_HANDLE)
	{
		handle = *(ase_fio_hnd_t*)path;
	}
	else
	{
		int desired_access = 0;
	#ifdef ASE_CHAR_IS_MCHAR
		const ase_mchar_t* path_mb = path;
	#else
		ase_mchar_t path_mb[PATH_MAX + 1];
		if (ase_wcstombs_strict (path, 
			path_mb, ASE_COUNTOF(path_mb)) == -1) return ASE_NULL;
	#endif

		if ((flags & ASE_FIO_READ) && 
		    (flags & ASE_FIO_WRITE)) desired_access |= O_RDWR;
		else if (flags & ASE_FIO_READ) desired_access |= O_RDONLY;
		else desired_access |= O_WRONLY;

		if (flags & ASE_FIO_APPEND) desired_access |= O_APPEND;
		if (flags & ASE_FIO_CREATE) desired_access |= O_CREAT;
		if (flags & ASE_FIO_TRUNCATE) desired_access |= O_TRUNC;
		if (flags & ASE_FIO_EXCLUSIVE) desired_access |= O_EXCL;
		if (flags & ASE_FIO_SYNC) desired_access |= O_SYNC;

	#if defined(O_LARGEFILE)
		desired_access |= O_LARGEFILE;
	#endif

	#ifdef SYS_open
		handle = syscall (SYS_open, path_mb, desired_access, mode);
	#else
		handle = open (path_mb, desired_access, mode);
	#endif
	}

	if (handle == -1) return ASE_NULL;

#endif

	fio->handle = handle;
	return fio;
}

void ase_fio_fini (ase_fio_t* fio)
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

ase_fio_hnd_t ase_fio_gethandle (ase_fio_t* fio)
{
	return fio->handle;
}

void ase_fio_sethandle (ase_fio_t* fio, ase_fio_hnd_t handle)
{
	fio->handle = handle;
}

ase_fio_off_t ase_fio_seek (
	ase_fio_t* fio, ase_fio_off_t offset, ase_fio_ori_t origin)
{
#ifdef _WIN32
	static int seek_map[] = 
	{
		FILE_BEGIN,
		FILE_CURRENT,
		FILE_END	
	};
	LARGE_INTEGER x, y;

	ASE_ASSERT (AES_SIZEOF(offset) <= AES_SIZEOF(x.QuadPart));

	x.QuadPart = offset;
	if (SetFilePointerEx (
		fio->handle, x, &y, seek_map[origin]) == FALSE) return -1;

	return (ase_fio_off_t)y.QuadPart;

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
		seek_map[origin]) == -1) return -1;

	return (ase_fio_off_t)tmp;

#elif defined(SYS_lseek)
	return syscall (SYS_lseek, fio->handle, offset, seek_map[origin]);
#elif !defined(_LP64) && defined(HAVE_LSEEK64)
	return lseek64 (fio->handle, offset, seek_map[origin]);
#else
	return lseek (fio->handle, offset, seek_map[origin]);
#endif

#endif
}

int ase_fio_truncate (ase_fio_t* fio, ase_fio_off_t size)
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

ase_ssize_t ase_fio_read (ase_fio_t* fio, void* buf, ase_size_t size)
{
#ifdef _WIN32
	DWORD count;
	if (size > ASE_TYPE_MAX(DWORD)) size = ASE_TYPE_MAX(DWORD);
	if (ReadFile(fio->handle, buf, size, &count, ASE_NULL) == FALSE) return -1;
	return (ase_ssize_t)count;
#else
	if (size > ASE_TYPE_MAX(size_t)) size = ASE_TYPE_MAX(size_t);
	#ifdef SYS_read 
	return syscall (SYS_read, fio->handle, buf, size);
	#else
	return read (fio->handle, buf, size);
	#endif
#endif
}

ase_ssize_t ase_fio_write (ase_fio_t* fio, const void* data, ase_size_t size)
{
#ifdef _WIN32
	DWORD count;
	if (size > ASE_TYPE_MAX(DWORD)) size = ASE_TYPE_MAX(DWORD);
	if (WriteFile(fio->handle, data, size, &count, ASE_NULL) == FALSE) return -1;
	return (ase_ssize_t)count;
#else
	if (size > ASE_TYPE_MAX(size_t)) size = ASE_TYPE_MAX(size_t);
	#ifdef SYS_write
	return syscall (SYS_write, fio->handle, data, size);
	#else
	return write (fio->handle, data, size);
	#endif
#endif
}
