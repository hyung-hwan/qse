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
#include <qse/cmn/path.h>
#include <qse/cmn/str.h>
#include "../cmn/mem-prv.h"

#define NO_RECURSION 1

/* internal flags. it must not overlap with qse_fs_cpfile_flag_t enumerators */
#define CPFILE_DST_ATTR (1 << 29)
#define CPFILE_DST_FSPATH_MERGED (1 << 30)

struct devino_t
{
	qse_uintmax_t dev;
	qse_uintmax_t ino;
};
typedef struct devino_t devino_t;

struct cpfile_t
{
	int flags;
	qse_fs_char_t* src_fspath;
	qse_fs_char_t* dst_fspath;
	qse_fattr_t src_attr;
	qse_fattr_t dst_attr;

};
typedef struct cpfile_t cpfile_t;

static int is_dir_remembered (qse_fs_t* fs, qse_fattr_t* attr)
{
	qse_size_t i;
	devino_t* di;

	di = (devino_t*)fs->ddr.ptr;
	for (i = 0; i < fs->ddr.len; i++)
	{
		if (attr->dev == di->dev && attr->ino == di->ino) return 1;
	}

	return 0;
}

static int remember_dir_to_history (qse_fs_t* fs, qse_fattr_t* attr)
{
	qse_size_t i;
	devino_t* di;

	di = (devino_t*)fs->ddr.ptr;
	for (i = 0; i < fs->ddr.len; i++)
	{
		if (attr->dev == di->dev  && attr->ino == di->ino) 
		{
			fs->errnum = QSE_FS_EEXIST;
			return -1;
		}
	}

/* TODO: table baded lookup instead of sequential search? */
	if (fs->ddr.len >= fs->ddr.capa)
	{
		qse_size_t newcapa;

		newcapa = QSE_ALIGNTO(fs->ddr.capa + 1, 256);
		di = (devino_t*)QSE_MMGR_REALLOC (fs->mmgr, di, newcapa * QSE_SIZEOF(*di));

		if (!di) 
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}

		fs->ddr.ptr = di;
		fs->ddr.capa = newcapa;
	}

	di[fs->ddr.len].dev = attr->dev;
	di[fs->ddr.len].ino = attr->ino;
	fs->ddr.len++;

	return 0;
}

