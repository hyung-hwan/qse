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

#include <qse/cmn/dir.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/path.h>
#include <qse/cmn/lda.h>
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


#define STATUS_OPENED    (1 << 0)
#define STATUS_DONE      (1 << 1)
#define STATUS_DONE_ERR  (1 << 2)
#define STATUS_POPHEAP   (1 << 3)
#define STATUS_SORT_ERR  (1 << 4)

#define IS_CURDIR_M(x) ((x)[0] == QSE_MT('.') && (x)[1] == QSE_MT('\0'))
#define IS_PREVDIR_M(x) ((x)[0] == QSE_MT('.') && (x)[1] == QSE_MT('.') && (x)[2] == QSE_MT('\0'))

#define IS_CURDIR_W(x) ((x)[0] == QSE_WT('.') && (x)[1] == QSE_WT('\0'))
#define IS_PREVDIR_W(x) ((x)[0] == QSE_WT('.') && (x)[1] == QSE_WT('.') && (x)[2] == QSE_WT('\0'))

#if defined(QSE_CHAR_IS_MCHAR)
#	define IS_CURDIR(x) IS_CURDIR_M(x)
#	define IS_PREVDIR(x) IS_PREVDIR_M(x)
#else
#	define IS_CURDIR(x) IS_CURDIR_W(x)
#	define IS_PREVDIR(x) IS_PREVDIR_W(x)
#endif

struct qse_dir_t
{
	qse_mmgr_t* mmgr;
	qse_dir_errnum_t errnum;
	int flags;

	qse_wcs_t wbuf;
	qse_mbs_t mbuf;

	qse_lda_t* stab;
	int status;

#if defined(_WIN32)
	HANDLE h;
	WIN32_FIND_DATA wfd;
#elif defined(__OS2__)
	HDIR h;
	#if defined(FIL_STANDARDL) 
	FILEFINDBUF3L ffb;
	#else
	FILEFINDBUF3 ffb;
	#endif
	ULONG count;
#elif defined(__DOS__)
	struct find_t f;
#else
	QSE_DIR* dp;
#endif
};

int qse_dir_init (qse_dir_t* dir, qse_mmgr_t* mmgr, const qse_char_t* path, int flags);
void qse_dir_fini (qse_dir_t* dir);

static void close_dir_safely (qse_dir_t* dir);
static int reset_to_path (qse_dir_t* dir, const qse_char_t* path);
static int read_ahead_and_sort (qse_dir_t* dir, const qse_char_t* path);

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
	else if (errnum) *errnum = QSE_DIR_ENOMEM;

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

static int compare_dirent (qse_lda_t* lda, const void* dptr1, qse_size_t dlen1, const void* dptr2, qse_size_t dlen2)
{
	int n = QSE_MEMCMP (dptr1, dptr2, ((dlen1 < dlen2)? dlen1: dlen2));
	if (n == 0 && dlen1 != dlen2) n = (dlen1 > dlen2)? 1: -1;
	return -n;
}

