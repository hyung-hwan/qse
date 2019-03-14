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

#include <qse/si/fs.h>
#include <qse/cmn/str.h>
#include <qse/cmn/path.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include "../cmn/syscall.h"
#include "../cmn/mem-prv.h"
#include <stdlib.h>

/* ==========================================================================
 * CURRENT WORKING DIRECTORY
 * ========================================================================== */
qse_mchar_t* qse_get_current_mbsdir (qse_mchar_t* buf, qse_size_t size)
{
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();
	qse_mchar_t* tmp;

	if (buf)
	{
		if (!getcwd(buf, size)) return QSE_NULL;
		return buf;
	}

	if (!mmgr) return QSE_NULL;

	size = QSE_PATH_MAX;

again:
	tmp = QSE_MMGR_REALLOC(mmgr, buf, size * QSE_SIZEOF(*buf));
	if (!tmp) return QSE_NULL;
	buf = tmp;

	if (!getcwd(buf, size))
	{
/* TODO: handle ENAMETOOLONG 
 *       or read /proc/self/cwd if available? */
		if (errno == ERANGE)
		{
			/* grow buffer and continue */
			size <<= 1;
			goto again;
		}

		QSE_MMGR_FREE (mmgr, buf);
		return QSE_NULL;
	}

	return buf;
}

qse_wchar_t* qse_get_current_wcsdir (qse_wchar_t* buf, qse_size_t size)
{
/* TODO: for WIN32, use the unicode API directly */
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();
	qse_mchar_t* mbsdir;
	qse_wchar_t* wcsdir;
	qse_size_t wcslen;

	mbsdir = qse_get_current_mbsdir(QSE_NULL, 0);
	if (!mbsdir) return QSE_NULL;

	wcsdir = qse_mbstowcsdup(mbsdir, &wcslen, mmgr);
	QSE_MMGR_FREE (mmgr, mbsdir);
	if (!wcsdir) return QSE_NULL;

	if (buf)
	{
		if (wcslen >= size) 
		{
			/* cannot copy the path in full */
			QSE_MMGR_FREE (mmgr, wcsdir);
			return QSE_NULL;
		}
		qse_wcscpy (buf, wcsdir);
		QSE_MMGR_FREE (mmgr, wcsdir);
		return buf;
	}

	return wcsdir;
}


/* ==========================================================================
 * FILE ATTRIBUTES
 * ========================================================================== */
void qse_stat_to_attr (const qse_stat_t* st, qse_fattr_t* attr)
{
	QSE_MEMSET (attr, 0, QSE_SIZEOF(*attr));

	attr->isdir = !!S_ISDIR(st->st_mode);
	attr->islnk = !!S_ISLNK(st->st_mode);
	attr->isreg = !!S_ISREG(st->st_mode);
	attr->isblk = !!S_ISBLK(st->st_mode);
	attr->ischr = !!S_ISCHR(st->st_mode);

	attr->mode = st->st_mode;
	attr->size = st->st_size;
	attr->ino = st->st_ino;
	attr->dev = st->st_dev;
	attr->uid = st->st_uid;
	attr->gid = st->st_gid;

#if defined(HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
	attr->atime.sec = st->st_atim.tv_sec;
	attr->atime.nsec = st->st_atim.tv_nsec;
	attr->mtime.sec = st->st_mtim.tv_sec;
	attr->mtime.nsec = st->st_mtim.tv_nsec;
	attr->ctime.sec = st->st_ctim.tv_sec;
	attr->ctime.nsec = st->st_ctim.tv_nsec;
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
	attr->atime.sec = st->st_atimespec.tv_sec;
	attr->atime.nsec = st->st_atimespec.tv_nsec;
	attr->mtime.sec = st->st_mtimespec.tv_sec;
	attr->mtime.nsec = st->st_mtimespec.tv_nsec;
	attr->ctime.sec = st->st_ctimespec.tv_sec;
	attr->ctime.nsec = st->st_ctimespec.tv_nsec;
#else
	attr->atime.sec = st->st_atime;
	attr->mtime.sec = st->st_mtime;
	attr->ctime.sec = st->st_ctime;
#endif
}

int qse_get_mbsfile_attr (const qse_mchar_t* file, int flags, qse_fattr_t* attr)
{
#if defined(_WIN32)
	return -1;

#elif defined(__OS2__)
	return -1;

#elif defined(__DOS__)
	return -1;

#elif defined(HAVE_FSTATAT) && defined(AT_SYMLINK_NOFOLLOW)
	qse_fstatat_t st;
	int sysflags = 0;

	if (flags & QSE_FILE_ATTR_SYMLINK) sysflags |= AT_SYMLINK_NOFOLLOW;
	if (QSE_FSTATAT(AT_FDCWD, file, &st, sysflags) <= -1) return -1;

	qse_stat_to_attr (&st, attr);
	return 0;

#else
	qse_stat_t st;
	int x;

	if (flags & QSE_FILE_ATTR_SYMLINK)
		x = QSE_LSTAT(file, &st);
	else
		x = QSE_STAT(file, &st);
	if (x <= -1) return -1;

	qse_stat_to_attr (&st, attr);
	return 0;
#endif
}

