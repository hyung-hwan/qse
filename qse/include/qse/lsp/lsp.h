/*
 * $Id: lsp.h 183 2008-06-03 08:18:55Z baconevi $
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

#ifndef _QSE_LSP_LSP_H_
#define _QSE_LSP_LSP_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 *  The file provides interface to a LISP interpreter.
 */

typedef struct qse_lsp_t qse_lsp_t;
typedef struct qse_lsp_obj_t qse_lsp_obj_t;
typedef struct qse_lsp_prm_t qse_lsp_prm_t;

/** 
 * The qse_lsp_loc_t defines a structure to store location information.
 */
struct qse_lsp_loc_t
{
	const qse_char_t* file; /**< file */
	qse_size_t        line; /**< line  */
	qse_size_t        colm; /**< column */
};
typedef struct qse_lsp_loc_t qse_lsp_loc_t;

typedef int (*qse_lsp_sprintf_t) (
	void* data, qse_char_t* buf, qse_size_t size, 
	const qse_char_t* fmt, ...);

struct qse_lsp_prm_t
{
	qse_lsp_sprintf_t sprintf;
	void* udd;
};

/**
 * The qse_lsp_io_cmd_t type defines I/O commands.
 */
enum qse_lsp_io_cmd_t
{
	QSE_LSP_IO_OPEN   = 0,
	QSE_LSP_IO_CLOSE  = 1,
	QSE_LSP_IO_READ   = 2,
	QSE_LSP_IO_WRITE  = 3
};
typedef enum qse_lsp_io_cmd_t qse_lsp_io_cmd_t;

/**
 * The qse_lsp_io_arg_t type defines a data structure for an I/O handler.
 */
struct qse_lsp_io_arg_t
{
	void*             handle;
	const qse_char_t* path;
};
typedef struct qse_lsp_io_arg_t qse_lsp_io_arg_t;

/**
 * The qse_lsp_io_fun_t type defines an I/O handler function.
 */
typedef qse_ssize_t (*qse_lsp_io_fun_t) (
	qse_lsp_t*        lsp,
	qse_lsp_io_cmd_t  cmd,
	qse_lsp_io_arg_t* arg,
	qse_char_t*       data, 
	qse_size_t        count
);

/**
 * The qse_lsp_io_t type defines a I/O handler set.
 */
struct qse_lsp_io_t
{
	qse_lsp_io_fun_t in;
	qse_lsp_io_fun_t out;
};
typedef struct qse_lsp_io_t qse_lsp_io_t;


/* option code */
enum
{
	QSE_LSP_UNDEFSYMBOL = (1 << 0)
};

/**
 * The qse_lsp_errnum_t type defines error numbers.
 */
enum qse_lsp_errnum_t
{
	QSE_LSP_ENOERR,
	QSE_LSP_ENOMEM,

	QSE_LSP_EEXIT,
	QSE_LSP_EEND,
	QSE_LSP_EENDSTR,
	QSE_LSP_ENOINP,
	QSE_LSP_EINPUT,
	QSE_LSP_ENOOUTP,
	QSE_LSP_EOUTPUT,

	QSE_LSP_EINTERN,
	QSE_LSP_ESYNTAX,
	QSE_LSP_ELSTDEEP,
	QSE_LSP_ERPAREN,
	QSE_LSP_EARGBAD,
	QSE_LSP_EARGFEW,
	QSE_LSP_EARGMANY,
	QSE_LSP_EUNDEFFN,
	QSE_LSP_EBADFN,
	QSE_LSP_EDUPFML,
	QSE_LSP_EBADSYM,
	QSE_LSP_EUNDEFSYM,
	QSE_LSP_EEMPBDY,
	QSE_LSP_EVALBAD,
	QSE_LSP_EDIVBY0
};
typedef enum qse_lsp_errnum_t qse_lsp_errnum_t;

typedef const qse_char_t* (*qse_lsp_errstr_t) (
	qse_lsp_t*       lsp,   /**< lisp */
	qse_lsp_errnum_t num    /**< error number */
);

typedef qse_lsp_obj_t* (*qse_lsp_prim_t) (
	qse_lsp_t*     lsp,
	qse_lsp_obj_t* obj
);

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (lsp)

qse_lsp_t* qse_lsp_open (
	qse_mmgr_t*          mmgr, 
	qse_size_t           xtnsize,
	const qse_lsp_prm_t* prm,
	qse_size_t           mem_ubound,
	qse_size_t           mem_ubound_inc
);

void qse_lsp_close (
	qse_lsp_t* lsp /**< lisp */
);

qse_lsp_errstr_t qse_lsp_geterrstr (
	qse_lsp_t* lsp    /**< lisp */
);

void qse_lsp_seterrstr (
     qse_lsp_t*       lsp,   /**< lisp */
     qse_lsp_errstr_t errstr /**< an error string getter */
);

qse_lsp_errnum_t qse_lsp_geterrnum (
	qse_lsp_t* lsp /**< lisp */
);

const qse_lsp_loc_t* qse_lsp_geterrloc (
	qse_lsp_t* lsp /**< lisp */
);

const qse_char_t* qse_lsp_geterrmsg (
	qse_lsp_t* lsp /**< lisp */
);

void qse_lsp_geterror (
	qse_lsp_t*         lsp,    /**< lisp */
	qse_lsp_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg, /**< error message */
	qse_lsp_loc_t*     errloc  /**< error location */
);

void qse_lsp_seterrnum (
	qse_lsp_t*        lsp,    /**< lisp */
	qse_lsp_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

void qse_lsp_seterrmsg (
	qse_lsp_t*        lsp,      /**< lisp */
	qse_lsp_errnum_t  errnum,   /**< error number */
	const qse_char_t* errmsg,   /**< error message */
	const qse_lsp_loc_t* errloc /**< error location */
);

void qse_lsp_seterror (
	qse_lsp_t*           lsp,    /**< lisp */
	qse_lsp_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< array of arguments for formatting 
	                              *   an error message */
	const qse_lsp_loc_t* errloc  /**< error location */
);

/**
 * The qse_lsp_attachio() function attaches I/O handlers.
 * Upon attachment, it opens input and output streams by calling
 * the I/O handlers with the #QSE_LSP_IO_OPEN command. 
 */
int qse_lsp_attachio (
	qse_lsp_t*    lsp,  /**< lisp */
	qse_lsp_io_t* io    /**< I/O handler set */
);

/**
 * The qse_lsp_detachio() function detaches I/O handlers.
 * It closes the streams for both input and output by calling the I/O handlers
 * with the #QSE_LSP_IO_CLOSE command.
 */
void qse_lsp_detachio (
	qse_lsp_t*    lsp   /**< lisp */
);

qse_lsp_obj_t* qse_lsp_read (qse_lsp_t* lsp);
qse_lsp_obj_t* qse_lsp_eval (qse_lsp_t* lsp, qse_lsp_obj_t* obj);
int qse_lsp_print (qse_lsp_t* lsp, const qse_lsp_obj_t* obj);


/**
 * The qse_lsp_gc() function invokes the garbage collector
 */
void qse_lsp_gc (
	qse_lsp_t* lsp /**< lisp */
);

int qse_lsp_addprim (
	qse_lsp_t* lsp, const qse_char_t* name, qse_size_t name_len, 
	qse_lsp_prim_t prim, qse_size_t min_args, qse_size_t max_args);
int qse_lsp_removeprim (qse_lsp_t* lsp, const qse_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