int qse_dir_init (qse_dir_t* dir, qse_mmgr_t* mmgr, const qse_char_t* path, int flags)
{
	int n;
	int path_flags;

	path_flags = flags & (QSE_DIR_MBSPATH | QSE_DIR_WCSPATH);
	if (path_flags == (QSE_DIR_MBSPATH | QSE_DIR_WCSPATH) || path_flags == 0)
	{
		/* if both are set or none are set, force it to the default */
	#if defined(QSE_CHAR_IS_MCHAR)
		flags |= QSE_DIR_MBSPATH;
		flags &= ~QSE_DIR_WCSPATH;
	#else
		flags |= QSE_DIR_WCSPATH;
		flags &= ~QSE_DIR_MBSPATH;
	#endif
	}

	QSE_MEMSET (dir, 0, QSE_SIZEOF(*dir));

	dir->mmgr = mmgr;
	dir->flags = flags;

	if (qse_wcs_init (&dir->wbuf, mmgr, 256) <= -1) goto oops_0;
	if (qse_mbs_init (&dir->mbuf, mmgr, 256) <= -1) goto oops_1;

#if defined(_WIN32)
	dir->h = INVALID_HANDLE_VALUE;
#endif

	n = reset_to_path  (dir, path);
	if (n <= -1) goto oops_2;

	if (dir->flags & QSE_DIR_SORT)
	{
		dir->stab = qse_lda_open (dir->mmgr, 0, 128);
		if (dir->stab == QSE_NULL) goto oops_3;

		/*qse_lda_setscale (dir->stab, 1);*/
		qse_lda_setcopier (dir->stab, QSE_LDA_COPIER_INLINE);
		qse_lda_setcomper (dir->stab, compare_dirent);
		if (read_ahead_and_sort (dir, path) <= -1) goto oops_4;
	}

	return n;

oops_4:
	qse_lda_close (dir->stab);
oops_3:
	close_dir_safely (dir);
oops_2:
	qse_mbs_fini (&dir->mbuf);
oops_1:
	qse_wcs_fini (&dir->wbuf);
oops_0:
	return -1;
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
	if (dir->status & STATUS_OPENED)
	{
		DosFindClose (dir->h);
		dir->status &= ~STATUS_OPENED;
	}
#elif defined(__DOS__)
	if (dir->status & STATUS_OPENED)
	{
		_dos_findclose (&dir->f);
		dir->status &= ~STATUS_OPENED;
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
	qse_wcs_fini (&dir->wbuf);

	if (dir->stab) qse_lda_close (dir->stab);
}

static qse_mchar_t* wcs_to_mbuf (qse_dir_t* dir, const qse_wchar_t* wcs, qse_mbs_t* mbuf)
{
	qse_size_t ml, wl;

	if (qse_wcstombs (wcs, &wl, QSE_NULL, &ml) <= -1)
	{
		dir->errnum = QSE_DIR_EINVAL;
		return QSE_NULL;
	}

	if (qse_mbs_setlen (mbuf, ml) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return QSE_NULL;
	}

	qse_wcstombs (wcs, &wl, QSE_MBS_PTR(mbuf), &ml);
	return QSE_MBS_PTR(mbuf);
}

static qse_wchar_t* mbs_to_wbuf (qse_dir_t* dir, const qse_mchar_t* mbs, qse_wcs_t* wbuf)
{
	qse_size_t ml, wl;

	if (qse_mbstowcs (mbs, &ml, QSE_NULL, &wl) <= -1)
	{
		dir->errnum = QSE_DIR_EINVAL;
		return QSE_NULL;
	}
	if (qse_wcs_setlen (wbuf, wl) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return QSE_NULL;
	}

	qse_mbstowcs (mbs, &ml, QSE_WCS_PTR(wbuf), &wl);
	return QSE_WCS_PTR(wbuf);
}

static qse_wchar_t* wcs_to_wbuf (qse_dir_t* dir, const qse_wchar_t* wcs, qse_wcs_t* wbuf)
{
	if (qse_wcs_cpy (&dir->wbuf, wcs) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return QSE_NULL;
	}

	return QSE_WCS_PTR(wbuf);
}

static qse_mchar_t* mbs_to_mbuf (qse_dir_t* dir, const qse_mchar_t* mbs, qse_mbs_t* mbuf)
{
	if (qse_mbs_cpy (&dir->mbuf, mbs) == (qse_size_t)-1) 
	{
		dir->errnum = QSE_DIR_ENOMEM;
		return QSE_NULL;
	}

	return QSE_MBS_PTR(mbuf);
}

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
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

static qse_wchar_t* make_wcsdos_path (qse_dir_t* dir, const qse_wchar_t* wpath)
{
	if (wpath[0] == QSE_WT('\0'))
	{
		if (qse_wcs_cpy (&dir->wbuf, QSE_WT("*.*")) == (qse_size_t)-1) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return QSE_NULL;
		}
	}
	else
	{
		qse_size_t len;
		if ((len = qse_wcs_cpy (&dir->wbuf, wpath)) == (qse_size_t)-1 ||
		    (!QSE_ISPATHWCSEP(wpath[len - 1]) && 
		     !qse_iswcsdrivecurpath(wpath) &&
		     qse_wcs_ccat (&dir->wbuf, QSE_WT('\\')) == (qse_size_t)-1) ||
		    qse_wcs_cat (&dir->wbuf, QSE_WT("*.*")) == (qse_size_t)-1)
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return QSE_NULL;
		}
	}

	return QSE_WCS_PTR(&dir->wbuf);
}
#endif

