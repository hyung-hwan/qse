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

#include "fs.h"
#include <qse/cmn/mbwc.h>
#include <qse/cmn/path.h>
#include <qse/cmn/str.h>
#include "mem.h"

/*
OVERWRITE AND FORCE handled by callback???
QSE_FS_MOVE_UPDATE
QSE_FS_MOVE_BACKUP_SIMPLE
QSE_FS_MOVE_BACKUP_NUMBERED
*/
enum fop_flag_t
{
	FOP_OLD_STAT = (1 << 0),
	FOP_NEW_STAT = (1 << 1),

	FOP_CROSS_DEV = (1 << 2)
};

struct fop_t
{
	int flags;

#if defined(_WIN32)
	/* nothing yet */
#elif defined(__OS2__)
	qse_mchar_t* old_path;
	qse_mchar_t* new_path;
#elif defined(__DOS__)
	qse_mchar_t* old_path;
	qse_mchar_t* new_path;
#else
	qse_mchar_t* old_path;
	qse_mchar_t* new_path;
	qse_mchar_t* new_path2;

	qse_lstat_t old_stat;
	qse_lstat_t new_stat;
#endif
};

typedef struct fop_t fop_t;

int qse_fs_move (
	qse_fs_t* fs, const qse_char_t* oldpath, const qse_char_t* newpath)
{

#if defined(_WIN32)
	/* ------------------------------------------------------ */
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
	/* ------------------------------------------------------ */

#elif defined(__OS2__)
	/* ------------------------------------------------------ */

	/* TODO: improve it */
	int ret = 0;
	fop_t fop;

	QSE_MEMSET (&fop, 0, QSE_SIZEOF(fop));

	#if defined(QSE_CHAR_IS_MCHAR)
	fop.old_path = oldpath;
	fop.new_path = newpath;
	#else
	fop.old_path = qse_wcstombsdup (oldpath, QSE_NULL, fs->mmgr);
	fop.new_path = qse_wcstombsdup (newpath, QSE_NULL, fs->mmgr);
	if (fop.old_path == QSE_NULL || fop.old_path == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		ret = -1;
	}
	#endif

	if (ret == 0)
	{
		APIRET rc;

		rc = DosMove (fop.old_path, fop.new_path);
		if (rc == ERROR_ALREADY_EXISTS || rc == ERROR_ACCESS_DENIED)
		{
			DosDelete (fop.new_path);
			rc = DosMove (fop.old_path, fop.new_path);
		}
		if (rc != NO_ERROR)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
			ret = -1;
		}
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing special */
	#else
	if (fop.old_path) QSE_MMGR_FREE (fs->mmgr, fop.old_path);
	if (fop.new_path) QSE_MMGR_FREE (fs->mmgr, fop.new_path);
	#endif
	return ret;


	/* ------------------------------------------------------ */

#elif defined(__DOS__)

	/* ------------------------------------------------------ */
/* TODO: improve it */
	fop_t fop;
	int ret = 0;

	QSE_MEMSET (&fop, 0, QSE_SIZEOF(fop));

	#if defined(QSE_CHAR_IS_MCHAR)
	fop.old_path = oldpath;
	fop.new_path = newpath;
	#else
	fop.old_path = qse_wcstombsdup (oldpath, QSE_NULL, fs->mmgr);
	fop.new_path = qse_wcstombsdup (newpath, QSE_NULL, fs->mmgr);
	if (fop.old_path == QSE_NULL || fop.old_path == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		ret = -1;
	}
	#endif

	if (ret == 0)
	{
		if (rename (fop.old_path, fop.new_path) <= -1)
		{
			/* FYI, rename() on watcom seems to set 
			 * errno to EACCES when the new path exists. */

			unlink (fop.new_path);
			if (rename (fop.old_path, fop.new_path) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				ret = -1;
			}
		}
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing special */
	#else
	if (fop.old_path) QSE_MMGR_FREE (fs->mmgr, fop.old_path);
	if (fop.new_path) QSE_MMGR_FREE (fs->mmgr, fop.new_path);
	#endif
	return ret;

	/* ------------------------------------------------------ */

#else

	/* ------------------------------------------------------ */
	fop_t fop;
	QSE_MEMSET (&fop, 0, QSE_SIZEOF(fop));

	#if defined(QSE_CHAR_IS_MCHAR)
	fop.old_path = oldpath;
	fop.new_path = newpath;
	#else
	fop.old_path = qse_wcstombsdup (oldpath, QSE_NULL, fs->mmgr);
	fop.new_path = qse_wcstombsdup (newpath, QSE_NULL, fs->mmgr);	
	if (fop.old_path == QSE_NULL || fop.old_path == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		goto oops;
	}
	#endif

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

	/* use lstat because we need to move the symbolic link 
	 * itself if the file is a symbolic link */ 
	if (QSE_LSTAT (fop.old_path, &fop.old_stat) == -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		goto oops;
	}
	else fop.flags |= FOP_OLD_STAT;

	if (QSE_LSTAT (fop.new_path, &fop.new_stat) == -1)
	{
		if (errno == ENOENT)
		{
			/* entry doesn't exist */
		}
	}
	else fop.flags |= FOP_NEW_STAT;

	if (fop.flags & FOP_NEW_STAT)
	{
		/* destination file exits */
		if (fop.old_stat.st_dev != fop.new_stat.st_dev)
		{
			/* cross-device */
			fop.flags |= FOP_CROSS_DEV;
		}
		else
		{
			if (fop.old_stat.st_ino == fop.new_stat.st_ino)
			{
				/* both source and destination are the same.
				 * this operation is not allowed */
				fs->errnum = QSE_FS_EACCES;
				goto oops;
			}

/* TODO: destination should point to an actual file or directory for this check to work */
/* TOOD: ask to overwrite the source */
			if (S_ISDIR(fop.new_stat.st_mode))
			{
				/* the destination is a directory. move the source file
				 * into the destination directory */
				const qse_wchar_t* arr[4];

				arr[0] = newpath;
				arr[1] = QSE_T("/");
				arr[2] = qse_basename(oldpath);
				arr[3] = QSE_NULL;
			#if defined(QSE_CHAR_IS_MCHAR)
				fop.new_path2 = qse_stradup (arr, QSE_NULL, fs->mmgr);
			#else
				fop.new_path2 = qse_wcsatombsdup (arr, QSE_NULL, fs->mmgr);	
			#endif
				if (fop.new_path2 == QSE_NULL)
				{
					fs->errnum = QSE_FS_ENOMEM;
					goto oops;
				}
			}
			else
			{
				/* the destination file is not directory, unlink first 
				 * TODO: but is this necessary? RENAME will do it */
				QSE_UNLINK (fop.new_path);
			}
		}
	}

	if (!(fop.flags & FOP_CROSS_DEV))
	{
	/* TODO: make it better to be able to move non-empty diretories
    	     improve it to be able to move by copy/delete across volume */
		if (QSE_RENAME (fop.old_path, fop.new_path) == -1)
		{
			if (errno != EXDEV)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				goto oops;
			}

			fop.flags |= FOP_CROSS_DEV;
		}
		else goto done;
	}

	QSE_ASSERT (fop.flags & FOP_CROSS_DEV);

	if (!S_ISDIR(fop.old_stat.st_mode))
	{
		/* copy a single file */
		/* ............ */	
	}

#if 0
	if (!recursive)
	{
		fs->errnum = QSE_FS_E....;	
		goto oops;
	}

	copy recursively...
#endif


done:
	#if defined(QSE_CHAR_IS_MCHAR)
	if (fop.new_path2) QSE_MMGR_FREE (fs->mmgr, fop.new_path2);
	#else
	if (fop.new_path2) QSE_MMGR_FREE (fs->mmgr, fop.new_path2);
	QSE_MMGR_FREE (fs->mmgr, fop.old_path);
	QSE_MMGR_FREE (fs->mmgr, fop.new_path);
	#endif
	return 0;

oops:
	#if defined(QSE_CHAR_IS_MCHAR)
	if (fop.new_path2) QSE_MMGR_FREE (fs->mmgr, fop.new_path2);
	#else
	if (fop.new_path2) QSE_MMGR_FREE (fs->mmgr, fop.new_path2);
	if (fop.old_path) QSE_MMGR_FREE (fs->mmgr, fop.old_path);
	if (fop.new_path) QSE_MMGR_FREE (fs->mmgr, fop.new_path);
	#endif
	return -1;
	/* ------------------------------------------------------ */

#endif
}

