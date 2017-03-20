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

#include "fs-prv.h"
#include "../cmn/mem-prv.h"


static void stat_to_attr (const qse_stat_t* st, qse_fs_attr_t* attr)
{
	QSE_MEMSET (attr, 0, QSE_SIZEOF(*attr));

	if (S_ISDIR(st->st_mode)) attr->isdir = 1;
	if (S_ISLNK(st->st_mode)) attr->islnk = 1;
	if (S_ISREG(st->st_mode)) attr->isreg = 1;
	if (S_ISBLK(st->st_mode)) attr->isblk = 1;
	if (S_ISCHR(st->st_mode)) attr->ischr = 1;

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

int qse_fs_getattronfd (qse_fs_t* fs, qse_fs_handle_t fd, qse_fs_attr_t* attr, int flags)
{
#if defined(_WIN32)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__OS2__)

	/* TODO */
	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__DOS__)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(HAVE_FSTAT)
	qse_fstat_t st;

	if (QSE_FSTAT (fd, &st) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

	stat_to_attr (&st, attr);
	return 0;
#else
	fs->errnum = QSE_FS_ENOIMPL;
	return -1;
#endif
}

int qse_fs_getattrsys (qse_fs_t* fs, const qse_fs_char_t* fspath, qse_fs_attr_t* attr, int flags)
{
#if defined(_WIN32)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__OS2__)

	/* TODO */
	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__DOS__)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(HAVE_FSTATAT) && defined(AT_SYMLINK_NOFOLLOW)

	qse_fstatat_t st;
	int sysflags = 0;

	if (flags & QSE_FS_GETATTR_SYMLINK) sysflags |= AT_SYMLINK_NOFOLLOW;
	if (QSE_FSTATAT(AT_FDCWD, fspath, &st, sysflags) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

	stat_to_attr (&st, attr);
	return 0;

#else
	qse_stat_t st;
	int x;

	if (flags & QSE_FS_GETATTR_SYMLINK)
		x = QSE_LSTAT (fspath, &st);
	else
		x = QSE_STAT (fspath, &st);

	if (x <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

	stat_to_attr (&st, attr);
	return 0;
#endif
}

int qse_fs_getattrmbs (qse_fs_t* fs, const qse_mchar_t* path, qse_fs_attr_t* attr, int flags)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspathformbs (fs, path);
	if (!fspath) return -1;

	ret = qse_fs_getattrsys (fs, fspath, attr, flags);

	qse_fs_freefspathformbs (fs, path, fspath);
	return ret;
}

int qse_fs_getattrwcs (qse_fs_t* fs, const qse_wchar_t* path, qse_fs_attr_t* attr, int flags)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspathforwcs (fs, path);
	if (!fspath) return -1;

	ret = qse_fs_getattrsys (fs, fspath, attr, flags);

	qse_fs_freefspathforwcs (fs, path, fspath);
	return ret;
}

/* -------------------------------------------------------------------------- */

