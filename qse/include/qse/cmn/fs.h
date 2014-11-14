/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_FS_H_
#define _QSE_CMN_FS_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/time.h>

enum qse_fs_errnum_t
{
	QSE_FS_ENOERR = 0,
	QSE_FS_EOTHER,
     QSE_FS_ENOIMPL,    /**< not implemented */
     QSE_FS_ESYSERR,    /**< subsystem error */
     QSE_FS_EINTERN,    /**< internal error */

     QSE_FS_ENOMEM,     /**< out of memory */
     QSE_FS_EINVAL,     /**< invalid parameter */
     QSE_FS_EACCES,     /**< access denied */
     QSE_FS_ENOENT,     /**< no such file */
     QSE_FS_EEXIST,     /**< already exist */
     QSE_FS_EINTR,      /**< interrupted */
	QSE_FS_ENODIR,
	QSE_FS_EISDIR,
	QSE_FS_EXDEV 
};
typedef enum qse_fs_errnum_t qse_fs_errnum_t;

enum qse_fs_ent_flag_t
{
	QSE_FS_ENT_NAME = (1 << 0),
	QSE_FS_ENT_TYPE = (1 << 1),
	QSE_FS_ENT_SIZE = (1 << 2),
	QSE_FS_ENT_TIME = (1 << 3)
};

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
	int                flags;

	struct
	{
		qse_char_t* base;
		qse_char_t* path;
	} name;
	qse_fs_ent_type_t type;
	qse_foff_t         size;

	struct
	{
		qse_ntime_t create; 
		qse_ntime_t access;
		qse_ntime_t modify;
		qse_ntime_t change;	 /* inode status change */
	} time;
};

typedef struct qse_fs_ent_t qse_fs_ent_t;

struct qse_fs_t
{
	qse_mmgr_t*     mmgr;
	qse_fs_errnum_t errnum;
	qse_fs_ent_t    ent;
	qse_char_t*     curdir;
	void*           info;
};

typedef struct qse_fs_t qse_fs_t;

enum qse_fs_option_t
{ 
	/**< don't follow a symbolic link in qse_fs_chdir() */
	QSE_FS_NOFOLLOW = (1 << 1),

	/**< check directories against file system in qse_fs_chdir() */
	QSE_FS_REALPATH = (1 << 2)  
};

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

QSE_EXPORT qse_fs_errnum_t qse_fs_geterrnum (
	qse_fs_t* fs
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

QSE_EXPORT int qse_fs_move (
	qse_fs_t*         fs,
	const qse_char_t* oldpath,
	const qse_char_t* newpath
);
	
QSE_EXPORT int qse_fs_delete (
	qse_fs_t*         fs,
	const qse_char_t* path
);

#if defined(__cplusplus)
}
#endif

#endif
