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
 * conforming to the ::qse_sed_io_impl_t type.
 */
typedef struct qse_sed_t qse_sed_t;

typedef struct qse_sed_loc_t qse_sed_loc_t;
typedef struct qse_sed_adr_t qse_sed_adr_t; 
typedef struct qse_sed_cmd_t qse_sed_cmd_t;

/** 
 * The qse_sed_loc_t defines a structure to store location information.
 */
struct qse_sed_loc_t
{
	qse_size_t line; /**< line  */
	qse_size_t colm; /**< column */
};

struct qse_sed_adr_t
{
	enum
	{
		QSE_SED_ADR_NONE,     /* no address */
		QSE_SED_ADR_DOL,      /* $ - last line */
		QSE_SED_ADR_LINE,     /* specified line */
		QSE_SED_ADR_REX,      /* lines matching regular expression */
		QSE_SED_ADR_STEP,     /* line steps - only in the second address */
		QSE_SED_ADR_RELLINE,  /* relative line - only in second address */
		QSE_SED_ADR_RELLINEM  /* relative line in the multiples - only in second address */
	} type;

	union 
	{
		qse_size_t lno;
		void*      rex;
	} u;
};

typedef struct qse_sed_cut_sel_t qse_sed_cut_sel_t;

struct qse_sed_cut_sel_t
{
	qse_size_t len;

	struct
	{
		enum
		{
			QSE_SED_CUT_SEL_CHAR = QSE_T('c'),
			QSE_SED_CUT_SEL_FIELD = QSE_T('f')
		} id;
		qse_size_t start;
		qse_size_t end;
	} range[128];

	qse_sed_cut_sel_t* next;
};

#define QSE_SED_CMD_NOOP            QSE_T('\0')
#define QSE_SED_CMD_QUIT            QSE_T('q')
#define QSE_SED_CMD_QUIT_QUIET      QSE_T('Q')
#define QSE_SED_CMD_APPEND          QSE_T('a')
#define QSE_SED_CMD_INSERT          QSE_T('i')
#define QSE_SED_CMD_CHANGE          QSE_T('c')
#define QSE_SED_CMD_DELETE          QSE_T('d')
#define QSE_SED_CMD_DELETE_FIRSTLN  QSE_T('D')
#define QSE_SED_CMD_PRINT_LNNUM     QSE_T('=')
#define QSE_SED_CMD_PRINT           QSE_T('p')
#define QSE_SED_CMD_PRINT_FIRSTLN   QSE_T('P')
#define QSE_SED_CMD_PRINT_CLEARLY   QSE_T('l')
#define QSE_SED_CMD_HOLD            QSE_T('h')
#define QSE_SED_CMD_HOLD_APPEND     QSE_T('H')
#define QSE_SED_CMD_RELEASE         QSE_T('g')
#define QSE_SED_CMD_RELEASE_APPEND  QSE_T('G')
#define QSE_SED_CMD_EXCHANGE        QSE_T('x') 
#define QSE_SED_CMD_NEXT            QSE_T('n')
#define QSE_SED_CMD_NEXT_APPEND     QSE_T('N')
#define QSE_SED_CMD_READ_FILE       QSE_T('r')
#define QSE_SED_CMD_READ_FILELN     QSE_T('R')
#define QSE_SED_CMD_WRITE_FILE      QSE_T('w')
#define QSE_SED_CMD_WRITE_FILELN    QSE_T('W')
#define QSE_SED_CMD_BRANCH          QSE_T('b') 
#define QSE_SED_CMD_BRANCH_COND     QSE_T('t')
#define QSE_SED_CMD_SUBSTITUTE      QSE_T('s')
#define QSE_SED_CMD_TRANSLATE       QSE_T('y')
#define QSE_SED_CMD_CLEAR_PATTERN   QSE_T('z')
#define QSE_SED_CMD_CUT             QSE_T('C')

struct qse_sed_cmd_t
{
	qse_char_t type;

	const qse_char_t* lid;
	qse_sed_loc_t     loc;

	int negated;

	qse_sed_adr_t a1; /* optional start address */
	qse_sed_adr_t a2; /* optional end address */

	union
	{
		/* text for the a, i, c commands */
		qse_xstr_t text;  

		/* file name for r, w, R, W */
		qse_xstr_t file;

		/* data for the s command */
		struct
		{
			void* rex; /* regular expression */
			qse_xstr_t rpl;  /* replacement */

			/* flags */
			qse_xstr_t file; /* file name for w */
			unsigned short occ;
			unsigned short g: 1; /* global */
			unsigned short p: 1; /* print */
			unsigned short i: 1; /* case insensitive */
		} subst;

