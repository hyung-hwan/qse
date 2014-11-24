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

static int delete_file (qse_fs_t* fs, const qse_fs_char_t* fspath)
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

static int delete_directory (qse_fs_t* fs, const qse_fs_char_t* fspath)
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

static int purge_path (qse_fs_t* fs, const qse_char_t* path)
{
	qse_dir_t* dir;
	qse_dir_errnum_t errnum;
	qse_dir_ent_t ent;
	qse_fs_char_t* fspath;
	int ret, x;

	dir = qse_dir_open (fs->mmgr, 0, path, 0, &errnum);
	if (!dir)
	{
		/* not a directory. attempt to delete it as a file */
		fspath = qse_fs_makefspath(fs, path);
		if (!fspath) return -1;

/*TODO query: */
/*if (fs->cb.delete) fs->cb.delete (path);*/
		ret = delete_file (fs, fspath);
		qse_fs_freefspath (fs, path, fspath);

		return ret;
	}
	else
	{
		/* it must be a directory. delete all entries under it */
		const qse_char_t* seg[4];
		qse_char_t* joined_path;

		while (1)
		{
			x = qse_dir_read (dir, &ent);
			if (x <= -1) 
			{
				/* TODO: CONVERT dir->errnum to fs errnum */
				goto oops;
			}
			if (x == 0) break; /* no more entries */

			/* skip . and .. */
			if (IS_CURDIR(ent.name) || IS_PREVDIR(ent.name)) continue;

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

			/* join path and ent->name.... */
			fspath = qse_fs_makefspath(fs, joined_path);
			if (!fspath) goto oops;
/*TODO query: */
/*if (fs->cb.delete) fs->cb.delete (path);*/
			ret = delete_file (fs, fspath);
			qse_fs_freefspath  (fs, ent.name, fspath);
			if (ret <= -1) ret = purge_path(fs, joined_path);
			
			QSE_MMGR_FREE (fs->mmgr, joined_path);
			if (ret <= -1) goto oops;
		}

		qse_dir_close (dir);

		fspath = qse_fs_makefspath (fs, path);
		if (!fspath) goto oops;

		ret = delete_directory (fs, fspath);

		qse_fs_freefspath (fs, path, fspath);

		return ret;

	oops:
		qse_dir_close (dir);
		return -1;
	}
}


/* --------------------------------------------------------------------- */


static int delete_file_for_glob (const qse_cstr_t* path, void* ctx)
{
	qse_fs_t* fs = (qse_fs_t*)ctx;
	qse_fs_char_t* fspath;
	int ret;

	/* skip . and .. */
	if (IS_CURDIR(path->ptr) || IS_PREVDIR(path->ptr)) return 0;

	fspath = qse_fs_makefspath (fs, path->ptr);
	if (!fspath) return -1;

/*TODO query: */
/*if (fs->cb.delete) fs->cb.delete (path);*/
	ret = delete_file (fs, fspath);

	qse_fs_freefspath (fs, path->ptr, fspath);

	return ret;
}

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

		ret = qse_glob (xpath, delete_file_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr);

		free_str_with_mbs (fs, path, xpath);

		if (ret <= -1)
		{
			fs->errnum = QSE_FS_EGLOB;
			return -1;
		}
	}
	else
	{
		qse_fs_char_t* fspath;

		fspath = qse_fs_makefspathformbs (fs, path);
		if (!fspath) return -1;

/* TODO: query */
/*if (fs->cb.delete) fs->cb.delete (path);*/
		ret = delete_file (fs, fspath);

		qse_fs_freefspathformbs (fs, path, fspath);
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

		ret = qse_glob (xpath, delete_file_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr);

		free_str_with_wcs (fs, path, xpath);

		if (ret <= -1)
		{
			fs->errnum = QSE_FS_EGLOB;
			return -1;
		}
	}
	else
	{
		qse_fs_char_t* fspath;

		fspath = qse_fs_makefspathforwcs (fs, path);
		if (!fspath) return -1;

/* TODO: query */
/*if (fs->cb.delete) fs->cb.delete (path);*/
		ret = delete_file (fs, fspath);

		qse_fs_freefspathforwcs (fs, path, fspath);
	}

	return ret;
}


/* --------------------------------------------------------------------- */

static int delete_directory_for_glob (const qse_cstr_t* path, void* ctx)
{
	qse_fs_t* fs = (qse_fs_t*)ctx;
	qse_fs_char_t* fspath;
	int ret;

	/* skip . and .. */
	if (IS_CURDIR(path->ptr) || IS_PREVDIR(path->ptr)) return 0;

	fspath = qse_fs_makefspath (fs, path->ptr);
	if (!fspath) return -1;

/*TODO query: */
/*if (fs->cb.delete) fs->cb.delete (path);*/
	ret = delete_directory (fs, fspath);

	qse_fs_freefspath (fs, path->ptr, fspath);

	return ret;
}

static int purge_path_for_glob (const qse_cstr_t* path, void* ctx)
{
	qse_fs_t* fs = (qse_fs_t*)ctx;
	int ret;

	/* skip . and .. */
	if (IS_CURDIR(path->ptr) || IS_PREVDIR(path->ptr)) return 0;

/*TODO query: */
/*if (fs->cb.delete) fs->cb.delete (joined_path);*/
	return purge_path (fs, path->ptr);
}

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
			ret = qse_glob (xpath, purge_path_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr);
		}
		else
		{
			ret = qse_glob (xpath, delete_directory_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr);
		}

		free_str_with_mbs (fs, path, xpath);

		if (ret <= -1) fs->errnum = QSE_FS_EGLOB;
	}
	else if (flags & QSE_FS_DELDIRMBS_RECURSIVE)
	{
		qse_char_t* xpath;

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
		qse_fs_char_t* fspath;

		fspath = qse_fs_makefspathformbs (fs, path);
		if (!fspath) return -1;

/* TODO: query */
/*if (fs->cb.delete) fs->cb.delete (path);*/
		ret = delete_directory (fs, fspath);

		qse_fs_freefspathformbs (fs, path, fspath);
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
			ret = qse_glob (xpath, purge_path_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr);
		}
		else
		{
			ret = qse_glob (xpath, delete_directory_for_glob, fs, DEFAULT_GLOB_FLAGS, fs->mmgr);
		}

		free_str_with_wcs (fs, path, xpath);

		if (ret <= -1) fs->errnum = QSE_FS_EGLOB;
	}
	else if (flags & QSE_FS_DELDIRWCS_RECURSIVE)
	{
		qse_char_t* xpath;

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
		qse_fs_char_t* fspath;

		fspath = qse_fs_makefspathforwcs (fs, path);
		if (!fspath) return -1;

/* TODO: query */
/*if (fs->cb.delete) fs->cb.delete (path);*/
		ret = delete_directory (fs, fspath);

		qse_fs_freefspathforwcs (fs, path, fspath);
	}

	return ret;
}
