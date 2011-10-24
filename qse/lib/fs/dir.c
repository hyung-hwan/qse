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

#include <qse/fs/dir.h>
#include <qse/cmn/str.h>
#include <qse/cmn/path.h>
#include "../cmn/mem.h"

#if defined(_WIN32)
#	include <windows.h>
#elif defined(__OS2__)
#	error NOT IMPLEMENTED
#elif defined(__DOS__)
#	error NOT IMPLEMENTED
#else
#	include <sys/types.h>
#    include <sys/stat.h>
#	include <dirent.h>
#	include <errno.h>
#endif

typedef struct info_t info_t;
struct info_t
{
	qse_xstr_t name;

#if defined(_WIN32)
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	unsigned int first_after_change;
#elif defined(__OS2__)
#elif defined(__DOS__)
#else
	DIR* handle;
#endif
};

QSE_IMPLEMENT_COMMON_FUNCTIONS (dir)

qse_dir_t* qse_dir_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_dir_t* dir;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	dir = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*dir) + xtnsize);
	if (dir == QSE_NULL) return QSE_NULL;

	if (qse_dir_init (dir, mmgr) <= -1) 
	{
		QSE_MMGR_FREE (mmgr, dir);
		return QSE_NULL;
	}

	return dir;
}

void qse_dir_close (qse_dir_t* dir)
{
	qse_dir_fini (dir);
	QSE_MMGR_FREE (dir->mmgr, dir);
}

int qse_dir_init (qse_dir_t* dir, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (dir, 0, QSE_SIZEOF(*dir));
	dir->mmgr = mmgr;
	return 0;
}

void qse_dir_fini (qse_dir_t* dir)
{
	info_t* info;

	info = dir->info;
	if (info)
	{
		if (info->name.ptr)
		{
			QSE_ASSERT (info->name.len > 0);
			QSE_MMGR_FREE (dir->mmgr, info->name.ptr);
			info->name.ptr = QSE_NULL;
			info->name.len = 0;
		}

#if defined(_WIN32)
		if (info->handle != INVALID_HANDLE_VALUE) 
		{
			FindClose (info->handle);
			info->handle = INVALID_HANDLE_VALUE;
		}
#elif defined(__OS2__)
#	error NOT IMPLEMENTED
#elif defined(__DOS__)
#	error NOT IMPLEMENTED
#else
		if (info->handle)
		{
			closedir (info->handle);
			info->handle = QSE_NULL;
		}
#endif

		QSE_MMGR_FREE (dir->mmgr, info);
		dir->info = QSE_NULL;
	}

	if (dir->curdir) 
	{
		QSE_MMGR_FREE (dir->mmgr, dir->curdir);
		dir->curdir = QSE_NULL;
	}
}

int qse_dir_change (qse_dir_t* dir, const qse_char_t* name)
{
	qse_char_t* dirname;
	info_t* info;

#if defined(_WIN32)
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	const qse_char_t* tmp_name[4];
	qse_size_t idx;
#elif defined(__OS2__)
#	error NOT IMPLEMENTED
#elif defined(__DOS__)
#	error NOT IMPLEMENTED
#else
	DIR* handle;
	qse_mchar_t* mbsdirname;
#endif

	if (name[0] == QSE_T('\0'))
	{
		dir->errnum = QSE_DIR_EINVAL;
		return -1;
	}

	info = dir->info;
	if (info == QSE_NULL)
	{
		info = QSE_MMGR_ALLOC (dir->mmgr, QSE_SIZEOF(*info));
		if (info == QSE_NULL) 
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return -1;
		}

		QSE_MEMSET (info, 0, QSE_SIZEOF(*info));
#if defined(_WIN32)
		info->handle = INVALID_HANDLE_VALUE;
#endif
		dir->info = info;
	}

