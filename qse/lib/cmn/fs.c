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
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/path.h>
#include "mem.h"

#if defined(_WIN32)
	/* nothing else */
#elif defined(__OS2__)
	/* nothing else */
#elif defined(__DOS__)
	/* nothing else */
#else
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
	int just_changed_fs;
#elif defined(__OS2__)
#elif defined(__DOS__)
#else
	DIR* handle;
	qse_mchar_t* mcurdir;
#endif
};

qse_fs_t* qse_fs_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_fs_t* fs;

	fs = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*fs) + xtnsize);
	if (fs == QSE_NULL) return QSE_NULL;

	if (qse_fs_init (fs, mmgr) <= -1) 
	{
		QSE_MMGR_FREE (mmgr, fs);
		return QSE_NULL;
	}

	QSE_MEMSET (fs + 1, 0, xtnsize);
	return fs;
}

void qse_fs_close (qse_fs_t* fs)
{
	qse_fs_fini (fs);
	QSE_MMGR_FREE (fs->mmgr, fs);
}

int qse_fs_init (qse_fs_t* fs, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (fs, 0, QSE_SIZEOF(*fs));
	fs->mmgr = mmgr;
	return 0;
}

void qse_fs_fini (qse_fs_t* fs)
{
	info_t* info;

	info = fs->info;
	if (info)
	{
		if (info->name.ptr)
		{
			QSE_ASSERT (info->name.len > 0);
			QSE_MMGR_FREE (fs->mmgr, info->name.ptr);
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
		/* TODO: implement this */
#elif defined(__DOS__)
		/* TODO: implement this */
#else
		if (info->mcurdir && info->mcurdir != fs->curdir)
			QSE_MMGR_FREE (fs->mmgr, info->mcurdir);
		info->mcurdir = QSE_NULL;
			
		if (info->handle)
		{
			closedir (info->handle);
			info->handle = QSE_NULL;
		}
#endif

		QSE_MMGR_FREE (fs->mmgr, info);
		fs->info = QSE_NULL;
	}

	if (fs->curdir) 
	{
		QSE_MMGR_FREE (fs->mmgr, fs->curdir);
		fs->curdir = QSE_NULL;
	}
}

qse_mmgr_t* qse_fs_getmmgr (qse_fs_t* fs)
{
	return fs->mmgr;
}

void* qse_fs_getxtn (qse_fs_t* fs)
{
	return QSE_XTN (fs);
}

static QSE_INLINE info_t* get_info (qse_fs_t* fs)
{
	info_t* info;

	info = fs->info;
	if (info == QSE_NULL)
	{
		info = QSE_MMGR_ALLOC (fs->mmgr, QSE_SIZEOF(*info));
		if (info == QSE_NULL) 
		{
			fs->errnum = QSE_FS_ENOMEM;
			return QSE_NULL;
		}

		QSE_MEMSET (info, 0, QSE_SIZEOF(*info));
#if defined(_WIN32)
		info->handle = INVALID_HANDLE_VALUE;
#endif
		fs->info = info;
	}

	return info;
}

int qse_fs_chdir (qse_fs_t* fs, const qse_char_t* name)
{
	qse_char_t* fsname;
	info_t* info;

#if defined(_WIN32)
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	const qse_char_t* tmp_name[4];
	qse_size_t idx;
#elif defined(__OS2__)
	/* TODO: implement this */
#elif defined(__DOS__)
	/* TODO: implement this */
#else
	DIR* handle;
	qse_mchar_t* mfsname;
	const qse_char_t* tmp_name[4];
	qse_size_t idx;
#endif

	if (name[0] == QSE_T('\0'))
	{
		fs->errnum = QSE_FS_EINVAL;
		return -1;
	}

	info = get_info (fs);
	if (info == QSE_NULL) return -1;

#if defined(_WIN32)
	idx = 0;
	if (!qse_isabspath(name) && fs->curdir)
		tmp_name[idx++] = fs->curdir;
	tmp_name[idx++] = name;

	if (qse_isdrivecurpath(name)) 
		tmp_name[idx++] = QSE_T(" ");
	else
		tmp_name[idx++] = QSE_T("\\ ");

	tmp_name[idx] = QSE_NULL;

	fsname = qse_stradup (tmp_name, QSE_NULL, fs->mmgr);
	if (fsname == QSE_NULL) 
	{
		fs->errnum = QSE_FS_ENOMEM; 
		return -1;
	}

	idx = qse_canonpath (fsname, fsname, 0);
	/* Put an asterisk after canonicalization to prevent side-effects.
	 * otherwise, .\* would be transformed to * by qse_canonpath() */
	fsname[idx-1] = QSE_T('*'); 

	/* Using FindExInfoBasic won't resolve cAlternatFileName.
	 * so it can get faster a little bit. The problem is that
	 * it is not supported on old windows. just stick to the
	 * simple API instead. */
	#if 0
	handle = FindFirstFileEx (
		fsname, FindExInfoBasic,
		&wfd, FindExSearchNameMatch, 
		NULL, 0/*FIND_FIRST_EX_CASE_SENSITIVE*/);
	#endif
	handle = FindFirstFile (fsname, &wfd);
	if (handle == INVALID_HANDLE_VALUE) 
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
		QSE_MMGR_FREE (fs->mmgr, fsname);
		return -1;
	}

	if (info->handle != INVALID_HANDLE_VALUE) 
		FindClose (info->handle);

	QSE_MEMSET (info, 0, QSE_SIZEOF(*info));
	info->handle = handle;
	info->wfd = wfd;
	info->just_changed_fs = 1;

	if (fs->curdir) QSE_MMGR_FREE (fs->mmgr, fs->curdir);
	fsname[idx-1] = QSE_T('\0'); /* drop the asterisk */
	fs->curdir = fsname;

	return 0;

#elif defined(__OS2__)
	/* TODO: implement this */
	return 0;
#elif defined(__DOS__)
	/* TODO: implement this */
	return 0;
#else

	idx = 0;
	if (!qse_isabspath(name) && fs->curdir)
	{
		tmp_name[idx++] = fs->curdir;
		tmp_name[idx++] = QSE_T("/");
	}
	tmp_name[idx++] = name;
	tmp_name[idx] = QSE_NULL;

	fsname = qse_stradup (tmp_name, QSE_NULL, fs->mmgr);
	if (fsname == QSE_NULL)
	{	
		fs->errnum = QSE_FS_ENOMEM;
		return -1;
	}

	qse_canonpath (fsname, fsname, 0);

#if defined(QSE_CHAR_IS_MCHAR)
	mfsname = fsname;
#else
	mfsname = qse_wcstombsdup (fsname, QSE_NULL, fs->mmgr);
	if (mfsname == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		QSE_MMGR_FREE (fs->mmgr, fsname);
		return -1;
	}
#endif

	handle = opendir (mfsname);

	if (handle == QSE_NULL)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		if (mfsname != fsname) 
			QSE_MMGR_FREE (fs->mmgr, mfsname);
		QSE_MMGR_FREE (fs->mmgr, fsname);
		return -1;
	}

	if (info->handle) closedir (info->handle);
	info->handle = handle;

	if (info->mcurdir && info->mcurdir != fs->curdir)
		QSE_MMGR_FREE (fs->mmgr, info->mcurdir);
	info->mcurdir = mfsname;

	if (fs->curdir) QSE_MMGR_FREE (fs->mmgr, fs->curdir);
	fs->curdir = fsname;

	return 0;