int qse_get_wcsfile_attr (const qse_wchar_t* file, int flags, qse_fattr_t* attr)
{
/* TODO: for WIN32, use the unicode API directly */
	qse_mchar_t* mbsfile;
	qse_mchar_t mbsfile_buf[QSE_PATH_MAX];
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();
	qse_size_t wl, ml;
	int n;

	ml = QSE_COUNTOF(mbsfile_buf);
	n = qse_wcstombs(file, &wl, mbsfile_buf, &ml);
	if (n == -2)
	{
		/* buffer too small */
		mbsfile = qse_wcstombsdup(file, QSE_NULL, mmgr);
		if (!mbsfile) return -1;
	}
	else if (n <= -1)
	{
		/* other errors */
		return -1;
	}
	else mbsfile = mbsfile_buf;

	n = qse_get_mbsfile_attr(mbsfile, flags, attr);

	if (mbsfile != mbsfile_buf)	QSE_MMGR_FREE (mmgr, mbsfile);
	return n;
}

int qse_check_mbsfile_perm (const qse_mchar_t* file, int flags, qse_fperm_t perm)
{
	return -1;
}

int qse_check_wcsfile_perm (const qse_wchar_t* file, int flags, qse_fperm_t perm)
{
/* TODO: for WIN32, use the unicode API directly */
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL();
	qse_mchar_t* mbsfile;
	qse_mchar_t mbsfile_buf[QSE_PATH_MAX];
	qse_size_t wl, ml;
	int n;

	ml = QSE_COUNTOF(mbsfile_buf);
	n = qse_wcstombs(file, &wl, mbsfile_buf, &ml);
	if (n == -2)
	{
		/* buffer too small */
		mbsfile = qse_wcstombsdup(file, QSE_NULL, mmgr);
		if (!mbsfile) return -1;
	}
	else if (n <= -1)
	{
		/* other errors */
		return -1;
	}
	else mbsfile = mbsfile_buf;

	n = qse_check_mbsfile_perm(mbsfile, flags, perm);

	if (mbsfile != mbsfile_buf)	QSE_MMGR_FREE (mmgr, mbsfile);
	return n;
}

int qse_get_prog_path_with_mmgr (const qse_char_t* argv0, qse_char_t* buf, qse_size_t size, qse_mmgr_t* mmgr)
{
#if defined(_WIN32)
	if (GetModuleFileName(QSE_NULL, buf, size) == 0) return -1;
#else
	if (argv0 == QSE_NULL) return -1;

	if (argv0[0] == QSE_T('/')) 
	{
		/*qse_strxcpy (buf, size, argv0);*/
		qse_canonpath(argv0, buf, size);
	}
	else if (qse_strchr(argv0, QSE_T('/'))) 
	{
		if (!qse_get_current_dir(buf, size)) return -1;
		qse_strxcajoin (buf, size, QSE_T("/"), argv0, QSE_NULL);
		qse_canonpath (buf, buf, size);
	}
	else 
	{
		qse_char_t *p, *q, * px = QSE_NULL;
		qse_fattr_t attr;
		qse_char_t dir[QSE_PATH_MAX + 1];
		qse_char_t pbuf[QSE_PATH_MAX + 1];
		int first = 1;

#if defined(QSE_CHAR_IS_MCHAR)
		p = getenv("PATH");
		if (!p) p = (qse_char_t*)QSE_T("/bin:/usr/bin");
#else
		qse_mchar_t* mp = getenv ("PATH");

		if (!mp) p = (qse_char_t*)QSE_T("/bin:/usr/bin");
		else 
		{
			px = qse_mbstowcsdup(mp, QSE_NULL, mmgr);
			if (!px) return -1;
			p = px;
		}
#endif
		for (;;) 
		{
			while (*p == QSE_T(':')) p++;
			if (*p == QSE_T('\0')) 
			{
				if (first) 
				{
					p = (qse_char_t*)QSE_T("./");
					first = 0;
				}
				else 
				{
					if (px) QSE_MMGR_FREE (mmgr, px);
					return -1;
				}
			}

			q = p;
			while (*p != QSE_T(':') && *p != QSE_T('\0')) p++;

			if (p - q >= QSE_COUNTOF(dir) - 1) 
			{
				if (px) QSE_MMGR_FREE (mmgr, px);
				return -1;
			}

			qse_strxncpy (dir, QSE_COUNTOF(dir), q, p - q);
			qse_canonpath (dir, dir, QSE_COUNTOF(dir));

			qse_strxjoin (pbuf, QSE_COUNTOF(pbuf), dir, QSE_T("/"), argv0, QSE_NULL);

			if (qse_check_file_perm(pbuf, 0, QSE_FPERM_EXEC) >= 0 && 
			    qse_get_file_attr(pbuf, 0, &attr) >= 0 && attr.isreg) break;
			/*
			if (qse_access(pbuf, QSE_ACCESS_EXECUTE) == 0 && 
			    qse_stat(pbuf, &st) == 0 && S_ISREG(st.st_mode)) 
			{
				break;
			}*/
		}

		if (px) QSE_MMGR_FREE (mmgr, px);

		if (pbuf[0] == QSE_T('/')) qse_strxcpy (buf, size, pbuf);
		else 
		{
			if (!qse_get_current_dir(buf, size)) return -1;
			qse_strxcajoin (buf, size, QSE_T("/"), pbuf, QSE_NULL);
			qse_canonpath (buf, buf, size);
		}
	}
#endif

	return 0;
}

int qse_get_prog_path (const qse_char_t* argv0, qse_char_t* buf, qse_size_t size)
{
	return qse_get_prog_path_with_mmgr(argv0, buf, size, QSE_MMGR_GETDFL());
}
