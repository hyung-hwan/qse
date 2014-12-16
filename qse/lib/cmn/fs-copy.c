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
#include <qse/cmn/path.h>
#include <qse/cmn/str.h>
#include "mem.h"

/* internal flags. it must not overlap with qse_fs_cpfile_flag_t enumerators */
#define CPFILE_DST_ATTR (1 << 27)
#define CPFILE_DST_PATH_DUP (1 << 28)
#define CPFILE_DST_FSPATH_DUP (1 << 29)
#define CPFILE_DST_FSPATH_MERGED (1 << 30)

struct cpfile_t
{
	int flags;

	qse_fs_char_t* src_fspath;
	qse_fs_char_t* dst_fspath;

	qse_char_t* src_path;
	qse_char_t* dst_path;

	qse_fs_attr_t src_attr;
	qse_fs_attr_t dst_attr;
};
typedef struct cpfile_t cpfile_t;

static int merge_dstdir_and_file (qse_fs_t* fs, cpfile_t* cpfile)
{
	qse_fs_char_t* fstmp;

	/* if the destination is directory, copy the base name of the source
	 * and append it to the end of the destination, targetting at an entry
	 * in the directory */
	QSE_ASSERT (cpfile->dst_attr.isdir);

	if (cpfile->dst_path)
	{
		qse_char_t* tmp;

		tmp = qse_mergepathdup (cpfile->dst_path, qse_basename (cpfile->src_path), fs->mmgr);
		if (!tmp) 
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		if (cpfile->flags & CPFILE_DST_PATH_DUP) 
			QSE_MMGR_FREE (fs->mmgr, cpfile->dst_path);

		cpfile->dst_path = tmp;
		cpfile->flags |= CPFILE_DST_PATH_DUP;
	}


	fstmp = merge_fspath_dup (cpfile->dst_fspath, get_fspath_base (cpfile->src_fspath), fs->mmgr);
	if (!fstmp)
	{
		fs->errnum = QSE_FS_ENOMEM;
		return -1;
	}

	if (cpfile->flags & CPFILE_DST_FSPATH_DUP) 
		QSE_MMGR_FREE (fs->mmgr, cpfile->dst_fspath);
	cpfile->dst_fspath = fstmp;
	cpfile->flags |= CPFILE_DST_FSPATH_DUP;

	if (qse_fs_getattr (fs, cpfile->dst_fspath, &cpfile->dst_attr) >= 0) 
	{
		cpfile->flags |= CPFILE_DST_ATTR;
	}
	else
	{
		cpfile->flags &= ~CPFILE_DST_ATTR;
	}

	cpfile->flags |= CPFILE_DST_FSPATH_MERGED;


	return 0;
}



/*
#if defined(_WIN32)
DWORD copy_file_progress (
	LARGE_INTEGER TotalFileSize,
	LARGE_INTEGER TotalBytesTransferred,
	LARGE_INTEGER StreamSize,
	LARGE_INTEGER StreamBytesTransferred,
	DWORD dwStreamNumber,
	DWORD dwCallbackReason,
	HANDLE hSourceFile,
	HANDLE hDestinationFile,
	LPVOID lpData)
{
}
#endif
*/

/* copy
 * -> progress
 * -> abort/cancel
 * -> replace/overwrite
 * -> symbolic link
 */

#if defined(_WIN32) && !defined(COPY_FILE_COPY_SYMLINK)
	/* winbase.h defines this only if _WIN32_WINNT >= 0x0600 */
#	define COPY_FILE_COPY_SYMLINK 0x00000800L
#endif

static int copy_file_in_fs (qse_fs_t* fs, cpfile_t* cpfile)
{
#if defined(_WIN32)
	/* ------------------------------------------------------ */
	DWORD copy_flags = 0;

	if (cpfile->flags & QSE_FS_CPFILE_SYMLINK)
		copy_flags |= COPY_FILE_COPY_SYMLINK;
	if (!(cpfile->flags & QSE_FS_CPFILE_REPLACE))
		copy_flags |= COPY_FILE_FAIL_IF_EXISTS;

/*
	if (fs->cbs.cp)
	{
		Specify callback???
	}
*/

	if (CopyFileEx (cpfile->src_fspath, cpfile->dst_fspath,  QSE_NULL, QSE_NULL, QSE_NULL, copy_flags) == FALSE)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
		return -1;
	}

	return 0;
	/* ------------------------------------------------------ */