static int merge_dstdir_and_srcbase (qse_fs_t* fs, cpfile_t* cpfile)
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

	if (qse_fs_getattrsys (fs, cpfile->dst_fspath, &cpfile->dst_attr, QSE_FILE_ATTR_SYMLINK) <= -1) 
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
		/* the source file is a symbolic link and the SYMLINK option is on.*/

		qse_fs_char_t* tmpbuf;

		/* TODO: use a static buffer is size is small enough */
		tmpbuf = QSE_MMGR_ALLOC (fs->mmgr, QSE_SIZEOF(*tmpbuf) * (cpfile->src_attr.size + 1));
		if (tmpbuf == QSE_NULL)
		{
			fs->errnum = QSE_FS_ENOMEM;
			return -1;
		}
		QSE_MEMSET (tmpbuf, 0, QSE_SIZEOF(*tmpbuf) * (cpfile->src_attr.size + 1));

		if (QSE_READLINK (cpfile->src_fspath, tmpbuf, cpfile->src_attr.size) <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			QSE_MMGR_FREE (fs->mmgr, tmpbuf);
			return -1;
		}

		if (QSE_SYMLINK (tmpbuf, cpfile->dst_fspath) <= -1)
		{
			if (cpfile->flags & QSE_FS_CPFILE_FORCE)
			{
				QSE_UNLINK (cpfile->dst_fspath);
				if (fs->cbs.actcb && qse_fs_invokeactcb (fs, QSE_FS_RMFILE, cpfile->dst_fspath, QSE_NULL, 0, 0) <= -1) goto oops_1;

				if (QSE_SYMLINK (tmpbuf, cpfile->dst_fspath) <= -1)
				{
					fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
					goto oops_1;
				}
			}
		}
		if (fs->cbs.actcb && qse_fs_invokeactcb (fs, QSE_FS_SYMLINK, tmpbuf, cpfile->dst_fspath, 0, 0) <= -1) goto oops_1;

		QSE_MMGR_FREE (fs->mmgr, tmpbuf);
		return 0;

	oops_1:
		if (tmpbuf) QSE_MMGR_FREE (fs->mmgr, tmpbuf);
		return -1;
	}
	else
	{
		int in = -1, out = -1;
		qse_ssize_t in_len, out_len;
		qse_uint8_t* bp;
		qse_uintmax_t bytes_copied = 0;
		qse_char_t* srcpath = QSE_NULL, * dstpath = QSE_NULL;

		in = QSE_OPEN (cpfile->src_fspath, O_RDONLY, 0);
		if (in <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			goto oops_2;
		}

		out = QSE_OPEN (cpfile->dst_fspath, O_CREAT | O_WRONLY | O_TRUNC, cpfile->src_attr.mode);
		if (out <= -1 && (cpfile->flags & QSE_FS_CPFILE_FORCE))
		{
			/* if forced, delete it and try to open it again */
/* TODO: call cbs.rm callback??? */
			QSE_UNLINK (cpfile->dst_fspath);
			out = QSE_OPEN (cpfile->dst_fspath, O_CREAT | O_WRONLY | O_TRUNC, cpfile->src_attr.mode); 
		}

		if (out <= -1)
		{
			fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
			goto oops_2;
		}

		if (fs->cbs.actcb)
		{
			srcpath = (qse_char_t*)make_str_with_fspath (fs, cpfile->src_fspath);
			dstpath = (qse_char_t*)make_str_with_fspath (fs, cpfile->dst_fspath);
		}

		while (1)
		{
/* TODO: use vmsplice() and family for faster copying... */
			in_len = QSE_READ (in, fs->cpbuf, QSE_SIZEOF(fs->cpbuf));
			if (in_len <= 0) 
			{
				if (bytes_copied == 0 && srcpath && dstpath)
				{
					if (fs->cbs.actcb (fs, QSE_FS_CPFILE, srcpath, dstpath, cpfile->src_attr.size, bytes_copied) <= -1) goto oops_2;
				}
				break;
			}

			bp = fs->cpbuf;
			while (in_len > 0)
			{
				out_len = QSE_WRITE (out, bp, in_len);
				if (out_len <= -1) goto oops_2;
				bp += out_len;
				in_len -= out_len;
				bytes_copied += out_len;
			}

			if (srcpath && dstpath)
			{
				if (fs->cbs.actcb (fs, QSE_FS_CPFILE, srcpath, dstpath, cpfile->src_attr.size, bytes_copied) <= -1) goto oops_2;
			}
		}

		if (cpfile->flags & QSE_FS_CPFILE_PRESERVE)
		{
			if (qse_fs_setattronfd (fs, out, &cpfile->src_attr, QSE_FILE_ATTR_TIME | QSE_FILE_ATTR_OWNER | QSE_FILE_ATTR_MODE) <= -1)
			{
				if (fs->errnum == QSE_FS_ENOIMPL)
				{
					if (qse_fs_setattrsys (fs, cpfile->dst_fspath, &cpfile->src_attr, QSE_FILE_ATTR_TIME | QSE_FILE_ATTR_OWNER | QSE_FILE_ATTR_MODE) <= -1) goto oops_2;
				}
			}
		}

		QSE_CLOSE (out);
		QSE_CLOSE (in);

		if (srcpath) free_str_with_fspath (fs, cpfile->src_fspath, srcpath);
		if (dstpath) free_str_with_fspath (fs, cpfile->dst_fspath, dstpath);
		return 0;

	oops_2:
		if (out >= 0) QSE_CLOSE (out);
		if (in >= 0) QSE_CLOSE (in);

		if (srcpath) free_str_with_fspath (fs, cpfile->src_fspath, srcpath);
		if (dstpath) free_str_with_fspath (fs, cpfile->dst_fspath, dstpath);
		return -1;
	}
#endif
}

