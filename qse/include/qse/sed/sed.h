/*
 * $Id$
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_SED_SED_H_
#define _QSE_SED_SED_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>
#include <qse/cmn/map.h>

/** @file
 * A stream editor performs text transformation on a text stream. 
 *
 * @code
 * sed = qse_sed_open ();
 * qse_sed_comp (sed);
 * qse_sed_exec (sed);
 * qse_sed_close (sed);
 * @endcode
 *
 * @example sed01.c
 * This example shows how to embed a basic stream editor.
 */

/** @class qse_sed_t
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
 * the qse_sed_errnum_t type defines error numbers.
 */
enum qse_sed_errnum_t
{
	QSE_SED_ENOERR,  /**< no error occurred */
	QSE_SED_ENOMEM,  /**< insufficient memory is available */
	QSE_SED_ECMDNR,  /**< a command is not recognized */
	QSE_SED_ECMDMS,  /**< a command is missing */
	QSE_SED_ECMDIC,  /**< a command is incomplete */
	QSE_SED_EREXIC,  /**< regular expression incomplete */
	QSE_SED_EREXBL,  /**< regular expression build error */
	QSE_SED_EREXMA,  /**< regular expression match error */
	QSE_SED_EA1PHB,  /**< address 1 prohibited */
	QSE_SED_EA2PHB,  /**< address 2 prohibited */
	QSE_SED_EASTEP,  /**< invalid step address */
	QSE_SED_ENEWLN,  /**< a new line is expected */
	QSE_SED_EBSEXP,  /**< \ is expected */
	QSE_SED_EBSDEL,  /**< \ used a delimiter */
	QSE_SED_EGBABS,  /**< garbage after \ */
	QSE_SED_ESCEXP,  /**< ; is expected */
	QSE_SED_ELABEM,  /**< label name is empty */
	QSE_SED_ELABDU,  /**< duplicate label name */
	QSE_SED_ELABNF,  /**< label not found */
	QSE_SED_EFILEM,  /**< file name is empty */
	QSE_SED_EFILIL,  /**< illegal file name */
	QSE_SED_ETSNSL,  /**< translation set not the same length*/
	QSE_SED_EGRNBA,  /**< group brackets not balanced */
	QSE_SED_EGRNTD,  /**< group nested too deeply */
	QSE_SED_EOCSDU,  /**< multiple occurrence specifiers */
	QSE_SED_EOCSZE,  /**< occurrence specifier to s is zero */
	QSE_SED_EOCSTL,  /**< occurrence specifier too large */
	QSE_SED_EIOFIL,  /**< file io error */
	QSE_SED_EIOUSR   /**< user io error */
};
typedef enum qse_sed_errnum_t qse_sed_errnum_t;

/** 
 * The qse_sed_option_t type defines various option codes for a stream editor.
 * Options can be XOR'ed with each other and be passed to a stream editor with
 * the qse_sed_setoption() function.
 */
enum qse_sed_option_t
{
	QSE_SED_STRIPLS  = (1 << 0),  /**< strip leading spaces from text */
	QSE_SED_KEEPTBS  = (1 << 1),  /**< keep an trailing backslash */
	QSE_SED_ENSURENL = (1 << 2),  /**< ensure NL at the text end */
	QSE_SED_QUIET    = (1 << 3),  /**< do not print pattern space */
	QSE_SED_CLASSIC  = (1 << 4)   /**< disable extended features */
};

/**
 * The qse_sed_io_cmd_t type defines IO command codes. The code indicates 
 * the action to take in an IO handler.
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
 * The qse_sed_io_arg_t type defines a data structure required by an IO handler.
 */
union qse_sed_io_arg_t
{
	struct
	{
		void*             handle; /* out */
		const qse_char_t* path;   /* in */
	} open;

	struct
	{
		void*             handle; /* in */
		qse_char_t*       buf;    /* out */
		qse_size_t        len;    /* in */
	} read;

