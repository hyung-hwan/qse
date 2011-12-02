/* 
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include "fs.h"

qse_fs_errnum_t qse_fs_geterrnum (qse_fs_t* fs)
{
	return fs->errnum;
}

const qse_char_t* qse_fs_geterrmsg (qse_fs_t* fs)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("internal error that should never have happened"),

		QSE_T("insufficient memory"),
		QSE_T("invalid parameter or data"),
		QSE_T("access denied"),
		QSE_T("operation not permitted"),
		QSE_T("no such entry"),
		QSE_T("no working directory set"),
		QSE_T("operation not permitted on directory"),
		QSE_T("entry already exists"),
		QSE_T("cross-device operation not allowed"),
		QSE_T("system error")
	};

	return (fs->errnum >= 0 && fs->errnum < QSE_COUNTOF(errstr))?
		errstr[fs->errnum]: QSE_T("unknown error");
}

qse_fs_errnum_t qse_fs_syserrtoerrnum (qse_fs_t* fs, qse_fs_syserr_t e)
{
#if defined(_WIN32)
	switch (e)
	{
		case ERROR_INVALID_NAME:
		case ERROR_DIRECTORY:
			return QSE_FS_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_FS_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_FS_ENOENT;

		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return QSE_FS_ENOMEM;

		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return QSE_FS_EEXIST;
	
		default:
			return QSE_FS_ESYSTEM;
	}
#else
	switch (e)
	{
		case EINVAL:
			return QSE_FS_EINVAL;

		case ENOMEM:
			return QSE_FS_ENOMEM;

		case EACCES:
			return QSE_FS_EACCES;

		case EPERM:
			return QSE_FS_EPERM;

		case ENOENT:
		case ENOTDIR:
			return QSE_FS_ENOENT;

		case EISDIR: 
			return QSE_FS_EISDIR;

		case EEXIST:
			return QSE_FS_EEXIST;

		case EXDEV:
			return QSE_FS_EXDEV;

		default:
			return QSE_FS_ESYSTEM;
	}
#endif
}