static int prepare_cpfile (qse_fs_t* fs, cpfile_t* cpfile)
{
	/* error if the source file can't be stat'ed.
	 * ok if the destination file can't be stat'ed */
/* TODO: check if flags to getattrsys() is correct */
	if (qse_fs_getattrsys (fs, cpfile->src_fspath, &cpfile->src_attr, QSE_FILE_ATTR_SYMLINK) <= -1) return -1;
	if (qse_fs_getattrsys (fs, cpfile->dst_fspath, &cpfile->dst_attr, QSE_FILE_ATTR_SYMLINK) >= 0) cpfile->flags |= CPFILE_DST_ATTR;
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
	qse_dir_t* dir = QSE_NULL;
	qse_dir_errnum_t direrr;
	qse_dir_ent_t dirent;
	int x;

start_over:
	if (cpfile->src_attr.isdir)
	{
		/* source is a directory */
		if (cpfile->flags & CPFILE_DST_ATTR)
		{
			if (!cpfile->dst_attr.isdir)
			{
				/* cannot copy a directory to a file */
				fs->errnum = QSE_FS_ENOTDIR;
				goto oops;
			}

			/* the destination is also a directory */

			if (cpfile->src_attr.ino == cpfile->dst_attr.ino &&
			    cpfile->src_attr.dev == cpfile->dst_attr.dev)
			{
				/* cannot copy a directory to itself */
				fs->errnum = QSE_FS_EPERM;
				goto oops;
			}

			if (merge_dstdir_and_srcbase (fs, cpfile) <= -1) goto oops;

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


		if (is_dir_remembered (fs, &cpfile->src_attr))
		{
			/* skip this source directory as it's one of the destination 
			 * directores earlier. see the call to remember_dir_to_history()
			 * below. */
			goto skip_dir;
		}

		/* both the source and the destion is a directory */
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

		if (qse_fs_mkdirsys(fs, cpfile->dst_fspath, 0777) <= -1)  /* TODO: copy mode? */
		{
			/* it's ok if the destination directory already exists */
			if (fs->errnum != QSE_FS_EEXIST) goto oops;
		}

		/* (re)load the attribute. */
		if (qse_fs_getattrsys (fs, cpfile->dst_fspath, &cpfile->dst_attr, QSE_FILE_ATTR_SYMLINK) <= -1) goto oops;
		cpfile->flags |= CPFILE_DST_ATTR;

		if (remember_dir_to_history (fs, &cpfile->dst_attr) <= -1) 
		{
			/* if the directory has been copied once, fs->errnum should be QSE_FS_EEXIST */
			if (fs->errnum == QSE_FS_EEXIST) goto close_dir;
			goto oops;
		}

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
				goto oops;
			}
			x = copy_file (fs, &sub_cpfile); /* TODO: remove recursion */

			clear_cpfile (fs, &sub_cpfile);
			if (x <= -1) goto oops;
		#endif
		}

	close_dir:
		qse_dir_close (dir);
		dir = QSE_NULL;

	skip_dir:
		/* do nothing more */ ;
	}
	else
	{
		/* source is a file */

	copy_file:
		if (cpfile->flags & CPFILE_DST_ATTR) 
		{
			/* attribute of the destination path is available */

			if (cpfile->src_attr.ino == cpfile->dst_attr.ino && 
			    cpfile->src_attr.dev == cpfile->dst_attr.dev)
			{
				/* cannot copy a file to itself */
				fs->errnum = QSE_FS_EPERM;
				goto oops;
			}

			if (cpfile->dst_attr.isdir)
			{
				/* arrange to copy a file into a directory */
				if (merge_dstdir_and_srcbase (fs, cpfile) <= -1) return -1;
				goto copy_file;
			}

			if (!(cpfile->flags & QSE_FS_CPFILE_REPLACE))
			{
				fs->errnum = QSE_FS_EEXIST;
				goto oops;
			}
		}

		/* both source and target are files */
		if (copy_file_in_fs (fs, cpfile) <= -1) goto oops;
	}

skip:
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

/* ========================================================================= */

struct glob_ctx_t
{
	qse_fs_t* fs;
	int flags;
	void* dstpath;
	int wcs;
};
typedef struct glob_ctx_t glob_ctx_t;

