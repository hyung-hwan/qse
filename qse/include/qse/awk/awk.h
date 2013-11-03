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

#ifndef _QSE_AWK_AWK_H_
#define _QSE_AWK_AWK_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/str.h>

/** \file
 * An embeddable AWK interpreter is defined in this header file.
 *
 * \todo
 * - make enhancement to treat a function as a value
 * - improve performance of qse_awk_rtx_readio() if RS is logner than 2 chars.
 * - consider something like ${1:3,5} => $1, $2, $3, and $5 concatenated
 */

/** \struct qse_awk_t
 * The #qse_awk_t type defines an AWK interpreter. It provides an interface
 * to parse an AWK script and run it to manipulate input and output data.
 *
 * In brief, you need to call APIs with user-defined handlers to run a typical
 * AWK script as shown below:
 * 
 * \code
 * qse_awk_t* awk;
 * qse_awk_rtx_t* rtx;
 * qse_awk_sio_t sio; // need to initialize it with callback functions
 * qse_awk_rio_t rio; // need to initialize it with callback functions
 *
 * awk = qse_awk_open (mmgr, 0, prm); // create an interpreter 
 * qse_awk_parse (awk, &sio);          // parse a script 
 * rtx = qse_awk_rtx_open (awk, 0, &rio); // create a runtime context 
 * retv = qse_awk_rtx_loop (rtx);     // run a standard AWK loop 
 * if (retv != QSE_NULL) 
 *    qse_awk_rtx_refdownval (rtx, retv); // free return value
 * qse_awk_rtx_close (rtx);           // destroy the runtime context
 * qse_awk_close (awk);               // destroy the interpreter
 * \endcode
 *
 * It provides an interface to change the conventional behavior of the 
 * interpreter; most notably, you can call a particular function with 
 * qse_awk_rtx_call() instead of entering the BEGIN, pattern-action blocks, END
 * loop. By doing this, you may utilize a script in an event-driven way.
 *
 * \sa qse_awk_rtx_t qse_awk_open qse_awk_close
 */
typedef struct qse_awk_t qse_awk_t;

/** \struct qse_awk_rtx_t
 * The #qse_awk_rtx_t type defines a runtime context. A runtime context 
 * maintains runtime state for a running script. You can create multiple
 * runtime contexts out of a single AWK interpreter; in other words, you 
 * can run the same script with different input and output data by providing
 * customized I/O handlers when creating a runtime context with 
 * qse_awk_rtx_open().
 *
 * I/O handlers are categoriezed into three kinds: console, file, pipe.
 * The #qse_awk_rio_t type defines as a callback a set of I/O handlers 
 * to handle runtime I/O:
 * - getline piped in from a command reads from a pipe.
 *   ("ls -l" | getline line)
 * - print and printf piped out to a command write to a pipe.
 *   (print 2 | "sort")
 * - getline redirected in reads from a file.
 *   (getline line < "file")
 * - print and printf redirected out write to a file.
 *   (print num > "file")
 * - The pattern-action loop and getline with no redirected input
 *   read from a console. (/susie/ { ... })
 * - print and printf write to a console. (print "hello, world")
 *
 * \sa qse_awk_t qse_awk_rtx_open qse_awk_rio_t
 */
typedef struct qse_awk_rtx_t qse_awk_rtx_t;

/**
 * The qse_awk_loc_t type defines a structure to hold location.
 */
struct qse_awk_loc_t
{
	const qse_char_t* file; /**< file */
	qse_size_t        line; /**< line */
	qse_size_t        colm; /**< column */
};
typedef struct qse_awk_loc_t qse_awk_loc_t;

/**
 * The #QSE_AWK_VAL_HDR defines the common header for a value.
 * Three common fields are:
 * - type - type of a value from #qse_awk_val_type_t
 * - ref - reference count
 * - stat - static value 
 * - nstr - numeric string marker, 1 -> long, 2 -> real
 */
/*
#define QSE_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 26; \
	unsigned int stat: 1; \
	unsigned int nstr: 2; 
*/
#define QSE_AWK_VAL_HDR \
	qse_uintptr_t type: 3; \
	qse_uintptr_t ref: ((QSE_SIZEOF_UINTPTR_T * 8) - 6); \
	qse_uintptr_t stat: 1; \
	qse_uintptr_t nstr: 2;

/**
 * The qse_awk_val_t type is an abstract value type. A value commonly contains:
 * - type of a value
 * - reference count
 * - indicator for a numeric string
 */
struct qse_awk_val_t
{
	QSE_AWK_VAL_HDR;	
};
typedef struct qse_awk_val_t qse_awk_val_t;

/**
 * The qse_awk_val_nil_t type is a nil value type. The type field is 
 * #QSE_AWK_VAL_NIL.
 */
struct qse_awk_val_nil_t
{
	QSE_AWK_VAL_HDR;
};
typedef struct qse_awk_val_nil_t  qse_awk_val_nil_t;

/**
 * The qse_awk_val_int_t type is an integer number type. The type field is
 * #QSE_AWK_VAL_INT.
 */
struct qse_awk_val_int_t
{
	QSE_AWK_VAL_HDR;
	qse_long_t val;
	void*      nde;
};
typedef struct qse_awk_val_int_t qse_awk_val_int_t;

/**
 * The qse_awk_val_flt_t type is a floating-point number type. The type field
 * is #QSE_AWK_VAL_FLT.
 */
struct qse_awk_val_flt_t
{
	QSE_AWK_VAL_HDR;
	qse_flt_t val;
	void*     nde;
};
typedef struct qse_awk_val_flt_t qse_awk_val_flt_t;

/**
 * The qse_awk_val_str_t type is a string type. The type field is
 * #QSE_AWK_VAL_STR.
 */
struct qse_awk_val_str_t
{
	QSE_AWK_VAL_HDR;
	qse_xstr_t  val;
};
typedef struct qse_awk_val_str_t  qse_awk_val_str_t;

/**
 * The qse_awk_val_rex_t type is a regular expression type.  The type field 
 * is #QSE_AWK_VAL_REX.
 */
struct qse_awk_val_rex_t
{
	QSE_AWK_VAL_HDR;
	qse_xstr_t  str;
	void*       code[2];
};
typedef struct qse_awk_val_rex_t  qse_awk_val_rex_t;

/**
 * The qse_awk_val_map_t type defines a map type. The type field is 
 * #QSE_AWK_VAL_MAP.
 */
struct qse_awk_val_map_t
{
	QSE_AWK_VAL_HDR;

	/* TODO: make val_map to array if the indices used are all 
	 *       integers switch to map dynamically once the 
	 *       non-integral index is seen.
	 */
	qse_htb_t* map; 
};
typedef struct qse_awk_val_map_t  qse_awk_val_map_t;

/**
 * The qse_awk_val_ref_t type defines a reference type that is used
 * internally only. The type field is #QSE_AWK_VAL_REF.
 */
struct qse_awk_val_ref_t
{
	QSE_AWK_VAL_HDR;

	enum
	{
		/* keep these items in the same order as corresponding items
		 * in qse_awk_nde_type_t. */
		QSE_AWK_VAL_REF_NAMED,    /**< plain named variable */
		QSE_AWK_VAL_REF_GBL,      /**< plain global variable */
		QSE_AWK_VAL_REF_LCL,      /**< plain local variable */
		QSE_AWK_VAL_REF_ARG,      /**< plain function argument */
		QSE_AWK_VAL_REF_NAMEDIDX, /**< member of named map variable */
		QSE_AWK_VAL_REF_GBLIDX,   /**< member of global map variable */
		QSE_AWK_VAL_REF_LCLIDX,   /**< member of local map variable */
		QSE_AWK_VAL_REF_ARGIDX,   /**< member of map argument */
		QSE_AWK_VAL_REF_POS       /**< positional variable */
	} id;

	/* if id is QSE_AWK_VAL_REF_POS, adr holds the index of a 
	 * positional variable. If id is QSE_AWK_VAL_REF_GBL, adr hold
	 * the index of a global variable. Otherwise, adr points to the value 
	 * directly. */
	qse_awk_val_t** adr;
};
typedef struct qse_awk_val_ref_t  qse_awk_val_ref_t;

/**
 * The qse_awk_val_map_itr_t type defines the iterator to map value fields.
 */
struct qse_awk_val_map_itr_t
{
	qse_htb_pair_t* pair;
	qse_size_t      buckno;
};
typedef struct qse_awk_val_map_itr_t qse_awk_val_map_itr_t;

/**
 * The #QSE_AWK_VAL_MAP_ITR_KEY macro get the pointer to the key part 
 * of a map value.
 */
#define QSE_AWK_VAL_MAP_ITR_KEY(itr) \
	((const qse_cstr_t*)QSE_HTB_KPTL((itr)->pair))

/**
 * The #QSE_AWK_VAL_MAP_ITR_VAL macro get the pointer to the value part 
 * of a map value.
 */
#define QSE_AWK_VAL_MAP_ITR_VAL(itr) \
	((const qse_awk_val_t*)QSE_HTB_VPTR((itr)->pair))


/**
 * The qse_awk_val_map_data_type_t type defines the type of
 * map value data for the #qse_awk_val_map_data_t structure.
 */
enum qse_awk_val_map_data_type_t
{
	QSE_AWK_VAL_MAP_DATA_INT  = 0,
	QSE_AWK_VAL_MAP_DATA_FLT,
	QSE_AWK_VAL_MAP_DATA_STR,
	QSE_AWK_VAL_MAP_DATA_MBS,
	QSE_AWK_VAL_MAP_DATA_WCS,
	QSE_AWK_VAL_MAP_DATA_CSTR,
	QSE_AWK_VAL_MAP_DATA_XSTR,
	QSE_AWK_VAL_MAP_DATA_MCSTR,
	QSE_AWK_VAL_MAP_DATA_MXSTR,
	QSE_AWK_VAL_MAP_DATA_WCSTR,
	QSE_AWK_VAL_MAP_DATA_WXSTR
};
typedef enum qse_awk_val_map_data_type_t qse_awk_val_map_data_type_t;

/**
 * The qse_awk_val_map_data_t type defines a structure that
 * describes a key/value pair for a map value to be created
 * with qse_awk_makemapvalwithdata().
 */
struct qse_awk_val_map_data_t
{
	qse_cstr_t                   key;
	qse_awk_val_map_data_type_t  type;
	void*                        vptr;
};

typedef struct qse_awk_val_map_data_t qse_awk_val_map_data_t;

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_nde_type_t defines the node types.
 */
enum qse_awk_nde_type_t
{
	QSE_AWK_NDE_NULL,

	/* statement */
	QSE_AWK_NDE_BLK,
	QSE_AWK_NDE_IF,
	QSE_AWK_NDE_WHILE,
	QSE_AWK_NDE_DOWHILE,
	QSE_AWK_NDE_FOR,
	QSE_AWK_NDE_FOREACH,
	QSE_AWK_NDE_BREAK,
	QSE_AWK_NDE_CONTINUE,
	QSE_AWK_NDE_RETURN,
	QSE_AWK_NDE_EXIT,
	QSE_AWK_NDE_NEXT,
	QSE_AWK_NDE_NEXTFILE,
	QSE_AWK_NDE_DELETE,
	QSE_AWK_NDE_RESET,

	/* expression */
	/* if you change the following values including their order,
	 * you should change __eval_func of __eval_expression 
	 * in run.c accordingly */
	QSE_AWK_NDE_GRP, 
	QSE_AWK_NDE_ASS,
	QSE_AWK_NDE_EXP_BIN,
	QSE_AWK_NDE_EXP_UNR,
	QSE_AWK_NDE_EXP_INCPRE,
	QSE_AWK_NDE_EXP_INCPST,
	QSE_AWK_NDE_CND,
	QSE_AWK_NDE_FNC,
	QSE_AWK_NDE_FUN,
	QSE_AWK_NDE_INT,
	QSE_AWK_NDE_FLT,
	QSE_AWK_NDE_STR,
	QSE_AWK_NDE_REX,

