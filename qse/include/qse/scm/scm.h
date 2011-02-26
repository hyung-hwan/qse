/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#ifndef _QSE_SCM_SCM_H_
#define _QSE_SCM_SCM_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 *  The file provides interface to a scheme interpreter.
 */

typedef struct qse_scm_t qse_scm_t;
typedef struct qse_scm_ent_t qse_scm_ent_t;

/** 
 * The qse_scm_loc_t defines a structure to store location information.
 */
struct qse_scm_loc_t
{
	const qse_char_t* file; /**< file */
	qse_size_t        line; /**< line  */
	qse_size_t        colm; /**< column */
};
typedef struct qse_scm_loc_t qse_scm_loc_t;

/**
 * The qse_scm_io_cmd_t type defines I/O commands.
 */
enum qse_scm_io_cmd_t
{
	QSE_SCM_IO_OPEN   = 0,
	QSE_SCM_IO_CLOSE  = 1,
	QSE_SCM_IO_READ   = 2,
	QSE_SCM_IO_WRITE  = 3
};
typedef enum qse_scm_io_cmd_t qse_scm_io_cmd_t;

/**
 * The qse_scm_io_arg_t type defines a data structure for an I/O handler.
 */
struct qse_scm_io_arg_t
{
	void*             handle;
	const qse_char_t* path;
};
typedef struct qse_scm_io_arg_t qse_scm_io_arg_t;

/**
 * The qse_scm_io_fun_t type defines an I/O handler function.
 */
typedef qse_ssize_t (*qse_scm_io_fun_t) (
	qse_scm_t*        scm,
	qse_scm_io_cmd_t  cmd,
	qse_scm_io_arg_t* arg,
	qse_char_t*       data, 
	qse_size_t        count
);

/**
 * The qse_scm_io_t type defines a I/O handler set.
 */
struct qse_scm_io_t
{
	qse_scm_io_fun_t in;
	qse_scm_io_fun_t out;
};
typedef struct qse_scm_io_t qse_scm_io_t;

/**
 * The qse_scm_errnum_t type defines error numbers.
 */
enum qse_scm_errnum_t
{
	QSE_SCM_ENOERR,
	QSE_SCM_ENOMEM,

	QSE_SCM_EEXIT,
	QSE_SCM_EEND,

	QSE_SCM_EIO,
	QSE_SCM_EENDSTR,
	QSE_SCM_ESHARP,
	QSE_SCM_EDOT,

	QSE_SCM_EINTERN,
	QSE_SCM_ELSTDEEP,
	QSE_SCM_ELPAREN,
	QSE_SCM_ERPAREN,
	QSE_SCM_EARGBAD,
	QSE_SCM_EARGFEW,
	QSE_SCM_EARGMANY,
	QSE_SCM_EUNDEFFN,
	QSE_SCM_EBADFN,
	QSE_SCM_EDUPFML,
	QSE_SCM_EBADSYM,
	QSE_SCM_EUNDEFSYM,
	QSE_SCM_EEMPBDY,
	QSE_SCM_EVALBAD,
	QSE_SCM_EDIVBY0
};
typedef enum qse_scm_errnum_t qse_scm_errnum_t;

typedef const qse_char_t* (*qse_scm_errstr_t) (
	qse_scm_t*       scm,   /**< scheme */
	qse_scm_errnum_t num    /**< error number */
);

typedef qse_scm_ent_t* (*qse_scm_prim_t) (
	qse_scm_t*     scm,
	qse_scm_ent_t* obj
);

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (scm)

qse_scm_t* qse_scm_open (
	qse_mmgr_t*          mmgr, 
	qse_size_t           xtnsize,
	qse_size_t           mem_ubound,
	qse_size_t           mem_ubound_inc
);

void qse_scm_close (
	qse_scm_t* scm /**< scheme */
);

qse_scm_errstr_t qse_scm_geterrstr (
	qse_scm_t* scm    /**< scheme */
);

void qse_scm_seterrstr (
     qse_scm_t*       scm,   /**< scheme */
     qse_scm_errstr_t errstr /**< an error string getter */
);

qse_scm_errnum_t qse_scm_geterrnum (
	qse_scm_t* scm /**< scheme */
);

const qse_scm_loc_t* qse_scm_geterrloc (
	qse_scm_t* scm /**< scheme */
);

const qse_char_t* qse_scm_geterrmsg (
	qse_scm_t* scm /**< scheme */
);

void qse_scm_geterror (
	qse_scm_t*         scm,    /**< scheme */
	qse_scm_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg, /**< error message */
	qse_scm_loc_t*     errloc  /**< error location */
);

void qse_scm_seterrnum (
	qse_scm_t*        scm,    /**< scheme */
	qse_scm_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

void qse_scm_seterrmsg (
	qse_scm_t*        scm,      /**< scheme */
	qse_scm_errnum_t  errnum,   /**< error number */
	const qse_char_t* errmsg,   /**< error message */
	const qse_scm_loc_t* errloc /**< error location */
);

void qse_scm_seterror (
	qse_scm_t*           scm,    /**< scheme */
	qse_scm_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< array of arguments for formatting 
	                              *   an error message */
	const qse_scm_loc_t* errloc  /**< error location */
);

/**
 * The qse_scm_attachio() function attaches I/O handlers.
 * Upon attachment, it opens input and output streams by calling
 * the I/O handlers with the #QSE_SCM_IO_OPEN command. 
 */
int qse_scm_attachio (
	qse_scm_t*    scm,  /**< scheme */
	qse_scm_io_t* io    /**< I/O handler set */
);

/**
 * The qse_scm_detachio() function detaches I/O handlers.
 * It closes the streams for both input and output by calling the I/O handlers
 * with the #QSE_SCM_IO_CLOSE command.
 */
void qse_scm_detachio (
	qse_scm_t* scm   /**< scheme */
);

qse_scm_ent_t* qse_scm_read (
	qse_scm_t* scm /**< scheme */
);

qse_scm_ent_t* qse_scm_eval (
	qse_scm_t*     scm, /**< scheme */
	qse_scm_ent_t* obj
);

int qse_scm_print (
	qse_scm_t*           scm, /**< scheme */
	const qse_scm_ent_t* obj
);

/**
 * The qse_scm_gc() function invokes the garbage collector
 */
void qse_scm_gc (
	qse_scm_t* scm /**< scheme */
);


int qse_scm_addprim (
	qse_scm_t* scm,
	const qse_char_t* name,
	qse_size_t name_len, 
	qse_scm_prim_t prim,
	qse_size_t min_args,
	qse_size_t max_args
);

int qse_scm_removeprim (
	qse_scm_t* scm,
	const qse_char_t* name
);

#ifdef __cplusplus
}
#endif

#endif
