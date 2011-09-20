/*
 * $Id: sed.h 569 2011-09-19 06:51:02Z hyunghwan.chung $
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

#ifndef _QSE_SED_SED_H_
#define _QSE_SED_SED_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file defines data types and functions to use for creating a custom
 * stream editor commonly available on many platforms. A stream editor is 
 * a non-interactive text editing tool that reads text from an input stream, 
 * stores it to pattern space, manipulates the pattern space by applying a set
 * of editing commands, and writes the pattern space to an output stream. 
 * Typically, the input and output streams are a console or a file.
 *
 * @code
 * sed = qse_sed_open ();
 * qse_sed_comp (sed);
 * qse_sed_exec (sed);
 * qse_sed_close (sed);
 * @endcode
 *
 * @todo 
 * - enhance execution of the l(ell) command - consider adding a callback
 *
 */

/**
 * @example sed.c
 * This example shows how to write a basic stream editor.
 */

/** @struct qse_sed_t
 * The qse_sed_t type defines a stream editor. The structural details are 
 * hidden as it is a relatively complex data type and fragile to external
 * changes. To use a stream editor, you typically can:
 *
 * - create a stream editor object with qse_sed_open().
 * - compile stream editor commands with qse_sed_comp().
 * - execute them over input and output streams with qse_sed_exec().
 * - destroy it with qse_sed_close() when done.
 *
 * The input and output streams needed by qse_sed_exec() are implemented in
 * the form of callback functions. You should implement two functions 
 * conforming to the ::qse_sed_io_fun_t type.
 */
typedef struct qse_sed_t qse_sed_t;

/** 
 * The qse_sed_loc_t defines a structure to store location information.
 */
struct qse_sed_loc_t
{
	qse_size_t line; /**< line  */
	qse_size_t colm; /**< column */
};
typedef struct qse_sed_loc_t qse_sed_loc_t;

/**
 * the qse_sed_errnum_t type defines error numbers.
 */
enum qse_sed_errnum_t
{
	QSE_SED_ENOERR,  /**< no error */
	QSE_SED_ENOMEM,  /**< insufficient memory */
	QSE_SED_EINVAL,  /**< invalid parameter or data */
	QSE_SED_ECMDNR,  /**< command '${0}' not recognized */
	QSE_SED_ECMDMS,  /**< command code missing */
	QSE_SED_ECMDIC,  /**< command '${0}' incomplete */
	QSE_SED_EREXIC,  /**< regular expression '${0}' incomplete */
	QSE_SED_EREXBL,  /**< failed to compile regular expression '${0}' */
	QSE_SED_EREXMA,  /**< failed to match regular expression */
	QSE_SED_EA1PHB,  /**< address 1 prohibited for '${0}' */
	QSE_SED_EA1MOI,  /**< address 1 missing or invalid */
	QSE_SED_EA2PHB,  /**< address 2 prohibited */
	QSE_SED_EA2MOI,  /**< address 2 missing or invalid */
	QSE_SED_ENEWLN,  /**< newline expected */
	QSE_SED_EBSEXP,  /**< backslash expected */
	QSE_SED_EBSDEL,  /**< backslash used as delimiter */
	QSE_SED_EGBABS,  /**< garbage after backslash */
	QSE_SED_ESCEXP,  /**< semicolon expected */
	QSE_SED_ELABEM,  /**< empty label name */
	QSE_SED_ELABDU,  /**< duplicate label name '${0}' */
	QSE_SED_ELABNF,  /**< label '${0}' not found */
	QSE_SED_EFILEM,  /**< empty file name */
	QSE_SED_EFILIL,  /**< illegal file name */
	QSE_SED_ETSNSL,  /**< strings in translation set not the same length*/
	QSE_SED_EGRNBA,  /**< group brackets not balanced */
	QSE_SED_EGRNTD,  /**< group nesting too deep */
	QSE_SED_EOCSDU,  /**< multiple occurrence specifiers */
	QSE_SED_EOCSZE,  /**< occurrence specifier zero */
	QSE_SED_EOCSTL,  /**< occurrence specifier too large */
	QSE_SED_ENPREX,  /**< no previous regular expression */
	QSE_SED_EIOFIL,  /**< io error with file '${0}'*/
	QSE_SED_EIOUSR   /**< error returned by user io handler */
};
typedef enum qse_sed_errnum_t qse_sed_errnum_t;