	/* keep this order for the following items otherwise, you may have 
	 * to change eval_incpre and eval_incpst in run.c as well as
	 * QSE_AWK_VAL_REF_XXX in qse_awk_val_ref_t. also do_assignment_map()
	 * in run.c converts QSE_AWK_NDE_XXXIDX to QSE_AWK_NDE_XXX by
	 * decrementing by 4. */
	QSE_AWK_NDE_NAMED,
	QSE_AWK_NDE_GBL,
	QSE_AWK_NDE_LCL,
	QSE_AWK_NDE_ARG,
	QSE_AWK_NDE_NAMEDIDX,
	QSE_AWK_NDE_GBLIDX,
	QSE_AWK_NDE_LCLIDX,
	QSE_AWK_NDE_ARGIDX,
	QSE_AWK_NDE_POS,
	/* ---------------------------------- */

	QSE_AWK_NDE_GETLINE,
	QSE_AWK_NDE_PRINT,
	QSE_AWK_NDE_PRINTF
};
typedef enum qse_awk_nde_type_t qse_awk_nde_type_t;

#define QSE_AWK_NDE_HDR \
	qse_awk_nde_type_t type; \
	qse_awk_loc_t      loc; \
	qse_awk_nde_t*     next

/** \struct qse_awk_nde_t
 * The qse_awk_nde_t type defines a common part of a node.
 */
typedef struct qse_awk_nde_t  qse_awk_nde_t;
struct qse_awk_nde_t
{
	QSE_AWK_NDE_HDR;
};

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_fun_t type defines a structure to maintain functions
 * defined with the keyword 'function'.
 */
struct qse_awk_fun_t
{
	qse_xstr_t     name;
	qse_size_t     nargs;
	qse_awk_nde_t* body;
};
typedef struct qse_awk_fun_t qse_awk_fun_t;

/* ------------------------------------------------------------------------ */

typedef qse_flt_t (*qse_awk_math1_t) (
	qse_awk_t* awk,
	qse_flt_t x
);

typedef qse_flt_t (*qse_awk_math2_t) (
	qse_awk_t* awk,
	qse_flt_t x, 
	qse_flt_t y
);

/* ------------------------------------------------------------------------ */

typedef struct qse_awk_mod_spec_t qse_awk_mod_spec_t;

struct qse_awk_mod_spec_t
{
	const qse_char_t* prefix;
	const qse_char_t* postfix;
	const qse_char_t* name;
};

typedef void* (*qse_awk_modopen_t) (
	qse_awk_t*                awk,
	const qse_awk_mod_spec_t* spec
);

typedef void* (*qse_awk_modsym_t) (
	qse_awk_t*        awk,
	void*             handle,
	const qse_char_t* name
);

typedef void (*qse_awk_modclose_t) (
	qse_awk_t* awk,
	void*      handle
);

/* ------------------------------------------------------------------------ */

#if 0
typedef void* (*qse_awk_buildrex_t) (
	qse_awk_t*        awk,
	const qse_char_t* ptn, 
	qse_size_t        len
);

typedef int (*qse_awk_matchrex_t) (
	qse_awk_t*         awk,
	void*              code,
	int                option,
	const qse_char_t*  str,
	qse_size_t         len, 
	const qse_char_t** mptr,
	qse_size_t*        mlen
);

typedef void (*qse_awk_freerex_t) (
	qse_awk_t* awk,
	void*      code
);

typedef int (*qse_awk_isemptyrex_t) (
	qse_awk_t* awk,
	void*      code
);
#endif

/**
 * The qse_awk_sio_cmd_t type defines I/O commands for a script stream.
 */
enum qse_awk_sio_cmd_t
{
	QSE_AWK_SIO_OPEN   = 0, /**< open a script stream */
	QSE_AWK_SIO_CLOSE  = 1, /**< close a script stream */
	QSE_AWK_SIO_READ   = 2, /**< read text from an input script stream */
	QSE_AWK_SIO_WRITE  = 3  /**< write text to an output script stream */
};
typedef enum qse_awk_sio_cmd_t qse_awk_sio_cmd_t;

/**
 * The qse_awk_sio_lxc_t type defines a structure to store a character
 * with its location information.
 */
struct qse_awk_sio_lxc_t
{
	qse_cint_t        c;    /**< character */
	qse_size_t        line; /**< line */
	qse_size_t        colm; /**< column */
	const qse_char_t* file; /**< file */
};
typedef struct qse_awk_sio_lxc_t qse_awk_sio_lxc_t;

typedef struct qse_awk_sio_arg_t qse_awk_sio_arg_t;
struct qse_awk_sio_arg_t 
{
	/** 
	 * [IN/OUT] name of I/O object. 
	 * It is #QSE_NULL for the top-level stream. It points to a stream name
	 * for an included stream. This can be changed by an I/O handler.
	 */
	const qse_char_t* name;   

	/** 
	 * [OUT] I/O handle set by a handler. 
	 * The source stream handler can set this field when it opens a stream.
	 * All subsequent operations on the stream see this field as set
	 * during opening.
	 */
	void* handle;

	/**
 	 * [IN] points to the includer. #QSE_NULL for the toplevel.
	 * 
	 */
	qse_awk_sio_arg_t* prev;

	/*-- from here down, internal use only --*/
	struct
	{
		qse_char_t buf[1024];
		qse_size_t pos;
		qse_size_t len;
	} b;

	qse_size_t line;
	qse_size_t colm;

	qse_awk_sio_lxc_t last;
};

/**
 * The qse_awk_sio_impl_t type defines a source IO function
 */
typedef qse_ssize_t (*qse_awk_sio_impl_t) (
	qse_awk_t*         awk,
	qse_awk_sio_cmd_t  cmd, 
	qse_awk_sio_arg_t* arg,
	qse_char_t*        data,
	qse_size_t         count
);

/**
 * The qse_awk_rio_cmd_t type defines runtime I/O request types.
 */
enum qse_awk_rio_cmd_t
{
	QSE_AWK_RIO_OPEN   = 0, /**< open a stream */
	QSE_AWK_RIO_CLOSE  = 1, /**< close a stream */
	QSE_AWK_RIO_READ   = 2, /**< read a stream */
	QSE_AWK_RIO_WRITE  = 3, /**< write to a stream */
	QSE_AWK_RIO_FLUSH  = 4, /**< flush buffered data to a stream */
	QSE_AWK_RIO_NEXT   = 5  /**< close the current stream and 
	                             open the next stream (only for console) */
};
typedef enum qse_awk_rio_cmd_t qse_awk_rio_cmd_t;

/**
 * The qse_awk_rio_mode_t type defines the I/O modes used by I/O handlers.
 * Each I/O handler should inspect the requested mode and open an I/O
 * stream accordingly for subsequent operations.
 */
enum qse_awk_rio_mode_t
{
	QSE_AWK_RIO_PIPE_READ      = 0, /**< open a pipe for read */
	QSE_AWK_RIO_PIPE_WRITE     = 1, /**< open a pipe for write */
	QSE_AWK_RIO_PIPE_RW        = 2, /**< open a pipe for read and write */

	QSE_AWK_RIO_FILE_READ      = 0, /**< open a file for read */
	QSE_AWK_RIO_FILE_WRITE     = 1, /**< open a file for write */
	QSE_AWK_RIO_FILE_APPEND    = 2, /**< open a file for append */

	QSE_AWK_RIO_CONSOLE_READ   = 0, /**< open a console for read */
	QSE_AWK_RIO_CONSOLE_WRITE  = 1  /**< open a console for write */
};
typedef enum qse_awk_rio_mode_t qse_awk_rio_mode_t;

/*
 * The qse_awk_rio_rwcmode_t type defines I/O closing modes, especially for 
 * a two-way pipe.
 */
enum qse_awk_rio_rwcmode_t
{
	QSE_AWK_RIO_CLOSE_FULL  = 0, /**< close both read and write end */
	QSE_AWK_RIO_CLOSE_READ  = 1, /**< close the read end */
	QSE_AWK_RIO_CLOSE_WRITE = 2  /**< close the write end */
};
typedef enum qse_awk_rio_rwcmode_t qse_awk_rio_rwcmode_t;

/**
 * The qse_awk_rio_arg_t defines the data structure passed to a runtime 
 * I/O handler. An I/O handler should inspect the \a mode field and the 
 * \a name field and store an open handle to the \a handle field when 
 * #QSE_AWK_RIO_OPEN is requested. For other request type, it can refer
 * to the \a handle field set previously.
 */
struct qse_awk_rio_arg_t 
{
	/* read-only. a user handler shouldn't change any of these fields */
	qse_awk_rio_mode_t    mode;      /**< opening mode */
	qse_char_t*           name;      /**< name of I/O object */
	qse_awk_rio_rwcmode_t rwcmode;   /**< closing mode for rwpipe */

	/* read-write. a user handler can do whatever it likes to do with these. */
	void*                 handle;    /**< I/O handle set by a handler */
	int                   uflags;    /**< flags set by a handler */

	/*--  from here down, internal use only --*/
	int type; 
	int rwcstate;   /* closing state for rwpipe */

	struct
	{
		qse_char_t buf[2048];
		qse_size_t pos;
		qse_size_t len;
		int        eof;
		int        eos;
	} in;

	struct
	{
		int eof;
		int eos;
	} out;

	struct qse_awk_rio_arg_t* next;
};
typedef struct qse_awk_rio_arg_t qse_awk_rio_arg_t;

/**
 * The qse_awk_rio_impl_t type defines a runtime I/O handler.
 */
typedef qse_ssize_t (*qse_awk_rio_impl_t) (
	qse_awk_rtx_t*      rtx,
	qse_awk_rio_cmd_t   cmd,
	qse_awk_rio_arg_t*  arg,
	qse_char_t*         data,
	qse_size_t          count
);

/**
 * The qse_awk_prm_t type defines primitive functions required to perform
 * a set of primitive operations.
 */
struct qse_awk_prm_t
{
	struct
	{
		qse_awk_math2_t pow; /**< floating-point power function */
		qse_awk_math2_t mod; /**< floating-point remainder function */
		qse_awk_math1_t sin;
		qse_awk_math1_t cos;
		qse_awk_math1_t tan;
		qse_awk_math1_t atan;
		qse_awk_math2_t atan2;
		qse_awk_math1_t log;
		qse_awk_math1_t log10;
		qse_awk_math1_t exp;
		qse_awk_math1_t sqrt;
	} math;

	qse_awk_modopen_t modopen;
	qse_awk_modclose_t modclose;
	qse_awk_modsym_t modsym;

#if 0
	struct 
	{
		/* TODO: accept regular expression handling functions */
		qse_awk_buildrex_t build;
		qse_awk_matchrex_t match;
		qse_awk_freerex_t free;
		qse_awk_isemptyrex_t isempty;
	} rex;
#endif
};
typedef struct qse_awk_prm_t qse_awk_prm_t;

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_sio_t type defines a script stream handler set.
 * The qse_awk_parse() function calls the input and output handler to parse
 * a script and optionally deparse it. Typical input and output handlers 
 * are shown below:
 *
 * \code
 * qse_ssize_t in (
 *    qse_awk_t* awk, qse_awk_sio_cmd_t cmd,
 *    qse_char_t* buf, qse_size_t size)
 * {
 *    if (cmd == QSE_AWK_SIO_OPEN) open input stream;
 *    else if (cmd == QSE_AWK_SIO_CLOSE) close input stream;
 *    else read input stream and fill buf up to size characters;
 * }
 *
 * qse_ssize_t out (
 *    qse_awk_t* awk, qse_awk_sio_cmd_t cmd,
 *    qse_char_t* data, qse_size_t size)
 * {
 *    if (cmd == QSE_AWK_SIO_OPEN) open_output_stream;
 *    else if (cmd == QSE_AWK_SIO_CLOSE) close_output_stream;
 *    else write data of size characters to output stream;
 * }
 * \endcode
 *
 * For #QSE_AWK_SIO_OPEN, a handler must return:
 * - -1 if it failed to open a stream.
 * - 0 if it has opened a stream but has reached the end.
 * - 1 if it has successfully opened a stream.
 * 
 * For #QSE_AWK_SIO_CLOSE, a handler must return:
 * - -1 if it failed to close a stream.
 * - 0 if it has closed a stream.
 *
 * For #QSE_AWK_SIO_READ and #QSE_AWK_SIO_WRITE, a handler must return:
 * - -1 if there was an error occurred during operation.
 * - 0 if it has reached the end.
 * - the number of characters read or written on success.
 */