int qse_fs_setattronfd (qse_fs_t* fs, qse_fs_handle_t fd, const qse_fs_attr_t* attr, int flags)
{
#if defined(_WIN32)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__OS2__)

	/* TODO */
	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__DOS__)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#else
	if (flags & QSE_FS_SETATTR_TIME)
	{
	#if defined(HAVE_FUTIMENS) && defined(HAVE_STRUCT_TIMESPEC)
		struct timespec ts[2];

		QSE_MEMSET (&ts, 0, QSE_SIZEOF(ts));
		ts[0].tv_sec = attr->atime.sec;
		ts[0].tv_nsec = attr->atime.nsec;
		ts[1].tv_sec = attr->mtime.sec;
		ts[1].tv_nsec = attr->mtime.nsec;
		if (QSE_FUTIMENS (fd, ts) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}

	#elif defined(HAVE_FUTIMES)
		struct timeval tv[2];

		QSE_MEMSET (&tv, 0, QSE_SIZEOF(tv));
		tv[0].tv_sec = attr->atime.sec;
		tv[0].tv_usec = QSE_NSEC_TO_USEC(attr->atime.nsec);
		tv[1].tv_sec = attr->mtime.sec;
		tv[1].tv_usec = QSE_NSEC_TO_USEC(attr->mtime.nsec);
		if (QSE_FUTIMES (fd, tv) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	#else
		fs->errnum = QSE_FS_ENOIMPL;
		return -1;
	#endif
	}

	if (flags & QSE_FS_SETATTR_OWNER)
	{
	#if defined(HAVE_FCHOWN)
		if (QSE_FCHOWN (fd, attr->uid, attr->gid) <= -1) 
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	#else
		fs->errnum = QSE_FS_ENOIMPL;
		return -1;
	#endif
	}

	if (flags & QSE_FS_SETATTR_MODE)
	{
	#if defined(HAVE_FCHMOD)
		if (QSE_FCHMOD(fd, attr->mode) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	#else
		fs->errnum = QSE_FS_ENOIMPL;
		return -1;
	#endif
	}

	return 0;
#endif
}

int qse_fs_setattrsys (qse_fs_t* fs, qse_fs_char_t* path, const qse_fs_attr_t* attr, int flags)
{

#if defined(_WIN32)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__OS2__)

	/* TODO */
	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#elif defined(__DOS__)

	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

#else
	if (flags & QSE_FS_SETATTR_TIME)
	{
	#if defined(HAVE_UTIMENSAT) && defined(AT_SYMLINK_NOFOLLOW)
		struct timespec ts[2];
		int sysflags = 0;

		if (flags & QSE_FS_SETATTR_SYMLINK) sysflags |= AT_SYMLINK_NOFOLLOW;

		QSE_MEMSET (&ts, 0, QSE_SIZEOF(ts));
		ts[0].tv_sec = attr->atime.sec;
		ts[0].tv_nsec = attr->atime.nsec;
		ts[1].tv_sec = attr->mtime.sec;
		ts[1].tv_nsec = attr->mtime.nsec;

		if (QSE_UTIMENSAT(AT_FDCWD, path, NULL, sysflags) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}

	#else
		if (flags & QSE_FS_SETATTR_SYMLINK)
		{
		#if defined(HAVE_LUTIMES)
			struct timeval tv[2];

			QSE_MEMSET (&tv, 0, QSE_SIZEOF(tv));
			tv[0].tv_sec = attr->atime.sec;
			tv[0].tv_usec = QSE_NSEC_TO_USEC(attr->atime.nsec);
			tv[1].tv_sec = attr->mtime.sec;
			tv[1].tv_usec = QSE_NSEC_TO_USEC(attr->mtime.nsec);

			if (QSE_LUTIMES (path, tv) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				return -1;
			}
		#else
			fs->errnum = QSE_FS_ENOIMPL;
			return -1;
		#endif
		}
		else
		{
		#if defined(HAVE_UTIMES)
			struct timeval tv[2];

			QSE_MEMSET (&tv, 0, QSE_SIZEOF(tv));
			tv[0].tv_sec = attr->atime.sec;
			tv[0].tv_usec = QSE_NSEC_TO_USEC(attr->atime.nsec);
			tv[1].tv_sec = attr->mtime.sec;
			tv[1].tv_usec = QSE_NSEC_TO_USEC(attr->mtime.nsec);

			if (QSE_UTIMES (path, tv) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				return -1;
			}

		#elif defined(HAVE_UTIME)
			struct utimbuf ub;

			QSE_MEMSET (&ub, 0, QSE_SIZEOF(ub));
			ub.actime = attr->atime.sec;
			ub.modtime = attr->mtime.sec;
			if (QSE_UTIME (path, &ub) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				return -1;
			}

		#else
			fs->errnum = QSE_FS_ENOIMPL;
			return -1;
		#endif
		}
	#endif
	}

	if (flags & QSE_FS_SETATTR_OWNER)
	{
	#if defined(HAVE_FCHOWNAT) && defined(AT_SYMLINK_NOFOLLOW)
		int sysflags = 0;

		if (flags & QSE_FS_SETATTR_SYMLINK) sysflags |= AT_SYMLINK_NOFOLLOW;

		if (QSE_FCHOWNAT(AT_FDCWD, path, attr->uid, attr->gid, sysflags) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	#else
		int x;

		if (flags & QSE_FS_SETATTR_SYMLINK)
			x = QSE_LCHOWN (path, attr->uid, attr->gid);
		else
			x = QSE_CHOWN (path, attr->uid, attr->gid);

		if (x <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	#endif
	}

	if (flags & QSE_FS_SETATTR_MODE)
	{
	#if defined(HAVE_FCHMODAT) && defined(AT_SYMLINK_NOFOLLOW)
		int sysflags = 0;

		if (flags & QSE_FS_SETATTR_SYMLINK) sysflags |= AT_SYMLINK_NOFOLLOW;

		if (QSE_FCHMODAT(AT_FDCWD, path, attr->mode, sysflags) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}
	#else
		if (flags & QSE_FS_SETATTR_SYMLINK)
		{
			/* not supported. symlink permission is kind of fixed.
			 * do nothing */
		}
		else
		{
			if (QSE_CHMOD(path, attr->mode) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				return -1;
			}
		}
	#endif
	}

	return 0;
#endif
}


int qse_fs_setattrmbs (qse_fs_t* fs, qse_mchar_t* path, const qse_fs_attr_t* attr, int flags)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspathformbs (fs, path);
	if (!fspath) return -1;

	ret = qse_fs_setattrsys (fs, fspath, attr, flags);

	qse_fs_freefspathformbs (fs, path, fspath);
	return ret;
}


int qse_fs_setattrwcs (qse_fs_t* fs, qse_wchar_t* path, const qse_fs_attr_t* attr, int flags)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspathforwcs (fs, path);
	if (!fspath) return -1;

	ret = qse_fs_setattrsys (fs, fspath, attr, flags);

	qse_fs_freefspathforwcs (fs, path, fspath);
	return ret;
}
