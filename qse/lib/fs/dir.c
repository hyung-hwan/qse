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
#include "../cmn/mem.h"
#include <qse/cmn/str.h>

#if defined(_WIN32)
#	include <windows.h>
#endif

struct qse_dir_t
{
     QSE_DEFINE_COMMON_FIELDS (dir)

	qse_dir_ent_t ent;

#if defined(_WIN32)
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	int no_more_files;
#else

#endif
};

QSE_IMPLEMENT_COMMON_FUNCTIONS (dir)

int qse_dir_init (qse_dir_t* dir, qse_mmgr_t* mmgr, const qse_char_t* name);
void qse_dir_fini (qse_dir_t* dir);

qse_dir_t* qse_dir_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_char_t* name)
{
	qse_dir_t* dir;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	dir = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*dir));
	if (dir == QSE_NULL) return QSE_NULL;

	if (qse_dir_init (dir, mmgr, name) <= -1) 
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

int qse_dir_init (qse_dir_t* dir, qse_mmgr_t* mmgr, const qse_char_t* name)
{
#if defined(_WIN32)
	qse_char_t* dirname;
#endif

	QSE_MEMSET (dir, 0, QSE_SIZEOF(*dir));
	dir->mmgr = mmgr;

#if defined(_WIN32)
	dirname = qse_strdup2 (name, QSE_T("\\*"), dir->mmgr);
	if (dirname == QSE_NULL) 
	{
	/*	dir->errnum = QSE_DIR_ENOMEM; */
		return -1;
	}
	dir->handle = FindFirstFile (dirname, &dir->wfd);
	QSE_MMGR_FREE (dir->mmgr, dirname);

	if (dir->handle == INVALID_HANDLE_VALUE) 
	{
		return -1;
	}

	return 0;
#else
	return -1;
#endif
}

void qse_dir_fini (qse_dir_t* dir)
{
#if defined(_WIN32)
	FindClose (dir->handle);
#endif
}

qse_dir_ent_t* qse_dir_read (qse_dir_t* dir)
{
#if defined(_WIN32)
	if (dir->no_more_files) return QSE_NULL;

	if (dir->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		dir->ent.size = 0;
		dir->ent.type = QSE_DIR_ENT_DIRECTORY;
	}
	else
	{
		LARGE_INTEGER li;
		li.LowPart = dir->wfd.nFileSizeLow;
		li.HighPart = dir->wfd.nFileSizeHigh;
		dir->ent.size = li.QuadPart;
		dir->ent.type = QSE_DIR_ENT_UNKNOWN;
	}

// TODO: proper management...
	dir->ent.name = qse_strdup (dir->wfd.cFileName, dir->mmgr);

	if (FindNextFile (dir->handle, &dir->wfd) == FALSE) 
	{
		/*if (GetLastError() == ERROR_NO_MORE_FILES) */
		dir->no_more_files = 1;
	}
#endif

	return &dir->ent;
}

int qse_dir_rewind (qse_dir_t* dir)
{
	return 0;
}

#if 0
int qse_dir_change (qse_dir_t* dir, const qse_char_t* name)
{
#if defined(_WIN32)
	qse_strxdup (dir->current, 
#else
	return -1;
#endif
}
#endif
