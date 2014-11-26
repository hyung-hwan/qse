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

/* NOTE:
 * The current implementation require mbs/wcs conversion as
 * qse_dir_xxx() and qse_glob()  don't support mbs and wcs separately.
 * while the functions here support them. */

static int delete_file_from_fs (qse_fs_t* fs, const qse_fs_char_t* fspath)
{

#if defined(_WIN32)

	if (DeleteFile (fspath) == FALSE)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
		return -1;
	}

#elif defined(__OS2__)

	APIRET rc;

	rc = DosDelete (fspath);
	if (rc != NO_ERROR)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		return -1;
	}

#elif defined(__DOS__)

	if (unlink (fspath) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#else

	if (QSE_UNLINK (fspath) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#endif

	return 0;
}

static int delete_directory_from_fs (qse_fs_t* fs, const qse_fs_char_t* fspath)
{
#if defined(_WIN32)

	if (RemoveDirectory (fspath) == FALSE)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
		return -1;
	}

#elif defined(__OS2__)

	APIRET rc;

	rc = DosRmDir (fspath);
	if (rc != NO_ERROR)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		return -1;
	}

#elif defined(__DOS__)

	if (rmdir (fspath) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#else

	if (QSE_RMDIR (fspath) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#endif

	return 0;
}

/* --------------------------------------------------------------------- */

static int purge_directory_contents (qse_fs_t* fs, const qse_char_t* path);
static int purge_path (qse_fs_t* fs, const qse_char_t* path);
static int delete_directory_nocbs (qse_fs_t* fs, const qse_char_t* path);

static int delete_file (qse_fs_t* fs, const qse_char_t* path, int purge)
{
	qse_fs_char_t* fspath;
	int ret;

	if (fs->cbs.del) 
	{
		int x;
		x = fs->cbs.del (fs, path);
		if (x <= -1) return -1;
		if (x == 0) return 0; /* skipped */
	}

	fspath = qse_fs_makefspath(fs, path);
	if (!fspath) return -1;

	ret = delete_file_from_fs (fs, fspath);
	qse_fs_freefspath (fs, path, fspath);

	if (ret <= -1 && purge) 
	{
		ret = purge_directory_contents (fs, path);
		if (ret == -99)
		{
			/* it has attempted to delete path as a file above. 
			 * i don't attempt to delete it as a file again here 
			 * unlike purge path. */
			ret = -1;
		}
		else if (ret <= -1)
		{
			/* do nothing */
		}
		else
		{
			/* path is a directory name and contents have been purged. 
			 * call delete_directory_nocbs() instead of delete_directory()
			 * to avoid double calls to cb.del(). it has been called for
			 * 'path' * in this function above. */
			ret = delete_directory_nocbs (fs, path);
		}
	}

	return ret;
}

static int delete_directory_nocbs (qse_fs_t* fs, const qse_char_t* path)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspath(fs, path);
	if (!fspath) return -1;

	ret = delete_directory_from_fs (fs, fspath);
	qse_fs_freefspath (fs, path, fspath);

	return ret;
}

static int delete_directory (qse_fs_t* fs, const qse_char_t* path)
{
	if (fs->cbs.del) 
	{
		int x;
		x = fs->cbs.del (fs, path);
		if (x <= -1) return -1;
		if (x == 0) return 0; /* skipped */
	}

	return delete_directory_nocbs (fs, path);
}

