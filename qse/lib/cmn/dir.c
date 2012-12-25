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

#include <qse/cmn/dir.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/path.h>
#include "mem.h"

#if defined(_WIN32) 
#	include <windows.h>
#elif defined(__OS2__) 
#	define INCL_DOSFILEMGR
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__) 
#	include <dos.h>
#	include <errno.h>
#else
#	include "syscall.h"
#endif

struct qse_dir_t
{
	qse_mmgr_t* mmgr;
	qse_dir_errnum_t errnum;
	int flags;

	qse_str_t tbuf;
	qse_mbs_t mbuf;

#if defined(_WIN32)
	HANDLE h;
	WIN32_FIND_DATA wfd;	
	int done;
#elif defined(__OS2__)
	HDIR h;
	#if defined(FIL_STANDARDL) 
	FILEFINDBUF3L ffb;	
	#else
	FILEFINDBUF3 ffb;	
	#endif
	ULONG count;
	int opened;
#elif defined(__DOS__)
	struct find_t f;
	int done;
	int opened;
#else
	DIR* dp;
#endif
};

int qse_dir_init (qse_dir_t* dir, qse_mmgr_t* mmgr, const qse_char_t* path, int flags);
void qse_dir_fini (qse_dir_t* dir);

static int reset_to_path (qse_dir_t* dir, const qse_char_t* path);

#include "syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (dir, DIR)

qse_dir_t* qse_dir_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, 
	const qse_char_t* path, int flags, qse_dir_errnum_t* errnum)
{
	qse_dir_t* dir;

	dir = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*dir) + xtnsize);
	if (dir)
	{
		if (qse_dir_init (dir, mmgr, path, flags) <= -1)
		{
			if (errnum) *errnum = qse_dir_geterrnum (dir);
			QSE_MMGR_FREE (mmgr, dir);
			dir = QSE_NULL;
		}
		else QSE_MEMSET (dir + 1, 0, xtnsize);
	}
	else
	{
		if (errnum) *errnum = QSE_DIR_ENOMEM;
	}

	return dir;
}

void qse_dir_close (qse_dir_t* dir)
{
	qse_dir_fini (dir);
	QSE_MMGR_FREE (dir->mmgr, dir);
}

qse_mmgr_t* qse_dir_getmmgr (qse_dir_t* dir)
{
	return dir->mmgr;
}

void* qse_dir_getxtn (qse_dir_t* dir)
{
	return QSE_XTN (dir);
}

qse_dir_errnum_t qse_dir_geterrnum (qse_dir_t* dir)
{
	return dir->errnum;
}

int qse_dir_init (qse_dir_t* dir, qse_mmgr_t* mmgr, const qse_char_t* path, int flags)
{
	int n;

	QSE_MEMSET (dir, 0, QSE_SIZEOF(*dir));

	dir->mmgr = mmgr;
	dir->flags = flags;

	if (qse_str_init (&dir->tbuf, mmgr, 256) <= -1) return -1;
	if (qse_mbs_init (&dir->mbuf, mmgr, 256) <= -1) 
	{
		qse_str_fini (&dir->tbuf);
		return -1;
	}

#if defined(_WIN32)
	dir->h = INVALID_HANDLE_VALUE;
#endif

	n = reset_to_path  (dir, path);
	if (n <= -1)
	{
		qse_mbs_fini (&dir->mbuf);
		qse_str_fini (&dir->tbuf);
	}

	return n;
}

static void close_dir_safely (qse_dir_t* dir)
{
#if defined(_WIN32)
	if (dir->h != INVALID_HANDLE_VALUE)
	{
		FindClose (dir->h);
		dir->h = INVALID_HANDLE_VALUE;
	}
#elif defined(__OS2__)
	if (dir->opened)
	{
		DosFindClose (dir->h);
		dir->opened = 0;
	}
#elif defined(__DOS__)
	if (dir->opened)
	{
		_dos_findclose (&dir->f);
		dir->opened = 0;
	}
#else
	if (dir->dp)
	{
		QSE_CLOSEDIR (dir->dp);
		dir->dp = QSE_NULL;
	}
#endif
}

void qse_dir_fini (qse_dir_t* dir)
{
	close_dir_safely (dir);

	qse_mbs_fini (&dir->mbuf);
	qse_str_fini (&dir->tbuf);
}

