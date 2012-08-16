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

#ifndef _QSE_CUT_CUT_H_
#define _QSE_CUT_CUT_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>

/** @file
 * This file defines a text cutter utility.
 * 
 * @todo QSE_CUT_ORDEREDSEL - A selector 5,3,1 is ordered to 1,3,5
 */

/**
 * @example cut.c
 * This example implements a simple cut utility.
 */

/** @struct qse_cut_t
 * The qse_cut_t type defines a text cutter. The details are hidden as it is
 * a large complex structure vulnerable to unintended changes. 
 */
typedef struct qse_cut_t qse_cut_t;

/**
 * the qse_cut_errnum_t type defines error numbers.
 */
enum qse_cut_errnum_t
{
	QSE_CUT_ENOERR,  /**< no error */
	QSE_CUT_ENOMEM,  /**< insufficient memory */
	QSE_CUT_EINVAL,  /**< invalid parameter or data */
	QSE_CUT_ESELNV,  /**< selector not valid */
	QSE_CUT_EIOFIL,  /**< io error with file '${0}'*/
	QSE_CUT_EIOUSR   /**< error returned by user io handler */
};
typedef enum qse_cut_errnum_t qse_cut_errnum_t;

/**
 * The qse_cut_errstr_t type defines a error string getter. It should return 
 * an error formatting string for an error number requested. A new string
 * should contain the same number of positional parameters (${X}) as in the
 * default error formatting string. You can set a new getter into a stream
 * editor with the qse_cut_seterrstr() function to customize an error string.
 */
typedef const qse_char_t* (*qse_cut_errstr_t) (
	qse_cut_t*       cut,   /**< text cutter */
	qse_cut_errnum_t num    /**< an error number */
);

/** 
 * The qse_cut_option_t type defines various option codes for a text cutter.
 * Options can be OR'ed with each other and be passed to a text cutter with
 * the qse_cut_setoption() function.
 */
enum qse_cut_option_t
{
	/** show delimited line only. if not set, undelimited lines are 
	 *  shown in its entirety */
	QSE_CUT_DELIMONLY    = (1 << 0),

	/** treat any whitespaces as an input delimiter */
	QSE_CUT_WHITESPACE   = (1 << 2),

	/** fold adjacent delimiters */
	QSE_CUT_FOLDDELIMS   = (1 << 3),

	/** trim leading and trailing whitespaces off the input line */
	QSE_CUT_TRIMSPACE    = (1 << 4),

	/** normalize whitespaces in the input line */
	QSE_CUT_NORMSPACE    = (1 << 5)
};
typedef enum qse_cut_option_t qse_cut_option_t;

/**
 * The qse_cut_io_cmd_t type defines I/O command codes. The code indicates 
 * the action to take in an I/O handler.
 */
enum qse_cut_io_cmd_t
{
	QSE_CUT_IO_OPEN  = 0,
	QSE_CUT_IO_CLOSE = 1,
	QSE_CUT_IO_READ  = 2,
	QSE_CUT_IO_WRITE = 3
};
typedef enum qse_cut_io_cmd_t qse_cut_io_cmd_t;

/**
 * The qse_cut_io_arg_t type defines a data structure required by 
 * an I/O handler.
 */
struct qse_cut_io_arg_t
{
	void* handle; /**< I/O handle */
};
typedef struct qse_cut_io_arg_t qse_cut_io_arg_t;

/** 
 * The qse_cut_io_fun_t type defines an I/O handler. qse_cut_exec() calls
 * I/O handlers to read from and write to a text stream.
 */
typedef qse_ssize_t (*qse_cut_io_fun_t) (
	qse_cut_t*        cut,
	qse_cut_io_cmd_t  cmd,
	qse_cut_io_arg_t* arg,
	qse_char_t*       data,
	qse_size_t        count
);

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (cut)