struct qse_awk_sio_t
{
	qse_awk_sio_impl_t in;  /**< input script stream handler */
	qse_awk_sio_impl_t out; /**< output script stream handler */
};
typedef struct qse_awk_sio_t qse_awk_sio_t;

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_rio_t type defines a runtime I/O handler set.
 * \sa qse_awk_rtx_t
 */
struct qse_awk_rio_t
{
	qse_awk_rio_impl_t pipe;    /**< pipe handler */
	qse_awk_rio_impl_t file;    /**< file handler */
	qse_awk_rio_impl_t console; /**< console handler */
};
typedef struct qse_awk_rio_t qse_awk_rio_t;

/* ------------------------------------------------------------------------ */

typedef struct qse_awk_fnc_t      qse_awk_fnc_t;
typedef struct qse_awk_fnc_spec_t qse_awk_fnc_spec_t;
typedef struct qse_awk_fnc_info_t qse_awk_fnc_info_t;

/**
 * The qse_awk_fnc_impl_t type defines a intrinsic function handler.
 */
typedef int (*qse_awk_fnc_impl_t) (
	qse_awk_rtx_t*            rtx,  /**< runtime context */
	const qse_awk_fnc_info_t* fi    /**< function information */
);

/**
 * The qse_awk_fnc_spec_t type defines a structure to hold the specification
 * of an intrinsic function or a module function.
 */ 
struct qse_awk_fnc_spec_t
{
	/** parameter specification */
	struct
	{
		qse_size_t min; /**< min. numbers of argument for a function */
		qse_size_t max; /**< max. numbers of argument for a function */
		const qse_char_t* spec;
	} arg;

	/** pointer to the function implementing this function */
	qse_awk_fnc_impl_t impl;

	/** 
	 * when this field is set to a non-zero value bitwise-ORed of 
	 * #qse_awk_trait_t enumerators, the function is available if 
	 * this field bitwise-ANDed the global trait option produces
	 * this field itself.
	 * 
	 * this field doesn't take effect for a module function.
	 */
	int trait; 
};

/* ------------------------------------------------------------------------ */

typedef struct qse_awk_mod_t qse_awk_mod_t;
typedef struct qse_awk_mod_sym_t qse_awk_mod_sym_t;

struct qse_awk_fnc_info_t
{
	qse_xstr_t name;

	/** #QSE_NULL if the function is not registered from module */
	qse_awk_mod_t* mod; 
};


typedef int (*qse_awk_mod_load_t) (
	qse_awk_mod_t* mod,
	qse_awk_t*     awk
);

typedef int (*qse_awk_mod_query_t) (
	qse_awk_mod_t*     mod,
	qse_awk_t*         awk,
	const qse_char_t*  name,
	qse_awk_mod_sym_t* sym
);

typedef void (*qse_awk_mod_unload_t) (
	qse_awk_mod_t* mod,
	qse_awk_t*     awk
);

typedef int (*qse_awk_mod_init_t) (
	qse_awk_mod_t* mod,
	qse_awk_rtx_t* rtx
);

typedef void (*qse_awk_mod_fini_t) (
	qse_awk_mod_t* mod,
	qse_awk_rtx_t* rtx
);

struct qse_awk_mod_t
{
	qse_awk_mod_query_t  query;
	qse_awk_mod_unload_t unload;

	qse_awk_mod_init_t   init;
	qse_awk_mod_fini_t   fini;

	void*                ctx;
};

enum qse_awk_mod_sym_type_t
{
	QSE_AWK_MOD_FNC = 0, 
	QSE_AWK_MOD_INT, /* constant */
	QSE_AWK_MOD_FLT  /* constant */
	/*QSE_AWK_MOD_STR,
	QSE_AWK_MOD_VAR,
	*/
};
typedef enum qse_awk_mod_sym_type_t qse_awk_mod_sym_type_t;
typedef qse_awk_fnc_spec_t qse_awk_mod_sym_fnc_t;
typedef struct qse_awk_mod_sym_int_t qse_awk_mod_sym_int_t;
typedef struct qse_awk_mod_sym_flt_t qse_awk_mod_sym_flt_t;

struct qse_awk_mod_sym_int_t
{
	qse_long_t val;
};

struct qse_awk_mod_sym_flt_t
{
	qse_flt_t val;
};

struct qse_awk_mod_sym_t
{
	qse_awk_mod_sym_type_t type; 
	union
	{
		qse_awk_mod_sym_fnc_t fnc;
		qse_awk_mod_sym_int_t in;
		qse_awk_mod_sym_flt_t flt;
	} u;
};


/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_ecb_close_t type defines the callback function
 * called when an awk object is closed.
 */
typedef void (*qse_awk_ecb_close_t) (
	qse_awk_t* awk  /**< awk */
);

/**
 * The qse_awk_ecb_clear_t type defines the callback function
 * called when an awk object is cleared.
 */
typedef void (*qse_awk_ecb_clear_t) (
	qse_awk_t* awk  /**< awk */
);

/**
 * The qse_awk_ecb_t type defines an event callback set.
 * You can register a callback function set with
 * qse_awk_pushecb().  The callback functions in the registered
 * set are called in the reverse order of registration.
 */
typedef struct qse_awk_ecb_t qse_awk_ecb_t;
struct qse_awk_ecb_t
{
	/**
	 * called by qse_awk_close().
	 */
	qse_awk_ecb_close_t close;

	/**
	 * called by qse_awk_clear().
	 */
	qse_awk_ecb_clear_t clear;

	/* internal use only. don't touch this field */
	qse_awk_ecb_t* next;
};

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_rtx_ecb_close_t type defines the callback function
 * called when the runtime context is closed.
 */
typedef void (*qse_awk_rtx_ecb_close_t) (
	qse_awk_rtx_t* rtx  /**< runtime context */
);


/**
 * The qse_awk_rtx_ecb_stmt_t type defines the callback function for each
 * statement.
 */
typedef void (*qse_awk_rtx_ecb_stmt_t) (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_awk_nde_t* nde  /**< node */
);

/**
 * The qse_awk_rtx_ecb_gblset_t type defines the callback function 
 * executed when a global variable is set with a value. It is not
 * called when a global variable is changed implicitly. For example,
 * it is not called when FNR is updated for each record read.
 */
typedef void (*qse_awk_rtx_ecb_gblset_t) (
	qse_awk_rtx_t*     rtx, /**< runtime context */
	qse_size_t         idx, /**< global variable index */
	qse_awk_val_t*     val  /**< value */
);

/**
 * The qse_awk_rtx_ecb_t type defines an event callback set for a
 * runtime context. You can register a callback function set with
 * qse_awk_rtx_pushecb().  The callback functions in the registered
 * set are called in the reverse order of registration.
 */
typedef struct qse_awk_rtx_ecb_t qse_awk_rtx_ecb_t;
struct qse_awk_rtx_ecb_t
{
	/**
	 * called by qse_awk_rtx_close().
	 */
	qse_awk_rtx_ecb_close_t close;

	/**
	 * called by qse_awk_rtx_loop() and qse_awk_rtx_call() for
	 * each statement executed.
	 */
	qse_awk_rtx_ecb_stmt_t stmt;

	/**
	 * called when a global variable is set with a value.
	 */
	qse_awk_rtx_ecb_gblset_t gblset;

	/* internal use only. don't touch this field */
	qse_awk_rtx_ecb_t* next;
};

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_opt_t type defines various option types.
 */
enum qse_awk_opt_t
{
	/** trait option. 0 or bitwise-ORed of ::qse_awk_trait_t values */
	QSE_AWK_TRAIT,  

	QSE_AWK_MODPREFIX,
	QSE_AWK_MODPOSTFIX,

	QSE_AWK_INCLUDEDIRS,

	QSE_AWK_DEPTH_INCLUDE,
	QSE_AWK_DEPTH_BLOCK_PARSE,
	QSE_AWK_DEPTH_BLOCK_RUN,
	QSE_AWK_DEPTH_EXPR_PARSE,
	QSE_AWK_DEPTH_EXPR_RUN,
	QSE_AWK_DEPTH_REX_BUILD,
	QSE_AWK_DEPTH_REX_MATCH
};
typedef enum qse_awk_opt_t qse_awk_opt_t;

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_trait_t type defines various options to change the behavior
 * of #qse_awk_t.
 */
enum qse_awk_trait_t
{ 
	/** allows undeclared variables */
	QSE_AWK_IMPLICIT = (1 << 0),

	/** enables nextofile and NEXTOFILE */
	QSE_AWK_NEXTOFILE = (1 << 1),

	/** supports \b getline, \b print, \b printf, \b close, \b fflush,
	 *  piping, and file rediction */
	QSE_AWK_RIO = (1 << 3), 

	/** enables the two-way pipe if #QSE_AWK_RIO is on */
	QSE_AWK_RWPIPE = (1 << 4),

	/** a new line can terminate a statement */
	QSE_AWK_NEWLINE = (1 << 5),

	/** 
	 * removes empty fields when splitting a record if FS is a regular
	 * expression and the match is all spaces.
	 *
	 * \code
	 * BEGIN { FS="[[:space:]]+"; } 
	 * { 
	 *    print "NF=" NF; 
	 *    for (i = 0; i < NF; i++) print i " [" $(i+1) "]";
	 * }
	 * \endcode
	 * " a b c " is split to [a], [b], [c] if #QSE_AWK_STRIPRECSPC is on.
	 * Otherwise, it is split to [], [a], [b], [c], [].
	 *
	 * \code
	 * BEGIN { 
	 *   n=split("   oh my  noodle  ", x, /[ o]+/); 
	 *   for (i=1;i<=n;i++) print "[" x[i] "]"; 
	 * }
	 * \endcode
	 * This example splits the string to [], [h], [my], [n], [dle]
	 * if #QSE_AWK_STRIPRECSPC is on. Otherwise, it results in
	 * [], [h], [my], [n], [dle], []. Note that the first empty field is not 
	 * removed as the field separator is not all spaces. (space + 'o').
	 */
	QSE_AWK_STRIPRECSPC = (1 << 6),

	/** strips off leading spaces when converting a string to a number. */
	QSE_AWK_STRIPSTRSPC = (1 << 7),

	/** enable implicit concatenation. 
	 *  if this is off, you need %% for concatenation.  */
	QSE_AWK_BLANKCONCAT = (1 << 8),

	/** CR + LF by default */
	QSE_AWK_CRLF = (1 << 10),

	/** treats a map value more flexibly. a function can return
	 *  a map. you can override a map with a scalar value without 
	 *  'delete' or '@reset'. 
	 */
	QSE_AWK_FLEXMAP = (1 << 11),

	/** allows \b BEGIN, \b END, pattern-action blocks */
	QSE_AWK_PABLOCK = (1 << 12),

	/** allows {n,m} in a regular expression. */
	QSE_AWK_REXBOUND = (1 << 13),

	/** 
	 * performs numeric comparison when a string convertable
	 * to a number is compared with a number or vice versa.
	 *
	 * For an expression (9 > "10.9"),
	 * - 9 is greater if #QSE_AWK_NCMPONSTR is off;
	 * - "10.9" is greater if #QSE_AWK_NCMPONSTR is on
	 */
	QSE_AWK_NCMPONSTR = (1 << 14),

	/**
	 * enables the strict naming rule.
	 * - a parameter name can not be the same as the owning function name.
	 * - a local variable name can not be the same as the owning 
	 *   function name.
	 */
	QSE_AWK_STRICTNAMING = (1 << 15),

	/**
	 * makes AWK more fault-tolerant.
	 * - prevents termination due to print and printf failure.
	 * - achieves this by handling print and printf as if
	 *   they are functions like getline.
	 * - allows an expression group in a normal context
	 *   without the 'in' operator. the evaluation result
	 *   of the last expression is returned as that of
	 *   the expression group.
	 * - e.g.) a = (1, 3 * 3, 4, 5 + 1);  # a is assigned 6.
	 */
	QSE_AWK_TOLERANT = (1 << 17),