static qse_mchar_t* wcs_to_mbuf (qse_dir_t* dir, const qse_wchar_t* wcs, qse_mbs_t* mbs)
{
	qse_size_t ml, wl;

	if (qse_wcstombs (wcs, &wl, QSE_NULL, &ml) <= -1)
	{
		dir->errnum = QSE_DIR_EINVAL;
		return QSE_NULL;
	}

	if (qse_mbs_setlen (mbs, ml) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return QSE_NULL;
	}

	qse_wcstombs (wcs, &wl, QSE_MBS_PTR(mbs), &ml);
	return QSE_MBS_PTR(mbs);
}

static qse_wchar_t* mbs_to_wbuf (qse_dir_t* dir, const qse_mchar_t* mbs, qse_wcs_t* wcs)
{
	qse_size_t ml, wl;

	if (qse_mbstowcs (mbs, &ml, QSE_NULL, &wl) <= -1)
	{
		dir->errnum = QSE_DIR_EINVAL;
		return QSE_NULL;
	}
	if (qse_wcs_setlen (wcs, wl) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return QSE_NULL;
	}

	qse_mbstowcs (mbs, &ml, QSE_WCS_PTR(wcs), &wl);
	return QSE_WCS_PTR(wcs);
}

static qse_mchar_t* make_mbsdos_path (qse_dir_t* dir, const qse_mchar_t* mpath)
{
	if (mpath[0] == QSE_MT('\0'))
	{
		if (qse_mbs_cpy (&dir->mbuf, QSE_MT("*.*")) == (qse_size_t)-1) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return QSE_NULL;
		}
	}
	else
	{
		qse_size_t len;
		if ((len = qse_mbs_cpy (&dir->mbuf, mpath)) == (qse_size_t)-1 ||
		    (!QSE_ISPATHMBSEP(mpath[len - 1]) && 
		     !qse_ismbsdrivecurpath(mpath) &&
		     qse_mbs_ccat (&dir->mbuf, QSE_MT('\\')) == (qse_size_t)-1) ||
		    qse_mbs_cat (&dir->mbuf, QSE_MT("*.*")) == (qse_size_t)-1)
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return QSE_NULL;
		}
	}

	return QSE_MBS_PTR(&dir->mbuf);
}

static qse_char_t* make_dos_path (qse_dir_t* dir, const qse_char_t* path)
{
	if (path[0] == QSE_T('\0'))
	{
		if (qse_str_cpy (&dir->tbuf, QSE_T("*.*")) == (qse_size_t)-1) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return QSE_NULL;
		}
	}
	else
	{
		qse_size_t len;
		if ((len = qse_str_cpy (&dir->tbuf, path)) == (qse_size_t)-1 ||
		    (!QSE_ISPATHSEP(path[len - 1]) && 
		     !qse_isdrivecurpath(path) &&
		     qse_str_ccat (&dir->tbuf, QSE_T('\\')) == (qse_size_t)-1) ||
		    qse_str_cat (&dir->tbuf, QSE_T("*.*")) == (qse_size_t)-1)
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return QSE_NULL;
		}
	}

	return QSE_STR_PTR(&dir->tbuf);
}


static qse_mchar_t* mkdospath (qse_dir_t* dir, const qse_char_t* path)
{

#if defined(QSE_CHAR_IS_MCHAR)
	return make_dos_path (dir, path);
#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		return make_mbsdos_path (dir, (const qse_mchar_t*) path);
	}
	else
	{
		qse_char_t* tptr;
		qse_mchar_t* mptr;

		tptr = make_dos_path (dir, path);
		if (tptr == QSE_NULL) return QSE_NULL;

		mptr = wcs_to_mbuf (dir, QSE_STR_PTR(&dir->tbuf), &dir->mbuf);
		if (mptr == QSE_NULL) return QSE_NULL;

		return mptr;
	}
#endif

}