static int copy_file_for_glob (const void* path, void* ctx)
{
	glob_ctx_t* gc = (glob_ctx_t*)ctx;
	cpfile_t cpfile;
	int ret;

	QSE_MEMSET (&cpfile, 0, QSE_SIZEOF(cpfile));

	cpfile.flags = gc->flags & QSE_FS_CPFILE_ALL; /* keep public flags only */

	if (gc->wcs)
	{
		cpfile.src_fspath = (qse_fs_char_t*)qse_fs_dupfspathforwcs (gc->fs, ((qse_wcstr_t*)path)->ptr);
		cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_dupfspathforwcs (gc->fs, gc->dstpath);
	}
	else
	{
		cpfile.src_fspath = (qse_fs_char_t*)qse_fs_dupfspathformbs (gc->fs, ((qse_mcstr_t*)path)->ptr);
		cpfile.dst_fspath = (qse_fs_char_t*)qse_fs_dupfspathformbs (gc->fs, gc->dstpath);
	}
	if (!cpfile.src_fspath || !cpfile.dst_fspath || prepare_cpfile (gc->fs, &cpfile) <= -1) goto oops;

	ret = copy_file (gc->fs, &cpfile);

	clear_cpfile (gc->fs, &cpfile);

	return ret;

oops:
	clear_cpfile (gc->fs, &cpfile);
	return -1;
}

int qse_fs_cpfilembs (qse_fs_t* fs, const qse_mchar_t* srcpath, const qse_mchar_t* dstpath, int flags)
{
	glob_ctx_t ctx;
	int ret = 0;

	ctx.fs = fs;
	ctx.flags = flags;
	ctx.dstpath = (void*)dstpath;
	ctx.wcs = 0;

	if (flags & QSE_FS_CPFILE_GLOB)
	{
		fs->errnum = QSE_FS_ENOERR;
		if (qse_globmbs(srcpath, copy_file_for_glob, &ctx, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr) <= -1)
		{
			if (fs->errnum == QSE_FS_ENOERR) fs->errnum = QSE_FS_EGLOB;
			ret = -1;
		}
	}
	else
	{
		qse_mcstr_t s;
		s.ptr = (qse_mchar_t*)srcpath;
		s.len = 0; /* not important */
		/* let me use the glob callback function for simplicity */
		ret = copy_file_for_glob (&s, &ctx);
	}

	if (fs->ddr.ptr)
	{
		QSE_MMGR_FREE (fs->mmgr, fs->ddr.ptr);
		fs->ddr.ptr = QSE_NULL;
		fs->ddr.len = 0;
		fs->ddr.capa = 0;
	}
	return ret;
}

int qse_fs_cpfilewcs (qse_fs_t* fs, const qse_wchar_t* srcpath, const qse_wchar_t* dstpath, int flags)
{
	glob_ctx_t ctx;
	int ret = 0;

	ctx.fs = fs;
	ctx.flags = flags;
	ctx.dstpath = (void*)dstpath;
	ctx.wcs = 1;

	if (flags & QSE_FS_CPFILE_GLOB)
	{
		fs->errnum = QSE_FS_ENOERR;
		if (qse_globwcs(srcpath, copy_file_for_glob, &ctx, DEFAULT_GLOB_FLAGS, fs->mmgr, fs->cmgr) <= -1)
		{
			if (fs->errnum == QSE_FS_ENOERR) fs->errnum = QSE_FS_EGLOB;
			ret = -1;
		} 
	}
	else
	{
		/* let me use the glob callback function for simplicity */
		qse_wcstr_t s;
		s.ptr = (qse_wchar_t*)srcpath;
		s.len = 0; /* not important */
		ret = copy_file_for_glob (&s, &ctx);
	}

	if (fs->ddr.ptr)
	{
		QSE_MMGR_FREE (fs->mmgr, fs->ddr.ptr);
		fs->ddr.ptr = QSE_NULL;
		fs->ddr.len = 0;
		fs->ddr.capa = 0;
	}

	return ret;
}