/**
 * The qse_sed_errstr_t type defines an error string getter. It should return 
 * an error formatting string for an error number requested. A new string
 * should contain the same number of positional parameters (${X}) as in the
 * default error formatting string. You can set a new getter into a stream
 * editor with the qse_sed_seterrstr() function to customize an error string.
 */
typedef const qse_char_t* (*qse_sed_errstr_t) (
	qse_sed_t*       sed,   /**< stream editor */
	qse_sed_errnum_t num    /**< an error number */
);

/** 
 * The qse_sed_option_t type defines various option codes for a stream editor.
 * Options can be OR'ed with each other and be passed to a stream editor with
 * the qse_sed_setoption() function.
 */
enum qse_sed_option_t
{
	QSE_SED_STRIPLS      = (1 << 0), /**< strip leading spaces from text */
	QSE_SED_KEEPTBS      = (1 << 1), /**< keep an trailing backslash */
	QSE_SED_ENSURENL     = (1 << 2), /**< ensure NL at the text end */
	QSE_SED_QUIET        = (1 << 3), /**< do not print pattern space */
	QSE_SED_STRICT       = (1 << 4), /**< do strict address check */
	QSE_SED_STARTSTEP    = (1 << 5), /**< allow start~step */
	QSE_SED_ZEROA1       = (1 << 6), /**< allow 0,/regex/ */
	QSE_SED_SAMELINE     = (1 << 7), /**< allow text on the same line as c, a, i */
	QSE_SED_EXTENDEDREX  = (1 << 8), /**< use extended regex */
	QSE_SED_NONSTDEXTREX = (1 << 9)  /**< enable non-standard extensions to regex */
};
typedef enum qse_sed_option_t qse_sed_option_t;

/**
 * The qse_sed_io_cmd_t type defines I/O command codes. The code indicates 
 * the action to take in an I/O handler.
 */
enum qse_sed_io_cmd_t
{
	QSE_SED_IO_OPEN  = 0,
	QSE_SED_IO_CLOSE = 1,
	QSE_SED_IO_READ  = 2,
	QSE_SED_IO_WRITE = 3
};
typedef enum qse_sed_io_cmd_t qse_sed_io_cmd_t;

/**
 * The qse_sed_io_arg_t type defines a data structure required by 
 * an I/O handler.
 */
struct qse_sed_io_arg_t
{
	void*             handle; /**< I/O handle */
	const qse_char_t* path;   /**< file path. QSE_NULL for a console */
};
typedef struct qse_sed_io_arg_t qse_sed_io_arg_t;

/** 
 * The qse_sed_io_fun_t type defines an I/O handler. I/O handlers are called by
 * qse_sed_exec().
 */
typedef qse_ssize_t (*qse_sed_io_fun_t) (
	qse_sed_t*        sed,
	qse_sed_io_cmd_t  cmd,
	qse_sed_io_arg_t* arg,
	qse_char_t*       data,
	qse_size_t        count
);

/**
 * The qse_sed_lformatter_t type defines a text formatter for the 'l' command.
 */
typedef int (*qse_sed_lformatter_t) (
	qse_sed_t*        sed,
	const qse_char_t* str,
	qse_size_t        len,
	int (*cwriter) (qse_sed_t*, qse_char_t)
);

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (sed)

/**
 * The qse_sed_open() function creates a stream editor object. A memory
 * manager provided is used to allocate and destory the object and any dynamic
 * data through out its lifetime. An extension area is allocated if an
 * extension size greater than 0 is specified. You can access it with the
 * qse_sed_getxtn() function and use it to store arbitrary data associated
 * with the object. See #QSE_DEFINE_COMMON_FUNCTIONS() for qse_sed_getxtn().
 * When done, you should destroy the object with the qse_sed_close() function
 * to avoid any resource leaks including memory. 
 * @return pointer to a stream editor on success, QSE_NULL on failure
 */
qse_sed_t* qse_sed_open (
	qse_mmgr_t*    mmgr,   /**< memory manager */
	qse_size_t     xtnsize /**< extension size in bytes */
);

/**
 * The qse_sed_close() function destroys a stream editor.
 */