	/** 
	 * makes #qse_awk_t to behave compatibly with classical AWK
	 * implementations
	 */
	QSE_AWK_CLASSIC = 
		QSE_AWK_IMPLICIT | QSE_AWK_RIO | 
		QSE_AWK_NEWLINE | QSE_AWK_BLANKCONCAT | QSE_AWK_PABLOCK | 
		QSE_AWK_STRIPSTRSPC | QSE_AWK_STRICTNAMING,

	QSE_AWK_MODERN =
		QSE_AWK_CLASSIC | QSE_AWK_FLEXMAP |
		QSE_AWK_RWPIPE | QSE_AWK_TOLERANT | QSE_AWK_NEXTOFILE  /*| QSE_AWK_NCMPONSTR*/
};
typedef enum qse_awk_trait_t qse_awk_trait_t;

/* ------------------------------------------------------------------------ */

/**
 * The qse_awk_errnum_t type defines error codes.
 */
enum qse_awk_errnum_t
{
	QSE_AWK_ENOERR,  /**< no error */
	QSE_AWK_EOTHER,  /**< other error */
	QSE_AWK_ENOIMPL, /**< not implemented */
	QSE_AWK_ESYSERR, /**< subsystem error */
	QSE_AWK_EINTERN, /**< internal error */

	/* common errors */
	QSE_AWK_ENOMEM,  /**< insufficient memory */
	QSE_AWK_EINVAL,  /**< invalid parameter or data */
	QSE_AWK_EACCES,  /**< access denied */
	QSE_AWK_ENOSUP,  /**< not supported */
	QSE_AWK_ENOPER,  /**< operation not allowed */
	QSE_AWK_ENOENT,  /**< '${0}' not found */
	QSE_AWK_EEXIST,  /**< '${0}' already exists */
	QSE_AWK_EIOERR,  /**< I/O error */

	/* mostly parse errors */
	QSE_AWK_EOPEN,   /**< cannot open '${0}' */
	QSE_AWK_EREAD,   /**< cannot read '${0}' */
	QSE_AWK_EWRITE,  /**< cannot write '${0}' */
	QSE_AWK_ECLOSE,  /**< cannot close '${0}' */

	QSE_AWK_EBLKNST, /**< block nested too deeply */
	QSE_AWK_EEXPRNST,/**< expression nested too deeply */

	QSE_AWK_ELXCHR,  /**< invalid character '${0}' */
	QSE_AWK_ELXDIG,  /**< invalid digit '${0}' */

	QSE_AWK_EEOF,    /**< unexpected end of source */
	QSE_AWK_ECMTNC,  /**< comment not closed properly */
	QSE_AWK_ESTRNC,  /**< string or regular expression not closed */
	QSE_AWK_ELBRACE, /**< left brace expected in place of '${0}' */
	QSE_AWK_ELPAREN, /**< left parenthesis expected in place of '${0}' */
	QSE_AWK_ERPAREN, /**< right parenthesis expected in place of '${0}' */
	QSE_AWK_ERBRACK, /**< right bracket expected in place of '${0}' */
	QSE_AWK_ECOMMA,  /**< comma expected in place of '${0}' */
	QSE_AWK_ESCOLON, /**< semicolon expected in place of '${0}' */
	QSE_AWK_ECOLON,  /**< colon expected in place of '${0}' */
	QSE_AWK_ESTMEND, /**< statement not ending with a semicolon */
	QSE_AWK_EKWIN,   /**< keyword 'in' expected in place of '${0}' */
	QSE_AWK_ENOTVAR, /**< right-hand side of 'in' not a variable */
	QSE_AWK_EEXPRNR, /**< expression not recognized around '${0}' */

	QSE_AWK_EKWFNC,    /**< keyword 'function' expected in place of '${0}' */
	QSE_AWK_EKWWHL,    /**< keyword 'while' expected in place of '${0}' */
	QSE_AWK_EASSIGN,   /**< assignment statement expected */
	QSE_AWK_EIDENT,    /**< identifier expected in place of '${0}' */
	QSE_AWK_EFUNNAM,   /**< '${0}' not a valid function name */
	QSE_AWK_EBLKBEG,   /**< BEGIN not followed by left bracket on the same line */
	QSE_AWK_EBLKEND,   /**< END not followed by left bracket on the same line */
	QSE_AWK_EKWRED,    /**< keyword '${0}' redefined */
	QSE_AWK_EFNCRED,   /**< intrinsic function '${0}' redefined */
	QSE_AWK_EFUNRED,   /**< function '${0}' redefined */
	QSE_AWK_EGBLRED,   /**< global variable '${0}' redefined */
	QSE_AWK_EPARRED,   /**< parameter '${0}' redefined */
	QSE_AWK_EVARRED,   /**< variable '${0}' redefined */
	QSE_AWK_EDUPPAR,   /**< duplicate parameter name '${0}' */
	QSE_AWK_EDUPGBL,   /**< duplicate global variable name '${0}' */
	QSE_AWK_EDUPLCL,   /**< duplicate local variable name '${0}' */
	QSE_AWK_EBADPAR,   /**< '${0}' not a valid parameter name */
	QSE_AWK_EBADVAR,   /**< '${0}' not a valid variable name */
	QSE_AWK_EVARMS,    /**< variable name missing */
	QSE_AWK_EUNDEF,    /**< undefined identifier '${0}' */
	QSE_AWK_ELVALUE,   /**< l-value required */
	QSE_AWK_EGBLTM,    /**< too many global variables */
	QSE_AWK_ELCLTM,    /**< too many local variables */
	QSE_AWK_EPARTM,    /**< too many parameters */
	QSE_AWK_ESEGTM,    /**< too many segments */
	QSE_AWK_ESEGTL,    /**< segment '${0}' too long */
	QSE_AWK_EBADARG,   /**< bad argument */
	QSE_AWK_ENOARG,    /**< no argument */
	QSE_AWK_EBREAK,    /**< 'break' outside a loop */
	QSE_AWK_ECONTINUE, /**< 'continue' outside a loop */
	QSE_AWK_ENEXTBEG,  /**< 'next' illegal in BEGIN block */
	QSE_AWK_ENEXTEND,  /**< 'next' illegal in END block */
	QSE_AWK_ENEXTFBEG, /**< 'nextfile' illegal in BEGIN block */
	QSE_AWK_ENEXTFEND, /**< 'nextfile' illegal in END block */
	QSE_AWK_EPREPST,   /**< both prefix and postfix incr/decr operator present */
	QSE_AWK_EINCDECOPR,/**< illegal operand for incr/decr operator */
	QSE_AWK_EINCLSTR,  /**< 'include' not followed by a string */
	QSE_AWK_EINCLTD,   /**< include level too deep */
	QSE_AWK_EXKWNR,    /**< @word '${0}' not recognized */
	QSE_AWK_EXKWEM,    /**< @ not followed by a valid word  */

	/* run time error */
	QSE_AWK_EDIVBY0,       /**< divide by zero */
	QSE_AWK_EOPERAND,      /**< invalid operand */
	QSE_AWK_EPOSIDX,       /**< wrong position index */
	QSE_AWK_EARGTF,        /**< too few arguments */
	QSE_AWK_EARGTM,        /**< too many arguments */
	QSE_AWK_EFUNNF,        /**< function '${0}' not found */
	QSE_AWK_ENOTDEL,       /**< '${0}' not deletable */
	QSE_AWK_ENOTMAP,       /**< value not a map */
	QSE_AWK_ENOTMAPIN,     /**< right-hand side of 'in' not a map */
	QSE_AWK_ENOTMAPNILIN,  /**< right-hand side of 'in' not a map nor nil */
	QSE_AWK_ENOTREF,       /**< value not referenceable */
	QSE_AWK_EMAPRET,       /**< map cannot be returned */
	QSE_AWK_EMAPTOPOS,     /**< map cannot be assigned to a positional */
	QSE_AWK_EMAPTOIDX,     /**< map cannot be assigned to an indexed variable */
	QSE_AWK_EMAPTONVAR,    /**< map cannot be assigned to an variable '${0}' */
	QSE_AWK_EMAPTOSCALAR,  /**< cannot change a map to a scalar value */
	QSE_AWK_ESCALARTOMAP,  /**< cannot change a scalar to a map */
	QSE_AWK_ENMAPTOMAP,    /**< cannot change a map '${0}' to another map */
	QSE_AWK_ENMAPTOSCALAR, /**< cannot change a map '${0}' to a scalar */
	QSE_AWK_ENSCALARTOMAP, /**< cannot change a scalar '${0}' to a map */
	QSE_AWK_EVALTOSTR,     /**< invalid value to convert to a string */
	QSE_AWK_EVALTONUM,     /**< invalid value to convert to a number */
	QSE_AWK_EVALTOCHR,     /**< invalid value to convert to a character */
	QSE_AWK_EHASHVAL,      /**< invalid value to hash */
	QSE_AWK_ERNEXTBEG,     /**< 'next' called from BEGIN block */
	QSE_AWK_ERNEXTEND,     /**< 'next' called from END block */
	QSE_AWK_ERNEXTFBEG,    /**< 'nextfile' called from BEGIN block */
	QSE_AWK_ERNEXTFEND,    /**< 'nextfile' called from END block */
	QSE_AWK_EFNCIMPL,      /**< intrinsic function handler for '${0}' failed */
	QSE_AWK_EIOUSER,       /**< wrong user io handler implementation */
	QSE_AWK_EIOIMPL,       /**< I/O callback returned an error */
	QSE_AWK_EIONMNF,       /**< no such I/O name found */
	QSE_AWK_EIONMEM,       /**< I/O name empty */
	QSE_AWK_EIONMNL,       /**< I/O name '${0}' containing '\\0' */
	QSE_AWK_EFMTARG,       /**< not sufficient arguments to formatting sequence */
	QSE_AWK_EFMTCNV,       /**< recursion detected in format conversion */
	QSE_AWK_ECONVFMTCHR,   /**< invalid character in CONVFMT */
	QSE_AWK_EOFMTCHR,      /**< invalid character in OFMT */

	/* regular expression error */
	QSE_AWK_EREXBL,        /**< failed to build regular expression */
	QSE_AWK_EREXMA,        /**< failed to match regular expression */
	QSE_AWK_EREXRECUR,     /**< recursion too deep */
	QSE_AWK_EREXRPAREN,    /**< a right parenthesis is expected */
	QSE_AWK_EREXRBRACK,    /**< a right bracket is expected */
	QSE_AWK_EREXRBRACE,    /**< a right brace is expected */
	QSE_AWK_EREXCOLON,     /**< a colon is expected */
	QSE_AWK_EREXCRANGE,    /**< invalid character range */
	QSE_AWK_EREXCCLASS,    /**< invalid character class */
	QSE_AWK_EREXBOUND,     /**< invalid occurrence bound */
	QSE_AWK_EREXSPCAWP,    /**< special character at wrong position */
	QSE_AWK_EREXPREEND,    /**< premature end of regular expression */

	/* the number of error numbers, internal use only */
	QSE_AWK_NUMERRNUM 
};
typedef enum qse_awk_errnum_t qse_awk_errnum_t;

/** 
 * The qse_awk_errinf_t type defines a placeholder for error information.
 */
struct qse_awk_errinf_t
{
	qse_awk_errnum_t num;      /**< error number */
	qse_char_t       msg[256]; /**< error message */
	qse_awk_loc_t    loc;      /**< error location */
};
typedef struct qse_awk_errinf_t qse_awk_errinf_t;

/**
 * The qse_awk_errstr_t type defines an error string getter. It should return 
 * an error formatting string for an error number requested. A new string
 * should contain the same number of positional parameters (${X}) as in the
 * default error formatting string. You can set a new getter into an awk
 * object with the qse_awk_seterrstr() function to customize an error string.
 */
typedef const qse_char_t* (*qse_awk_errstr_t) (
	const qse_awk_t* awk,   /**< awk */
	qse_awk_errnum_t num    /**< error number */
);

/**
 * The qse_awk_gbl_id_t type defines intrinsic globals variable IDs.
 */
enum qse_awk_gbl_id_t
{
	/* this table should match gtab in parse.c.
	 * in addition, qse_awk_rtx_setgbl also counts 
	 * on the order of these values.
	 * 
	 * note that set_global() in run.c contains code 
	 * preventing these global variables from being assigned
	 * with a map value. if you happen to add one that can 
	 * be a map, don't forget to change code in set_global().
	 * but is this check really necessary???
	 */

