/*
 * $Id: sysapi.c,v 1.45 2007/06/20 14:54:11 baconevi Exp $
 */

#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#include <fcntl.h>
#endif

#ifdef _WIN32
	static int handle_map_inited = 0;
	static ase_map_t handle_map;
#endif


ase_hnd_t ase_open (const ase_char_t* path, int flags, ...)
{
	ase_hnd_t handle;

#ifdef _WIN32

	DWORD desired_access = 0;
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD creation_disposition = 0;
	DWORD attributes = FILE_ATTRIBUTE_NORMAL;
	DWORD file_type;
	
	if (!handle_map_inited)
	{
		ase_map_init (&handle_map, ...);
		handle_map_inited = 1;
	}

	if (flags & ASE_OPEN_READ) desired_access |= GENERIC_READ;
	if (flags & ASE_OPEN_WRITE) desired_access |= GENERIC_WRITE;

	if (flags & ASE_OPEN_CREATE) 
	{
		creation_disposition = 
			(flags & ASE_OPEN_EXCLUSIVE)? CREATE_NEW:
			(flags & ASE_OPEN_TRUNCATE)? CREATE_ALWAYS: OPEN_ALWAYS;
	}
	else if (flags & ASE_OPEN_TRUNCATE) 
	{
		creation_disposition = TRUNCATE_EXISTING;
	}
	else creation_disposition = OPEN_EXISTING;

	/*if (flags & ASE_FIO_SYNC) attributes |= FILE_FLAG_WRITE_THROUGH; */
	/* TODO: share mode */

	handle = CreateFile (path, 
		desired_access, share_mode, ASE_NULL, 
		creation_disposition, attributes, 0);
	if (handle == ASE_HND_INVALID) return ASE_HND_INVALID;

/* TODO: improve append */
	if (flags & ASE_OPEN_APPEND) 
	{
		/*fio->flags |= ASE_OPEN_APPEND;*/
		if (ase_seek (handle, 0, ASE_SEEK_END) == -1)
		{
			CloseHandle (handle);
			return ASE_HND_INVALID;
		}
	}

/* TODO: implement nonblock...
	if (flags & ASE_OPEN_NONBLOCK) desired_access |= O_NONBLOCK;
*/

	file_type = GetFileType(handle);
	if (file_type == FILE_TYPE_UNKNOWN) 
	{
		CloseHandle (handle);
		return ASE_HND_INVALID;
	}

	/*
	if (file_type == FILE_TYPE_CHAR) 
	else if (file_type == FILE_TYPE_PIPE)
	*/
	/* TODO: a lot more */

#else
	int desired_access = 0;
	ase_va_list arg;
	ase_mode_t mode;

	#ifdef ASE_CHAR_IS_MCHAR
	const ase_mchar_t* path_mb;
	#else
	ase_mchar_t path_mb[ASE_PATH_MAX + 1];
	#endif

	ase_va_start (arg, flags);
	if (ase_sizeof(ase_mode_t) < ase_sizeof(int))
		mode = ase_va_arg (arg, int);
	else
		mode = ase_va_arg (arg, ase_mode_t);
	ase_va_end (arg);

	#ifdef ASE_CHAR_IS_MCHAR
	path_mb = path;
	#else
	if (ase_wcstomcs_strict (
		path, path_mb, ase_countof(path_mb)) == -1) return -1;
	#endif

	if (flags & ASE_OPEN_READ) desired_access = O_RDONLY;
	if (flags & ASE_OPEN_WRITE) {
		if (desired_access == 0) desired_access |= O_WRONLY;
		else desired_access = O_RDWR;
	}

	if (flags & ASE_OPEN_APPEND) desired_access |= O_APPEND;
	if (flags & ASE_OPEN_CREATE) desired_access |= O_CREAT;
	if (flags & ASE_OPEN_TRUNCATE) desired_access |= O_TRUNC;
	if (flags & ASE_OPEN_EXCLUSIVE) desired_access |= O_EXCL;
	if (flags & ASE_OPEN_NONBLOCK) desired_access |= O_NONBLOCK;

	handle = open (path_mb, desired_access, mode);
#endif

	return handle;
}

