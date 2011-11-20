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
#include <qse/cmn/str.h>
#include "mem.h"

int qse_fs_move (qse_fs_t* fs, const qse_char_t* oldpath, const qse_char_t* newpath)
{
#if defined(_WIN32)
	/* TODO: improve it... */

/* TODO: support cross-volume move, move by copy/delete, etc ... */
	if (MoveFile (oldpath, newpath) == FALSE)
	{
		DWORD e = GetLastError();
		if (e == ERROR_ALREADY_EXISTS)
		{
			DeleteFile (newpath);
			if (MoveFile (oldpath, newpath) == FALSE)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
				return -1;
			}
		}
		else
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, e);
			return -1;
		}
	}

	return 0;

#elif defined(__OS2__)
#	error NOT IMPLEMENTED
#elif defined(__DOS__)
#	error NOT IMPLEMENTED
#else

	const qse_mchar_t* mbsoldpath;
	const qse_mchar_t* mbsnewpath;

#if defined(QSE_CHAR_IS_MCHAR)
	mbsoldpath = oldpath;
	mbsnewpath = newpath;
#else
	mbsoldpath = qse_wcstombsdup (oldpath, fs->mmgr);
	mbsnewpath = qse_wcstombsdup (newpath, fs->mmgr);	

	if (mbsoldpath == QSE_NULL || mbsnewpath == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		goto oops;
	}

/* TOOD: implement confirmatio
	if (overwrite_callback is set)
	{
		checkif the the mbsnewpat exists.
		if (it exists)
		{
			call fs->confirm_overwrite_callback (....)
		}
	}
*/

/* TODO: make it better to be able to move non-empty diretories
         improve it to be able to move by copy/delete across volume */
	if (QSE_RENAME (mbsoldpath, mbsnewpath) == -1)
	{
		if (errno == EXDEV)
		{
			/* TODO: move it by copy and delete intead of returnign error... */
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			goto oops;
		}
		else
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			goto oops;
		}
	}

#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to free */
#else
	QSE_MMGR_FREE (fs->mmgr, mbsoldpath);
	QSE_MMGR_FREE (fs->mmgr, mbsnewpath);
#endif
	return 0;

oops:
#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to free */
#else
	if (mbsoldpath) QSE_MMGR_FREE (fs->mmgr, mbsoldpath);
	if (mbsnewpath) QSE_MMGR_FREE (fs->mmgr, mbsnewpath);
#endif

	return -1;

#endif

#endif
}