static int reset_to_path (qse_dir_t* dir, const qse_char_t* path)
{
#if defined(_WIN32)
	/* ------------------------------------------------------------------- */
	qse_char_t* tptr;

	dir->done = 0;

	#if defined(QSE_CHAR_IS_MCHAR)
	tptr = make_dos_path (dir, path);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		qse_mchar_t* mptr = make_mbsdos_path (dir, (const qse_mchar_t*) path);
		if (mptr == QSE_NULL) return -1;
		tptr = mbs_to_wbuf (dir, mptr, &dir->tbuf);
	}
	else
	{
		tptr = make_dos_path (dir, path);
		if (tptr == QSE_NULL) return -1;
	}
	#endif
	if (tptr == QSE_NULL) return -1;

	dir->h = FindFirstFile (tptr, &dir->wfd);
	if (dir->h == INVALID_HANDLE_VALUE) 
	{
		dir->errnum = syserr_to_errnum (GetLastError());
		return -1;
	}

	return 0;
	/* ------------------------------------------------------------------- */

#elif defined(__OS2__)

	/* ------------------------------------------------------------------- */
	APIRET rc;
	qse_mchar_t* mptr;

	dir->h = HDIR_CREATE;
	dir->count = 1;

	#if defined(QSE_CHAR_IS_MCHAR)
	mptr = make_dos_path (dir, path);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		mptr = make_mbsdos_path (dir, (const qse_mchar_t*) path);
	}
	else
	{
		qse_char_t* tptr = make_dos_path (dir, path);
		if (tptr == QSE_NULL) return -1;
		mptr = wcs_to_mbuf (dir, tptr, &dir->mbuf);
	}
	#endif
	if (mptr == QSE_NULL) return -1;

	rc = DosFindFirst (
		mptr,
		&dir->h, 
		FILE_DIRECTORY | FILE_READONLY,
		&dir->ffb,
		QSE_SIZEOF(dir->ffb),
		&dir->count,
#if defined(FIL_STANDARDL) 
		FIL_STANDARDL
#else
		FIL_STANDARD
#endif
	);

	if (rc != NO_ERROR)
	{
		dir->errnum = syserr_to_errnum (rc);
		return -1;
	}

	dir->opened = 1;
	return 0;
	/* ------------------------------------------------------------------- */

#elif defined(__DOS__)

	/* ------------------------------------------------------------------- */
	unsigned int rc;
	qse_mchar_t* mptr;

	dir->done = 0;

	#if defined(QSE_CHAR_IS_MCHAR)
	mptr = make_dos_path (dir, path);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		mptr = make_mbsdos_path (dir, (const qse_mchar_t*) path);
	}
	else
	{
		qse_char_t* tptr = make_dos_path (dir, path);
		if (tptr == QSE_NULL) return -1;
		mptr = wcs_to_mbuf (dir, tptr, &dir->mbuf);
	}
	#endif
	if (mptr == QSE_NULL) return -1;

	rc = _dos_findfirst (mptr, _A_NORMAL | _A_SUBDIR, &dir->f);
	if (rc != 0) 
	{
		dir->errnum = syserr_to_errnum (errno);
		return -1;
	}

	dir->opened = 1;
	return 0;
	/* ------------------------------------------------------------------- */

#else
	DIR* dp;

	#if defined(QSE_CHAR_IS_MCHAR)
	dp = QSE_OPENDIR (path[0] == QSE_MT('\0')? QSE_T("."): path);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		const qse_mchar_t* mpath = (const qse_mchar_t*)path;
		dp = QSE_OPENDIR (mpath == QSE_MT('\0')? QSE_MT("."): mpath);
	}
	else
	{
		if (path[0] == QSE_T('\0'))
		{
			dp = QSE_OPENDIR (QSE_MT("."));
		}
		else
		{
			qse_mchar_t* mptr;

			mptr = wcs_to_mbuf (dir, path, &dir->mbuf);
			if (mptr == QSE_NULL) return -1;

			dp = QSE_OPENDIR (mptr);
		}
	}
	#endif 
	if (dp == QSE_NULL) 
	{
		dir->errnum = syserr_to_errnum (errno);
		return -1;
	}

	dir->dp = dp;
	return 0;
#endif 
}

int qse_dir_reset (qse_dir_t* dir, const qse_char_t* path)
{
	close_dir_safely (dir);
	return reset_to_path (dir, path);
}