void qse_sed_close (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_getoption() function retrieves the current options set in
 * a stream editor.
 * @return 0 or a number OR'ed of #qse_sed_option_t values 
 */
int qse_sed_getoption (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_setoption() function sets the option code.
 */
void qse_sed_setoption (
	qse_sed_t* sed, /**< stream editor */
	int        opt  /**< 0 or a number OR'ed of #qse_sed_option_t values */
);

/**
 * The qse_sed_geterrstr() gets an error string getter.
 */
qse_sed_errstr_t qse_sed_geterrstr (
	qse_sed_t*       sed    /**< stream editor */
);

/**
 * The qse_sed_seterrstr() sets an error string getter that is called to
 * compose an error message when its retrieval is requested.
 *
 * Here is an example of changing the formatting string for the #QSE_SED_ECMDNR 
 * error.
 * @code
 * qse_sed_errstr_t orgerrstr;
 *
 * const qse_char_t* myerrstr (qse_sed_t* sed, qse_sed_errnum_t num)
 * {
 *   if (num == QSE_SED_ECMDNR) return QSE_T("unrecognized command ${0}");
 *   return orgerrstr (sed, num);
 * }
 * int main ()
 * {
 *    qse_sed_t* sed;
 *    ...
 *    orgerrstr = qse_sed_geterrstr (sed);
 *    qse_sed_seterrstr (sed, myerrstr);
 *    ...
 * }
 * @endcode
 */
void qse_sed_seterrstr (
	qse_sed_t*       sed,   /**< stream editor */
	qse_sed_errstr_t errstr /**< an error string getter */
);

/**
 * The qse_sed_geterrnum() function gets the number of the last error.
 * @return the number of the last error
 */
qse_sed_errnum_t qse_sed_geterrnum (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_geterrloc() function gets the location where the last error 
 * has occurred.
 * @return error location
 */
const qse_sed_loc_t* qse_sed_geterrloc (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_geterrmsg() function gets a string describing the last error.
 * @return a pointer to an error message
 */
const qse_char_t* qse_sed_geterrmsg (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_geterror() function gets an error number, an error location, 
 * and an error message. The information is set to the memory area pointed 
 * to by each parameter.
 */
void qse_sed_geterror (
	qse_sed_t*         sed,    /**< stream editor */
	qse_sed_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg, /**< error message */
	qse_sed_loc_t*     errloc  /**< error location */
);

/**
 * The qse_sed_seterrnum() function sets error information omitting error
 * location.
 */
void qse_sed_seterrnum (
	qse_sed_t*        sed,    /**< stream editor */
	qse_sed_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

/**
 * The qse_sed_seterrmsg() function sets error information with a customized 
 * message for a given error number.
 */
void qse_sed_seterrmsg (
	qse_sed_t*        sed,      /**< stream editor */
	qse_sed_errnum_t  errnum,   /**< error number */
	const qse_char_t* errmsg,   /**< error message */
	const qse_sed_loc_t* errloc /**< error location */
);

/**
 * The qse_sed_seterror() function sets an error number, an error location, and
 * an error message. An error string is composed of a formatting string
 * and an array of formatting parameters.
 */
void qse_sed_seterror (
	qse_sed_t*           sed,    /**< stream editor */
	qse_sed_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< array of arguments for formatting 
	                              *   an error message */
	const qse_sed_loc_t* errloc  /**< error location */
);

/**
 * The qse_sed_comp() function compiles editing commands into an internal form.
 * @return 0 on success, -1 on error 
 */
int qse_sed_comp (
	qse_sed_t*        sed, /**< stream editor */
	const qse_char_t* ptr, /**< pointer to a string containing commands */
	qse_size_t        len  /**< the number of characters in the string */ 
);

/**
 * The qse_sed_exec() function executes the compiled commands.
 * @return 0 on success, -1 on error
 */
int qse_sed_exec (
	qse_sed_t*        sed,  /**< stream editor */
	qse_sed_io_fun_t  inf,  /**< stream reader */
	qse_sed_io_fun_t  outf  /**< stream writer */
);

/**
 * The qse_sed_getlformatter() function gets the text formatter for the 'l'
 * command.
 */
qse_sed_lformatter_t qse_sed_getlformatter (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_setlformatter() function sets the text formatter for the 'l'
 * command. The text formatter must output the text with a character writer
 * provided and return -1 on failure and 0 on success.
 */
void qse_sed_setlformatter (
	qse_sed_t*           sed,       /**< stream editor */
	qse_sed_lformatter_t lformatter /**< text formatter */
);

/**
 * The qse_sed_getlinnum() function gets the current input line number.
 * @return current input line number
 */
qse_size_t qse_sed_getlinnum (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_setlinnum() function changes the current input line number.
 */
void qse_sed_setlinnum (
	qse_sed_t* sed,   /**< stream editor */
	qse_size_t num    /**< a line number */
);

#ifdef __cplusplus
}
#endif

#endif
