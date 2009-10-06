/*
 * $Id: cut.h 287 2009-09-15 10:01:02Z baconevi $
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

#ifndef _QSE_CUT_CUT_H_
#define _QSE_CUT_CUT_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>

/** @file
 * cut utility
 */

/** @struct qse_cut_t
 */
typedef struct qse_cut_t qse_cut_t;

/**
 * the qse_cut_errnum_t type defines error numbers.
 */
enum qse_cut_errnum_t
{
	QSE_CUT_ENOERR,  /**< no error */
	QSE_CUT_ENOMEM,  /**< insufficient memory */
	QSE_CUT_ESELNV,  /**< selector not valid */
	QSE_CUT_EREXIC,  /**< regular expression '${0}' incomplete */
	QSE_CUT_EREXBL,  /**< failed to compile regular expression '${0}' */
	QSE_CUT_EREXMA,  /**< failed to match regular expression */
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
	qse_cut_t* sed,         /**< stream editor */
	qse_cut_errnum_t num    /**< an error number */
);

/** 
 * The qse_cut_option_t type defines various option codes for a stream editor.
 * Options can be OR'ed with each other and be passed to a stream editor with
 * the qse_cut_setoption() function.
 */
enum qse_cut_option_t
{
	QSE_CUT_STRIPLS   = (1 << 0), /**< strip leading spaces from text */
	QSE_CUT_KEEPTBS   = (1 << 1), /**< keep an trailing backslash */
	QSE_CUT_ENSURENL  = (1 << 2), /**< ensure NL at the text end */
	QSE_CUT_QUIET     = (1 << 3), /**< do not print pattern space */
	QSE_CUT_STRICT    = (1 << 4), /**< do strict address check */
	QSE_CUT_STARTSTEP = (1 << 5), /**< allow start~step */
	QSE_CUT_REXBOUND  = (1 << 6), /**< allow {n,m} in regular expression */
	QSE_CUT_SAMELINE  = (1 << 7), /**< allow text on the same line as c, a, i */
};
typedef enum qse_cut_option_t qse_cut_option_t;

/**
 * The qse_cut_sel_id_t type defines selector types.
 */
enum qse_cut_sel_id_t
{
	QSE_CUT_SEL_CHAR, /**< character */
	QSE_CUT_SEL_FIELD /**< field */
};
typedef enum qse_cut_sel_id_t qse_cut_sel_id_t;

/**
 * The qse_cut_depth_t type defines IDs for qse_cut_getmaxdepth() and 
 * qse_cut_setmaxdepth().
 */
enum qse_cut_depth_t
{
	QSE_CUT_DEPTH_REX_BUILD = (1 << 0),
	QSE_CUT_DEPTH_REX_MATCH = (1 << 1)
};
typedef enum qse_cut_depth_t qse_cut_depth_t;

/**
 * The qse_cut_io_cmd_t type defines IO command codes. The code indicates 
 * the action to take in an IO handler.
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
 * The qse_cut_io_arg_t type defines a data structure required by an IO handler.
 */
struct qse_cut_io_arg_t
{
	void*             handle; /**< IO handle */
};
typedef struct qse_cut_io_arg_t qse_cut_io_arg_t;

/** 
 * The qse_cut_io_fun_t type defines an IO handler. An IO handler is called by
 * qse_cut_exec().
 */