#elif defined(__OS2__)
	/* ------------------------------------------------------ */

	APIRET rc;
	ULONG opmode = 0;

	if (cpfile->flags & QSE_FS_CPFILE_REPLACE) opmode |= 1; /* set bit 0 */

	rc = DosCopy (cpfile->src_fspath, cpfile->dst_fspath, opmode);
	if (rc != NO_ERROR)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		return -1;
	}

	return 0;

	/* ------------------------------------------------------ */

#elif defined(__DOS__)

	/* ------------------------------------------------------ */
	
/* TOOD: IMPLEMENT THIS */
	fs->errnum = QSE_FS_ENOIMPL;
	return -1;

	/* ------------------------------------------------------ */

#else
	if ((cpfile->flags & QSE_FS_CPFILE_SYMLINK) && cpfile->src_attr.islnk)
	{
		qse_fs_char_t* tmpbuf;

		/* TODO: use a static buffer is size is small enough */
		tmpbuf = QSE_MMGR_ALLOC (fs->mmgr, QSE_SIZEOF(*tmpbuf) * (cpfile->src_attr.size + 1));
		if (tmpbuf == QSE_NULL)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		if (QSE_READLINK (cpfile->src_fspath, tmpbuf, cpfile->src_attr.size) <= -1 ||
		    QSE_SYMLINK (tmpbuf, cpfile->dst_fspath) <= -1)
		{
			QSE_MMGR_FREE (fs->mmgr, tmpbuf);
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			return -1;
		}

		QSE_MMGR_FREE (fs->mmgr, tmpbuf);
		return 0;
	}
	else
	{
		int in = -1, out = -1;
		qse_ssize_t in_len, out_len;
		qse_uint8_t* bp;

		in = QSE_OPEN (cpfile->src_fspath, O_RDONLY, 0);
		if (in <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			goto oops;
		}

		out = QSE_OPEN (cpfile->dst_fspath, O_CREAT | O_WRONLY | O_TRUNC, 0777); /* TODO: proper mode */
		if (out <= -1 && (cpfile->flags & QSE_FS_CPFILE_FORCE))
		{
			/* if forced, delete it and try to open it again */
			QSE_UNLINK (cpfile->dst_fspath);
			out = QSE_OPEN (cpfile->dst_fspath, O_CREAT | O_WRONLY | O_TRUNC, 0777); /* TODO: proper mode */
		}
		if (out <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			goto oops;
		}

		while (1)
		{
			in_len = QSE_READ (in, fs->cpbuf, QSE_SIZEOF(fs->cpbuf));
			if (in_len <= 0) break;

	/* TODO: call progress callback */

			bp = fs->cpbuf;
			while (in_len > 0)
			{
				out_len = QSE_WRITE (out, bp, in_len);
				if (out_len <= -1) goto oops;
				bp += out_len;
				in_len -= out_len;
			}
		}

		if (cpfile->flags & QSE_FS_CPFILE_PRESERVE)
		{
		#if defined(HAVE_FUTIMENS)
			struct timespec ts[2];
		#elif defined(HAVE_FUTIMES)
			struct timeval tv[2];
		#endif

			if (QSE_FCHOWN (out, cpfile->src_attr.uid, cpfile->src_attr.gid) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				goto oops;
			}

		#if defined(HAVE_FUTIMENS)
			ts[0].tv_sec = cpfile->src_attr.atime.sec;
			ts[0].tv_nsec = cpfile->src_attr.atime.nsec;
			ts[1].tv_sec = cpfile->src_attr.mtime.sec;
			ts[1].tv_nsec = cpfile->src_attr.mtime.nsec;
			if (QSE_FUTIMENS (out, ts) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				goto oops;
			}
		#elif defined(HAVE_FUTIME)
			tv[0].tv_sec = cpfile->src_attr.atime.sec;
			tv[0].tv_usec = QSE_NSEC_TO_USEC(cpfile->src_attr.atime.nsec);
			tv[1].tv_sec = cpfile->src_attr.mtime.sec;
			tv[1].tv_usec = QSE_NSEC_TO_USEC(cpfile->src_attr.mtime.nsec);
			if (QSE_FUTIMES (out, tv) <= -1)
			{
				fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
				goto oops;
			}
		#else
		#	error neither futimens nor futimes exist
		#endif
		}

		QSE_CLOSE (out);
		QSE_CLOSE (in);

		return 0;

	oops:
		if (out >= 0) QSE_CLOSE (out);
		if (in >= 0) QSE_CLOSE (in);
		return -1;
	}
#endif
}