static int read_dir_to_tbuf (qse_dir_t* dir, void** name)
{
#if defined(_WIN32)

	/* ------------------------------------------------------------------- */
	if (dir->done) return (dir->done > 0)? 0: -1;

	#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_str_cpy (&dir->tbuf, dir->wfd.cFileName) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return -1;
	}
	*name = QSE_STR_PTR(&dir->tbuf);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		if (wcs_to_mbuf (dir, dir->wfd.cFileName, &dir->mbuf) == QSE_NULL) return -1;
		*name = QSE_STR_PTR(&dir->mbuf);
	}
	else
	{
		if (qse_str_cpy (&dir->tbuf, dir->wfd.cFileName) == (qse_size_t)-1) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return -1;
		}
		*name = QSE_STR_PTR(&dir->tbuf);
	}
	#endif

	if (FindNextFile (dir->h, &dir->wfd) == FALSE) 
	{
		DWORD x = GetLastError();
		if (x == ERROR_NO_MORE_FILES) dir->done = 1;
		else
		{
			dir->errnum = syserr_to_errnum (x);
			dir->done = -1;
		}
	}

	return 1;
	/* ------------------------------------------------------------------- */

#elif defined(__OS2__)

	/* ------------------------------------------------------------------- */
	APIRET rc;

	if (dir->count <= 0) return 0;

	#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_str_cpy (&dir->tbuf, dir->ffb.achName) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return -1;
	}
	*name = QSE_STR_PTR(&dir->tbuf);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		if (qse_mbs_cpy (&dir->mbuf, dir->ffb.achName) == (qse_size_t)-1) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return -1;
		}
		*name = QSE_MBS_PTR(&dir->mbuf);
	}
	else
	{
		if (mbs_to_wbuf (dir, dir->ffb.achName, &dir->tbuf) == QSE_NULL) return -1;
		*name = QSE_STR_PTR(&dir->tbuf);
	}
	#endif

	rc = DosFindNext (dir->h, &dir->ffb, QSE_SIZEOF(dir->ffb), &dir->count);
	if (rc == ERROR_NO_MORE_FILES) dir->count = 0;
	else if (rc != NO_ERROR)
	{
		dir->errnum = syserr_to_errnum (rc);
		return -1;
	}

	return 1;
	/* ------------------------------------------------------------------- */

#elif defined(__DOS__)

	/* ------------------------------------------------------------------- */

	if (dir->done) return (dir->done > 0)? 0: -1;

	#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_str_cpy (&dir->tbuf, dir->f.name) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return -1;
	}
	*name = QSE_STR_PTR(&dir->tbuf);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		if (qse_mbs_cpy (&dir->mbuf, dir->f.name) == (qse_size_t)-1) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return -1;
		}
		*name = QSE_MBS_PTR(&dir->mbuf);
	}
	else
	{
		if (mbs_to_wbuf (dir, dir->f.name, &dir->tbuf) == QSE_NULL) return -1;
		*name = QSE_STR_PTR(&dir->tbuf);
	}
	#endif

	if (_dos_findnext (&dir->f) != 0)
	{
		if (errno == ENOENT) dir->done = 1;
		else
		{
			dir->errnum = syserr_to_errnum (errno);
			dir->done = -1;
		}
	}

	return 1;
	/* ------------------------------------------------------------------- */

#else

	/* ------------------------------------------------------------------- */
	qse_dirent_t* de;

	errno = 0;
	de = QSE_READDIR (dir->dp);
	if (de == NULL) 
	{
		if (errno == 0) return 0;
		dir->errnum = syserr_to_errnum (errno);
		return -1;
	}

	#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_str_cpy (&dir->tbuf, de->d_name) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return -1;
	}

	*name = QSE_STR_PTR(&dir->tbuf);
	#else
	if (dir->flags & QSE_DIR_MBSPATH)
	{
		if (qse_mbs_cpy (&dir->mbuf, de->d_name) == (qse_size_t)-1) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return -1;
		}
		*name = QSE_MBS_PTR(&dir->mbuf);
	}
	else
	{
		if (mbs_to_wbuf (dir, de->d_name, &dir->tbuf) == QSE_NULL) return -1;
		*name = QSE_STR_PTR(&dir->tbuf);
	}
	#endif	

	return 1;
	/* ------------------------------------------------------------------- */

#endif
}

int qse_dir_read (qse_dir_t* dir, qse_dir_ent_t* ent)
{
	int x;
	void* name;

	x = read_dir_to_tbuf (dir, &name);
	if (x >= 1)
	{
		QSE_MEMSET (ent, 0, QSE_SIZEOF(ent));
		ent->name = name;

		if (dir->flags & QSE_DIR_STAT)
		{
			/* TODO: more information */
		}
	}

	return x;
}