#endif
}

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
static int set_entry_name (qse_fs_t* fs, const qse_char_t* name)
#else
static int set_entry_name (qse_fs_t* fs, const qse_mchar_t* name)
#endif
{
	info_t* info;
	qse_size_t len;

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	/* nothing more to declare */
#else
	qse_size_t mlen;
#endif

	info = fs->info;
	QSE_ASSERT (info != QSE_NULL);

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	len = qse_strlen (name);
#else
	/* TODO: ignore MBWCERR */
	if (qse_mbstowcs (name, &mlen, QSE_NULL, &len) <= -1)
	{
		/* invalid name ??? */
		return -1;
	}
#endif

	if (len > info->name.len)
	{
		qse_char_t* tmp;

/* TOOD: round up len to the nearlest multiples of something (32, 64, ??)*/
		tmp = QSE_MMGR_REALLOC (
			fs->mmgr, 
			info->name.ptr, 
			(len + 1) * QSE_SIZEOF(*tmp)
		);
		if (tmp == QSE_NULL)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		info->name.len = len;
		info->name.ptr = tmp;
	}

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	qse_strcpy (info->name.ptr, name);
#else
	len++; /* for terminating null */
	qse_mbstowcs (name, &mlen, info->name.ptr, &len);
#endif

	fs->ent.name.base = info->name.ptr;
	fs->ent.flags |= QSE_FS_ENT_NAME;
	return 0;
}