#if defined(_WIN32)
	idx = 0;
	if (!qse_isabspath(name) && dir->curdir)
		tmp_name[idx++] = dir->curdir;
	tmp_name[idx++] = name;
	tmp_name[idx++] = QSE_T("\\ ");
	tmp_name[idx] = QSE_NULL;

	dirname = qse_stradup (tmp_name, dir->mmgr);
	if (dirname == QSE_NULL) 
	{
		dir->errnum = QSE_DIR_ENOMEM; 
		return -1;
	}

	idx = qse_canonpath (dirname, dirname);
	/* Put the asterisk after canonicalization to prevent side-effects.
	 * otherwise, .\* would be transformed to * by qse_canonpath() */
	dirname[idx-1] = QSE_T('*'); 

	/*
	I don't return a short name so FindExInfoBasic can speed up the function.
	FindFirstFileEx (
		dirname, FindExInfoBasic, 
		&wfd, FindExSearchNameMatch, 
		NULL, FIND_FIRST_EX_CASE_SENSITIVE);
	*/
	handle = FindFirstFile (dirname, &wfd);
	if (handle == INVALID_HANDLE_VALUE) 
	{
		DWORD e = GetLastError();
		dir->errnum = (e == ERROR_ACCESS_DENIED)?  QSE_DIR_EACCES:
		              (e == ERROR_FILE_NOT_FOUND)? QSE_DIR_ENOENT:	
		              (e == ERROR_INVALID_NAME)?   QSE_DIR_EINVAL:	
		              (e == ERROR_DIRECTORY)?      QSE_DIR_EINVAL:	
		                                           QSE_DIR_ESYSTEM;
		QSE_MMGR_FREE (dir->mmgr, dirname);
		return -1;
	}

	if (info->handle != INVALID_HANDLE_VALUE) 
		FindClose (info->handle);

	QSE_MEMSET (info, 0, QSE_SIZEOF(*info));
	info->handle = handle;
	info->wfd = wfd;

	if (dir->curdir) QSE_MMGR_FREE (dir->mmgr, dir->curdir);
	dirname[idx-1] = QSE_T('\0'); /* drop the asterisk */
	dir->curdir = dirname;
	dir->first_after_change = 1;

	return 0;

#elif defined(__OS2__)
#	error NOT IMPLEMENTED
#elif defined(__DOS__)
#	error NOT IMPLEMENTED
#else
	dirname = qse_strdup (name, dir->mmgr);
	if (dirname == QSE_NULL)
	{	
		dir->errnum = QSE_DIR_ENOMEM;
		return -1;
	}

	qse_canonpath (dirname, dirname);

#if defined(QSE_CHAR_IS_MCHAR)
	mbsdirname = dirname;
#else
	mbsdirname = qse_wcstombsdup (name, dir->mmgr);
	if (mbsdirname == QSE_NULL)
	{
		dir->errnum = QSE_DIR_ENOMEM;
		QSE_MMGR_FREE (dir->mmgr, dirname);
		return -1;
	}
#endif

	handle = opendir (mbsdirname);

#if defined(QSE_CHAR_IS_MCHAR)
	/* do nothing */
#else
	QSE_MMGR_FREE (dir->mmgr, mbsdirname);
#endif

	if (handle == QSE_NULL)
	{
		dir->errnum = (errno == EACCES)?  QSE_DIR_EACCES:
		              (errno == ENOENT)?  QSE_DIR_ENOENT:
		              (errno == ENOTDIR)? QSE_DIR_ENOTDIR:
		              (errno == ENOMEM)?  QSE_DIR_ENOMEM:
		                                  QSE_DIR_ESYSTEM;
	
		QSE_MMGR_FREE (dir->mmgr, dirname);
		return -1;
	}

	if (info->handle) closedir (info->handle);
	info->handle = handle;

	if (dir->curdir) QSE_MMGR_FREE (dir->mmgr, dir->curdir);
	dir->curdir = dirname;

	return 0;
#endif
}

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
static int set_entry_name (qse_dir_t* dir, const qse_char_t* name)
#else
static int set_entry_name (qse_dir_t* dir, const qse_mchar_t* name)
#endif
{
	info_t* info;
	qse_size_t len;

	info = dir->info;
	QSE_ASSERT (info != QSE_NULL);

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	len = qse_strlen (name);
#else
	{
		qse_size_t mlen;

		/* TODO: ignore MBWCERR */
		mlen = qse_mbstowcslen (name, &len);	
		if (name[mlen] != QSE_MT('\0')) 
		{
			/* invalid name ??? */
			return -1;
		}
	}
#endif

	if (len > info->name.len)
	{
		qse_char_t* tmp;

/* TOOD: round up len to the nearlest multiples of something (32, 64, ??)*/
		tmp = QSE_MMGR_REALLOC (
			dir->mmgr, 
			info->name.ptr, 
			(len + 1) * QSE_SIZEOF(*tmp)
		);
		if (tmp == QSE_NULL)
		{
			dir->errnum = QSE_DIR_ENOMEM;
			return -1;
		}

		info->name.len = len;
		info->name.ptr = tmp;
	}

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	qse_strcpy (info->name.ptr, name);
#else
	len++;
	qse_mbstowcs (name, info->name.ptr, &len);
#endif
	dir->ent.name = info->name.ptr;
	return 0;
}