int ase_close (ase_hnd_t handle)
{
#ifdef _WIN32
	if (CloseHandle(handle) == FALSE) return -1;
	return 0;
#else
	return close (handle);
#endif
}

ase_ssize_t ase_read (ase_hnd_t handle, void* buf, ase_size_t size)
{
#ifdef _WIN32

	DWORD bytes_read;
	ase_size_t n = 0;
	ase_byte_t* p = (ase_byte_t*)buf;

	while (1) 
	{
		if (size <= ASE_TYPE_MAX(DWORD)) 
		{
			if (ReadFile(handle, p, size, 
				&bytes_read, ASE_NULL) == FALSE) return -1;
			n += bytes_read;
			break;
		}

		if (ReadFile(handle, p, ASE_TYPE_MAX(DWORD),
			&bytes_read, ASE_NULL) == FALSE) return -1;
		if (bytes_read == 0) break; /* reached the end of a file */

		p += bytes_read;
		n += bytes_read;
		size -= bytes_read;
	}

	return (ase_ssize_t)n;

#else
	return read (handle, buf, size);
#endif
}

ase_ssize_t ase_write (ase_hnd_t handle, const void* data, ase_size_t size)
{
#ifdef _WIN32

	DWORD bytes_written;
	ase_size_t n = 0;
	const ase_byte_t* p = (const ase_byte_t*)data;

	/* TODO:...
	if (fio->flags & ASE_OPEN_APPEND) 
	{
		if (ase_seek (handle, 0, ASE_SEEK_END) == -1) return -1;
	}*/

	for (;;)
	{
		if (size <= ASE_TYPE_MAX(DWORD)) 
		{
			if (WriteFile(handle, p, size, 
				&bytes_written, ASE_NULL) == FALSE) return -1;
			n += bytes_written;
			break;
		}

		if (WriteFile(handle, p, ASE_TYPE_MAX(DWORD), 
			&bytes_written, ASE_NULL) == FALSE) return -1;
		if (bytes_written == 0) break;

		p += bytes_written;
		n += bytes_written;
		size -= bytes_written;
	}

	return (ase_ssize_t)n;

#else
	return write (handle, data, size);
#endif
}

ase_off_t ase_seek (ase_hnd_t handle, ase_off_t offset, int origin)
{
#ifdef _WIN32

	static int __seek_map[] = 
	{
		FILE_BEGIN,
		FILE_CURRENT,
		FILE_END	
	};
	LARGE_INTEGER x, y;

	ase_assert (ase_sizeof(offset) <= ase_sizeof(x.QuadPart));

	x.QuadPart = offset;
	if (SetFilePointerEx (
		handle, x, &y, __seek_map[origin]) == FALSE) return -1;

	return (ase_off_t)y.QuadPart;

#else

	static int __seek_map[] = 
	{
		SEEK_SET,
		SEEK_CUR,
		SEEK_END
	};

	return lseek (handle, offset, __seek_map[origin]);

#endif
}

int ase_htruncate (ase_hnd_t handle, ase_off_t size)
{
#ifdef _WIN32
	#ifndef INVALID_SET_FILE_POINTER
	#define INVALID_SET_FILE_POINTER ((DWORD)-1)
	#endif

	/* TODO: support lsDistanceToMoveHigh (high 32bits of 64bit offset) */
	if (SetFilePointer(
	    	handle,0,NULL,FILE_CURRENT) == INVALID_SET_FILE_POINTER ||
	    SetFilePointer(
	    	handle,size,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
	    SetEndOfFile(handle) == FALSE) return -1;

	return 0;
#else
	return ftruncate (handle, size);
#endif
}

