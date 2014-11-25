/* 
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fs.h"

qse_fs_errnum_t qse_fs_geterrnum (qse_fs_t* fs)
{
	return fs->errnum;
}

qse_fs_errnum_t qse_fs_syserrtoerrnum (qse_fs_t* fs, qse_fs_syserr_t e)
{
#if defined(_WIN32)
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return QSE_FS_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
		case ERROR_DIRECTORY:
			return QSE_FS_EINVAL;

		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
			return QSE_FS_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_FS_ENOENT;

		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return QSE_FS_EEXIST;

		case ERROR_NOT_SAME_DEVICE:
			return QSE_FS_EXDEV;

		case ERROR_DIR_NOT_EMPTY;
			return QSE_FS_ENOTEMPTY;

		default:
			return QSE_FS_ESYSERR;
	}
#elif defined(__OS2__)

	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
			return QSE_FS_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_FS_EINVAL;

		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
			return QSE_FS_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_FS_ENOENT;

		case ERROR_ALREADY_EXISTS:
			return QSE_FS_EEXIST;

		case ERROR_NOT_SAME_DEVICE:
			return QSE_FS_EXDEV;

		default:
			return QSE_FS_ESYSERR;
	}

#elif defined(__DOS__)

	switch (e)
	{
		case ENOMEM:
			return QSE_FS_ENOMEM;

		case EINVAL:
			return QSE_FS_EINVAL;

		case EACCES:
		case EPERM:
			return QSE_FS_EACCES;

		case ENOENT:
			return QSE_FS_ENOENT;

		case EEXIST:
			return QSE_FS_EEXIST;

		case EISDIR: 
			return QSE_FS_EISDIR;

		case ENOTDIR:
			return QSE_FS_ENOTDIR;

		default:
			return QSE_FS_ESYSERR;
	}

#else
	switch (e)
	{
		case ENOMEM:
			return QSE_FS_ENOMEM;

		case EINVAL:
			return QSE_FS_EINVAL;

		case EACCES:
		case EPERM:
			return QSE_FS_EACCES;

		case ENOENT:
			return QSE_FS_ENOENT;

		case EEXIST:
			return QSE_FS_EEXIST;

		case EINTR:
			return QSE_FS_EINTR;

		case EISDIR: 
			return QSE_FS_EISDIR;

		case ENOTDIR:
			return QSE_FS_ENOTDIR;

		case ENOTEMPTY:
			return QSE_FS_ENOTVOID;

		case EXDEV:
			return QSE_FS_EXDEV;

		default:
			return QSE_FS_ESYSERR;
	}
#endif
}


qse_fs_errnum_t qse_fs_direrrtoerrnum (qse_fs_t* fs, qse_dir_errnum_t e)
{
	switch (e)
	{
		case QSE_DIR_ENOERR:
			return QSE_FS_ENOERR;

		case QSE_DIR_EOTHER:
			return QSE_FS_EOTHER;

		case QSE_DIR_ENOIMPL:
			return QSE_FS_ENOIMPL;

		case QSE_DIR_ESYSERR:
			return QSE_FS_ESYSERR;

		case QSE_DIR_EINTERN:
			return QSE_FS_EINTERN;

		case QSE_DIR_ENOMEM:
			return QSE_FS_ENOMEM;

		case QSE_DIR_EINVAL:
			return QSE_FS_EINVAL;

		case QSE_DIR_EACCES:
			return QSE_FS_EACCES;

		case QSE_DIR_ENOENT:
			return QSE_FS_ENOENT;

		case QSE_DIR_EEXIST:
			return QSE_FS_EEXIST;

		case QSE_DIR_EINTR:
			return QSE_FS_EINTR;

		case QSE_DIR_EPIPE:
			return QSE_FS_EPIPE;

		case QSE_DIR_EAGAIN:
			return QSE_FS_EAGAIN;

		default:
			return QSE_FS_EOTHER;
	}
}