qse_dir_ent_t* qse_dir_read (qse_dir_t* dir)
{

	info_t* info;

	info = dir->info;
	if (info == QSE_NULL) 
	{
		/*
		dir->errnum = NO CHANGE DIR YET;
		*/
		return QSE_NULL;
	}

#if defined(_WIN32)

#if 0
	if (info->no_more_files) 
	{
		dir->errnum = QSE_DIR_ENOERR;
		return QSE_NULL;
	}
#endif
	if (info->first_after_change)
	{
		/* call set_entry_name before changing other fields
		 * in dir->ent not to pollute it in case set_entry_name fails */
		if (set_entry_name (dir, info->wfd.cFileName) <= -1) return QSE_NULL;

		info->first_after_change = 0;
	}
	else
	{
		if (FindNextFile (info->handle, &info->wfd) == FALSE) 
		{
			DWORD e = GetLastError();
			if (e == ERROR_NO_MORE_FILES)
			{
				//info->no_more_files = 1;
				dir->errnum = QSE_DIR_ENOERR;
				return QSE_NULL;
			}
			else
			{
				dir->errnum = (e == ERROR_ACCESS_DENIED)?  QSE_DIR_EACCES:
				              (e == ERROR_FILE_NOT_FOUND)? QSE_DIR_ENOENT:	
				              (e == ERROR_INVALID_NAME)?   QSE_DIR_EINVAL:	
				              (e == ERROR_DIRECTORY)?      QSE_DIR_EINVAL:	
				                                           QSE_DIR_ESYSTEM;
				return QSE_NULL;
			}
		}

		/* call set_entry_name before changing other fields
		 * in dir->ent not to pollute it in case set_entry_name fails */
		if (set_entry_name (dir, info->wfd.cFileName) <= -1) return QSE_NULL;
	}


	if (info->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		dir->ent.size = 0;
		dir->ent.type = QSE_DIR_ENT_DIRECTORY;
	}
	else
	{
		LARGE_INTEGER li;
		li.LowPart = info->wfd.nFileSizeLow;
		li.HighPart = info->wfd.nFileSizeHigh;
		dir->ent.size = li.QuadPart;
		dir->ent.type = QSE_DIR_ENT_UNKNOWN;
	}

#elif defined(__OS2__)
#	error NOT IMPLEMENTED
#elif defined(__DOS__)
#	error NOT IMPLEMENTED
#else
	struct dirent* ent;

	ent = readdir (info->handle);
	if (ent == QSE_NULL)
	{
		/*dir->errnum = QSE_DIR_ENOENT;*/ /* TODO: to be more specific */
		return QSE_NULL;
	}

#if defined(HAVE_STRUCT_DIRENT_D_TYPE)
	if (ent->d_type != DT_DIR)
#endif
	{
		int x;
	#if defined(HAVE_LSTAT64)
		struct stat64 st;
		x = lstat64 (ent->d_name, &st);
	#else
		struct stat st;
		x = lstat (ent->d_name, &st);
	#endif
		if (x == -1)
		{
			/*TODO: dir->errnum = ... */
			return QSE_NULL;
		}
	}

	if (set_entry_name (dir, ent->d_name) <= -1) return QSE_NULL;

	#if defined(HAVE_STRUCT_DIRENT_D_TYPE)
	switch (ent->d_type)
	{
		case DT_DIR:
			dir->ent.size = 0;
			dir->ent.type = QSE_DIR_ENT_DIRECTORY;
			break;
			
		case DT_REG:
			dir->ent.type = QSE_DIR_ENT_REGULAR;
			break;
			
		default:
			dir->ent.size = 0;
			dir->ent.type = QSE_DIR_ENT_UNKNOWN;
			break;
	}
	#endif

#endif
	return &dir->ent;
}

int qse_dir_rewind (qse_dir_t* dir)
{
	return 0;
}

qse_dir_errnum_t qse_dir_geterrnum (qse_dir_t* dir)
{
	return dir->errnum;
}

const qse_char_t* qse_dir_geterrmsg (qse_dir_t* dir)
{
	static const qse_char_t* errstr[] =
 	{
		QSE_T("no error"),
		QSE_T("internal error that should never have happened"),

		QSE_T("insufficient memory"),
		QSE_T("invalid parameter or data"),
		QSE_T("permission denined"),
		QSE_T("no such entry"),
		QSE_T("not a directory"),
		QSE_T("system error")
	};

	return (dir->errnum >= 0 && dir->errnum < QSE_COUNTOF(errstr))?
		errstr[dir->errnum]: QSE_T("unknown error");
}
