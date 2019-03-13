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

#ifndef _QSE_SI_FS_H_
#define _QSE_SI_FS_H_

/**  \file
 * This file defines data types and functions for manipulating files and
 * directories on a file system. 
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/time.h>

#if defined(_WIN32) && defined(QSE_CHAR_IS_WCHAR)
	typedef qse_wchar_t qse_fs_char_t;
#	define QSE_FS_CHAR_IS_WCHAR
#	define QSE_SIZEOF_FS_CHAR_T QSE_SIZEOF_WCHAR_T
#else
	typedef qse_mchar_t qse_fs_char_t;
#	define QSE_FS_CHAR_IS_MCHAR
#	define QSE_SIZEOF_FS_CHAR_T QSE_SIZEOF_MCHAR_T
#endif

typedef qse_fmode_t qse_fs_mode_t;

enum qse_fs_errnum_t
{
	QSE_FS_ENOERR = 0,
	QSE_FS_EOTHER,
	QSE_FS_ENOIMPL,     /**< not implemented */
	QSE_FS_ESYSERR,     /**< subsystem error */
	QSE_FS_EINTERN,     /**< internal error */

	QSE_FS_ENOMEM,      /**< out of memory */
	QSE_FS_EINVAL,      /**< invalid parameter */
	QSE_FS_EACCES,      /**< access denied */
	QSE_FS_EPERM,       /**< operation not permitted */
	QSE_FS_ENOENT,      /**< no such file or directory */
	QSE_FS_EEXIST,      /**< already exist */
	QSE_FS_EINTR,       /**< interrupted */
	QSE_FS_EPIPE,       /**< broken pipe */
	QSE_FS_EAGAIN,      /**< resource temporarily unavailable */

	QSE_FS_EISDIR,      /**< is a directory */
	QSE_FS_ENOTDIR,     /**< not a directory */
	QSE_FS_ENOTVOID,    /**< directory not empty */
	QSE_FS_EXDEV,       /**< cross device */
	QSE_FS_EGLOB        /**< glob error */
};
typedef enum qse_fs_errnum_t qse_fs_errnum_t;

enum qse_fs_ent_flag_t
{
	QSE_FS_ENT_NAME = (1 << 0),
	QSE_FS_ENT_TYPE = (1 << 1),
	QSE_FS_ENT_SIZE = (1 << 2),
	QSE_FS_ENT_TIME = (1 << 3)
};
typedef enum qse_fs_ent_flag_t qse_fs_ent_flag_t;

enum qse_fs_ent_type_t
{
	QSE_FS_ENT_UNKNOWN,
	QSE_FS_ENT_SUBDIR,
	QSE_FS_ENT_REGULAR,
	QSE_FS_ENT_CHRDEV,
	QSE_FS_ENT_BLKDEV,
	QSE_FS_ENT_SYMLINK,
	QSE_FS_ENT_PIPE
};

typedef enum qse_fs_ent_type_t qse_fs_ent_type_t;

struct qse_fs_ent_t
{
	int flags;

	struct
	{
		qse_char_t* base;
		qse_char_t* path;
	} name;
	qse_fs_ent_type_t type;
	qse_foff_t        size;

	struct
	{
		qse_ntime_t create; 
		qse_ntime_t access;
		qse_ntime_t modify;
		qse_ntime_t change; /* inode status change */
	} time;
};

typedef struct qse_fs_ent_t qse_fs_ent_t;

struct qse_fs_attr_t
{
	unsigned int isdir: 1;
	unsigned int islnk: 1;
	unsigned int isreg: 1;
	unsigned int isblk: 1;
	unsigned int ischr: 1;

	qse_uintptr_t mode;

	qse_uintmax_t size;
	qse_uintmax_t ino;
	qse_uintmax_t dev;
	qse_uintptr_t uid;
	qse_uintptr_t gid;

	qse_ntime_t atime; /* last access */
	qse_ntime_t mtime; /* last modification */
	qse_ntime_t ctime; /* last status change */
};

typedef struct qse_fs_attr_t qse_fs_attr_t;

enum qse_fs_getattr_flag_t
{
	QSE_FS_GETATTR_SYMLINK = (1 << 15)
};
typedef enum qse_fs_getattr_flag_t qse_fs_getattr_flag_t;