typedef qse_ssize_t (*qse_cut_io_fun_t) (
        qse_cut_t*        sed,
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
 * The qse_cut_open() function creates a stream editor object. A memory
 * manager provided is used to allocate and destory the object and any dynamic
 * data through out its lifetime. An extension area is allocated if an
 * extension size greater than 0 is specified. You can access it with the
 * qse_cut_getxtn() function and use it to store arbitrary data associated
 * with the object. See #QSE_DEFINE_COMMON_FUNCTIONS() for qse_cut_getxtn().
 * When done, you should destroy the object with the qse_cut_close() function
 * to avoid any resource leaks including memory. 
 * @return A pointer to a stream editor on success, QSE_NULL on failure
 */
qse_cut_t* qse_cut_open (
	qse_mmgr_t*    mmgr, /**< a memory manager */
	qse_size_t     xtn   /**< the size of extension in bytes */
);

/**
 * The qse_cut_close() function destroys a stream editor.
 */
void qse_cut_close (
	qse_cut_t* cut /**< stream editor */
);

/**
 * The qse_cut_getoption() function retrieves the current options set in
 * a stream editor.
 * @return 0 or a number OR'ed of #qse_cut_option_t values 
 */
int qse_cut_getoption (
	qse_cut_t* cut /**< stream editor */
);

/**
 * The qse_cut_setoption() function sets the option code.
 */
void qse_cut_setoption (
	qse_cut_t* cut, /**< stream editor */
	int        opt  /**< 0 or a number OR'ed of #qse_cut_option_t values */
);

/**
 * The qse_cut_getmaxdepth() gets the maximum processing depth.
 */
qse_size_t qse_cut_getmaxdepth (
	qse_cut_t*      cut, /**< stream editor */
	qse_cut_depth_t id   /**< one of qse_cut_depth_t values */
);

/**
 * The qse_cut_setmaxdepth() sets the maximum processing depth.
 */
void qse_cut_setmaxdepth (
	qse_cut_t* cut,  /**< stream editor */
	int        ids,  /**< 0 or a number OR'ed of #qse_cut_depth_t values */
	qse_size_t depth /**< maximum depth level */
);

/**
 * The qse_cut_geterrstr() gets an error string getter.
 */
qse_cut_errstr_t qse_cut_geterrstr (
	qse_cut_t*       cut    /**< stream editor */
);

/**
 * The qse_cut_seterrstr() sets an error string getter that is called to
 * compose an error message when its retrieval is requested.
 *
 * Here is an example of changing the formatting string for the #QSE_CUT_ECMDNR 
 * error.
 * @code
 * qse_cut_errstr_t orgerrstr;
 *
 * const qse_char_t* myerrstr (qse_cut_t* cut, qse_cut_errnum_t num)
 * {
 *   if (num == QSE_CUT_ECMDNR) return QSE_T("unrecognized command ${0}");
 *   return orgerrstr (cut, num);
 * }
 * int main ()
 * {
 *    qse_cut_t* cut;
 *    ...
 *    orgerrstr = qse_cut_geterrstr (cut);
 *    qse_cut_seterrstr (cut, myerrstr);
 *    ...
 * }
 * @endcode
 */
void qse_cut_seterrstr (
	qse_cut_t*       cut,   /**< stream editor */
	qse_cut_errstr_t errstr /**< an error string getter */
);

/**
 * The qse_cut_geterrnum() function gets the number of the last error.
 * @return the number of the last error
 */
qse_cut_errnum_t qse_cut_geterrnum (
	qse_cut_t* cut /**< stream editor */
);

/**
 * The qse_cut_geterrmsg() function gets a string describing the last error.
 * @return a pointer to an error message
 */
const qse_char_t* qse_cut_geterrmsg (
	qse_cut_t* cut /**< stream editor */
);

/**
 * The qse_cut_geterror() function gets an error number, an error location, 
 * and an error message. The information is set to the memory area pointed 
 * to by each parameter.
 */
void qse_cut_geterror (
	qse_cut_t*         cut,    /**< stream editor */
	qse_cut_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg  /**< error message */
);

/**
 * The qse_cut_seterrnum() function sets error information omitting error
 * location.
 */
void qse_cut_seterrnum (
        qse_cut_t*        cut,    /**< stream editor */
	qse_cut_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

/**
 * The qse_cut_seterrmsg() function sets error information with a customized 
 * message for a given error number.
 */
void qse_cut_seterrmsg (
        qse_cut_t*        cut,      /**< stream editor */
	qse_cut_errnum_t  errnum,   /**< error number */
        const qse_char_t* errmsg    /**< error message */
);

/**
 * The qse_cut_seterror() function sets an error number, an error location, and
 * an error message. An error string is composed of a formatting string
 * and an array of formatting parameters.
 */
void qse_cut_seterror (
	qse_cut_t*           cut,    /**< stream editor */
	qse_cut_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg  /**< array of arguments for formatting 
	                              *   an error message */
);

/**
 * The qse_cut_comp() function compiles a selector into an internal form.
 * @return 0 on success, -1 on error 
 */
int qse_cut_comp (
	qse_cut_t*        cut, /**< stream editor */
	qse_cut_sel_id_t  sel, /**< initial selector type */
	const qse_char_t* ptr, /**< pointer to a string containing commands */
	qse_size_t        len  /**< the number of characters in the string */ 
);

/**
 * The qse_cut_exec() function executes the compiled commands.
 * @return 0 on success, -1 on error
 */
int qse_cut_exec (
	qse_cut_t*        cut,  /**< stream editor */
	qse_cut_io_fun_t  inf,  /**< stream reader */
	qse_cut_io_fun_t  outf  /**< stream writer */
);

#ifdef __cplusplus
}
#endif

#endif
