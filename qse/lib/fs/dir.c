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
#	include <dirent.h>
#endif

typedef struct info_t info_t;
struct info_t
{
	qse_xstr_t name;

#if defined(_WIN32)
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	int no_more_files;
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
	info_t* info = dir->info;
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

static int is_absolute (const qse_char_t* name)
{
	if (name[0] == QSE_T('/')) return 1;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (name[0] == QSE_T('\\')) return 1;
	if (((name[0] >= QSE_T('A') && name[0] <= QSE_T('Z')) ||
	    (name[0] >= QSE_T('a') && name[0] <= QSE_T('a'))) &&
	    name[1] == QSE_T(':')) return 1;
#endif
	return 0;
}

int qse_dir_change (qse_dir_t* dir, const qse_char_t* name)
{
	info_t* info = dir->info;
#if defined(_WIN32)
	qse_char_t* dirname;
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	const qse_char_t* tmp_name[4];
	qse_size_t idx;
#endif

	if (name[0] == QSE_T('\0'))
	{
		/* dir->errnum = QSE_DIR_EINVAL; */
		return -1;
	}


/* TODO: if name is a relative path??? combine it with the current path
 and canonicalize it to get the actual path */

	if (info == QSE_NULL)
	{
		info = QSE_MMGR_ALLOC (dir->mmgr, QSE_SIZEOF(*info));
		if (info == QSE_NULL) return -1;

		QSE_MEMSET (info, 0, QSE_SIZEOF(*info));
#if defined(_WIN32)
		info->handle = INVALID_HANDLE_VALUE;
#endif

		dir->info = info;
	}

#if defined(_WIN32)
	idx = 0;
	if (!is_absolute(name) && dir->curdir)
		tmp_name[idx++] = dir->curdir;
	tmp_name[idx++] = name;
	tmp_name[idx++] = QSE_T("\\ ");
	tmp_name[idx] = QSE_NULL;

	dirname = qse_stradup (tmp_name, dir->mmgr);
	if (dirname == QSE_NULL) 
	{
	/*	dir->errnum = QSE_DIR_ENOMEM; */
		return -1;
	}

	idx = qse_canonpath (dirname, dirname);
	/* Put the asterisk after canonicalization to prevent side-effects.
	 * otherwise, .\* would be transformed to * by qse_canonpath() */
	dirname[idx-1] = QSE_T('*'); 

	handle = FindFirstFile (dirname, &wfd);
	if (handle == INVALID_HANDLE_VALUE) 
	{
	/*	dir->errnum = QSE_DIR_ESYSTEM; */
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

	return 0;
#else
	return -1;
#endif
}

static int set_entry_name (qse_dir_t* dir, const qse_char_t* name)
{
	info_t* info = dir->info;
	qse_size_t len;

	QSE_ASSERT (info != QSE_NULL);

	len = qse_strlen (name);
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
			/* dir->errnum = QSE_DIR_ENOMEM; */
			return -1;
		}

		info->name.len = len;
		info->name.ptr = tmp;
	}

	qse_strcpy (info->name.ptr, name);
	dir->ent.name = info->name.ptr;
	return 0;
}

qse_dir_ent_t* qse_dir_read (qse_dir_t* dir)
{
	info_t* info = dir->info;
	if (info == QSE_NULL) return QSE_NULL;

#if defined(_WIN32)
	if (info->no_more_files) 
	{
		/* 
		dir->errnum = QSE_DIR_ENOENT;
		*/
		return QSE_NULL;
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

	if (set_entry_name (dir, info->wfd.cFileName) <= -1) return QSE_NULL;

	if (FindNextFile (info->handle, &info->wfd) == FALSE) 
	{
		/*if (GetLastError() == ERROR_NO_MORE_FILES) */
		info->no_more_files = 1;
	}
#endif

	return &dir->ent;
}

int qse_dir_rewind (qse_dir_t* dir)
{
	return 0;
}