/*
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
*/

static int reset_to_path (qse_dir_t* dir, const qse_char_t* path)
{
#if defined(_WIN32)
	/* ------------------------------------------------------------------- */
	const qse_char_t* tptr;

	dir->status &= ~STATUS_DONE;
	dir->status &= ~STATUS_DONE_ERR;

	if (dir->flags & QSE_DIR_MBSPATH)
	{
		qse_mchar_t* mptr;

		mptr = make_mbsdos_path (dir, (const qse_mchar_t*)path);
		if (mptr == QSE_NULL) return -1;

	#if defined(QSE_CHAR_IS_MCHAR)
		tptr = mptr;
	#else
		tptr = mbs_to_wbuf (dir, mptr, &dir->wbuf);
	#endif
	}
	else
	{
		qse_wchar_t* wptr;
		QSE_ASSERT (dir->flags & QSE_DIR_WCSPATH);

		wptr = make_wcsdos_path (dir, (const qse_wchar_t*)path);
		if (wptr == QSE_NULL) return -1;

	#if defined(QSE_CHAR_IS_MCHAR)
		tptr = wcs_to_mbuf (dir, wptr, &dir->mbuf);
	#else
		tptr = wptr;
	#endif
	}
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
	const qse_mchar_t* mptr;

	dir->h = HDIR_CREATE;
	dir->count = 1;

	if (dir->flags & QSE_DIR_MBSPATH)
	{
		mptr = make_mbsdos_path (dir, (const qse_mchar_t*)path);
	}
	else
	{
		qse_wchar_t* wptr;
		QSE_ASSERT (dir->flags & QSE_DIR_WCSPATH);

		wptr = make_wcsdos_path (dir, (const qse_wchar_t*)path);
		if (wptr == QSE_NULL) return -1;
		mptr = wcs_to_mbuf (dir, wptr, &dir->mbuf);
	}
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

	dir->status |= STATUS_OPENED;
	return 0;
	/* ------------------------------------------------------------------- */

#elif defined(__DOS__)

	/* ------------------------------------------------------------------- */
	unsigned int rc;
	const qse_mchar_t* mptr;

	dir->status &= ~STATUS_DONE;
	dir->status &= ~STATUS_DONE_ERR;

	if (dir->flags & QSE_DIR_MBSPATH)
	{
		mptr = make_mbsdos_path (dir, (const qse_mchar_t*)path);
	}
	else
	{
		qse_wchar_t* wptr;

		QSE_ASSERT (dir->flags & QSE_DIR_WCSPTH);

		wptr = make_wcsdos_path (dir, (const qse_wchar_t*)path);
		if (wptr == QSE_NULL) return -1;
		mptr = wcs_to_mbuf (dir, wptr, &dir->mbuf);
	}
	if (mptr == QSE_NULL) return -1;

	rc = _dos_findfirst (mptr, _A_NORMAL | _A_SUBDIR, &dir->f);
	if (rc != 0) 
	{
		dir->errnum = syserr_to_errnum (errno);
		return -1;
	}

	dir->status |= STATUS_OPENED;
	return 0;
	/* ------------------------------------------------------------------- */

#else
	DIR* dp;

	if (dir->flags & QSE_DIR_MBSPATH)
	{
		const qse_mchar_t* mpath;

		mpath = (const qse_mchar_t*)path;
		dp = QSE_OPENDIR (mpath == QSE_MT('\0')? QSE_MT("."): mpath);
	}
	else
	{

		const qse_wchar_t* wpath;
		QSE_ASSERT (dir->flags & QSE_DIR_WCSPATH);

		wpath = (const qse_wchar_t*)path;
		if (wpath[0] == QSE_WT('\0'))
		{
			dp = QSE_OPENDIR (QSE_MT("."));
		}
		else
		{
			qse_mchar_t* mptr;

			mptr = wcs_to_mbuf (dir, wpath, &dir->mbuf);
			if (mptr == QSE_NULL) return -1;

			dp = QSE_OPENDIR (mptr);
		}
	}

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
	if (reset_to_path (dir, path) <= -1) return -1;

	if (dir->flags & QSE_DIR_SORT)
	{
		qse_lda_clear (dir->stab);
		if (read_ahead_and_sort (dir, path) <= -1) 
		{
			dir->status |= STATUS_SORT_ERR;
			return -1;
		}
		else
		{
			dir->status &= ~STATUS_SORT_ERR;
		}
	}

	return 0;
}

