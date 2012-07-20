/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_STX_STX_H_
#define _QSE_STX_STX_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 *  The file provides interface to a stx interpreter.
 */

typedef struct qse_stx_t qse_stx_t;

/** 
 * The qse_stx_loc_t defines a structure to store location information.
 */
struct qse_stx_loc_t
{
	const qse_char_t* file; /**< file */
	qse_size_t        line; /**< line  */
	qse_size_t        colm; /**< column */
};
typedef struct qse_stx_loc_t qse_stx_loc_t;

/**
 * The qse_stx_io_cmd_t type defines I/O commands.
 */
enum qse_stx_io_cmd_t
{
	QSE_STX_IO_OPEN   = 0,
	QSE_STX_IO_CLOSE  = 1,
	QSE_STX_IO_READ   = 2,
	QSE_STX_IO_WRITE  = 3
};
typedef enum qse_stx_io_cmd_t qse_stx_io_cmd_t;

/**
 * The qse_stx_io_arg_t type defines a data structure for an I/O handler.
 */
struct qse_stx_io_arg_t
{
	void*             handle;
	const qse_char_t* path;
};
typedef struct qse_stx_io_arg_t qse_stx_io_arg_t;

/**
 * The qse_stx_io_fun_t type defines an I/O handler function.
 */
typedef qse_ssize_t (*qse_stx_io_fun_t) (
	qse_stx_t*        stx,
	qse_stx_io_cmd_t  cmd,
	qse_stx_io_arg_t* arg,
	qse_char_t*       data, 
	qse_size_t        count
);

/**
 * The qse_stx_io_t type defines a I/O handler set.
 */
struct qse_stx_io_t
{
	qse_stx_io_fun_t in;
	qse_stx_io_fun_t out;
};
typedef struct qse_stx_io_t qse_stx_io_t;

/**
 * The qse_stx_errnum_t type defines error numbers.
 */
enum qse_stx_errnum_t
{
	QSE_STX_ENOERR,
	QSE_STX_ENOMEM,
	QSE_STX_EINTERN,

	QSE_STX_EEXIT,
	QSE_STX_EEND,

	QSE_STX_EIO,
	QSE_STX_EENDSTR,
	QSE_STX_ESHARP,
	QSE_STX_EDOT,
	QSE_STX_ELPAREN,
	QSE_STX_ERPAREN,
	QSE_STX_ELSTDEEP,

	QSE_STX_EVARBAD,
	QSE_STX_EARGBAD,
	QSE_STX_EARGFEW,
	QSE_STX_EARGMANY,
	QSE_STX_EUNDEFFN,
	QSE_STX_EBADFN,
	QSE_STX_EDUPFML,
	QSE_STX_EBADSYM,
	QSE_STX_EUNDEFSYM,
	QSE_STX_EEMPBDY,
	QSE_STX_EVALBAD,
	QSE_STX_EDIVBY0
};
typedef enum qse_stx_errnum_t qse_stx_errnum_t;

typedef const qse_char_t* (*qse_stx_errstr_t) (
	qse_stx_t*       stx,   /**< stx */
	qse_stx_errnum_t num    /**< error number */
);

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (stx)

qse_stx_t* qse_stx_open (
	qse_mmgr_t*          mmgr, 
	qse_size_t           xtnsize,
	qse_size_t           memcapa
);

void qse_stx_close (
	qse_stx_t* stx /**< stx */
);

qse_stx_errstr_t qse_stx_geterrstr (
	qse_stx_t* stx    /**< stx */
);

void qse_stx_seterrstr (
     qse_stx_t*       stx,   /**< stx */
     qse_stx_errstr_t errstr /**< an error string getter */
);

qse_stx_errnum_t qse_stx_geterrnum (
	qse_stx_t* stx /**< stx */
);

const qse_stx_loc_t* qse_stx_geterrloc (
	qse_stx_t* stx /**< stx */
);

const qse_char_t* qse_stx_geterrmsg (
	qse_stx_t* stx /**< stx */
);

void qse_stx_geterror (
	qse_stx_t*         stx,    /**< stx */
	qse_stx_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg, /**< error message */
	qse_stx_loc_t*     errloc  /**< error location */
);

void qse_stx_seterrnum (
	qse_stx_t*        stx,    /**< stx */
	qse_stx_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

void qse_stx_seterrmsg (
	qse_stx_t*        stx,      /**< stx */
	qse_stx_errnum_t  errnum,   /**< error number */
	const qse_char_t* errmsg,   /**< error message */
	const qse_stx_loc_t* errloc /**< error location */
);

void qse_stx_seterror (
	qse_stx_t*           stx,    /**< stx */
	qse_stx_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< array of arguments for formatting 
	                              *   an error message */
	const qse_stx_loc_t* errloc  /**< error location */
);

/**
 * The qse_stx_attachio() function attaches I/O handlers.
 * Upon attachment, it opens input and output streams by calling
 * the I/O handlers with the #QSE_STX_IO_OPEN command. 
 */
int qse_stx_attachio (
	qse_stx_t*    stx,  /**< stx */
	qse_stx_io_t* io    /**< I/O handler set */
);

/**
 * The qse_stx_detachio() function detaches I/O handlers.
 * It closes the streams for both input and output by calling the I/O handlers
 * with the #QSE_STX_IO_CLOSE command.
 */
void qse_stx_detachio (
	qse_stx_t* stx   /**< stx */
);

#ifdef __cplusplus
}
#endif

#endif