typedef struct del_op_t del_op_t;
struct del_op_t
{
#if defined(_WIN32)
	/* nothing */
#elif defined(__OS2__)
	qse_mchar_t* path;
#elif defined(__DOS__)
	qse_mchar_t* path;
#else
	qse_mchar_t* path;
#endif
};

int qse_fs_delete (qse_fs_t* fs, const qse_char_t* path)
{
	/* TODO: improve this function to support fs->curdir ... etc 
	 * delete directory ... etc */

#if defined(_WIN32)

	if (DeleteFile (path) == FALSE)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
		return -1;
	}

	return 0;


#elif defined(__OS2__)

	/* ------------------------------------------------------ */

	APIRET rc;
	del_op_t dop;
	

	QSE_MEMSET (&dop, 0, QSE_SIZEOF(dop));

	#if defined(QSE_CHAR_IS_MCHAR)
	dop.path = path;
	#else
	dop.path = qse_wcstombsdup (path, QSE_NULL, fs->mmgr);
	if (dop.path == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		goto oops;
	}
	#endif

	rc = DosDelete (dop.path);
	if (rc != NO_ERROR)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		goto oops;
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
	#else
	QSE_MMGR_FREE (fs->mmgr, dop.path);
	#endif
	return 0;

	/* ------------------------------------------------------ */

