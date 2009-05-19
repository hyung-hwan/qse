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

#ifndef _QSE_UTL_SED_H_
#define _QSE_UTL_SED_H_

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

/**
 * defines error numbers 
 */
enum qse_sed_errnum_t
{
	QSE_SED_ENOERR,  /**< no error occurred */
	QSE_SED_ENOMEM,  /**< insufficient memory is available */
	QSE_SED_ETMTXT,  /**< too much text */
	QSE_SED_ECMDNR,  /**< a command is not recognized */
	QSE_SED_ECMDMS,  /**< a command is missing */
	QSE_SED_ECMDGB,  /**< command garbled */
	QSE_SED_EREXBL,  /**< regular expression build error */
	QSE_SED_EREXMA,  /**< regular expression match error */
	QSE_SED_EA1PHB,  /**< address 1 prohibited */
	QSE_SED_EA2PHB,  /**< address 2 prohibited */
	QSE_SED_ENEWLN,  /**< a new line is expected */
	QSE_SED_EBSEXP,  /**< \ is expected */
	QSE_SED_EBSDEL,  /**< \ used a delimiter */
	QSE_SED_EGBABS,  /**< garbage after \ */
	QSE_SED_ESCEXP,  /**< ; is expected */
	QSE_SED_ELABTL,  /**< label too long */
	QSE_SED_ELABEM,  /**< label name is empty */
	QSE_SED_ELABDU,  /**< duplicate label name */
	QSE_SED_ELABNF,  /**< label not found */
	QSE_SED_EFILEM,  /**< file name is empty */
	QSE_SED_EFILIL,  /**< illegal file name */
	QSE_SED_ENOTRM,  /**< not terminated properly */
	QSE_SED_ETSNSL,  /**< translation set not the same length*/
	QSE_SED_EGRNBA,  /**< group brackets not balanced */
	QSE_SED_EGRNTD,  /**< group nested too deeply */
	QSE_SED_EOCSDU,  /**< multiple occurrence specifiers */
	QSE_SED_EOCSZE,  /**< occurrence specifier to s is zero */
	QSE_SED_EOCSTL,  /**< occurrence specifier too large */
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

typedef struct qse_sed_t qse_sed_t;

/**
 * The qse_sed_cmd_t type represents a compiled form of a stream editor
 * command. The details are hidden.
 */
typedef struct qse_sed_cmd_t qse_sed_cmd_t; 

/**
 * The qse_sed_iof_t type defines an IO handler. An IO handler is called by
 * qse_sed_execute().
 */
typedef qse_ssize_t (*qse_sed_iof_t) (
        qse_sed_t*        sed,
        qse_sed_io_cmd_t  cmd,
	qse_sed_io_arg_t* arg
);

/** 
 * The qse_sed_t type defines a stream editor 
 */
struct qse_sed_t
{
	QSE_DEFINE_COMMON_FIELDS (sed)

	qse_sed_errnum_t errnum; /**< stores an error number */
	int option;              /**< stores options */

	/** source text pointers */
	struct
	{
		const qse_char_t* ptr; /**< beginning of the source text */
		const qse_char_t* end; /**< end of the source text */
		const qse_char_t* cur; /**< current source text pointer */
	} src;

	qse_str_t rexbuf; /**< temporary regular expression buffer */

	/* command array */
	struct
	{
		qse_sed_cmd_t* buf;
		qse_sed_cmd_t* end;
		qse_sed_cmd_t* cur;
	} cmd;

	/** a table storing labels seen */
	qse_map_t labs; 

	struct
	{
		/** current level of command group nesting */
		int level;
		/** keeps track of the begining of a command group */
		qse_sed_cmd_t* cmd[128];
	} grp;

	/** data for execution */
	struct
	{
		/** data needed for output streams and files */
		struct
		{
			qse_sed_iof_t f; /**< an output handler */
			qse_sed_io_arg_t arg; /**< output handling data */

			qse_char_t buf[2048];
			qse_size_t len;
			int        eof;

			/*****************************************************/
			/* the following two fields are very tightly-coupled.
			 * don't make any partial changes */
			qse_map_t  files;
			qse_sed_t* files_ext;
			/*****************************************************/
		} out;

		/** data needed for input streams */
		struct
		{
			qse_sed_iof_t f; /**< an input handler */
			qse_sed_io_arg_t arg; /**< input handling data */

			qse_char_t xbuf[1]; /**< a read-ahead buffer */
			int xbuf_len; /**< data length in the buffer */

			qse_char_t buf[2048]; /**< input buffer */
			qse_size_t len; /**< data length in the buffer */
			qse_size_t pos; /**< current position in the buffer */
			int        eof; /**< EOF indicator */

			qse_str_t line; /**< pattern space */
			qse_size_t num; /**< current line number */
		} in;

		/** text buffers */
		struct
		{
			qse_lda_t appended;
			qse_str_t read;
			qse_str_t held;
			qse_str_t subst;
		} txt;

		/** indicates if a successful substitution has been made 
		 *  since the last read on the input stream.
		 */
		int subst_done;
	} e;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (sed)

/**
 * The qse_sed_open() function creates a stream editor.
 * @return A pointer to a stream editor on success, QSE_NULL on a failure
 */
qse_sed_t* qse_sed_open (
	qse_mmgr_t*    mmgr, /**< a memory manager */
	qse_size_t     xtn   /**< the size of extension in bytes */
);

/**
 * The qse_sed_close() function destroyes a stream editor.
 */
void qse_sed_close (
	qse_sed_t* sed /**< a stream editor */
);

/**
 * The qse_sed_init() function initializes a stream editor.
 */
qse_sed_t* qse_sed_init (
	qse_sed_t*     sed, /**< a stream editor */
	qse_mmgr_t*    mmgr /**< a memory manager */
);

/**
 * The qse_sed_init() function finalizes a stream editor.
 */
void qse_sed_fini (
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
 * The qse_sed_geterrmsg() function retrieves an error message
 * @return a pointer to a string describing an error occurred 
 */
const qse_char_t* qse_sed_geterrmsg (
	qse_sed_t* sed /**< a stream editor */
);

/**
 * The qse_sed_comp() function compiles stream editor commands into an 
 * internal form.
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
	qse_sed_t*    sed, /**< a stream editor */
	qse_sed_iof_t in,  /**< stream reader */
	qse_sed_iof_t out  /**< stream writer */
);

#ifdef __cplusplus
}
#endif

#endif
