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
	qse_fs_char_t* old_path;
	qse_fs_char_t* new_path;
#elif defined(__DOS__)
	qse_fs_char_t* old_path;
	qse_fs_char_t* new_path;
#else
	qse_fs_char_t* old_path;
	qse_fs_char_t* new_path;
	qse_fs_char_t* new_path2;

	#if defined(HAVE_LSTAT)
	qse_lstat_t old_stat;
	qse_lstat_t new_stat;
	#else
	qse_stat_t old_stat;
	qse_stat_t new_stat;
	#endif
#endif
};

typedef struct fop_t fop_t;


/* internal flags. it must not overlap with qse_fs_cpfile_flag_t enumerators */
#define CPFILE_DST_ATTR (1 << 30)

struct cpfile_t
{
	int flags;

	qse_fs_char_t* src_fspath;
	qse_fs_char_t* dst_fspath;

	qse_char_t* src_path;
	qse_char_t* dst_path;

	qse_fs_attr_t src_attr;
	qse_fs_attr_t dst_attr;
};
typedef struct cpfile_t cpfile_t;


int qse_fs_move (qse_fs_t* fs, const qse_char_t* oldpath, const qse_char_t* newpath)
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
	#if defined(HAVE_LSTAT)
	if (QSE_LSTAT (fop.old_path, &fop.old_stat) == -1)
	#else
	if (QSE_STAT (fop.old_path, &fop.old_stat) == -1) /* is this ok to use stat? */
	#endif
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		goto oops;
	}
	else fop.flags |= FOP_OLD_STAT;

	#if defined(HAVE_LSTAT)
	if (QSE_LSTAT (fop.new_path, &fop.new_stat) == -1)
	#else
	if (QSE_STAT (fop.new_path, &fop.new_stat) == -1)
	#endif
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

static int move_file_in_fs (qse_fs_t* fs, const qse_fs_char_t* oldpath, const qse_fs_char_t* newpath, int flags)
{
#if defined(_WIN32)
	/* ------------------------------------------------------ */

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
	APIRET rc;

	rc = DosMove (oldpath, newpath);
	if (rc == ERROR_ALREADY_EXISTS || rc == ERROR_ACCESS_DENIED)
	{
		DosDelete (fop.new_path);
		rc = DosMove (oldpath, newpath);
	}
	if (rc != NO_ERROR)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		return -1;
	}

	return 0;

	/* ------------------------------------------------------ */

#elif defined(__DOS__)

	/* ------------------------------------------------------ */
	if (rename (oldpath, newpath) <= -1)
	{
		/* FYI, rename() on watcom seems to set 
		 * errno to EACCES when the new path exists. */

		unlink (newpath);
		if (rename (oldpath, newpath) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	}

	return 0;

	/* ------------------------------------------------------ */

#else

	if (!(flags & QSE_FS_CPFILE_REPLACE))
	{
		qse_lstat_t st;
		if (QSE_LSTAT (newpath, &st) >= 0)
		{
			fs->errnum = QSE_FS_EEXIST;
			return -1;
		}
	}

	if (QSE_RENAME (oldpath, newpath) == -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

	return 0;

#endif
}













/*
#if defined(_WIN32)
DWORD copy_file_progress (
	LARGE_INTEGER TotalFileSize,
	LARGE_INTEGER TotalBytesTransferred,
	LARGE_INTEGER StreamSize,
	LARGE_INTEGER StreamBytesTransferred,
	DWORD dwStreamNumber,
	DWORD dwCallbackReason,
	HANDLE hSourceFile,
	HANDLE hDestinationFile,
	LPVOID lpData)
{
}
#endif
*/

/* copy
 * -> progress
 * -> abort/cancel
 * -> replace/overwrite
 * -> symbolic link
 */

static int copy_file_in_fs (qse_fs_t* fs, const cpfile_t* cpfile)
{
#if defined(_WIN32)
	/* ------------------------------------------------------ */
	DWORD copy_flags = 0;


	if (flags & QSE_FS_CPFILE_SYMLINK)
		copy_flags |= COPY_FILE_COPY_SYMLINK;
	if (!(flags & QSE_FS_CPFILE_REPLACE))
		copy_flags |= COPY_FILE_FAIL_IF_EXISTS;

/*
	if (fs->cbs.cp)
	{
		Specify callback???
	}
*/

	if (CopyFileEx (oldpath, newpath,  QSE_NULL, QSE_NULL, QSE_NULL, copy_flags) == FALSE)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, e);
		return -1;
	}

	return 0;
	/* ------------------------------------------------------ */