	QSE_AWK_GBL_CONVFMT,
	QSE_AWK_GBL_FILENAME,
	QSE_AWK_GBL_FNR,
	QSE_AWK_GBL_FS,
	QSE_AWK_GBL_IGNORECASE,
	QSE_AWK_GBL_NF,
	QSE_AWK_GBL_NR,
	QSE_AWK_GBL_OFILENAME,
	QSE_AWK_GBL_OFMT,
	QSE_AWK_GBL_OFS,
	QSE_AWK_GBL_ORS,
	QSE_AWK_GBL_RLENGTH,
	QSE_AWK_GBL_RS,
	QSE_AWK_GBL_RSTART,
	QSE_AWK_GBL_SUBSEP,

	/* these are not not the actual IDs and are used internally only 
	 * Make sure you update these values properly if you add more 
	 * ID definitions, however */
	QSE_AWK_MIN_GBL_ID = QSE_AWK_GBL_CONVFMT,
	QSE_AWK_MAX_GBL_ID = QSE_AWK_GBL_SUBSEP
};
typedef enum qse_awk_gbl_id_t qse_awk_gbl_id_t;

/**
 * The qse_awk_val_type_t type defines types of AWK values. Each value 
 * allocated is tagged with a value type in the \a type field.
 * \sa qse_awk_val_t QSE_AWK_VAL_HDR
 */
enum qse_awk_val_type_t
{
	/* the values between QSE_AWK_VAL_NIL and QSE_AWK_VAL_STR inclusive
	 * must be synchronized with an internal table of the __cmp_val 
	 * function in run.c */
	QSE_AWK_VAL_NIL  = 0, /**< nil */
	QSE_AWK_VAL_INT  = 1, /**< integer */
	QSE_AWK_VAL_FLT  = 2, /**< floating-pointer number */
	QSE_AWK_VAL_STR  = 3, /**< string */

	QSE_AWK_VAL_REX  = 4, /**< regular expression */
	QSE_AWK_VAL_MAP  = 5, /**< map */

	QSE_AWK_VAL_REF  = 6  /**< reference to other types */
};

/**
 * The values defined are used to set the type field of the 
 * #qse_awk_rtx_valtostr_out_t structure. The field should be one of the 
 * following values:
 *
 * - #QSE_AWK_RTX_VALTOSTR_CPL
 * - #QSE_AWK_RTX_VALTOSTR_CPLCPY
 * - #QSE_AWK_RTX_VALTOSTR_CPLDUP
 * - #QSE_AWK_RTX_VALTOSTR_STRP
 * - #QSE_AWK_RTX_VALTOSTR_STRPCAT
 *
 * and it can optionally be ORed with #QSE_AWK_RTX_VALTOSTR_PRINT.
 */
enum qse_awk_rtx_valtostr_type_t
{ 
	/** use u.cpl of #qse_awk_rtx_valtostr_out_t */
	QSE_AWK_RTX_VALTOSTR_CPL       = 0x00, 
	/** use u.cplcpy of #qse_awk_rtx_valtostr_out_t */
	QSE_AWK_RTX_VALTOSTR_CPLCPY    = 0x01, 
	/** use u.cpldup of #qse_awk_rtx_valtostr_out_t */
	QSE_AWK_RTX_VALTOSTR_CPLDUP    = 0x02,
	/** use u.strp of #qse_awk_rtx_valtostr_out_t */
	QSE_AWK_RTX_VALTOSTR_STRP      = 0x03,
	/** use u.strpcat of #qse_awk_rtx_valtostr_out_t */
	QSE_AWK_RTX_VALTOSTR_STRPCAT   = 0x04,
	/** convert for print */
	QSE_AWK_RTX_VALTOSTR_PRINT     = 0x10   
};

/**
 * The qse_awk_rtx_valtostr() function converts a value to a string as 
 * indicated in a parameter of the qse_awk_rtx_valtostr_out_t type.
 */
struct qse_awk_rtx_valtostr_out_t
{
	int type; /**< enum #qse_awk_rtx_valtostr_type_t */

	union
	{
		qse_cstr_t  cpl;
		qse_xstr_t  cplcpy;
		qse_xstr_t  cpldup;  /* need to free cpldup.ptr */
		qse_str_t*  strp;
		qse_str_t*  strpcat;
	} u;
};
typedef struct qse_awk_rtx_valtostr_out_t qse_awk_rtx_valtostr_out_t;


/* record filter using NR */
struct qse_awk_nrflt_t
{
	qse_long_t limit;
	qse_long_t size;
	qse_long_t rank;
};
typedef struct qse_awk_nrflt_t qse_awk_nrflt_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_awk_open() function creates a new qse_awk_t object. The object 
 * created can be passed to other qse_awk_xxx() functions and is valid until 
 * it is destroyed with the qse_awk_close() function. The function saves the 
 * memory manager pointer while it copies the contents of the primitive 
 * function structures. Therefore, you should keep the memory manager valid 
 * during the whole life cycle of an qse_awk_t object.
 *
 * \code
 * qse_awk_t* dummy()
 * {
 *     qse_mmgr_t mmgr;
 *     qse_awk_prm_t prm;
 *     return qse_awk_open (
 *        &mmgr, // NOT OK because the contents of mmgr is 
 *               // invalidated when dummy() returns. 
 *        0, 
 *        &prm   // OK 
 *     );
 * }
 * \endcode
 *
 * \return a pointer to a qse_awk_t object on success, #QSE_NULL on failure.
 */
QSE_EXPORT qse_awk_t* qse_awk_open ( 
	qse_mmgr_t*    mmgr,    /**< memory manager */
	qse_size_t     xtnsize, /**< extension size in bytes */
	qse_awk_prm_t* prm      /**< pointer to a primitive function structure */
);

/**
 *  The qse_awk_close() function destroys a qse_awk_t object.
 * \return 0 on success, -1 on failure 
 */
QSE_EXPORT int qse_awk_close (
	qse_awk_t* awk /**< awk */
);

/**
 * The qse_awk_getmmgr() function gets the memory manager used in
 * qse_awk_open().
 */
QSE_EXPORT qse_mmgr_t* qse_awk_getmmgr (
	qse_awk_t* awk
); 

/** 
 * The qse_awk_getxtn() function gets the poniter to the beginning
 * of the extension area created with qse_awk_open ().
 */
QSE_EXPORT void* qse_awk_getxtn (
	qse_awk_t* awk
);

/**
 * The qse_awk_getprm() function retrieves primitive functions
 * associated. Actual function pointers are copied into a 
 * structure specified by \a prm.
 */
QSE_EXPORT void qse_awk_getprm (
	qse_awk_t*     awk,
	qse_awk_prm_t* prm
);

/**
 * The qse_awk_setprm() function changes existing primitive
 * functions. 
 */
QSE_EXPORT void qse_awk_setprm (
	qse_awk_t*           awk,
	const qse_awk_prm_t* prm
);

/**
 * The qse_awk_clear() clears the internal state of \a awk. If you want to
 * reuse a qse_awk_t instance that finished being used, you may call 
 * qse_awk_clear() instead of destroying and creating a new
 * #qse_awk_t instance using qse_awk_close() and qse_awk_open().
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_awk_clear (
	qse_awk_t* awk 
);

/**
 * The qse_awk_geterrstr() gets an error string getter.
 */
QSE_EXPORT qse_awk_errstr_t qse_awk_geterrstr (
	const qse_awk_t* awk    /**< awk */
);

/**
 * The qse_awk_seterrstr() sets an error string getter that is called to
 * compose an error message when its retrieval is requested.
 *
 * Here is an example of changing the formatting string for the #QSE_SED_ECMDNR 
 * error.
 * \code
 * qse_awk_errstr_t orgerrstr;
 *
 * const qse_char_t* myerrstr (qse_awk_t* awk, qse_awk_errnum_t num)
 * {
 *   if (num == QSE_SED_ECMDNR) return QSE_T("unrecognized command ${0}");
 *   return orgerrstr (awk, num);
 * }
 * int main ()
 * {
 *    qse_awk_t* awk;
 *    ...
 *    orgerrstr = qse_awk_geterrstr (awk);
 *    qse_awk_seterrstr (awk, myerrstr);
 *    ...
 * }
 * \endcode
 */
QSE_EXPORT void qse_awk_seterrstr (
	qse_awk_t*       awk,   /**< awk */
	qse_awk_errstr_t errstr /**< error string getter */
);

/**
 * The qse_awk_geterrnum() function returns the number of the last error 
 * occurred.
 * \return error number
 */
QSE_EXPORT qse_awk_errnum_t qse_awk_geterrnum (
	const qse_awk_t* awk /**< awk */
);

/**
 * The qse_awk_geterrloc() function returns the location where the
 * last error has occurred.
 */
QSE_EXPORT const qse_awk_loc_t* qse_awk_geterrloc (
	const qse_awk_t* awk /**< awk */
);

/**
 * The qse_awk_geterrmsg() function returns the error message describing
 * the last error occurred. 
 *
 * \return error message
 */
QSE_EXPORT const qse_char_t* qse_awk_geterrmsg (
	const qse_awk_t* awk /**< awk */
);

/**
 * The qse_awk_geterrinf() function copies error information into memory
 * pointed to by \a errinf from \a awk.
 */
QSE_EXPORT void qse_awk_geterrinf (
	const qse_awk_t*  awk,   /**< awk */
	qse_awk_errinf_t* errinf /**< error information buffer */
);

/**
 * The qse_awk_seterrnum() function sets the error information omitting 
 * error location. You must pass a non-NULL for \a errarg if the specified
 * error number \a errnum requires one or more arguments to format an
 * error message.
 */
QSE_EXPORT void qse_awk_seterrnum (
	qse_awk_t*        awk,    /**< awk */
	qse_awk_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument array for formatting 
	                           *   an error message */
);

/**
 * The qse_awk_seterrinf() function sets the error information. This function
 * may be useful if you want to set a custom error message rather than letting
 * it automatically formatted.
 */
QSE_EXPORT void qse_awk_seterrinf (
	qse_awk_t*              awk,   /**< awk */
	const qse_awk_errinf_t* errinf /**< error information */
);

/**
 * The qse_awk_geterror() function gets error information via parameters.
 */
QSE_EXPORT void qse_awk_geterror (
	const qse_awk_t*   awk,    /**< awk */
	qse_awk_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg, /**< error message */
	qse_awk_loc_t*     errloc  /**< error location */
);

/**
 * The qse_awk_seterror() function sets error information.
 */
QSE_EXPORT void qse_awk_seterror (
	qse_awk_t*           awk,    /**< awk */
	qse_awk_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< argument array for formatting 
	                              *   an error message */
	const qse_awk_loc_t* errloc  /**< error location */
);

/**
 * The qse_awk_getopt() function gets the value of an option
 * specified by \a id into the buffer pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_awk_getopt (
	qse_awk_t*    awk,
	qse_awk_opt_t id,
	void*         value
);

/**
 * The qse_awk_setopt() function sets the value of an option 
 * specified by \a id to the value pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_awk_setopt (
	qse_awk_t*    awk,
	qse_awk_opt_t id,
	const void*   value
);

/**
 * The qse_awk_popecb() function pops an awk event callback set
 * and returns the pointer to it. If no callback set can be popped,
 * it returns #QSE_NULL.
 */
QSE_EXPORT qse_awk_ecb_t* qse_awk_popecb (
	qse_awk_t* awk /**< awk */
);

/**
 * The qse_awk_pushecb() function register a runtime callback set.
 */
QSE_EXPORT void qse_awk_pushecb (
	qse_awk_t*     awk, /**< awk */
	qse_awk_ecb_t* ecb  /**< callback set */
);

/**
 * The qse_awk_addgbl() function adds an intrinsic global variable.
 * \return the ID of the global variable added on success, -1 on failure.
 */
QSE_EXPORT int qse_awk_addgbl (
	qse_awk_t*        awk,   /**< awk */
	const qse_char_t* name   /**< variable name */
);

/**
 * The qse_awk_delgbl() function deletes an intrinsic global variable by name.
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_awk_delgbl (
	qse_awk_t*        awk,  /**< awk */
	const qse_char_t* name  /**< variable name */
);