enum qse_fs_setattr_flag_t
{
	QSE_FS_SETATTR_TIME  = (1 << 0),
	QSE_FS_SETATTR_OWNER = (1 << 1),
	QSE_FS_SETATTR_MODE  = (1 << 2),

	QSE_FS_SETATTR_SYMLINK = (1 << 15) /* work on the symbolic link itself. don't follow */
};
typedef enum qse_fs_setattr_flag_t qse_fs_setattr_flag_t;

#if defined(_WIN32)
typedef void* qse_fs_handle_t;
#else
typedef int qse_fs_handle_t;
#endif

typedef struct qse_fs_t qse_fs_t;

enum qse_fs_trait_t
{ 
	/**< don't follow a symbolic link in qse_fs_chdir() */
	QSE_FS_NOFOLLOW = (1 << 1),

	/**< check directories against file system in qse_fs_chdir() */
	QSE_FS_REALPATH = (1 << 2)  
};
typedef enum qse_fs_trait_t qse_fs_trait_t;


enum qse_fs_action_t
{
	QSE_FS_CPFILE,
	QSE_FS_RMFILE,
	QSE_FS_MKDIR,
	QSE_FS_RMDIR,
	QSE_FS_RENFILE,
	QSE_FS_SYMLINK
};
typedef enum qse_fs_action_t qse_fs_action_t;

/**
 * \return -1 on failure, 0 on success
 */
typedef int (*qse_fs_actcb_t) (
	qse_fs_t*          fs,
	qse_fs_action_t    action,
	const qse_char_t*  srcpath,
	const qse_char_t*  dstpath,
	qse_uintmax_t      bytes_total,
	qse_uintmax_t      bytes_copied
);

struct qse_fs_cbs_t
{
	qse_fs_actcb_t actcb;
	/*qse_fs_querycb_t querycb;*/
};
typedef struct qse_fs_cbs_t qse_fs_cbs_t;

struct qse_fs_t
{
	qse_mmgr_t*     mmgr;
	qse_cmgr_t*     cmgr; /* for name conversion */

	qse_fs_errnum_t errnum;
	int             trait;
	qse_fs_cbs_t    cbs;

	qse_fs_ent_t    ent;
	qse_char_t*     curdir;
	void*           info;

	qse_uint8_t     cpbuf[4096];
	void*           cfs; /* stack for recursive file copying */
	struct
	{
		void* ptr;
		qse_size_t len;
		qse_size_t capa;
	} ddr; /* destination directores remembers while copying */
};

enum qse_fs_opt_t
{
	QSE_FS_TRAIT,
	QSE_FS_CBS
};
typedef enum qse_fs_opt_t qse_fs_opt_t;

enum qse_fs_cpfile_flag_t
{
	QSE_FS_CPFILE_GLOB      = (1 << 0),
	QSE_FS_CPFILE_RECURSIVE = (1 << 1),
	QSE_FS_CPFILE_FORCE     = (1 << 2),
	QSE_FS_CPFILE_PRESERVE  = (1 << 3),
	QSE_FS_CPFILE_REPLACE   = (1 << 4),

	QSE_FS_CPFILE_SYMLINK   = (1 << 15),

	QSE_FS_CPFILE_ALL = (QSE_FS_CPFILE_GLOB | QSE_FS_CPFILE_RECURSIVE |
	                     QSE_FS_CPFILE_FORCE | QSE_FS_CPFILE_PRESERVE |
	                     QSE_FS_CPFILE_REPLACE | QSE_FS_CPFILE_SYMLINK)
};
typedef enum qse_fs_cpfile_flag_t qse_fs_cpfile_flag_t;

enum qse_fs_mkdir_flag_t
{
	QSE_FS_MKDIR_PARENT = (1 << 0),
	QSE_FS_MKDIR_IGNORE_UMASK = (1 << 1),

	QSE_FS_MKDIRMBS_PARENT = QSE_FS_MKDIR_PARENT,
	QSE_FS_MKDIRWCS_PARENT = QSE_FS_MKDIR_PARENT,

	QSE_FS_MKDIRMBS_IGNORE_UMASK = QSE_FS_MKDIR_IGNORE_UMASK,
	QSE_FS_MKDIRWCS_IGNORE_UMASK = QSE_FS_MKDIR_IGNORE_UMASK
};
typedef enum qse_fs_mkdir_flag_t qse_fs_mkdir_flag_t;