static int read_dir_to_buf (qse_dir_t* dir, void** name)
{
#if defined(_WIN32)

	/* ------------------------------------------------------------------- */
	if (dir->status & STATUS_DONE) return (dir->status & STATUS_DONE_ERR)? -1: 0;

	if (dir->flags & QSE_DIR_SKIPSPCDIR)
	{
		/* skip . and .. */
		while (IS_CURDIR(dir->wfd.cFileName) || IS_PREVDIR(dir->wfd.cFileName))
		{
			if (FindNextFile (dir->h, &dir->wfd) == FALSE) 
			{
				DWORD x = GetLastError();
				if (x == ERROR_NO_MORE_FILES) 
				{
					dir->status |= STATUS_DONE;
					return 0;
				}
				else
				{
					dir->errnum = syserr_to_errnum (x);
					dir->status |= STATUS_DONE;
					dir->status |= STATUS_DONE_ERR;
					return -1;
				}
			}
		}
	}

	if (dir->flags & QSE_DIR_MBSPATH)
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		if (mbs_to_mbuf (dir, dir->wfd.cFileName, &dir->mbuf) == QSE_NULL) return -1;
	#else
		if (wcs_to_mbuf (dir, dir->wfd.cFileName, &dir->mbuf) == QSE_NULL) return -1;
	#endif
		*name = QSE_MBS_PTR(&dir->mbuf);
	}
	else
	{
		QSE_ASSERT (dir->flags & QSE_DIR_WCSPATH);
	#if defined(QSE_CHAR_IS_MCHAR)
		if (mbs_to_wbuf (dir, dir->wfd.cFileName, &dir->wbuf) == QSE_NULL) return -1;
	#else
		if (wcs_to_wbuf (dir, dir->wfd.cFileName, &dir->wbuf) == QSE_NULL) return -1;
	#endif
		*name = QSE_WCS_PTR(&dir->wbuf);
	}

	if (FindNextFile (dir->h, &dir->wfd) == FALSE) 
	{
		DWORD x = GetLastError();
		if (x == ERROR_NO_MORE_FILES) dir->status |= STATUS_DONE;
		else
		{
			dir->errnum = syserr_to_errnum (x);
			dir->status |= STATUS_DONE;
			dir->status |= STATUS_DONE_ERR;
		}
	}

	return 1;
	/* ------------------------------------------------------------------- */

#elif defined(__OS2__)

	/* ------------------------------------------------------------------- */
	APIRET rc;

	if (dir->count <= 0) return 0;

	if (dir->flags & QSE_DIR_SKIPSPCDIR)
	{
		/* skip . and .. */
		while (IS_CURDIR_M(dir->ffb.achName) || IS_PREVDIR_M(dir->ffb.achName))
		{
			rc = DosFindNext (dir->h, &dir->ffb, QSE_SIZEOF(dir->ffb), &dir->count);
			if (rc == ERROR_NO_MORE_FILES) 
			{
				dir->count = 0;
				return 0;
			}
			else if (rc != NO_ERROR)
			{
				dir->errnum = syserr_to_errnum (rc);
				return -1;
			}
		}
	}

	if (dir->flags & QSE_DIR_MBSPATH)
	{
		if (mbs_to_mbuf (dir, dir->ffb.achName, &dir->mbuf) == QSE_NULL) return -1;
		*name = QSE_MBS_PTR(&dir->mbuf);
	}
	else
	{
		QSE_ASSERT (dir->flags & QSE_DIR_WCSPATH);
		if (mbs_to_wbuf (dir, dir->ffb.achName, &dir->wbuf) == QSE_NULL) return -1;
		*name = QSE_WCS_PTR(&dir->wbuf);
	}
	

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

	if (dir->status & STATUS_DONE) return (dir->status & STATUS_DONE_ERR)? -1: 0;

	if (dir->flags & QSE_DIR_SKIPSPCDIR)
	{
		/* skip . and .. */
		while (IS_CURDIR_M(dir->f.name) || IS_PREVDIR_M(dir->f.name))
		{
			if (_dos_findnext (&dir->f) != 0)
			{
				if (errno == ENOENT) 
				{
					dir->status |= STATUS_DONE;
					return 0;
				}
				else
				{
					dir->errnum = syserr_to_errnum (errno);
					dir->status |= STATUS_DONE;
					dir->status |= STATUS_DONE_ERR;
					return -1;
				}
			}
		}
	}

	if (dir->flags & QSE_DIR_MBSPATH)
	{
		if (mbs_to_mbuf (dir, dir->f.name, &dir->mbuf) == QSE_NULL) return -1;
		*name = QSE_MBS_PTR(&dir->mbuf);
	}
	else
	{
		QSE_ASSERT (dir->flags & QSE_DIR_WCSPATH);

		if (mbs_to_wbuf (dir, dir->f.name, &dir->wbuf) == QSE_NULL) return -1;
		*name = QSE_WCS_PTR(&dir->wbuf);
	}

	if (_dos_findnext (&dir->f) != 0)
	{
		if (errno == ENOENT) dir->status |= STATUS_DONE;
		else
		{
			dir->errnum = syserr_to_errnum (errno);
			dir->status |= STATUS_DONE;
			dir->status |= STATUS_DONE_ERR;
		}
	}

	return 1;
	/* ------------------------------------------------------------------- */

