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
#include "mem.h"

int qse_fs_sysgetattr (qse_fs_t* fs, const qse_fs_char_t* fspath, qse_fs_attr_t* attr)
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
	#if defined(HAVE_LSTAT)
	qse_lstat_t st;
	#else
	qse_stat_t st;
	#endif

	#if defined(HAVE_LSTAT)
	if (QSE_LSTAT (fspath, &st) == -1) 
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}
	#else
	/* is this ok to use stat? */
	if (QSE_STAT (fspath, &st) == -1) 
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}
	#endif

	QSE_MEMSET (attr, 0, QSE_SIZEOF(*attr));

	if (S_ISDIR(st.st_mode)) attr->isdir = 1;
	if (S_ISLNK(st.st_mode)) attr->islnk = 1;
	if (S_ISREG(st.st_mode)) attr->isreg = 1;
	if (S_ISBLK(st.st_mode)) attr->isblk = 1;
	if (S_ISCHR(st.st_mode)) attr->ischr = 1;

	attr->mode = st.st_mode;
	attr->size = st.st_size;
	attr->ino = st.st_ino;
	attr->dev = st.st_dev;
	attr->uid = st.st_uid;
	attr->gid = st.st_gid;
	
	#if defined(HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
		attr->atime.sec = st.st_atim.tv_sec;
		attr->atime.nsec = st.st_atim.tv_nsec;
		attr->mtime.sec = st.st_mtim.tv_sec;
		attr->mtime.nsec = st.st_mtim.tv_nsec;
		attr->ctime.sec = st.st_ctim.tv_sec;
		attr->ctime.nsec = st.st_ctim.tv_nsec;
	#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
		attr->atime.sec = st.st_atimespec.tv_sec;
		attr->atime.nsec = st.st_atimespec.tv_nsec;
		attr->mtime.sec = st.st_mtimespec.tv_sec;
		attr->mtime.nsec = st.st_mtimespec.tv_nsec;
		attr->ctime.sec = st.st_ctimespec.tv_sec;
		attr->ctime.nsec = st.st_ctimespec.tv_nsec;
	#else
		attr->atime.sec = st.st_atime;
		attr->mtime.sec = st.st_mtime;
		attr->ctime.sec = st.st_ctime;
	#endif
	return 0;
#endif
}

int qse_fs_getattrmbs (qse_fs_t* fs, const qse_mchar_t* path, qse_fs_attr_t* attr)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspathformbs (fs, path);
	if (!fspath) return -1;

	ret = qse_fs_sysgetattr (fs, fspath, attr);

	qse_fs_freefspathformbs (fs, path, fspath);
	return ret;
}

int qse_fs_getattrwcs (qse_fs_t* fs, const qse_wchar_t* path, qse_fs_attr_t* attr)
{
	qse_fs_char_t* fspath;
	int ret;

	fspath = qse_fs_makefspathforwcs (fs, path);
	if (!fspath) return -1;

	ret = qse_fs_sysgetattr (fs, fspath, attr);

	qse_fs_freefspathforwcs (fs, path, fspath);
	return ret;
}