static int copy_file (qse_fs_t* fs, cpfile_t* cpfile)
{
	if (cpfile->src_attr.isdir)
	{
		/* source is a directory */

		qse_dir_t* dir;
		qse_dir_errnum_t direrr;
		qse_dir_ent_t dirent;
		qse_char_t* src_path, * dst_path;
		int x;

	copy_dir:
		if (cpfile->flags  & CPFILE_DST_ATTR)
		{
			if (!cpfile->dst_attr.isdir)
			{
				/* cannot copy a directory to a file */
				fs->errnum = QSE_FS_ENOTDIR;
				return -1;
			}

			if (!(cpfile->flags & QSE_FS_CPFILE_NOTGTDIR) && 
			    cpfile->dst_attr.isdir)
			{
				if (cpfile->flags & CPFILE_DST_FSPATH_MERGED)
				{
					/* merge_dstdir_and_file() has been called already.
					 * no more getting into a subdirectory */
					fs->errnum = QSE_FS_EISDIR;
					return -1;
				}
				else
				{
					/* arrange to copy a file into a directory */
					if (merge_dstdir_and_file (fs, cpfile) <= -1) return -1;
					goto copy_dir;
				}
			}

			/*
			if (!(cpfile->flags & QSE_FS_CPFILE_REPLACE))
			{
				fs->errnum = QSE_FS_EEXIST;
				return -1;
			}*/
		}

		if (!(cpfile->flags & QSE_FS_CPFILE_RECURSIVE))
		{
			/* cann not copy a directory without recursion */
			fs->errnum = QSE_FS_EISDIR;
			return -1;
		}

		dir = qse_dir_open (fs->mmgr, 0, cpfile->src_path, QSE_DIR_SKIPSPCDIR, &direrr);
		if (!dir)
		{
			fs->errnum = qse_fs_direrrtoerrnum (fs, direrr);
			return -1;
		}

		if (qse_fs_sysmkdir (fs, cpfile->dst_fspath) <= -1)
		{
			qse_dir_close (dir);
			return -1;
		}

		while (1)
		{
			x = qse_dir_read (dir, &dirent);
			if (x <= -1) 
			{
				fs->errnum = qse_fs_direrrtoerrnum (fs, qse_dir_geterrnum(dir));
				qse_dir_close (dir);
				return -1;
			}
			if (x == 0) break; /* no more entries */


			src_path = qse_mergepathdup (cpfile->src_path, dirent.name, fs->mmgr);
			dst_path = qse_mergepathdup (cpfile->dst_path, dirent.name, fs->mmgr);
			if (!src_path  || !dst_path)
			{
				if (dst_path) QSE_MMGR_FREE (fs->mmgr, dst_path);
				if (src_path) QSE_MMGR_FREE (fs->mmgr, src_path);
				qse_dir_close (dir);
				return -1;
			}

			x = qse_fs_cpfile (fs, src_path, dst_path, (cpfile->flags & QSE_FS_CPFILE_ALL));

			QSE_MMGR_FREE (fs->mmgr, dst_path);
			QSE_MMGR_FREE (fs->mmgr, src_path);
			if (x <= -1)
			{
				qse_dir_close (dir);
				return -1;
			}
		}

		qse_dir_close (dir);
		return 0;
	}
	else
	{
		/* source is a file */

	copy_file:
		if (cpfile->flags & CPFILE_DST_ATTR) 
		{
			if (cpfile->src_attr.ino == cpfile->dst_attr.ino && 
			    cpfile->src_attr.dev == cpfile->dst_attr.dev)
			{
				/* cannot copy a file to itself */
				fs->errnum = QSE_FS_EINVAL; /* TODO: better error code */
				return -1;
			}

			if (!(cpfile->flags & QSE_FS_CPFILE_NOTGTDIR) && 
			    cpfile->dst_attr.isdir)
			{
				if (cpfile->flags & CPFILE_DST_FSPATH_MERGED)
				{
					/* merge_dstdir_and_file() has been called already.
					 * no more getting into a subdirectory */
					fs->errnum = QSE_FS_EISDIR;
					return -1;
				}
				else
				{
					/* arrange to copy a file into a directory */
					if (merge_dstdir_and_file (fs, cpfile) <= -1) return -1;
					goto copy_file;
				}
			}

			if (!(cpfile->flags & QSE_FS_CPFILE_REPLACE))
			{
				fs->errnum = QSE_FS_EEXIST;
				return -1;
			}
		}

		/* both source and target are files */
		return copy_file_in_fs (fs, cpfile);
	}
}