#else

	/* ------------------------------------------------------------------- */
	qse_dirent_t* de;

read:
	errno = 0;
	de = QSE_READDIR (dir->dp);
	if (de == NULL) 
	{
		if (errno == 0) return 0;
		dir->errnum = syserr_to_errnum (errno);
		return -1;
	}

	if (dir->flags & QSE_DIR_SKIPSPCDIR)
	{
		/* skip . and .. */
		if (IS_CURDIR_M(de->d_name) || 
		    IS_PREVDIR_M(de->d_name)) goto read;
	}

	if (dir->flags & QSE_DIR_MBSPATH)
	{
		if (mbs_to_mbuf (dir, de->d_name, &dir->mbuf) == QSE_NULL) return -1;
		*name = QSE_MBS_PTR(&dir->mbuf);
	}
	else
	{
		QSE_ASSERT (dir->flags & QSE_DIR_WCSPATH);

		if (mbs_to_wbuf (dir, de->d_name, &dir->wbuf) == QSE_NULL) return -1;
		*name = QSE_WCS_PTR(&dir->wbuf);
	}

	return 1;
	/* ------------------------------------------------------------------- */

#endif
}

static int read_ahead_and_sort (qse_dir_t* dir, const qse_char_t* path)
{
	int x;
	void* name;

	while (1)
	{
		x = read_dir_to_buf (dir, &name);
		if (x >= 1)
		{
			qse_size_t size;

			if (dir->flags & QSE_DIR_MBSPATH)
				size = (qse_mbslen(name) + 1) * QSE_SIZEOF(qse_mchar_t);
			else
				size = (qse_wcslen(name) + 1) * QSE_SIZEOF(qse_wchar_t);

			if (qse_lda_pushheap (dir->stab, name, size) == (qse_size_t)-1)
			{
				dir->errnum = QSE_DIR_ENOMEM;
				return -1;
			}
		}
		else if (x == 0) break;
		else return -1;
	}

	dir->status &= ~STATUS_POPHEAP;
	return 0;
}


int qse_dir_read (qse_dir_t* dir, qse_dir_ent_t* ent)
{
	if (dir->flags & QSE_DIR_SORT)
	{
		if (dir->status & STATUS_SORT_ERR) return -1;

		if (dir->status & STATUS_POPHEAP) qse_lda_popheap (dir->stab);
		else dir->status |= STATUS_POPHEAP;

		if (QSE_LDA_SIZE(dir->stab) <= 0) return 0; /* no more entry */

		ent->name = QSE_LDA_DPTR(dir->stab, 0);
		return 1;
	}
	else
	{
		int x;
		void* name;

		x = read_dir_to_buf (dir, &name);
		if (x >= 1)
		{
			QSE_MEMSET (ent, 0, QSE_SIZEOF(ent));
			ent->name = name;
		}

		return x;
	}
}