		/* translation set for the y command */
		qse_xstr_t transet;

		/* branch target for b and t */
		struct
		{
			qse_xstr_t label;
			qse_sed_cmd_t* target;
		} branch;

		/* cut command information */
		struct
		{
			qse_sed_cut_sel_t* fb;/**< points to the first block */
			qse_sed_cut_sel_t* lb; /**< points to the last block */

			qse_char_t         delim[2]; /**< input/output field delimiters */
			unsigned short     w: 1; /* whitespace for input delimiters. ignore delim[0]. */
			unsigned short     f: 1; /* fold delimiters */
			unsigned short     d: 1; /* delete if not delimited */

			qse_size_t         count;
			qse_size_t         fcount;
			qse_size_t         ccount;
		} cut;
	} u;	

	struct
	{
		int a1_matched;
		qse_size_t a1_match_line;

		int c_ready;

		/* points to the next command for fast traversal and 
		 * fast random jumps */
		qse_sed_cmd_t* next; 
	} state;
};

/**
 * the qse_sed_errnum_t type defines error numbers.
 */
enum qse_sed_errnum_t
{
	QSE_SED_ENOERR,  /**< no error */
	QSE_SED_EOTHER, /**< not implemented */
	QSE_SED_ENOIMPL, /**< not implemented */
	QSE_SED_ESYSERR, /**< not implemented */
	QSE_SED_EINTERN, /**< internal error */

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
	QSE_SED_ECSLNV,  /**< cut selector not valid */
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
	const qse_sed_t* sed,   /**< stream editor */
	qse_sed_errnum_t num    /**< an error number */
);

/**
 * The qse_sed_opt_t type defines various option types.
 */
enum qse_sed_opt_t
{
	QSE_SED_TRAIT,      /**< trait */
	QSE_SED_TRACER,     /**< tracer hook */
	QSE_SED_LFORMATTER, /**< formatter for the 'l' command */

	QSE_SED_DEPTH_REX_BUILD,
	QSE_SED_DEPTH_REX_MATCH
};
typedef enum qse_sed_opt_t qse_sed_opt_t;

/** 
 * The qse_sed_trait_t type defines various trait codes for a stream editor.
 * Options can be OR'ed with each other and be passed to a stream editor with
 * the qse_sed_setopt() function.
 */
enum qse_sed_trait_t
{
	QSE_SED_STRIPLS      = (1 << 0), /**< strip leading spaces from text */
	QSE_SED_KEEPTBS      = (1 << 1), /**< keep an trailing backslash */
	QSE_SED_ENSURENL     = (1 << 2), /**< ensure NL at the text end */
	QSE_SED_QUIET        = (1 << 3), /**< do not print pattern space */
	QSE_SED_STRICT       = (1 << 4), /**< do strict address and label check */
	QSE_SED_EXTENDEDADR  = (1 << 5), /**< allow start~step , addr1,+line, addr1,~line */
	QSE_SED_SAMELINE     = (1 << 7), /**< allow text on the same line as c, a, i */
	QSE_SED_EXTENDEDREX  = (1 << 8), /**< use extended regex */
	QSE_SED_NONSTDEXTREX = (1 << 9)  /**< enable non-standard extensions to regex */
};
typedef enum qse_sed_trait_t qse_sed_trait_t;

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
 * The qse_sed_io_impl_t type defines an I/O handler. I/O handlers are called by
 * qse_sed_exec().
 */