enum qse_fs_rmfile_flag_t
{
	QSE_FS_RMFILE_GLOB      = (1 << 0),
	QSE_FS_RMFILE_RECURSIVE = (1 << 1),

	QSE_FS_RMFILEMBS_GLOB      = QSE_FS_RMFILE_GLOB,
	QSE_FS_RMFILEMBS_RECURSIVE = QSE_FS_RMFILE_RECURSIVE,

	QSE_FS_RMFILEWCS_GLOB      = QSE_FS_RMFILE_GLOB,
	QSE_FS_RMFILEWCS_RECURSIVE = QSE_FS_RMFILE_RECURSIVE
};
typedef enum qse_fs_rmfile_flag_t qse_fs_rmfile_flag_t;

enum qse_fs_rmdir_flag_t
{
	QSE_FS_RMDIR_GLOB      = (1 << 0),
	QSE_FS_RMDIR_RECURSIVE = (1 << 1),

	QSE_FS_RMDIRMBS_GLOB      = QSE_FS_RMDIR_GLOB,
	QSE_FS_RMDIRMBS_RECURSIVE = QSE_FS_RMDIR_RECURSIVE,

	QSE_FS_RMDIRWCS_GLOB      = QSE_FS_RMDIR_GLOB,
	QSE_FS_RMDIRWCS_RECURSIVE = QSE_FS_RMDIR_RECURSIVE
};
typedef enum qse_fs_rmdir_flag_t qse_fs_rmdir_flag_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_fs_t* qse_fs_open (
	qse_mmgr_t* mmgr, 
	qse_size_t  xtnsize
);

QSE_EXPORT void qse_fs_close (
	qse_fs_t* fs
);

QSE_EXPORT int qse_fs_init (
	qse_fs_t*   fs,
	qse_mmgr_t* mmgr
);

QSE_EXPORT void qse_fs_fini (
	qse_fs_t* fs
);

QSE_EXPORT qse_mmgr_t* qse_fs_getmmgr (
	qse_fs_t* fs
);

QSE_EXPORT void* qse_fs_getxtn (
	qse_fs_t* fs
);

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE qse_fs_errnum_t qse_fs_geterrnum (qse_fs_t* fs) { return fs->errnum; }
#else
#	define qse_fs_geterrnum(fs) ((fs)->errnum)
#endif

QSE_EXPORT int qse_fs_getopt (
	qse_fs_t*    fs,
	qse_fs_opt_t id,
	void*        value
);

QSE_EXPORT int qse_fs_setopt (
	qse_fs_t*    fs,
	qse_fs_opt_t id,
	const void*  value
);

QSE_EXPORT qse_fs_ent_t* qse_fs_read (
	qse_fs_t* fs,
	int       flags
);

QSE_EXPORT int qse_fs_chdir (
	qse_fs_t*         fs,
	const qse_char_t* name
);

QSE_EXPORT int qse_fs_push (
	qse_fs_t* fs,
	const qse_char_t* name
);

QSE_EXPORT int qse_fs_pop (
	qse_fs_t* fs,
	const qse_char_t* name
);


QSE_EXPORT int qse_fs_getattronfd (
	qse_fs_t*            fs,
	qse_fs_handle_t      fd,
	qse_fs_attr_t*       attr,
	int                  flags
);

QSE_EXPORT int qse_fs_setattronfd (
	qse_fs_t*            fs,
	qse_fs_handle_t      fd,
	const qse_fs_attr_t* attr,
	int                  flags /** bitwise-ORed #qse_fs_setattr_flag_t enumerators */
);

QSE_EXPORT int qse_fs_getattrmbs (
	qse_fs_t*            fs,
	const qse_mchar_t*   path,
	qse_fs_attr_t*       attr,
	int                  flags
);

QSE_EXPORT int qse_fs_getattrwcs (
	qse_fs_t*            fs,
	const qse_wchar_t*   path,
	qse_fs_attr_t*       attr,
	int                  flags
);

QSE_EXPORT int qse_fs_setattrmbs (
	qse_fs_t*            fs,
	qse_mchar_t*         path,
	const qse_fs_attr_t* attr,
	int                  flags /** bitwise-ORed #qse_fs_setattr_flag_t enumerators */
);

