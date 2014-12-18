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

#define NO_RECURSION 1

/* internal flags. it must not overlap with qse_fs_cpfile_flag_t enumerators */
#define CPFILE_DST_ATTR (1 << 29)
#define CPFILE_DST_FSPATH_MERGED (1 << 30)

struct cpfile_t
{
	int flags;

	qse_fs_char_t* src_fspath;
	qse_fs_char_t* dst_fspath;

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

	fstmp = merge_fspath_dup (cpfile->dst_fspath, get_fspath_base (cpfile->src_fspath), fs->mmgr);
	if (!fstmp)
	{
		fs->errnum = QSE_FS_ENOMEM;
		return -1;
	}

	qse_fs_freefspath (fs, QSE_NULL, cpfile->dst_fspath);
	cpfile->dst_fspath = fstmp;

	if (qse_fs_sysgetattr (fs, cpfile->dst_fspath, &cpfile->dst_attr) <= -1) 
	{
		/* attribute on the new destination is not available */
		cpfile->flags &= ~CPFILE_DST_ATTR;
	}
	else
	{
		/* the attribute has been updated to reflect the new destination */
		cpfile->flags |= CPFILE_DST_ATTR;
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

		out = QSE_OPEN (cpfile->dst_fspath, O_CREAT | O_WRONLY | O_TRUNC, cpfile->src_attr.mode);
		if (out <= -1 && (cpfile->flags & QSE_FS_CPFILE_FORCE))
		{
			/* if forced, delete it and try to open it again */
			QSE_UNLINK (cpfile->dst_fspath);
			out = QSE_OPEN (cpfile->dst_fspath, O_CREAT | O_WRONLY | O_TRUNC, cpfile->src_attr.mode); 
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

			if (QSE_FCHOWN (out, cpfile->src_attr.uid, cpfile->src_attr.gid) <= -1 ||
			    QSE_FCHMOD (out,  cpfile->src_attr.mode) <= -1)
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

static int prepare_cpfile (qse_fs_t* fs, cpfile_t* cpfile)
{
	/* error if the source file can't be stat'ed.
	 * ok if the destination file can't be stat'ed */
	if (qse_fs_sysgetattr (fs, cpfile->src_fspath, &cpfile->src_attr) <= -1) return -1;
	if (qse_fs_sysgetattr (fs, cpfile->dst_fspath, &cpfile->dst_attr) >= 0) cpfile->flags |= CPFILE_DST_ATTR;
	return 0;
}

static void clear_cpfile (qse_fs_t* fs, cpfile_t* cpfile)
{
	if (cpfile->src_fspath) 
	{
		QSE_MMGR_FREE (fs->mmgr, cpfile->src_fspath);
		cpfile->src_fspath = QSE_NULL;
	}
	if (cpfile->dst_fspath) 
	{
		QSE_MMGR_FREE (fs->mmgr, cpfile->dst_fspath);
		cpfile->dst_fspath = QSE_NULL;
	}
}

/* copy file stack  - stack for file copying */
typedef struct cfs_t cfs_t;
struct cfs_t
{
	cpfile_t cpfile;
	qse_dir_t* dir;
	cfs_t* next;
};

static cfs_t* push_cfs (qse_fs_t* fs, const cpfile_t* cpfile, qse_dir_t* dir)
{
	cfs_t* cfs;

	cfs = (cfs_t*)QSE_MMGR_ALLOC (fs->mmgr, QSE_SIZEOF(*cfs));
	if (cfs == QSE_NULL)
	{
		fs->errnum = QSE_FS_ENOMEM;
		return QSE_NULL;
	}

	cfs->cpfile = *cpfile;
	cfs->dir = dir;
	cfs->next = fs->cfs;

	fs->cfs = cfs;
	return cfs;
}

static void pop_cfs (qse_fs_t* fs, cpfile_t* cpfile, qse_dir_t** dir)
{
	cfs_t* cfs;

	cfs = fs->cfs;
	QSE_ASSERT (cfs != QSE_NULL);
	fs->cfs = cfs->next;

	if (cpfile) *cpfile = cfs->cpfile;
	else clear_cpfile (fs, &cfs->cpfile);

	if (dir) *dir = cfs->dir;
	else qse_dir_close (cfs->dir);

	QSE_MMGR_FREE (fs->mmgr, cfs);
}


static int copy_file (qse_fs_t* fs, cpfile_t* cpfile)
{
#if defined(NO_RECURSION)
	cfs_t* cfs;
#else
	cpfile_t sub_cpfile;
#endif
	qse_dir_t* dir;
	qse_dir_errnum_t direrr;
	qse_dir_ent_t dirent;
	int x;

start_over:
	if (cpfile->src_attr.isdir)
	{
		/* source is a directory */
	copy_dir:
		if (cpfile->flags & CPFILE_DST_ATTR)
		{
			if (!cpfile->dst_attr.isdir)
			{
				/* cannot copy a directory to a file */
				fs->errnum = QSE_FS_ENOTDIR;
				goto oops;
			}

			if (!(cpfile->flags & QSE_FS_CPFILE_NOTGTDIR) && 
			    cpfile->dst_attr.isdir)
			{
				if (cpfile->flags & CPFILE_DST_FSPATH_MERGED)
				{
					/* merge_dstdir_and_file() has been called already.
					 * no more getting into a subdirectory */
					fs->errnum = QSE_FS_EISDIR;
					goto oops;
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
			/* cannot copy a directory without recursion */
			fs->errnum = QSE_FS_EISDIR;
			goto oops;
		}

		dir = qse_dir_open (
			fs->mmgr, 0, (const qse_char_t*)cpfile->src_fspath,
		#if defined(QSE_FS_CHAR_IS_MCHAR)
			QSE_DIR_SKIPSPCDIR | QSE_DIR_MBSPATH,
		#else
			QSE_DIR_SKIPSPCDIR | QSE_DIR_WCSPATH,
		#endif
			&direrr
		);
		if (!dir)
		{
			fs->errnum = qse_fs_direrrtoerrnum (fs, direrr);
			goto oops;
		}

		if (qse_fs_sysmkdir (fs, cpfile->dst_fspath) <= -1) goto oops;

		while (1)
		{
			x = qse_dir_read (dir, &dirent);
			if (x <= -1) 
			{
				fs->errnum = qse_fs_direrrtoerrnum (fs, qse_dir_geterrnum(dir));
				goto oops;
			}
			if (x == 0) break; /* no more entries */

		#if defined(NO_RECURSION)
			cfs = push_cfs (fs, cpfile, dir);
			if (!cfs) goto oops;

			QSE_MEMSET (cpfile, 0, QSE_SIZEOF(*cpfile));
			dir = QSE_NULL;

			cpfile->flags = cfs->cpfile.flags & QSE_FS_CPFILE_ALL; /* inherit public flags */
			cpfile->src_fspath = merge_fspath_dup (cfs->cpfile.src_fspath, (qse_fs_char_t*)dirent.name, fs->mmgr);
			cpfile->dst_fspath = merge_fspath_dup (cfs->cpfile.dst_fspath, (qse_fs_char_t*)dirent.name, fs->mmgr);
			if (!cpfile->src_fspath  || !cpfile->dst_fspath || prepare_cpfile (fs, cpfile) <= -1)
			{
				clear_cpfile (fs, cpfile);
				goto oops;
			}

			goto start_over;

		resume_copy_dir:
			/* do nothing. loop over */
			;

		#else
			QSE_MEMSET (&sub_cpfile, 0, QSE_SIZEOF(sub_cpfile));
			sub_cpfile.flags = cpfile->flags & QSE_FS_CPFILE_ALL; /* inherit public flags */
			sub_cpfile.src_fspath = merge_fspath_dup (cpfile->src_fspath, (qse_fs_char_t*)dirent.name, fs->mmgr);
			sub_cpfile.dst_fspath = merge_fspath_dup (cpfile->dst_fspath, (qse_fs_char_t*)dirent.name, fs->mmgr);
			if (!sub_cpfile.src_fspath  || !sub_cpfile.dst_fspath || prepare_cpfile (fs, &sub_cpfile) <= -1)
			{
				clear_cpfile (fs, &sub_cpfile);
				qse_dir_close (dir);
				return -1;
			}
			x = copy_file (fs, &sub_cpfile); /* TODO: remove recursion */

			clear_cpfile (fs, &sub_cpfile);
			if (x <= -1)
			{
				qse_dir_close (dir);
				return -1;
			}
		#endif
		}

		qse_dir_close (dir);
		dir = QSE_NULL;
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
				goto oops;
			}

			if (!(cpfile->flags & QSE_FS_CPFILE_NOTGTDIR) && 
			    cpfile->dst_attr.isdir)
			{
				if (cpfile->flags & CPFILE_DST_FSPATH_MERGED)
				{
					/* merge_dstdir_and_file() has been called already.
					 * no more getting into a subdirectory */
					fs->errnum = QSE_FS_EISDIR;
					goto oops;
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
		if (copy_file_in_fs (fs, cpfile) <= -1) goto oops;
	}

#if defined(NO_RECURSION)
	if (fs->cfs)
	{ 
		clear_cpfile (fs, cpfile); /* clear key data created for subitems in the directory */
		pop_cfs (fs, cpfile, &dir); /* restore key data for the original directory*/
		goto resume_copy_dir;
	}
#endif

	return 0;


oops:
#if defined(NO_RECURSION)
	while (fs->cfs) pop_cfs (fs, QSE_NULL, QSE_NULL);
#endif
	if (dir) qse_dir_close (dir);
	return -1;
}

int qse_fs_cpfilembs (qse_fs_t* fs, const qse_mchar_t* srcpath, const qse_mchar_t* dstpath, int flags)
{
	cpfile_t cpfile;
	int ret;

	QSE_MEMSET (&cpfile, 0, QSE_SIZEOF(cpfile));

	cpfile.flags = flags & QSE_FS_CPFILE_ALL; /* keep public flags only */

	cpfile.src_fspath = (qse_fs_char_t*)qse_fs_dupfspathformbs (fs, srcpath);
	cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_dupfspathformbs (fs, dstpath);
	if (!cpfile.src_fspath || !cpfile.dst_fspath || prepare_cpfile (fs, &cpfile) <= -1) goto oops;

	ret = copy_file (fs, &cpfile);

	clear_cpfile (fs, &cpfile);
	return ret;

oops:
	clear_cpfile (fs, &cpfile);
	return -1;
}

int qse_fs_cpfilewcs (qse_fs_t* fs, const qse_wchar_t* srcpath, const qse_wchar_t* dstpath, int flags)
{
	cpfile_t cpfile;
	int ret;

	QSE_MEMSET (&cpfile, 0, QSE_SIZEOF(cpfile));

	cpfile.flags = flags & QSE_FS_CPFILE_ALL; /* keep public flags only */

	cpfile.src_fspath = (qse_fs_char_t*)qse_fs_dupfspathforwcs (fs, srcpath);
	cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_dupfspathforwcs (fs, dstpath);
	if (!cpfile.src_fspath || !cpfile.dst_fspath || prepare_cpfile (fs, &cpfile) <= -1) goto oops;

	ret = copy_file (fs, &cpfile);

	clear_cpfile (fs, &cpfile);
	return ret;

oops:
	clear_cpfile (fs, &cpfile);
	return -1;
}