static int purge_directory_contents (qse_fs_t* fs, const qse_char_t* path)
{
	qse_dir_t* dir;
	qse_dir_errnum_t errnum;

	/* 'dir' is asked to skip special entries like . and .. */
	dir = qse_dir_open (fs->mmgr, 0, path, QSE_DIR_LIMITED, &errnum);
	if (dir)
	{
		/* it must be a directory. delete all entries under it */
		int ret, x;
		qse_dir_ent_t ent;
		const qse_char_t* seg[4];
		qse_char_t* joined_path;

		while (1)
		{
			x = qse_dir_read (dir, &ent);
			if (x <= -1) 
			{
				fs->errnum = qse_fs_direrrtoerrnum (fs, qse_dir_geterrnum(dir));
				goto oops;
			}
			if (x == 0) break; /* no more entries */

			/* join path and ent->name.... */
			seg[0] = path;
			seg[1] = DEFAULT_PATH_SEPARATOR;
			seg[2] = ent.name;
			seg[3] = QSE_NULL;

			joined_path = qse_stradup (seg, QSE_NULL, fs->mmgr);
			if (!joined_path)
			{
				fs->errnum = QSE_FS_ENOMEM;
				goto oops;
			}

			ret = delete_file (fs, joined_path, 1);

			QSE_MMGR_FREE (fs->mmgr, joined_path);
			if (ret <= -1) goto oops;
		}

		qse_dir_close (dir);
		return 0;

	oops:
		qse_dir_close (dir);
		return -1;
	}

	fs->errnum = qse_fs_direrrtoerrnum (fs, errnum);
	return -99; /* special return code to indicate no directory */
}

static int purge_path (qse_fs_t* fs, const qse_char_t* path)
{
	int x;

	x = purge_directory_contents (fs, path);
	if (x == -99)
	{
		/* purge_directory_contents() failed 
		 * because path is not a directory */
		return delete_file (fs, path, 0);
	}
	else if (x <= -1)
	{
		return x;
	}
	else
	{
		/* path is a directory name and contents have been purged */
		return delete_directory (fs, path);
	}
}

static int delete_from_fs_with_mbs (qse_fs_t* fs, const qse_mchar_t* path, int dir)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspathformbs (fs, path);
	if (!fspath) return -1;

	if (fs->cbs.del)
	{
		qse_char_t* xpath;
		int x;

		xpath = (qse_char_t*)make_str_with_mbs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		x = fs->cbs.del (fs, xpath);

		free_str_with_mbs (fs, path, xpath);

		if (x <= -1) return -1;
		if (x == 0) return 0; /* skipped */
	}

	ret = dir? delete_directory_from_fs (fs, fspath): 
	           delete_file_from_fs (fs, fspath);

	qse_fs_freefspathformbs (fs, path, fspath);

	return ret;
}

static int delete_from_fs_with_wcs (qse_fs_t* fs, const qse_wchar_t* path, int dir)
{
	qse_fs_char_t* fspath;
	int ret;

	if (fs->cbs.del)
	{
		qse_char_t* xpath;
		int x;

		xpath = (qse_char_t*)make_str_with_wcs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		x = fs->cbs.del (fs, xpath);

		free_str_with_wcs (fs, path, xpath);

		if (x <= -1) return -1;
		if (x == 0) return 0; /* skipped */
	}

	fspath = qse_fs_makefspathforwcs (fs, path);
	if (!fspath) return -1;

	ret = dir? delete_directory_from_fs (fs, fspath): 
	           delete_file_from_fs (fs, fspath);

	qse_fs_freefspathforwcs (fs, path, fspath);

	return ret;
}

/* --------------------------------------------------------------------- */

static int delete_file_for_glob (const qse_cstr_t* path, void* ctx)
{
	qse_fs_t* fs = (qse_fs_t*)ctx;
	return delete_file (fs, path->ptr, 0);
}

static int delete_directory_for_glob (const qse_cstr_t* path, void* ctx)
{
	qse_fs_t* fs = (qse_fs_t*)ctx;
	return delete_directory (fs, path->ptr);
}

static int purge_path_for_glob (const qse_cstr_t* path, void* ctx)
{
	qse_fs_t* fs = (qse_fs_t*)ctx;
	return purge_path (fs, path->ptr);
}

/* --------------------------------------------------------------------- */