/**
 * The qse_awk_findgbl() function returns the numeric ID of an intrinsic global
 * variable.
 * \return number >= 0 on success, -1 on failure
 */
QSE_EXPORT int qse_awk_findgbl (
	qse_awk_t*        awk,  /**< awk */
	const qse_char_t* name  /**< variable name */
);

/**
 * The qse_awk_addfnc() function adds an intrinsic function.
 */
QSE_EXPORT qse_awk_fnc_t* qse_awk_addfnc (
	qse_awk_t*                awk,
	const qse_char_t*         name,
	const qse_awk_fnc_spec_t* spec
);

/**
 * The qse_awk_delfnc() function deletes an intrinsic function by name.
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_awk_delfnc (
	qse_awk_t*        awk,  /**< awk */
	const qse_char_t* name  /**< function name */
);

/**
 * The qse_awk_clrfnc() function deletes all intrinsic functions
 */
QSE_EXPORT void qse_awk_clrfnc (
	qse_awk_t* awk /**< awk */
);

/**
 * The qse_awk_parse() function parses a source script, and optionally 
 * deparses it back. 
 *
 * It reads a source script by calling \a sio->in as shown in the pseudo code 
 * below:
 *
 * \code
 * n = sio->in (awk, QSE_AWK_SIO_OPEN);
 * if (n >= 0)
 * {
 *    while (n > 0)
 *       n = sio->in (awk, QSE_AWK_SIO_READ, buf, buf_size);
 *    sio->in (awk, QSE_AWK_SIO_CLOSE);
 * }
 * \endcode
 *
 * A negative number returned causes qse_awk_parse() to return failure;
 * 0 returned indicates the end of a stream; A positive number returned 
 * indicates successful opening of a stream or the length of the text read.
 *
 * If \a sio->out is not #QSE_NULL, it deparses the internal parse tree
 * composed of a source script and writes back the deparsing result by 
 * calling \a sio->out as shown below:
 *
 * \code
 * n = sio->out (awk, QSE_AWK_SIO_OPEN);
 * if (n >= 0)
 * {
 *    while (n > 0)
 *       n = sio->out (awk, QSE_AWK_SIO_WRITE, text, text_size);
 *    sio->out (awk, QSE_AWK_SIO_CLOSE);
 * }
 * \endcode
 * 
 * Unlike \a sf->in, the return value of 0 from \a sf->out is treated as
 * premature end of a stream; therefore, it causes qse_awk_parse() to return
 * failure.
 *
 * \return 0 on success, -1 on failure.
 */
QSE_EXPORT int qse_awk_parse (
	qse_awk_t*     awk, /**< awk */
	qse_awk_sio_t* sio  /**< source script I/O handler */
);

/**
 * The qse_awk_allocmem() function allocates dynamic memory.
 * \return a pointer to a memory block on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_awk_allocmem (
	qse_awk_t* awk,  /**< awk */
	qse_size_t size  /**< size of memory to allocate in bytes */
);

/**
 * The qse_awk_reallocmem() function resizes a dynamic memory block.
 * \return a pointer to a memory block on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_awk_reallocmem (
	qse_awk_t* awk,  /**< awk */
	void*      ptr,  /**< memory block */
	qse_size_t size  /**< new block size in bytes */
);

/**
 * The qse_awk_callocmem() function allocates a memory block of 
 * the size of \a size bytes and initializes it with 0.
 * \return a pointer to a memory block on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_awk_callocmem (
	qse_awk_t* awk,  /**< awk */
	qse_size_t size  /**< size of memory to allocate in bytes */
);

/**
 * The qse_awk_freemem() function frees dynamic memory allocated.
 */
QSE_EXPORT void qse_awk_freemem (
	qse_awk_t* awk, /**< awk */
	void*      ptr  /**< memory block to free */
);

/**
 * The qse_awk_strdup() function is used to duplicate a string using
 * the memory manager used by the associated qse_awk_t instance.
 * The new string must be freed using the qse_awk_freemem() function when
 * it's not needed any more.
 *
 * \return a pointer to a new string duplicated of \a s on success, 
 *         #QSE_NULL on failure.
 */
QSE_EXPORT qse_char_t* qse_awk_strdup (
	qse_awk_t*        awk, /**< awk */
	const qse_char_t* str  /**< string pointer */
);

/**
 * The qse_awk_strxdup() function is used to duplicate a string whose length
 * is as long as len characters using the memory manager used by the 
 * qse_awk_t instance. The new string must be freed using the qse_awk_freemem()
 * function it's not needed any more.
 *
 * \return a pointer to a new string duplicated of \a s on success, 
 *         #QSE_NULL on failure.
 */
QSE_EXPORT qse_char_t* qse_awk_strxdup (
	qse_awk_t*        awk, /**< awk */
	const qse_char_t* str, /**< string pointer */
	qse_size_t        len  /**< string length */
);

/**
 * The qse_awk_cstrdup() funcation duplicates \a str->len characters from a 
 * string pointed to by @str->ptr. The duplicated string must be freed with
 * the qse_awk_freemem() function when it's not needed any more.
 * 
 * \return pointer to a duplicated string on success,
 *         #QSE_NULL on failure.
 */
QSE_EXPORT qse_char_t* qse_awk_cstrdup (
	qse_awk_t*        awk, /**< awk */
	const qse_cstr_t* str  /**< string */
);

/**
 * The qse_awk_strxtolong() function converts a string to an integer.
 */
QSE_EXPORT qse_long_t qse_awk_strxtolong (
	qse_awk_t*         awk,
	const qse_char_t*  str,
	qse_size_t         len,
	int                base,
	const qse_char_t** endptr
);

/**
 * The qse_awk_strxtoflt() function converts a string to a floating-point
 * number.
 */
QSE_EXPORT qse_flt_t qse_awk_strxtoflt (
	qse_awk_t*         awk,
	const qse_char_t*  str,
	qse_size_t         len, 
	const qse_char_t** endptr
);

/**
 * The qse_awk_longtostr() functon convers an integer to a string.
 */
QSE_EXPORT qse_size_t qse_awk_longtostr (
	qse_awk_t*        awk,
	qse_long_t        value,
	int               radix,
	const qse_char_t* prefix,
	qse_char_t*       buf,
	qse_size_t        size
);

/**
 * The qse_awk_rtx_open() creates a runtime context associated with \a awk.
 * It also allocates an extra memory block as large as the \a xtn bytes.
 * You can get the pointer to the beginning of the block with 
 * qse_awk_rtx_getxtn(). The block is destroyed when the runtime context is
 * destroyed. 
 *
 * \return new runtime context on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_rtx_t* qse_awk_rtx_open (
	qse_awk_t*        awk, /**< awk */
	qse_size_t        xtn, /**< size of extension in bytes */
	qse_awk_rio_t*    rio  /**< runtime IO handlers */
);

/**
 * The qse_awk_rtx_close() function destroys a runtime context.
 */
QSE_EXPORT void qse_awk_rtx_close (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_loop() function executes the BEGIN block, pattern-action
 * blocks and the END blocks in an AWK program. It returns the global return 
 * value of which the reference count must be decremented when not necessary. 
 * Multiple invocations of the function for the lifetime of a runtime context 
 * is not desirable.
 *
 * The example shows typical usage of the function.
 * \code
 * rtx = qse_awk_rtx_open (awk, 0, rio);
 * if (rtx)
 * {
 *    retv = qse_awk_rtx_loop (rtx);
 *    if (retv) qse_awk_rtx_refdownval (rtx, retv);
 *    qse_awk_rtx_close (rtx);
 * }
 * \endcode
 *
 * \return return value on success, #QSE_NULL on failure.
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_loop (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_findfun() function finds the function structure by name
 * and returns the pointer to it if one is found. It returns #QSE_NULL if
 * it fails to find a function by the \a name.
 */
QSE_EXPORT qse_awk_fun_t* qse_awk_rtx_findfun (
	qse_awk_rtx_t*    rtx, /**< runtime context */
	const qse_char_t* name /**< function name */
);

/**
 * The qse_awk_rtx_callfun() function invokes an AWK function described by
 * the structure pointed to by \a fun.
 * \sa qse_awk_rtx_call
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_callfun (
	qse_awk_rtx_t*  rtx,     /**< runtime context */
	qse_awk_fun_t*  fun,     /**< function */
	qse_awk_val_t*  args[],  /**< arguments to the function */
	qse_size_t      nargs    /**< the number of arguments */
);

/**
 * The qse_awk_rtx_call() function invokes an AWK function named \a name. 
 * However, it is not able to invoke an intrinsic function such as split(). 
 * The #QSE_AWK_PABLOCK option can be turned off to make illegal the BEGIN 
 * blocks, the pattern-action blocks, and the END blocks.
 *
 * The example shows typical usage of the function.
 * \code
 * rtx = qse_awk_rtx_open (awk, 0, rio);
 * if (rtx)
 * {
 *     v = qse_awk_rtx_call (rtx, QSE_T("init"), QSE_NULL, 0);
 *     if (v) qse_awk_rtx_refdownval (rtx, v);
 *     qse_awk_rtx_call (rtx, QSE_T("fini"), QSE_NULL, 0);
 *     if (v) qse_awk_rtx_refdownval (rtx, v);
 *     qse_awk_rtx_close (rtx);
 * }
 * \endcode
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_call (
	qse_awk_rtx_t*    rtx,    /**< runtime context */
	const qse_char_t* name,   /**< function name */
	qse_awk_val_t*    args[], /**< arguments to the function */
	qse_size_t        nargs   /**< the number of arguments */
);

/**
 * The qse_awk_rtx_callwithstrs() function is the same as qse_awk_rtx_call()
 * except that you pass pointers to null-terminated strings. It creates values
 * from the null-terminated strings and calls qse_awk_rtx_call() with the 
 * values created.
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_callwithstrs (
	qse_awk_rtx_t*    rtx,    /**< runtime context */
	const qse_char_t* name,   /**< function name */
	const qse_char_t* args[], /**< arguments to the function */
	qse_size_t        nargs   /**< the number of arguments */
);

/**
 * The qse_awk_stopall() function aborts all active runtime contexts
 * associated with \a awk.
 */
QSE_EXPORT void qse_awk_stopall (
	qse_awk_t* awk /**< awk */
);

/**
 * The qse_awk_rtx_isstop() function tests if qse_awk_rtx_stop() has been 
 * called.
 */