#elif defined(__OS2__)
	/* ------------------------------------------------------ */

	APIRET rc;
	USHORT opmode = 0;

	if (flags & QSE_FS_CPFILE_REPLACE) opmode |= 1; /* set bit 0 */

	rc = DosCopy (oldpath, newpath, opmode, 0);
	if (rc != NO_ERROR)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		return -1;
	}

	return 0;

	/* ------------------------------------------------------ */

#elif defined(__DOS__)

	/* ------------------------------------------------------ */
	if (rename (oldpath, newpath) <= -1)
	{
		/* FYI, rename() on watcom seems to set 
		 * errno to EACCES when the new path exists. */

		unlink (newpath);
		if (rename (oldpath, newpath) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	}

	return 0;

	/* ------------------------------------------------------ */

#else
	if ((cpfile->flags & QSE_FS_CPFILE_SYMLINK) && cpfile->src_attr.islnk)
	{
		qse_fs_char_t* tmpbuf;

		/* TODO: use a static buffer is size is small enough */
		tmpbuf = QSE_MMGR_ALLOC (fs->mmgr, QSE_SIZEOF(*tmpbuf) * (cpfile->src_attr.size + 1));
		if (tmpbuf == QSE_NULL)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		if (QSE_READLINK (cpfile->src_fspath, tmpbuf, cpfile->src_attr.size) <= -1 ||
		    QSE_SYMLINK (tmpbuf, cpfile->dst_fspath) <= -1)
		{
			QSE_MMGR_FREE (fs->mmgr, tmpbuf);
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}

		QSE_MMGR_FREE (fs->mmgr, tmpbuf);
		return 0;
	}
	else
	{
		int in = -1, out = -1;
		qse_ssize_t in_len, out_len;
		qse_uint8_t* bp;

		in = QSE_OPEN (cpfile->src_fspath, O_RDONLY, 0);
		out = QSE_OPEN (cpfile->dst_fspath, O_CREAT | O_WRONLY | O_TRUNC, 0777); /* TODO: proper mode */

		if (in <= -1 || out <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			goto oops;
		}

		while (1)
		{
			in_len = QSE_READ (in, fs->cpbuf, QSE_SIZEOF(fs->cpbuf));
			if (in_len <= 0) break;

	/* TODO: call progress callback */

			bp = fs->cpbuf;
			while (in_len > 0)
			{
				out_len = QSE_WRITE (out, bp, in_len);
				if (out_len <= -1) goto oops;
				bp += out_len;
				in_len -= out_len;
			}
		}

		QSE_CLOSE (out);
		QSE_CLOSE (in);
		return 0;

	oops:
		if (out >= 0) QSE_CLOSE (out);
		if (in >= 0) QSE_CLOSE (in);
		return -1;
	}
#endif
}