/**
 * The qse_cut_open() function creates a text cutter.
 * @return A pointer to a text cutter on success, #QSE_NULL on failure
 */
qse_cut_t* qse_cut_open (
	qse_mmgr_t*    mmgr,   /**< memory manager */
	qse_size_t     xtnsize /**< extension size in bytes */
);

/**
 * The qse_cut_close() function destroys a text cutter.
 */
void qse_cut_close (
	qse_cut_t* cut /**< text cutter */
);

/**
 * The qse_cut_getoption() function retrieves the current options set in
 * a text cutter.
 * @return 0 or a number OR'ed of #qse_cut_option_t values 
 */
int qse_cut_getoption (
	qse_cut_t* cut /**< text cutter */
);

/**
 * The qse_cut_setoption() function sets the option code.
 */
void qse_cut_setoption (
	qse_cut_t* cut, /**< text cutter */
	int        opt  /**< 0 or a number OR'ed of #qse_cut_option_t values */
);

/**
 * The qse_cut_geterrstr() gets an error string getter.
 */
qse_cut_errstr_t qse_cut_geterrstr (
	qse_cut_t*       cut    /**< text cutter */
);

/**
 * The qse_cut_seterrstr() sets an error string getter that is called to
 * compose an error message when its retrieval is requested.
 */
void qse_cut_seterrstr (
	qse_cut_t*       cut,   /**< text cutter */
	qse_cut_errstr_t errstr /**< an error string getter */
);

/**
 * The qse_cut_geterrnum() function gets the number of the last error.
 * @return the number of the last error
 */
qse_cut_errnum_t qse_cut_geterrnum (
	qse_cut_t* cut /**< text cutter */
);

/**
 * The qse_cut_geterrmsg() function gets a string describing the last error.
 * @return a pointer to an error message
 */
const qse_char_t* qse_cut_geterrmsg (
	qse_cut_t* cut /**< text cutter */
);

/**
 * The qse_cut_geterror() function gets an error number, an error location, 
 * and an error message. The information is set to the memory area pointed 
 * to by each parameter.
 */
void qse_cut_geterror (
	qse_cut_t*         cut,    /**< text cutter */
	qse_cut_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg  /**< error message */
);

/**
 * The qse_cut_seterrnum() function sets error information omitting error
 * location.
 */
void qse_cut_seterrnum (
        qse_cut_t*        cut,    /**< text cutter */
	qse_cut_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

/**
 * The qse_cut_seterrmsg() function sets error information with a customized 
 * message for a given error number.
 */
void qse_cut_seterrmsg (
	qse_cut_t*        cut,      /**< text cutter */
	qse_cut_errnum_t  errnum,   /**< error number */
	const qse_char_t* errmsg    /**< error message */
);

/**
 * The qse_cut_seterror() function sets an error number, an error location, and
 * an error message. An error string is composed of a formatting string
 * and an array of formatting parameters.
 */
void qse_cut_seterror (
	qse_cut_t*           cut,    /**< text cutter */
	qse_cut_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg  /**< array of arguments for formatting 
	                              *   an error message */
);

/**
 * The qse_cut_clear() function clears memory buffers internally allocated.
 */
void qse_cut_clear (
	qse_cut_t* cut /**< text cutter */
);

/**
 * The qse_cut_comp() function compiles a selector into an internal form.
 * @return 0 on success, -1 on error 
 */
int qse_cut_comp (
	qse_cut_t*        cut, /**< text cutter */
	const qse_char_t* str, /**< selector pointer */
	qse_size_t        len  /**< selector length */ 
);

/**
 * The qse_cut_exec() function executes the compiled commands.
 * @return 0 on success, -1 on error
 */
int qse_cut_exec (
	qse_cut_t*        cut,  /**< text cutter */
	qse_cut_io_fun_t  inf,  /**< input text stream */
	qse_cut_io_fun_t  outf  /**< output text stream */
);

#ifdef __cplusplus
}
#endif

#endif