QSE_EXPORT int qse_awk_rtx_isstop (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_stop() function causes an active runtime context \a rtx to 
 * be aborted. 
 */
QSE_EXPORT void qse_awk_rtx_stop (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_getrio() function copies runtime I/O handlers
 * to the memory buffer pointed to by \a rio.
 */
QSE_EXPORT void qse_awk_rtx_getrio (
	qse_awk_rtx_t* rtx,
	qse_awk_rio_t* rio
);

/**
 * The qse_awk_rtx_getrio() function sets runtime I/O handlers
 * with the functions pointed to by \a rio.
 */
QSE_EXPORT void qse_awk_rtx_setrio (
	qse_awk_rtx_t*       rtx,
	const qse_awk_rio_t* rio
);

/**
 * The qse_awk_rtx_popecb() function pops a runtime callback set
 * and returns the pointer to it. If no callback set can be popped,
 * it returns #QSE_NULL.
 */
QSE_EXPORT qse_awk_rtx_ecb_t* qse_awk_rtx_popecb (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_pushecb() function register a runtime callback set.
 */
QSE_EXPORT void qse_awk_rtx_pushecb (
	qse_awk_rtx_t*     rtx, /**< runtime context */
	qse_awk_rtx_ecb_t* ecb  /**< callback set */
);

/**
 * The qse_awk_rtx_getnargs() gets the number of arguments passed to an 
 * intrinsic functon.
 */
QSE_EXPORT qse_size_t qse_awk_rtx_getnargs (
	qse_awk_rtx_t* rtx
);

/**
 * The qse_awk_rtx_getarg() function gets an argument passed to an intrinsic 
 * function. it doesn't touch the reference count of the value.
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_getarg (
	qse_awk_rtx_t* rtx,
	qse_size_t     idx
);

/**
 * The qse_awk_rtx_getsubsep() function returns the
 * pointer to the internal value of SUBSEP. It's a specialized
 * version of qse_awk_rtx_getgbl (rtx, QSE_AWK_GBL_SUBSEP).
 */
QSE_EXPORT const qse_xstr_t* qse_awk_rtx_getsubsep (
	qse_awk_rtx_t* rtx  /**< runtime context */
);

/**
 * The qse_awk_rtx_getgbl() gets the value of a global variable.
 * The global variable ID \a id is one of the predefined global 
 * variable IDs or a value returned by qse_awk_addgbl().
 * This function never fails so long as the ID is valid. Otherwise, 
 * you may get into trouble.
 *
 * \return value pointer
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_getgbl (
	qse_awk_rtx_t* rtx, /**< runtime context */
	int            id   /**< global variable ID */
);

/**
 * The qse_awk_rtx_setgbl() sets the value of a global variable.
 */
QSE_EXPORT int qse_awk_rtx_setgbl (
	qse_awk_rtx_t* rtx, 
	int            id,
	qse_awk_val_t* val
);

/**
 * The qse_awk_rtx_setretval() sets the return value of a function
 * when called from within a function handler. The caller doesn't
 * have to invoke qse_awk_rtx_refupval() and qse_awk_rtx_refdownval() 
 * with the value to be passed to qse_awk_rtx_setretval(). 
 * The qse_awk_rtx_setretval() will update its reference count properly
 * once the return value is set. 
 */
QSE_EXPORT void qse_awk_rtx_setretval (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_awk_val_t* val  /**< return value */
);

/**
 * The qse_awk_rtx_setfilename() function sets FILENAME.
 */
QSE_EXPORT int qse_awk_rtx_setfilename (
	qse_awk_rtx_t*    rtx, /**< runtime context */
	const qse_char_t* str, /**< name pointer */
	qse_size_t        len  /**< name length */
);

/**
 * The qse_awk_rtx_setofilename() function sets OFILENAME.
 */
QSE_EXPORT int qse_awk_rtx_setofilename (
	qse_awk_rtx_t*    rtx, /**< runtime context */
	const qse_char_t* str, /**< name pointer */
	qse_size_t        len  /**< name length */
);

/**
 * The qse_awk_rtx_getawk() function gets the owner of a runtime context \a rtx.
 * \return owner of a runtime context \a rtx.
 */
QSE_EXPORT qse_awk_t* qse_awk_rtx_getawk (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_getmmgr() function gets the memory manager of a runtime
 * context.
 */
QSE_EXPORT qse_mmgr_t* qse_awk_rtx_getmmgr (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_getxtn() function gets the pointer to the extension area
 * created with qse_awk_rtx_open().
 */
QSE_EXPORT void* qse_awk_rtx_getxtn (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_getnvmap() gets the map of named variables 
 */
QSE_EXPORT qse_htb_t* qse_awk_rtx_getnvmap (
	qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_geterrnum() function gets the number of the last error
 * occurred during runtime.
 * \return error number
 */
QSE_EXPORT qse_awk_errnum_t qse_awk_rtx_geterrnum (
	const qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_geterrloc() function gets the location of the last error
 * occurred during runtime. The 
 * \return error location
 */
QSE_EXPORT const qse_awk_loc_t* qse_awk_rtx_geterrloc (
	const qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_geterrmsg() function gets the string describing the last 
 * error occurred during runtime.
 * \return error message
 */
QSE_EXPORT const qse_char_t* qse_awk_rtx_geterrmsg (
	const qse_awk_rtx_t* rtx /**< runtime context */
);

/**
 * The qse_awk_rtx_geterrinf() function copies error information into memory
 * pointed to by \a errinf from a runtime context \a rtx.
 */
QSE_EXPORT void qse_awk_rtx_geterrinf (
	const qse_awk_rtx_t* rtx,   /**< runtime context */
	qse_awk_errinf_t*    errinf /**< error information */
);

/**
 * The qse_awk_rtx_geterror() function retrieves error information from a 
 * runtime context \a rtx. The error number is stored into memory pointed
 * to by \a errnum; the error message pointer into memory pointed to by 
 * \a errmsg; the error line into memory pointed to by \a errlin.
 */
QSE_EXPORT void qse_awk_rtx_geterror (
	const qse_awk_rtx_t* rtx,    /**< runtime context */
	qse_awk_errnum_t*    errnum, /**< error number */
	const qse_char_t**   errmsg, /**< error message */
	qse_awk_loc_t*       errloc  /**< error location */
);

/** 
 * The qse_awk_rtx_seterrnum() function sets the error information omitting
 * the error location.
 */
QSE_EXPORT void qse_awk_rtx_seterrnum (
	qse_awk_rtx_t*    rtx,    /**< runtime context */
	qse_awk_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< arguments to format error message */
);

/** 
 * The qse_awk_rtx_seterrinf() function sets error information.
 */
QSE_EXPORT void qse_awk_rtx_seterrinf (
	qse_awk_rtx_t*          rtx,   /**< runtime context */
	const qse_awk_errinf_t* errinf /**< error information */
);

/**
 * The qse_awk_rtx_seterror() function sets error information.
 */
QSE_EXPORT void qse_awk_rtx_seterror (
	qse_awk_rtx_t*       rtx,    /**< runtime context */
	qse_awk_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< argument array for formatting 
	                              *   an error message */
	const qse_awk_loc_t* errloc  /**< error line */
);

/**
 * The qse_awk_rtx_clrrec() function clears the input record ($0) 
 * and fields ($1 to $N).
 */
QSE_EXPORT int qse_awk_rtx_clrrec (
	qse_awk_rtx_t* rtx, /**< runtime context */
	int            skip_inrec_line 
);

/**
 * The qse_awk_rtx_setrec() function sets the input record ($0) or 
 * input fields ($1 to $N).
 */
QSE_EXPORT int qse_awk_rtx_setrec (
	qse_awk_rtx_t*    rtx, /**< runtime context */
	qse_size_t        idx, /**< 0 for $0, N for $N */
	const qse_cstr_t* str  /**< string */
);

/**
 * The qse_awk_rtx_isnilval(0 function determines if a value
 * is a nil value.
 */
QSE_EXPORT int qse_awk_rtx_isnilval (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

/**
 * The qse_awk_rtx_makenilval() function creates a nil value.
 * It always returns the pointer to the statically allocated
 * nil value. So it never fails.
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makenilval (
	qse_awk_rtx_t* rtx
);

/**
 * The qse_awk_rtx_makeintval() function creates an integer value.
 * If \a v is one of -1, 0, 1, this function never fails.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makeintval (
	qse_awk_rtx_t* rtx,
	qse_long_t     v
);

/**
 * The qse_awk_rtx_makefltval() function creates a floating-point value.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makefltval (
	qse_awk_rtx_t* rtx,
	qse_flt_t      v
);

/**
 * The qse_awk_rtx_makestrvalwithstr() function creates a string value.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrvalwithstr (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str
);

/**
 * The qse_awk_rtx_makestrvalwithmbs() function creates a string value
 * from a null-terminated multibyte string.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrvalwithmbs (
     qse_awk_rtx_t*     rtx,
	const qse_mchar_t* mbs
);

/**
 * The qse_awk_rtx_makestrvalwithwcs() function creates a string value
 * from a null-terminated wide-character string.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrvalwithwcs (
     qse_awk_rtx_t*     rtx,
	const qse_wchar_t* wcs
);

/**
 * The qse_awk_rtx_makestrvalwithcstr() function creates a string value.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrvalwithcstr (
	qse_awk_rtx_t*    rtx,
	const qse_cstr_t* str
);

QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrvalwithmcstr (
	qse_awk_rtx_t*     rtx,
	const qse_mcstr_t* mcstr
);

QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrvalwithwcstr (
	qse_awk_rtx_t*     rtx,
	const qse_wcstr_t* wcstr
);

/**
 * The qse_awk_rtx_makestrval() function creates a string value.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrval (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str,
	qse_size_t        len
);

/**
 * The qse_awk_rtx_makestrval2() function creates a string value combining
 * two strings.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makestrval2 (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str1,
	qse_size_t        len1, 
	const qse_char_t* str2,
	qse_size_t        len2
);


/**
 * The qse_awk_rtx_makenstrvalwithstr() function creates a numeric string 
 * value from a null-terminated string. A numeric string is a string value 
 * whose one of the header fields \b nstr is 1.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makenstrvalwithstr (
	qse_awk_rtx_t* rtx,
	const qse_char_t* str
);

/**
 * The qse_awk_rtx_makenstrvalwithcstr() function creates a numeric string 
 * value. A numeric string is a string value whose one of the header fields
 * \b nstr is 1.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makenstrvalwithcstr (
	qse_awk_rtx_t*    rtx,
	const qse_cstr_t* str 
);

/**
 * The qse_awk_rtx_makerexval() function creates a regular expression value.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makerexval (
	qse_awk_rtx_t*    rtx,
	const qse_cstr_t* str,
	void*             code[2]
);

/**
 * The qse_awk_rtx_makemapval() function creates an empty map value.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makemapval (
	qse_awk_rtx_t* rtx
);

/**
 * The qse_awk_rtx_makemapvalwithdata() function creates a map value
 * containing key/value pairs described in the structure array \a data.
 * It combines qse_awk_rtx_makemapval() an qse_awk_rtx_setmapvalfld()
 * for convenience sake.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makemapvalwithdata (
	qse_awk_rtx_t*         rtx,
	qse_awk_val_map_data_t data[]
);

/**
 * The qse_awk_rtx_setmapvalfld() function sets a field value in a map.
 * You must make sure that the type of \a map is #QSE_AWK_VAL_MAP.
 * \return value \a v on success, #QSE_NULL on failure.
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_setmapvalfld (
	qse_awk_rtx_t*    rtx,
	qse_awk_val_t*    map,
	const qse_char_t* kptr,
	qse_size_t        klen,
	qse_awk_val_t*    v
);

/**
 * The qse_awk_rtx_setmapvalfld() function gets the field value in a map.
 * You must make sure that the type of \a map is #QSE_AWK_VAL_MAP.
 * If the field is not found, the function fails and sets the error number
 * to #QSE_AWK_EINVAL. The function does not fail for other reasons.
 * \return field value on success, #QSE_NULL on failure.
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_getmapvalfld (
	qse_awk_rtx_t*     rtx,
	qse_awk_val_t*     map,
	const qse_char_t*  kptr,
	qse_size_t         klen
);

/**
 * The qse_awk_rtx_getfirstmapvalitr() returns the iterator to the
 * first pair in the map. It returns #QSE_NULL and sets the pair field of 
 * \a itr to #QSE_NULL if the map contains no pair. Otherwise, it returns
 * \a itr pointing to the first pair.
 */
QSE_EXPORT qse_awk_val_map_itr_t* qse_awk_rtx_getfirstmapvalitr (
     qse_awk_rtx_t*         rtx,
	qse_awk_val_t*         map,
	qse_awk_val_map_itr_t* itr
);

/**
 * The qse_awk_rtx_getnextmapvalitr() returns the iterator to the
 * next pair to \a itr in the map. It returns #QSE_NULL and sets the pair 
 * field of \a itr to #QSE_NULL if \a itr points to the last pair.
 * Otherwise, it returns \a itr pointing to the next pair.
 */
QSE_EXPORT qse_awk_val_map_itr_t* qse_awk_rtx_getnextmapvalitr (
     qse_awk_rtx_t*         rtx,
	qse_awk_val_t*         map,
	qse_awk_val_map_itr_t* itr
);


/**
 * The qse_awk_rtx_makerefval() function creates a reference value.
 * \return value on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_awk_val_t* qse_awk_rtx_makerefval (
	qse_awk_rtx_t*  rtx,
	int             id,
	qse_awk_val_t** adr
);

/**
 * The qse_awk_rtx_isstaticval() function determines if a value is static.
 * A static value is allocated once and reused until a runtime context @ rtx 
 * is closed.
 * \return QSE_TRUE if \a val is static, QSE_FALSE if \a val is false
 */
QSE_EXPORT int qse_awk_rtx_isstaticval (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_awk_val_t* val  /**< value to check */
);

/**
 * The qse_awk_rtx_refupval() function increments a reference count of a 
 * value \a val.
 */
QSE_EXPORT void qse_awk_rtx_refupval (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_awk_val_t* val  /**< value */
);

/**
 * The qse_awk_rtx_refdownval() function decrements a reference count of
 * a value \a val. It destroys the value if it has reached the count of 0.
 */
QSE_EXPORT void qse_awk_rtx_refdownval (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_awk_val_t* val  /**< value pointer */
);

/**
 * The qse_awk_rtx_refdownval() function decrements a reference count of
 * a value \a val. It does not destroy the value if it has reached the 
 * count of 0.
 */
QSE_EXPORT void qse_awk_rtx_refdownval_nofree (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_awk_val_t* val  /**< value pointer */
);

/**
 * The qse_awk_rtx_valtobool() function converts a value \a val to a boolean
 * value.
 */
QSE_EXPORT int qse_awk_rtx_valtobool (
	qse_awk_rtx_t*       rtx, /**< runtime context */
	const qse_awk_val_t* val  /**< value pointer */
);

/**
 * The qse_awk_rtx_valtostr() function converts a value \a val to a string as 
 * instructed in the parameter out. Before the call to the function, you 
 * should initialize a variable of the #qse_awk_rtx_valtostr_out_t type.
 *
 * The type field is one of the following qse_awk_rtx_valtostr_type_t values:
 *
 * - #QSE_AWK_RTX_VALTOSTR_CPL
 * - #QSE_AWK_RTX_VALTOSTR_CPLCPY
 * - #QSE_AWK_RTX_VALTOSTR_CPLDUP
 * - #QSE_AWK_RTX_VALTOSTR_STRP
 * - #QSE_AWK_RTX_VALTOSTR_STRPCAT
 *
 * It can optionally be ORed with #QSE_AWK_RTX_VALTOSTR_PRINT. The option
 * causes the function to use OFMT for real number conversion. Otherwise,
 * it uses \b CONVFMT. 
 *
 * You should initialize or free other fields before and after the call 
 * depending on the type field as shown below:
 *  
 * If you have a static buffer, use #QSE_AWK_RTX_VALTOSTR_CPLCPY.
 * the resulting string is copied to the buffer.
 * \code
 * qse_awk_rtx_valtostr_out_t out;
 * qse_char_t buf[100];
 * out.type = QSE_AWK_RTX_VALTOSTR_CPLCPY;
 * out.u.cplcpy.ptr = buf;
 * out.u.cplcpy.len = QSE_COUNTOF(buf);
 * if (qse_awk_rtx_valtostr (rtx, v, &out) <= -1) goto oops;
 * qse_printf (QSE_T("%.*s\n"), (int)out.u.cplcpy.len, out.u.cplcpy.ptr);
 * \endcode
 *
 * #QSE_AWK_RTX_VALTOSTR_CPL is different from #QSE_AWK_RTX_VALTOSTR_CPLCPY
 * in that it doesn't copy the string to the buffer if the type of the value
 * is #QSE_AWK_VAL_STR. It copies the resulting string to the buffer if
 * the value type is not #QSE_AWK_VAL_STR. 
 * \code
 * qse_awk_rtx_valtostr_out_t out;
 * qse_char_t buf[100];
 * out.type = QSE_AWK_RTX_VALTOSTR_CPL;
 * out.u.cpl.ptr = buf;
 * out.u.cpl.len = QSE_COUNTOF(buf);
 * if (qse_awk_rtx_valtostr (rtx, v, &out) <= -1) goto oops;
 * qse_printf (QSE_T("%.*s\n"), (int)out.u.cpl.len, out.u.cpl.ptr);
 * \endcode
 * 
 * When unsure of the size of the string after conversion, you can use
 * #QSE_AWK_RTX_VALTOSTR_CPLDUP. However, you should free the memory block
 * pointed to by the u.cpldup.ptr field after use.
 * \code
 * qse_awk_rtx_valtostr_out_t out;
 * out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
 * if (qse_awk_rtx_valtostr (rtx, v, &out) <= -1) goto oops;
 * qse_printf (QSE_T("%.*s\n"), (int)out.u.cpldup.len, out.u.cpldup.ptr);
 * qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
 * \endcode
 *
 * You may like to store the result in a dynamically resizable string.
 * Consider #QSE_AWK_RTX_VALTOSTR_STRP.
 * \code
 * qse_awk_rtx_valtostr_out_t out;
 * qse_str_t str;
 * qse_str_init (&str, qse_awk_rtx_getmmgr(rtx), 100);
 * out.type = QSE_AWK_RTX_VALTOSTR_STRP;
 * out.u.strp = str;
 * if (qse_awk_rtx_valtostr (rtx, v, &out) <= -1) goto oops;
 * qse_printf (QSE_T("%.*s\n"), 
 *     (int)QSE_STR_LEN(out.u.strp), QSE_STR_PTR(out.u.strp));
 * qse_str_fini (&str);
 * \endcode
 * 
 * If you want to append the converted string to an existing dynamically 
 * resizable string, #QSE_AWK_RTX_VALTOSTR_STRPCAT is the answer. The usage is
 * the same as #QSE_AWK_RTX_VALTOSTR_STRP except that you have to use the 
 * u.strpcat field instead of the u.strp field.
 *
 * In the context where \a val is determined to be of the type
 * #QSE_AWK_VAL_STR, you may access its string pointer and length directly
 * instead of calling this function.
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_awk_rtx_valtostr (
	qse_awk_rtx_t*              rtx, /**< runtime context */
	const qse_awk_val_t*        val, /**< value to convert */
	qse_awk_rtx_valtostr_out_t* out  /**< output buffer */
);

/**
 * The qse_awk_rtx_valtostrdup() function provides a shortcut to the 
 * qse_awk_rtx_valtostr() function with the #QSE_AWK_RTX_VALTOSTR_CPLDUP type.
 * It returns the pointer to a string converted from \a val and stores its 
 * length to memory pointed to by \a len. You should free the returned
 * memory block after use. See the code snippet below for a simple usage.
 *
 * \code
 * ptr = qse_awk_rtx_valtostrdup (rtx, v, &len);
 * if (str == QSE_NULL) handle_error();
 * qse_printf (QSE_T("%.*s\n"), (int)len, ptr);
 * qse_awk_rtx_free (rtx, ptr);
 * \endcode
 *
 * \return character pointer to a string converted on success,
 *         #QSE_NULL on failure
 */
QSE_EXPORT qse_char_t* qse_awk_rtx_valtostrdup (
	qse_awk_rtx_t*       rtx, /**< runtime context */
	const qse_awk_val_t* val, /**< value to convert */
	qse_size_t*          len  /**< result length */
);

QSE_EXPORT qse_mchar_t* qse_awk_rtx_valtombsdup (
	qse_awk_rtx_t*       rtx, /**< runtime context */
	const qse_awk_val_t* val, /**< value to convert */
	qse_size_t*          len  /**< result length */
);

QSE_EXPORT qse_wchar_t* qse_awk_rtx_valtowcsdup (
	qse_awk_rtx_t*       rtx, /**< runtime context */
	const qse_awk_val_t* val, /**< value to convert */
	qse_size_t*          len  /**< result length */
);

/**
 * The qse_awk_rtx_valtonum() function converts a value to a number. 
 * If the value is converted to a long number, it is stored in the memory
 * pointed to by l and 0 is returned. If the value is converted to a real 
 * number, it is stored in the memory pointed to by r and 1 is returned.
 * The function never fails as long as \a val points to a valid value.
 *
 * The code below shows how to convert a value to a number and determine
 * if it is an integer or a floating-point number.
 *
 * \code
 * qse_long_t l;
 * qse_flt_t r;
 * int n;
 * n = qse_awk_rtx_valtonum (v, &l, &r);
 * if (n <= -1) error ();
 * else if (n == 0) print_long (l);
 * else if (n >= 1) print_real (r);
 * \endcode
 *
 * \return -1 on failure, 0 if converted to a long number, 1 if converted to 
 *         a floating-point number.
 */
QSE_EXPORT int qse_awk_rtx_valtonum (
	qse_awk_rtx_t*       rtx,
	const qse_awk_val_t* val,
	qse_long_t*          l,
	qse_flt_t*          r
);

QSE_EXPORT int qse_awk_rtx_valtolong (
	qse_awk_rtx_t*       rtx,
	const qse_awk_val_t* val,
	qse_long_t*          l
);

QSE_EXPORT int qse_awk_rtx_valtoflt (
	qse_awk_rtx_t*       rtx,
	const qse_awk_val_t* val,
	qse_flt_t*           r
);

/**
 * The qse_awk_rtx_strtonum() function converts a string to a number.
 * A numeric string in the valid decimal, hexadecimal(0x), binary(0b), 
 * octal(0) notation is converted to an integer and it is stored into
 * memory pointed to by \a l; A string containng '.', 'E', or 'e' is 
 * converted to a floating-pointer number and it is stored into memory
 * pointed to by \a r. If \a strict is 0, the function takes up to the last
 * valid character and never fails. If \a strict is non-zero, an invalid 
 * character causes the function to return an error.
 *
 * \return 0 if converted to an integer,
 *         1 if converted to a floating-point number
 *         -1 on error.
 */
QSE_EXPORT int qse_awk_rtx_strtonum (
	qse_awk_rtx_t*    rtx, /**< runtime context */
	int               strict, /**< determines to perform strict check */
	const qse_char_t* ptr, /**< points to a string to convert */
	qse_size_t        len, /**< number of characters in a string */
	qse_long_t*       l,   /**< stores a converted integer */
	qse_flt_t*        r    /**< stores a converted floating-poing number */
);

/**
 * The qse_awk_rtx_hashval() function hashes a simple value
 * to a positive integer. It returns -1 for a inhashable value.
 */
QSE_EXPORT qse_long_t qse_awk_rtx_hashval (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* v
);

/**
 * The qse_awk_rtx_setrefval() function changes the value
 * of a variable referenced in \a ref. 
 * \return 0 on success, -1 on failure.
 */
QSE_EXPORT int qse_awk_rtx_setrefval (
	qse_awk_rtx_t*     rtx,
	qse_awk_val_ref_t* ref,
	qse_awk_val_t*     val
);

QSE_EXPORT void qse_awk_rtx_setnrflt (
	qse_awk_rtx_t*         rtx,
	const qse_awk_nrflt_t* nrflt
);

QSE_EXPORT void qse_awk_rtx_getnrflt (
	qse_awk_rtx_t*         rtx,
	qse_awk_nrflt_t*       nrflt
);

/**
 * The qse_awk_rtx_allocmem() function allocats a memory block of \a size bytes
 * using the memory manager associated with a runtime context \a rtx. 
 * \return the pointer to a memory block on success, #QSE_NULL on failure.
 */
QSE_EXPORT void* qse_awk_rtx_allocmem (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_size_t     size /**< block size in bytes */
);

/**
 * The qse_awk_rtx_reallocmem() function resizes a memory block pointed to
 * by \a ptr to \a size bytes using the memory manager associated with 
 * a runtime context \a rtx. 
 * \return the pointer to a memory block on success, #QSE_NULL on failure.
 */
QSE_EXPORT void* qse_awk_rtx_reallocmem (
	qse_awk_rtx_t* rtx, /**< runtime context */
	void*          ptr, /**< memory block */
	qse_size_t     size /**< block size in bytes */
);

/**
 * The qse_awk_rtx_callocmem() function allocates a memory block of 
 * the size of \a size bytes and initializes it with 0.
 * \return a pointer to a memory block on success, #QSE_NULL on failure
 */
QSE_EXPORT void* qse_awk_rtx_callocmem (
	qse_awk_rtx_t* rtx, /**< runtime context */
	qse_size_t     size /**< block size in bytes */
);

/**
 * The qse_awk_rtx_freemem() function frees a memory block pointed to by \a ptr
 * using the memory manager of a runtime ocntext \a rtx.
 */
QSE_EXPORT void qse_awk_rtx_freemem (
	qse_awk_rtx_t* rtx, /**< runtime context */
	void*          ptr  /**< memory block pointer */
);

/**
 * The qse_getawknilval() function returns the pointer to the predefined
 * nil value. you can call this without creating a runtime context. 
 */
QSE_EXPORT qse_awk_val_t* qse_getawknilval (
	void
);

#ifdef __cplusplus
}
#endif

#endif