int qse_fs_cpfilembs (qse_fs_t* fs, const qse_mchar_t* srcpath, const qse_mchar_t* dstpath, int flags)
{
	cpfile_t cpfile;
	int ret;

	QSE_MEMSET (&cpfile, 0, QSE_SIZEOF(cpfile));

	cpfile.flags = flags & QSE_FS_CPFILE_ALL; /* keep public flags only */

	cpfile.src_fspath = (qse_fs_char_t*)qse_fs_makefspathformbs (fs, srcpath);
	cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_makefspathformbs (fs, dstpath);
	cpfile.src_path = (qse_char_t*)make_str_with_mbs (fs, srcpath);
	cpfile.dst_path = (qse_char_t*)make_str_with_mbs (fs, dstpath);
	if (!cpfile.src_fspath || !cpfile.dst_fspath || !cpfile.src_path || !cpfile.dst_path) goto oops;
	
	if (cpfile.dst_fspath != dstpath) 
	{
		/* mark that it's been duplicated. */
		cpfile.flags |= CPFILE_DST_FSPATH_DUP;
	}

	if (qse_fs_getattr (fs, cpfile.src_fspath, &cpfile.src_attr) <= -1) goto oops;
	if (qse_fs_getattr (fs, cpfile.dst_fspath, &cpfile.dst_attr) >= 0) cpfile.flags |= CPFILE_DST_ATTR;

	ret = copy_file (fs, &cpfile);

	free_str_with_mbs (fs, dstpath, cpfile.dst_path);
	free_str_with_mbs (fs, srcpath, cpfile.src_path);
	qse_fs_freefspathformbs (fs, dstpath, cpfile.dst_fspath);
	qse_fs_freefspathformbs (fs, srcpath, cpfile.src_fspath);
	return ret;

oops:
	if (cpfile.dst_path) free_str_with_mbs (fs, dstpath, cpfile.dst_path);
	if (cpfile.src_path) free_str_with_mbs (fs, srcpath, cpfile.src_path);
	if (cpfile.dst_fspath) qse_fs_freefspathformbs (fs, srcpath, cpfile.dst_fspath);
	if (cpfile.src_fspath) qse_fs_freefspathformbs (fs, dstpath, cpfile.src_fspath);
	return -1;
}

int qse_fs_cpfilewcs (qse_fs_t* fs, const qse_wchar_t* srcpath, const qse_wchar_t* dstpath, int flags)
{
	cpfile_t cpfile;
	int ret;

	QSE_MEMSET (&cpfile, 0, QSE_SIZEOF(cpfile));

	cpfile.flags = flags & QSE_FS_CPFILE_ALL; /* keep public flags only */

	cpfile.src_fspath = (qse_fs_char_t*)qse_fs_makefspathforwcs (fs, srcpath);
	cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_makefspathforwcs (fs, dstpath);
	cpfile.src_path = (qse_char_t*)make_str_with_wcs (fs, srcpath);
	cpfile.dst_path = (qse_char_t*)make_str_with_wcs (fs, dstpath);
	if (!cpfile.src_fspath || !cpfile.dst_fspath || !cpfile.src_path || !cpfile.dst_path) goto oops;

	if (cpfile.dst_fspath != dstpath) 
	{
		/* mark that it's been duplicated */
		cpfile.flags |= CPFILE_DST_FSPATH_DUP; 
	}

	if (qse_fs_getattr (fs, cpfile.src_fspath, &cpfile.src_attr) <= -1) goto oops;
	if (qse_fs_getattr (fs, cpfile.dst_fspath, &cpfile.dst_attr) >= 0) cpfile.flags |= CPFILE_DST_ATTR;

	ret = copy_file (fs, &cpfile);

	free_str_with_wcs (fs, dstpath, cpfile.dst_path);
	free_str_with_wcs (fs, srcpath, cpfile.src_path);
	qse_fs_freefspathforwcs (fs, dstpath, cpfile.dst_fspath);
	qse_fs_freefspathforwcs (fs, srcpath, cpfile.src_fspath);
	return ret;

oops:
	if (cpfile.dst_path) free_str_with_wcs (fs, dstpath, cpfile.dst_path); 
	if (cpfile.src_path) free_str_with_wcs (fs, srcpath, cpfile.src_path);
	if (cpfile.dst_fspath) qse_fs_freefspathforwcs (fs, srcpath, cpfile.dst_fspath);
	if (cpfile.src_fspath) qse_fs_freefspathforwcs (fs, dstpath, cpfile.src_fspath);
	return -1;
}