typedef qse_ssize_t (*qse_sed_io_impl_t) (
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

/**
 * The qse_sed_ecb_close_t type defines the callback function
 * called when an sed object is closed.
 */
typedef void (*qse_sed_ecb_close_t) (
	qse_sed_t* sed  /**< sed */
);

/**
 * The qse_sed_ecb_t type defines an event callback set.
 * You can register a callback function set with
 * qse_sed_pushecb().  The callback functions in the registered
 * set are called in the reverse order of registration.
 */
typedef struct qse_sed_ecb_t qse_sed_ecb_t;
struct qse_sed_ecb_t
{
	/**
	 * called by qse_sed_close().
	 */
	qse_sed_ecb_close_t close;

	/* internal use only. don't touch this field */
	qse_sed_ecb_t* next;
};

enum qse_sed_tracer_op_t
{
	QSE_SED_TRACER_READ,
	QSE_SED_TRACER_WRITE,
	QSE_SED_TRACER_MATCH,
	QSE_SED_TRACER_EXEC
};
typedef enum qse_sed_tracer_op_t qse_sed_tracer_op_t;

typedef void (*qse_sed_tracer_t) (
	qse_sed_t*           sed,
	qse_sed_tracer_op_t  op,
	const qse_sed_cmd_t* cmd
);

/**
 * The qse_sed_space_t type defines the types of
 * sed bufferspaces. 
 */
enum qse_sed_space_t
{
	QSE_SED_SPACE_HOLD,   /**< hold space */
	QSE_SED_SPACE_PATTERN /**< pattern space */
};
typedef enum qse_sed_space_t qse_sed_space_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_sed_open() function creates a stream editor object. A memory
 * manager provided is used to allocate and destory the object and any dynamic
 * data through out its lifetime. An extension area is allocated if an
 * extension size greater than 0 is specified. You can access it with the
 * qse_sed_getxtn() function and use it to store arbitrary data associated
 * with the object.  When done, you should destroy the object with the 
 * qse_sed_close() function to avoid any resource leaks including memory. 
 * @return pointer to a stream editor on success, QSE_NULL on failure
 */
QSE_EXPORT qse_sed_t* qse_sed_open (
	qse_mmgr_t*    mmgr,   /**< memory manager */
	qse_size_t     xtnsize /**< extension size in bytes */
);

/**
 * The qse_sed_close() function destroys a stream editor.
 */
QSE_EXPORT void qse_sed_close (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The  qse_sed_getmmgr() function returns the memory
 * manager used in qse_sed_open().
 */
QSE_EXPORT qse_mmgr_t* qse_sed_getmmgr (
	qse_sed_t* sed
); 

/**
 * The qse_sed_getxtn() function returns the pointer
 * to the beginning of the extension area created with 
 * qse_sed_open().
 */
QSE_EXPORT void* qse_sed_getxtn (
	qse_sed_t* sed
);


/**
 * The qse_sed_getopt() function gets the value of an option
 * specified by \a id into the buffer pointed to by \a value.
 *
 * The \a value field is dependent on \a id:
 *  - #QSE_SED_TRAIT - int*, 0 or bitwised-ORed of #qse_sed_trait_t values
 *  - #QSE_SED_TRACER - qse_sed_tracer_t*
 *  - #QSE_SED_LFORMATTER - qse_sed_lformatter_t*
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_sed_getopt (
	qse_sed_t*    sed,
	qse_sed_opt_t id,
	void*         value
);

/**
 * The qse_sed_setopt() function sets the value of an option 
 * specified by \a id to the value pointed to by \a value.
 *
 * The \a value field is dependent on \a id:
 *  - #QSE_SED_TRAIT - const int*, 0 or bitwised-ORed of #qse_sed_trait_t values
 *  - #QSE_SED_TRACER - qse_sed_tracer_t
 *  - #QSE_SED_LFORMATTER - qse_sed_lformatter_t
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_sed_setopt (
	qse_sed_t*    sed,
	qse_sed_opt_t id,
	const void*   value
);

/**
 * The qse_sed_geterrstr() gets an error string getter.
 */
QSE_EXPORT qse_sed_errstr_t qse_sed_geterrstr (
	const qse_sed_t* sed    /**< stream editor */
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
QSE_EXPORT void qse_sed_seterrstr (
	qse_sed_t*       sed,   /**< stream editor */
	qse_sed_errstr_t errstr /**< an error string getter */
);

/**
 * The qse_sed_geterrnum() function gets the number of the last error.
 * @return error number
 */
QSE_EXPORT qse_sed_errnum_t qse_sed_geterrnum (
	const qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_geterrloc() function gets the location where the last error 
 * has occurred.
 * @return error location
 */
QSE_EXPORT const qse_sed_loc_t* qse_sed_geterrloc (
	const qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_geterrmsg() function gets a string describing the last error.
 * @return error message pointer
 */
QSE_EXPORT const qse_char_t* qse_sed_geterrmsg (
	const qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_geterror() function gets an error number, an error location, 
 * and an error message. The information is set to the memory area pointed 
 * to by each parameter.
 */
QSE_EXPORT void qse_sed_geterror (
	const qse_sed_t*   sed,    /**< stream editor */
	qse_sed_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg, /**< error message */
	qse_sed_loc_t*     errloc  /**< error location */
);

/**
 * The qse_sed_seterrnum() function sets error information omitting error
 * location.
 */
QSE_EXPORT void qse_sed_seterrnum (
	qse_sed_t*        sed,    /**< stream editor */
	qse_sed_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

/**
 * The qse_sed_seterrmsg() function sets error information with a customized 
 * message for a given error number.
 */
QSE_EXPORT void qse_sed_seterrmsg (
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
QSE_EXPORT void qse_sed_seterror (
	qse_sed_t*           sed,    /**< stream editor */
	qse_sed_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< array of arguments for formatting 
	                              *   an error message */
	const qse_sed_loc_t* errloc  /**< error location */
);

/**
 * The qse_sed_popecb() function pops an sed event callback set
 * and returns the pointer to it. If no callback set can be popped,
 * it returns #QSE_NULL.
 */
QSE_EXPORT qse_sed_ecb_t* qse_sed_popecb (
	qse_sed_t* sed /**< sed */
);

/**
 * The qse_sed_pushecb() function register a runtime callback set.
 */
QSE_EXPORT void qse_sed_pushecb (
	qse_sed_t*     sed, /**< sed */
	qse_sed_ecb_t* ecb  /**< callback set */
);

/**
 * The qse_sed_comp() function compiles editing commands into an internal form.
 * @return 0 on success, -1 on error 
 */
QSE_EXPORT int qse_sed_comp (
	qse_sed_t*        sed, /**< stream editor */
	qse_sed_io_impl_t  inf  /**< script stream reader */
);

/**
 * The qse_sed_exec() function executes the compiled commands.
 * @return 0 on success, -1 on error
 */
QSE_EXPORT int qse_sed_exec (
	qse_sed_t*        sed,  /**< stream editor */
	qse_sed_io_impl_t  inf,  /**< stream reader */
	qse_sed_io_impl_t  outf  /**< stream writer */
);

/**
 * The qse_sed_stop() function breaks running loop in qse_sed_exec().
 * It doesn't affect blocking calls in stream handlers.
 */
QSE_EXPORT void qse_sed_stop (
	qse_sed_t* sed   /**< stream editor */
);

/**
 * The qse_sed_isstop() functions tests if qse_sed_stop() is called.
 */
QSE_EXPORT int qse_sed_isstop (
	qse_sed_t* sed   /**< stream editor */
);
	
/**
 * The qse_sed_getcompid() function returns the latest
 * identifier successfully set with qse_sed_setcompid(). 
 */
QSE_EXPORT const qse_char_t* qse_sed_getcompid (
	qse_sed_t* sed
);

/**
 * The qse_sed_setcompid() functions duplicates a string
 * pointed to by @a id and stores it internally to identify
 * the script currently being compiled. The lid field of the 
 * current command being compiled in the script is set to the 
 * lastest identifer successfully set with this function.
 * If this function fails, the location set in the command
 * may be wrong.
 */
QSE_EXPORT const qse_char_t* qse_sed_setcompid (
	qse_sed_t*        sed,
	const qse_char_t* id
);

/**
 * The qse_sed_getlinnum() function gets the current input line number.
 * @return current input line number
 */
QSE_EXPORT qse_size_t qse_sed_getlinenum (
	qse_sed_t* sed /**< stream editor */
);

/**
 * The qse_sed_setlinenum() function changes the current input line number.
 */
QSE_EXPORT void qse_sed_setlinenum (
	qse_sed_t* sed,   /**< stream editor */
	qse_size_t num    /**< a line number */
);


/**
 * The qse_sed_allocmem() function allocates a chunk of memory using
 * the memory manager of \a sed.
 */
QSE_EXPORT void* qse_sed_allocmem (
	qse_sed_t* sed,
	qse_size_t size
);

/**
 * The qse_sed_allocmem() function allocates a chunk of memory using
 * the memory manager of \a sed and clears it to zeros.
 */
QSE_EXPORT void* qse_sed_callocmem (
	qse_sed_t* sed,
	qse_size_t size
);

/**
 * The qse_sed_allocmem() function reallocates a chunk of memory using
 * the memory manager of \a sed.
 */
QSE_EXPORT void* qse_sed_reallocmem (
	qse_sed_t* sed,
	void*      ptr,
	qse_size_t size
);

/**
 * The qse_sed_allocmem() function frees a chunk of memory using
 * the memory manager of \a sed.
 */
QSE_EXPORT void qse_sed_freemem (
	qse_sed_t* sed,
	void*      ptr
);

/**
 * The qse_sed_getspace() function gets the pointer and the length
 * to a buffer space specfied by \a space.
 */
QSE_EXPORT void qse_sed_getspace (
	qse_sed_t*      sed,
	qse_sed_space_t space,
	qse_cstr_t*     str
);

#ifdef __cplusplus
}
#endif

#endif