static int copy_file (qse_fs_t* fs, cpfile_t* cpfile)
{

	if (cpfile->src_attr.isdir)
	{
		fs->errnum = QSE_FS_ENOIMPL; /* TODO: copy a directory into a  directory */
		return -1;
	}
	else
	{
		/* TODO: check if it's itself */

		if (cpfile->flags & CPFILE_DST_ATTR) 
		{
			if (cpfile->src_attr.ino == cpfile->dst_attr.ino && 
			    cpfile->src_attr.dev == cpfile->dst_attr.dev)
			{
				/* cannot copy a file to itself */
				fs->errnum = QSE_FS_EINVAL; /* TODO: better error code */
				return -1;
			}

			if (cpfile->dst_attr.isdir)
			{
				/* copy it to directory */
				fs->errnum = QSE_FS_ENOIMPL; /* TODO: copy a file into a  directory */
				return -1;
			}

			if (!(cpfile->flags & QSE_FS_CPFILE_REPLACE))
			{
				fs->errnum = QSE_FS_EEXIST;
				return -1;
			}
		}

		if (!cpfile->src_attr.isdir) 
		{
			/* source is not a directory. */
			return copy_file_in_fs (fs, cpfile);
		}

		fs->errnum = QSE_FS_ENOIMPL; /* TODO: copy a file into a  directory */
		return -1;
	}
}

int qse_fs_cpfilembs (qse_fs_t* fs, const qse_mchar_t* srcpath, const qse_mchar_t* dstpath, int flags)
{
	cpfile_t cpfile;
	int ret;

	QSE_MEMSET (&cpfile, 0, QSE_SIZEOF(cpfile));

	cpfile.flags = flags & QSE_FS_CPFILE_ALL; /* public flags only */

	cpfile.src_fspath = (qse_fs_char_t*)qse_fs_makefspathformbs (fs, srcpath);
	cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_makefspathformbs (fs, dstpath);
	if (!cpfile.src_fspath || !cpfile.dst_fspath) goto oops;

	if (qse_fs_getattr (fs, cpfile.src_fspath, &cpfile.src_attr) <= -1) goto oops;
	if (qse_fs_getattr (fs, cpfile.dst_fspath, &cpfile.dst_attr) >= 0) cpfile.flags |= CPFILE_DST_ATTR;

	ret = copy_file (fs, &cpfile);

	qse_fs_freefspathformbs (fs, dstpath, cpfile.dst_fspath);
	qse_fs_freefspathformbs (fs, srcpath, cpfile.src_fspath);
	return ret;

oops:
	if (cpfile.dst_fspath) qse_fs_freefspathformbs (fs, srcpath, cpfile.dst_fspath);
	if (cpfile.src_fspath) qse_fs_freefspathformbs (fs, dstpath, cpfile.src_fspath);
	return -1;
}

int qse_fs_cpfilewcs (qse_fs_t* fs, const qse_wchar_t* srcpath, const qse_wchar_t* dstpath, int flags)
{
	cpfile_t cpfile;
	int ret;

	QSE_MEMSET (&cpfile, 0, QSE_SIZEOF(cpfile));

	cpfile.flags = flags & QSE_FS_CPFILE_ALL; /* public flags only */

	cpfile.src_fspath = (qse_fs_char_t*)qse_fs_makefspathforwcs (fs, srcpath);
	cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_makefspathforwcs (fs, dstpath);
	if (!cpfile.src_fspath || !cpfile.dst_fspath) goto oops;

	if (qse_fs_getattr (fs, cpfile.src_fspath, &cpfile.src_attr) <= -1) goto oops;
	if (qse_fs_getattr (fs, cpfile.dst_fspath, &cpfile.dst_attr) >= 0) cpfile.flags |= CPFILE_DST_ATTR;

	ret = copy_file (fs, &cpfile);

	qse_fs_freefspathforwcs (fs, dstpath, cpfile.dst_fspath);
	qse_fs_freefspathforwcs (fs, srcpath, cpfile.src_fspath);
	return ret;

oops:
	if (cpfile.dst_fspath) qse_fs_freefspathforwcs (fs, srcpath, cpfile.dst_fspath);
	if (cpfile.src_fspath) qse_fs_freefspathforwcs (fs, dstpath, cpfile.src_fspath);
	return -1;
}