oops:
	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
	#else
	if (dop.path) QSE_MMGR_FREE (fs->mmgr, dop.path);
	#endif
	return -1;

#elif defined(__DOS__)

	/* ------------------------------------------------------ */

	del_op_t dop;

	QSE_MEMSET (&dop, 0, QSE_SIZEOF(dop));

	#if defined(QSE_CHAR_IS_MCHAR)
	dop.path = path;
	#else
	dop.path = qse_wcstombsdup (path, QSE_NULL, fs->mmgr);
	if (dop.path == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		goto oops;
	}
	#endif

	if (unlink (dop.path) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		goto oops;
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
	#else
	QSE_MMGR_FREE (fs->mmgr, dop.path);
	#endif
	return 0;

oops:
	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
	#else
	if (dop.path) QSE_MMGR_FREE (fs->mmgr, dop.path);
	#endif
	return -1;

	/* ------------------------------------------------------ */

#else

	/* ------------------------------------------------------ */

	del_op_t dop;

	QSE_MEMSET (&dop, 0, QSE_SIZEOF(dop));

	#if defined(QSE_CHAR_IS_MCHAR)
	dop.path = path;
	#else
	dop.path = qse_wcstombsdup (path, QSE_NULL, fs->mmgr);
	if (dop.path == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		goto oops;
	}
	#endif

	if (QSE_UNLINK (dop.path) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		goto oops;
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
	#else
	QSE_MMGR_FREE (fs->mmgr, dop.path);
	#endif
	return 0;

oops:
	#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
	#else
	if (dop.path) QSE_MMGR_FREE (fs->mmgr, dop.path);
	#endif
	return -1;

	/* ------------------------------------------------------ */

#endif
}