	struct
	{
		void*             handle;  /* in */
		const qse_char_t* data;    /* in */
		qse_size_t        len;     /* in */
	} write;

	struct
	{
		void*             handle;  /* in */
	} close;
};
typedef union qse_sed_io_arg_t qse_sed_io_arg_t;

/** 
 * The qse_sed_io_fun_t type defines an IO handler. An IO handler is called by
 * qse_sed_execute().
 */
typedef qse_ssize_t (*qse_sed_io_fun_t) (
        qse_sed_t*        sed,
        qse_sed_io_cmd_t  cmd,
	qse_sed_io_arg_t* arg
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
 * @return A pointer to a stream editor on success, QSE_NULL on failure
 */
qse_sed_t* qse_sed_open (
	qse_mmgr_t*    mmgr, /**< a memory manager */
	qse_size_t     xtn   /**< the size of extension in bytes */
);

/**
 * The qse_sed_close() function destroys a stream editor.
 */
void qse_sed_close (
	qse_sed_t* sed /**< a stream editor */
);

/**
 * The qse_sed_getoption() function retrieves the current options set in
 * a stream editor.
 * @return 0 or a number XOR'ed of qse_sed_option_t values 
 */
int qse_sed_getoption (
	qse_sed_t* sed /**< a stream editor */
);

/**
 * The qse_sed_setoption() function sets the option code.
 */
void qse_sed_setoption (
	qse_sed_t* sed, /**< a stream editor */
	int        opt  /**< 0 or a number XOR'ed of qse_sed_option_t values */
);

/**
 * The qse_sed_geterrnum() function gets the number of the last error.
 * @return the number of the last error
 */
int qse_sed_geterrnum (
	qse_sed_t* sed /**< a stream editor */
);

/**
 * The qse_sed_geterrlin() function gets the number of the line where
 * the last error has occurred.
 * @return the line number of the last error
 */
qse_size_t qse_sed_geterrlin (
	qse_sed_t* sed /**< a stream editor */
);

/**
 * The qse_sed_geterrmsg() function gets a string describing the last error.
 * @return a pointer to an error message
 */
const qse_char_t* qse_sed_geterrmsg (
	qse_sed_t* sed /**< a stream editor */
);

/**
 * The qse_sed_geterror() function gets an error number, an error line, and 
 * an error message. The information is set to the memory area pointed to by
 * each parameter.
 */
void qse_sed_geterror (
	qse_sed_t*         sed,    /**< a stream editor */
	int*               errnum, /**< a pointer to an error number holder */
	qse_size_t*        errlin, /**< a pointer to an error line holder */
	const qse_char_t** errmsg  /**< a pointer to an error message */
);

/**
 * The qse_sed_seterror() function sets an error number, an error line, and
 * an error message. An error string is composed of a formatting string
 * and an array of formatting parameters.
 */
void qse_sed_seterror (
	qse_sed_t*        sed,    /**< a stream editor */
	int               errnum, /**< an error number */
	qse_size_t        errlin, /**< an error line */
	const qse_cstr_t* errarg  /**< a string array for formatting an error message */
);

/**
 * The qse_sed_comp() function compiles editing commands into an internal form.
 * @return 0 on success, -1 on error 
 */
int qse_sed_comp (
	qse_sed_t*        sed, /**< a stream editor */
	const qse_char_t* ptr, /**< a pointer to a string containing commands */
	qse_size_t        len  /**< the number of characters in the string */ 
);

/**
 * The qse_sed_exec() function executes the compiled commands.
 * @return 0 on success, -1 on error
 */
int qse_sed_exec (
	qse_sed_t*        sed,  /**< a stream editor */
	qse_sed_io_fun_t  inf,  /**< stream reader */
	qse_sed_io_fun_t  outf  /**< stream writer */
);

#ifdef __cplusplus
}
#endif

#endif
