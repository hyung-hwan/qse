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

#ifndef _QSE_FS_DIR_H_
#define _QSE_FS_DIR_H_

#include <qse/types.h>
#include <qse/macros.h>

enum qse_dir_errnum_t
{
	QSE_DIR_ENOERR = 0,
	QSE_DIR_EINTERN,

	QSE_DIR_ENOMEM,
	QSE_DIR_EINVAL,
	QSE_DIR_EACCES,
	QSE_DIR_ENOENT,
	QSE_DIR_ENODIR,
	QSE_DIR_ESYSTEM
};
typedef enum qse_dir_errnum_t qse_dir_errnum_t;

struct qse_dir_ent_t
{
	enum
	{
		QSE_DIR_ENT_UNKNOWN,
		QSE_DIR_ENT_DIRECTORY,
		QSE_DIR_ENT_REGULAR,
		QSE_DIR_ENT_FIFO,
		QSE_DIR_ENT_CHAR,
		QSE_DIR_ENT_BLOCK,
		QSE_DIR_ENT_LINK
	} type;
	qse_char_t* name;
	qse_foff_t  size;
};

typedef struct qse_dir_ent_t qse_dir_ent_t;

struct qse_dir_t
{
	QSE_DEFINE_COMMON_FIELDS (dir)
	qse_dir_errnum_t errnum;
	qse_dir_ent_t    ent;
	qse_char_t*      curdir;
	void*            info;
};

typedef struct qse_dir_t qse_dir_t;

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (dir)

qse_dir_t* qse_dir_open (
	qse_mmgr_t*       mmgr, 
	qse_size_t        xtnsize
);

void qse_dir_close (
	qse_dir_t*        dir
);

int qse_dir_init (
	qse_dir_t*  dir,
	qse_mmgr_t* mmgr
);

void qse_dir_fini (
	qse_dir_t* dir
);

qse_dir_errnum_t qse_dir_geterrnum (
	qse_dir_t* dir
);

const qse_char_t* qse_dir_geterrmsg (
	qse_dir_t* dir
);

qse_dir_ent_t* qse_dir_read (
	qse_dir_t*        dir
);

int qse_dir_change (
	qse_dir_t*        dir,
	const qse_char_t* name
);

int qse_dir_push (
	qse_dir_t*        dir,
	const qse_char_t* name
);

int qse_dir_pop (
	qse_dir_t*        dir,
	const qse_char_t* name
);

#ifdef __cplusplus
}
#endif

#endif