QSE_EXPORT int qse_fs_setattrwcs (
	qse_fs_t*            fs,
	qse_wchar_t*         path,
	const qse_fs_attr_t* attr,
	int                  flags /** bitwise-ORed #qse_fs_setattr_flag_t enumerators */
);


#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_fs_getattr(fs,path,attr,flags) qse_fs_getattrmbs(fs,path,attr,flags)
#	define qse_fs_setattr(fs,path,attr,flags) qse_fssetattrmbs(fs,path,attr,flags)
#else
#	define qse_fs_getattr(fs,path,attr,flags) qse_fs_getattrwcs(fs,path,attr,flags)
#	define qse_fs_setattr(fs,path,attr,flags) qse_fssetattrwcs(fs,path,attr,flags)
#endif


QSE_EXPORT int qse_fs_move (
	qse_fs_t*         fs,
	const qse_char_t* oldpath,
	const qse_char_t* newpath
);






QSE_EXPORT int qse_fs_cpfilembs (
	qse_fs_t*          fs,
	const qse_mchar_t* srcpath,
	const qse_mchar_t* dstpath,
	int                flags
);

QSE_EXPORT int qse_fs_cpfilewcs (
	qse_fs_t*          fs,
	const qse_wchar_t* srcpath,
	const qse_wchar_t* dstpath,
	int                flags
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_fs_cpfile(fs,srcpath,dstpath,flags) qse_fs_cpfilembs(fs,srcpath,dstpath,flags)
#else
#	define qse_fs_cpfile(fs,srcpath,dstpath,flags) qse_fs_cpfilewcs(fs,srcpath,dstpath,flags)
#endif

QSE_EXPORT int qse_fs_mkdirmbs (
	qse_fs_t*          fs,
	const qse_mchar_t* path,
	qse_fs_mode_t      mode,
	int                flags
);

QSE_EXPORT int qse_fs_mkdirwcs (
	qse_fs_t*          fs,
	const qse_wchar_t* path,
	qse_fs_mode_t      mode,
	int                flags
);

QSE_EXPORT int qse_fs_rmfilembs (
	qse_fs_t*          fs,
	const qse_mchar_t* path,
	int                flags
);

QSE_EXPORT int qse_fs_rmfilewcs (
	qse_fs_t*          fs,
	const qse_wchar_t* path,
	int                flags
);

QSE_EXPORT int qse_fs_rmdirmbs (
	qse_fs_t*          fs,
	const qse_mchar_t* path,
	int                flags
);

QSE_EXPORT int qse_fs_rmdirwcs (
	qse_fs_t*          fs,
	const qse_wchar_t* path,
	int                flags
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_fs_mkdir(fs,path,mode,flags)  qse_fs_mkdirmbs(fs,path,mode,flags)
#	define qse_fs_rmfile(fs,path,flags) qse_fs_rmfilembs(fs,path,flags)
#	define qse_fs_rmdir(fs,path,flags)  qse_fs_rmdirmbs(fs,path,flags)
#else
#	define qse_fs_mkdir(fs,path,mode,flags)  qse_fs_mkdirwcs(fs,path,mode,flags)
#	define qse_fs_rmfile(fs,path,flags) qse_fs_rmfilewcs(fs,path,flags)
#	define qse_fs_rmdir(fs,path,flags)  qse_fs_rmdirwcs(fs,path,flags)
#endif


/* =========================================================================
 * GLOBAL UTILITIES NOT USING THE FS OBJECT 
 * ========================================================================= */

QSE_EXPORT qse_mchar_t* qse_get_current_mbsdir (
	qse_mchar_t* buf,
	qse_size_t   size,
	qse_mmgr_t*  mmgr
);

QSE_EXPORT qse_wchar_t* qse_get_current_wcsdir (
	qse_wchar_t* buf,
	qse_size_t   size,
	qse_mmgr_t*  mmgr
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_get_current_dir(buf,size,mmgr) qse_get_current_mbsdir(buf,size,mmgr)
#else
#	define qse_get_current_dir(buf,size,mmgr) qse_get_current_wcsdir(buf,size,mmgr)
#endif

QSE_EXPORT int qse_get_prog_path (
	const qse_char_t* argv0,
	qse_char_t*       buf,
	qse_size_t        size,
	qse_mmgr_t*       mmgr
);

#if defined(__cplusplus)
}
#endif

#endif