int qse_fs_delfilembs (qse_fs_t* fs, const qse_mchar_t* path, int flags)
{
	int ret;

	if (flags & QSE_FS_DELFILEMBS_GLOB)
	{
		qse_char_t* xpath;

		xpath = (qse_char_t*)make_str_with_mbs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		if (flags & QSE_FS_DELFILEMBS_RECURSIVE)
		{
			ret = qse_glob (xpath, purge_path_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}
		else
		{
			ret = qse_glob (xpath, delete_file_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}

		free_str_with_mbs (fs, path, xpath);

		if (ret <= -1)
		{
			fs->errnum = QSE_FS_EGLOB;
			return -1;
		}
	}
	else if (flags & QSE_FS_DELFILEMBS_RECURSIVE)
	{
		qse_char_t* xpath;

		/* if RECURSIVE is set, it's not differnt from qse_fs_deldirmbs() */
		xpath = (qse_char_t*)make_str_with_mbs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		ret = purge_path (fs, xpath);

		free_str_with_mbs (fs, path, xpath);
	}
	else
	{
		ret = delete_from_fs_with_mbs (fs, path, 0);
	}

	return ret;
}

int qse_fs_delfilewcs (qse_fs_t* fs, const qse_wchar_t* path, int flags)
{
	int ret;

	if (flags & QSE_FS_DELFILEWCS_GLOB)
	{
		qse_char_t* xpath;

		xpath = (qse_char_t*)make_str_with_wcs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		if (flags & QSE_FS_DELFILEWCS_RECURSIVE)
		{
			ret = qse_glob (xpath, purge_path_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}
		else
		{
			ret = qse_glob (xpath, delete_file_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}

		free_str_with_wcs (fs, path, xpath);

		if (ret <= -1)
		{
			fs->errnum = QSE_FS_EGLOB;
			return -1;
		}
	}
	else if (flags & QSE_FS_DELFILEWCS_RECURSIVE)
	{
		qse_char_t* xpath;

		/* if RECURSIVE is set, it's not differnt from qse_fs_deldirwcs() */
		xpath = (qse_char_t*)make_str_with_wcs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		ret = purge_path (fs, xpath);

		free_str_with_wcs (fs, path, xpath);
	}
	else
	{
		ret = delete_from_fs_with_wcs (fs, path, 0);
	}

	return ret;
}


/* --------------------------------------------------------------------- */

int qse_fs_deldirmbs (qse_fs_t* fs, const qse_mchar_t* path, int flags)
{
	int ret;

	if (flags & QSE_FS_DELDIRMBS_GLOB)
	{
		qse_char_t* xpath;

		xpath = (qse_char_t*)make_str_with_mbs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		if (flags & QSE_FS_DELDIRMBS_RECURSIVE)
		{
			ret = qse_glob (xpath, purge_path_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}
		else
		{
			ret = qse_glob (xpath, delete_directory_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}

		free_str_with_mbs (fs, path, xpath);

		if (ret <= -1) fs->errnum = QSE_FS_EGLOB;
	}
	else if (flags & QSE_FS_DELDIRMBS_RECURSIVE)
	{
		qse_char_t* xpath;

		/* if RECURSIVE is set, it's not differnt from qse_fs_delfilembs() */
		xpath = (qse_char_t*)make_str_with_mbs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		ret = purge_path (fs, xpath);

		free_str_with_mbs (fs, path, xpath);
	}
	else
	{
		ret = delete_from_fs_with_mbs (fs, path, 1);
	}

	return ret;
}

int qse_fs_deldirwcs (qse_fs_t* fs, const qse_wchar_t* path, int flags)
{
	int ret;

	if (flags & QSE_FS_DELDIRWCS_GLOB)
	{
		qse_char_t* xpath;

		xpath = (qse_char_t*)make_str_with_wcs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		if (flags & QSE_FS_DELDIRWCS_RECURSIVE)
		{
			ret = qse_glob (xpath, purge_path_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}
		else
		{
			ret = qse_glob (xpath, delete_directory_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr);
		}

		free_str_with_wcs (fs, path, xpath);

		if (ret <= -1) fs->errnum = QSE_FS_EGLOB;
	}
	else if (flags & QSE_FS_DELDIRWCS_RECURSIVE)
	{
		qse_char_t* xpath;

		/* if RECURSIVE is set, it's not differnt from qse_fs_delfilewcs() */
		xpath = (qse_char_t*)make_str_with_wcs (fs, path);
		if (!xpath)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		ret = purge_path (fs, xpath);

		free_str_with_wcs (fs, path, xpath);
	}
	else
	{
		ret = delete_from_fs_with_wcs (fs, path, 1);
	}

	return ret;
}