#if defined(_WIN32)
static QSE_INLINE void filetime_to_ntime (const FILETIME* ft, qse_ntime_t* nt)
{
	/* reverse of http://support.microsoft.com/kb/167296/en-us */
	ULARGE_INTEGER li;

	li.LowPart = ft->dwLowDateTime;
	li.HighPart = ft->dwHighDateTime;

#if (QSE_SIZEOF_LONG_LONG>=8)
	li.QuadPart -= 116444736000000000ull;
#elif (QSE_SIZEOF___INT64>=8)
	li.QuadPart -= 116444736000000000ui64;
#else
#	error Unsupported 64bit integer type
#endif
	/*li.QuadPart /= 10000000;*/
	/*li.QuadPart /= 10000;
	return li.QuadPart;*/

	/* li.QuadPart is in the 100-nanosecond intervals */
	nt->sec =  li.QuadPart / (QSE_NSECS_PER_SEC / 100);
	nt->nsec = (li.QuadPart % (QSE_NSECS_PER_SEC / 100)) * 100;
}
#endif

qse_fs_ent_t* qse_fs_read (qse_fs_t* fs, int flags)
{
#if defined(_WIN32)
	info_t* info;

	info = fs->info;
	if (info == QSE_NULL) 
	{
		fs->errnum = QSE_FS_ENODIR;
		return QSE_NULL;
	}

	if (info->just_changed_fs)
	{
		info->just_changed_fs = 0;
	}
	else
	{
		if (FindNextFile (info->handle, &info->wfd) == FALSE) 
		{
			DWORD e = GetLastError();
			if (e == ERROR_NO_MORE_FILES)
			{
				fs->errnum = QSE_FS_ENOERR;
				return QSE_NULL;
			}
			else
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, e);
				return QSE_NULL;
			}
		}
	}

	/* call set_entry_name before changing other fields
	 * in fs->ent not to pollute it in case set_entry_name fails */
	QSE_MEMSET (&fs->ent, 0, QSE_SIZEOF(fs->ent));
	if (set_entry_name (fs, info->wfd.cFileName) <= -1) return QSE_NULL;

	if (flags & QSE_FS_ENT_TYPE)
	{
#if !defined(IO_REPARSE_TAG_SYMLINK)
#	define IO_REPARSE_TAG_SYMLINK 0xA000000C
#endif
		if (info->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			fs->ent.type = QSE_FS_ENT_SUBDIR;
		}
		else if ((info->wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
			    (info->wfd.dwReserved0 == IO_REPARSE_TAG_SYMLINK))
		{
			fs->ent.type = QSE_FS_ENT_SYMLINK;
		}
		else
		{
			HANDLE h;
			qse_char_t* tmp_name[4];
			qse_char_t* fname;

/* TODO: use a buffer in info... instead of allocating an deallocating every time */
			tmp_name[0] = fs->curdir;
			tmp_name[1] = QSE_T("\\");
			tmp_name[2] = info->wfd.cFileName;
			tmp_name[3] = QSE_NULL;
			fname = qse_stradup (tmp_name, QSE_NULL, fs->mmgr);
			if (fname == QSE_NULL)
			{
				fs->errnum = QSE_FS_ENOMEM;
				return QSE_NULL;
			}

			h = CreateFile (
				fname,
				GENERIC_READ, 
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
				QSE_NULL, 
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 
				0
			);

			QSE_MMGR_FREE (fs->mmgr, fname);

			if (h != INVALID_HANDLE_VALUE)
			{
				DWORD t = GetFileType (h);
				switch (t)
				{
					case FILE_TYPE_CHAR:
						fs->ent.type = QSE_FS_ENT_CHRDEV;
						break;
					case FILE_TYPE_DISK:
						fs->ent.type = QSE_FS_ENT_BLKDEV;
						break;
					case FILE_TYPE_PIPE:
						fs->ent.type = QSE_FS_ENT_PIPE;
						break;
					default:
						fs->ent.type = QSE_FS_ENT_UNKNOWN;
						break;
				}
				CloseHandle (h);
			}
			else
			{
				fs->ent.type = QSE_FS_ENT_UNKNOWN;
			}
		}
		fs->ent.type |= QSE_FS_ENT_TYPE;
	}

	if (flags & QSE_FS_ENT_SIZE)
	{
		LARGE_INTEGER li;
		li.LowPart = info->wfd.nFileSizeLow;
		li.HighPart = info->wfd.nFileSizeHigh;
		fs->ent.size = li.QuadPart;
		fs->ent.type |= QSE_FS_ENT_SIZE;
	}

	if (flags & QSE_FS_ENT_TIME)
	{
		filetime_to_ntime (&info->wfd.ftCreationTime, &fs->ent.time.create);
		filetime_to_ntime (&info->wfd.ftLastAccessTime, &fs->ent.time.access);
		filetime_to_ntime (&info->wfd.ftLastWriteTime, &fs->ent.time.modify);
		fs->ent.type |= QSE_FS_ENT_TIME;
	}

#elif defined(__OS2__)
	/* TODO: implement this */
#elif defined(__DOS__)
	/* TODO: implement this */
#else

	info_t* info;
	struct dirent* ent;
	int x;

	int stat_needed;
	qse_lstat_t st;

	info = fs->info;
	if (info == QSE_NULL) 
	{
		fs->errnum = QSE_FS_ENODIR;
		return QSE_NULL;
	}

	errno = 0;
	ent = readdir (info->handle);
	if (ent == QSE_NULL)
	{
		if (errno != 0) fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return QSE_NULL;
	}

	QSE_MEMSET (&fs->ent, 0, QSE_SIZEOF(fs->ent));
	if (set_entry_name (fs, ent->d_name) <= -1) return QSE_NULL;

	stat_needed =
	#if !defined(HAVE_STRUCT_DIRENT_D_TYPE)
		(flags & QSE_FS_ENT_TYPE) ||
	#endif
		(flags & QSE_FS_ENT_SIZE) ||
		(flags & QSE_FS_ENT_TIME);
	if (stat_needed)
	{
		qse_mchar_t* tmp_name[4];
		qse_mchar_t* mfname;

/* TODO: use a buffer in info... instead of allocating an deallocating every time */
		tmp_name[0] = info->mcurdir;
		tmp_name[1] = QSE_MT("/");
		tmp_name[2] = ent->d_name;
		tmp_name[3] = QSE_NULL;
		mfname = qse_mbsadup (tmp_name, QSE_NULL, fs->mmgr);
		if (mfname == QSE_NULL)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return QSE_NULL;
		}
	
		x = QSE_LSTAT (mfname, &st);
		QSE_MMGR_FREE (fs->mmgr, mfname);

		if (x == -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return QSE_NULL;
		}
	}

	if (flags & QSE_FS_ENT_TYPE)
	{
	#if defined(HAVE_STRUCT_DIRENT_D_TYPE)
		switch (ent->d_type)
		{
			case DT_DIR:
				fs->ent.type = QSE_FS_ENT_SUBDIR;
				break;

			case DT_REG:
				fs->ent.type = QSE_FS_ENT_REGULAR;
				break;

			case DT_LNK:
				fs->ent.type = QSE_FS_ENT_SYMLINK;
				break;
	
			case DT_BLK: 
				fs->ent.type = QSE_FS_ENT_BLKDEV;
				break;

			case DT_CHR:
				fs->ent.type = QSE_FS_ENT_CHRDEV;
				break;

			case DT_FIFO:
	#if defined(DT_SOCK)
			case DT_SOCK:
	#endif
				fs->ent.type = QSE_FS_ENT_PIPE;
				break;

			default:
				fs->ent.type = QSE_FS_ENT_UNKNOWN;
				break;
		}	

	#else
		#define IS_TYPE(st,type) ((st.st_mode & S_IFMT) == S_IFDIR)
		fs->ent.type = IS_TYPE(st,S_IFDIR)?  QSE_FS_ENT_SUBDIR:
		                IS_TYPE(st,S_IFREG)?  QSE_FS_ENT_REGULAR:
		                IS_TYPE(st,S_IFLNK)?  QSE_FS_ENT_SYMLINK:
		                IS_TYPE(st,S_IFCHR)?  QSE_FS_ENT_CHRDEV:
		                IS_TYPE(st,S_IFBLK)?  QSE_FS_ENT_BLKDEV:
		                IS_TYPE(st,S_IFIFO)?  QSE_FS_ENT_PIPE:
		                IS_TYPE(st,S_IFSOCK)? QSE_FS_ENT_PIPE:
		                                      QSE_FS_ENT_UNKNOWN;
		#undef IS_TYPE
	#endif
		fs->ent.flags |= QSE_FS_ENT_TYPE;
	}

	if (flags & QSE_FS_ENT_SIZE)
	{
		fs->ent.size = st.st_size;
		fs->ent.flags |= QSE_FS_ENT_SIZE;
	}

	if (flags & QSE_FS_ENT_TIME)
	{
	#if defined(HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
		#if defined(HAVE_STRUCT_STAT_ST_BIRTHTIM_TV_NSEC)
		fs->ent.time.create.secs = st.st_birthtim.tv_sec;
		fs->ent.time.create.nsecs = st.st_birthtim.tv_nsec;
		#endif

		fs->ent.time.access.sec = st.st_atim.tv_sec;
		fs->ent.time.access.nsec = st.st_atim.tv_nsec;
		fs->ent.time.modify.sec = st.st_mtim.tv_sec;
		fs->ent.time.modify.nsec = st.st_mtim.tv_nsec;
		fs->ent.time.change.sec = st.st_ctim.tv_sec;
		fs->ent.time.change.nsec = st.st_ctim.tv_nsec;
	#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
		#if defined(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC)
		fs->ent.time.create.sec = st.st_birthtimespec.tv_sec;
		fs->ent.time.create.nsec = st.st_birthtimespec.tv_nsec;
		#endif

		fs->ent.time.access.sec = st.st_atimespec.tv_sec;
		fs->ent.time.access.nsec = st.st_atimespec.tv_nsec;
		fs->ent.time.modify.sec = st.st_mtimespec.tv_sec;
		fs->ent.time.modify.nsec = st.st_mtimespec.tv_nsec;
		fs->ent.time.change.sec = st.st_ctimespec.tv_sec;
		fs->ent.time.change.nsec = st.st_ctimespec.tv_nsec;
	#else
		#if defined(HAVE_STRUCT_STAT_ST_BIRTHTIME)
		fs->ent.time.create.sec = st.st_birthtime;
		#endif
		fs->ent.time.access.sec = st.st_atime;
		fs->ent.time.modify.sec = st.st_mtime;
		fs->ent.time.change.sec = st.st_ctime;
	#endif
		fs->ent.flags |= QSE_FS_ENT_TIME;
	}
#endif

	return &fs->ent;
}

int qse_fs_rewind (qse_fs_t* fs)
{
	return 0;
}

