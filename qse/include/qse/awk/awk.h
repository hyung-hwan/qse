/*
 * $Id: awk.h 206 2009-06-21 13:33:05Z hyunghwan.chung $
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

#ifndef _QSE_AWK_AWK_H_
#define _QSE_AWK_AWK_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/map.h>
#include <qse/cmn/str.h>

/** @file
 * An embeddable AWK interpreter is defined in this header files.
 *
 * @example awk.c
 * This program demonstrates how to build a complete awk interpreter.
 * @example awk01.c
 * This program demonstrates how to use qse_awk_rtx_loop().
 * @example awk02.c
 * The program deparses the source code and prints it before executing it.
 * @example awk03.c
 * This program demonstrates how to use qse_awk_rtx_call().
 * It parses the program stored in the string src and calls the functions
 * stated in the array fnc. If no errors occur, it should print 24.
 */

/** @struct qse_awk_t
 * The qse_awk_t type defines an AWK interpreter. The details are hidden as
 * it is a complex type susceptible to misuse.
 */
typedef struct qse_awk_t qse_awk_t;

/** @struct qse_awk_rtx_t
 * The qse_awk_rtx_t type defines a runtime context. The details are hidden
 * as it is a complex type susceptible to misuse.
 */
typedef struct qse_awk_rtx_t qse_awk_rtx_t; /* (R)untime con(T)e(X)t */

/**
 * The QSE_AWK_VAL_HDR defines the common header of a value type.
 */
#if QSE_SIZEOF_INT == 2
#	define QSE_AWK_VAL_HDR \
		unsigned int type: 3; \
		unsigned int ref: 11; \
		unsigned int nstr: 2
#else
#	define QSE_AWK_VAL_HDR \
		unsigned int type: 3; \
		unsigned int ref: 27; \
		unsigned int nstr: 2
#endif

#define QSE_AWK_VAL_TYPE(x) ((x)->type)

/**
 * The qse_awk_val_t type is an abstract value type
 */
struct qse_awk_val_t
{
	QSE_AWK_VAL_HDR;	
};
typedef struct qse_awk_val_t qse_awk_val_t;

/**
 * The qse_awk_val_nil_t type is a nil value type. The type field is 
 * QSE_AWK_VAL_NIL.
 */
struct qse_awk_val_nil_t
{
	QSE_AWK_VAL_HDR;
};
typedef struct qse_awk_val_nil_t  qse_awk_val_nil_t;

/**
 * The qse_awk_val_int_t type is an integer number type. The type field is
 * QSE_AWK_VAL_INT.
 */
struct qse_awk_val_int_t
{
	QSE_AWK_VAL_HDR;
	qse_long_t val;
	void*      nde;
};
typedef struct qse_awk_val_int_t qse_awk_val_int_t;

/**
 * The qse_awk_val_real_t type is a floating-point number type. The type field
 * is QSE_AWK_VAL_REAL.
 */
struct qse_awk_val_real_t
{
	QSE_AWK_VAL_HDR;
	qse_real_t val;
	void*      nde;
};
typedef struct qse_awk_val_real_t qse_awk_val_real_t;

/**
 * The qse_awk_val_str_t type is a string type. The type field is
 * QSE_AWK_VAL_STR.
 */
struct qse_awk_val_str_t
{
	QSE_AWK_VAL_HDR;
	qse_char_t* ptr;
	qse_size_t  len;
};
typedef struct qse_awk_val_str_t  qse_awk_val_str_t;

/**
 * The qse_awk_val_rex_t type is a regular expression type.  The type field 
 * is QSE_AWK_VAL_REX.
 */
struct qse_awk_val_rex_t
{
	QSE_AWK_VAL_HDR;
	qse_char_t* ptr;
	qse_size_t  len;
	void*       code;
};
typedef struct qse_awk_val_rex_t  qse_awk_val_rex_t;

/* QSE_AWK_VAL_MAP */
struct qse_awk_val_map_t
{
	QSE_AWK_VAL_HDR;

	/* TODO: make val_map to array if the indices used are all 
	 *       integers switch to map dynamically once the 
	 *       non-integral index is seen.
	 */
	qse_map_t* map; 
};
typedef struct qse_awk_val_map_t  qse_awk_val_map_t;

/* QSE_AWK_VAL_REF */
struct qse_awk_val_ref_t
{
	QSE_AWK_VAL_HDR;

	int id;
	/* if id is QSE_AWK_VAL_REF_POS, adr holds an index of the 
	 * positional variable. Otherwise, adr points to the value 
	 * directly. */
	qse_awk_val_t** adr;
};
typedef struct qse_awk_val_ref_t  qse_awk_val_ref_t;

typedef qse_real_t (*qse_awk_pow_t) (
	qse_awk_t* awk,
	qse_real_t x, 
	qse_real_t y
);

typedef int (*qse_awk_sprintf_t) (
	qse_awk_t*        awk,
	qse_char_t*       buf,
	qse_size_t        size, 
	const qse_char_t* fmt,
	...
);

/**
 * The qse_awk_sio_cmd_t type defines source IO commands
 */
enum qse_awk_sio_cmd_t
{
	QSE_AWK_SIO_OPEN   = 0,
	QSE_AWK_SIO_CLOSE  = 1,
	QSE_AWK_SIO_READ   = 2,
	QSE_AWK_SIO_WRITE  = 3
};
typedef enum qse_awk_sio_cmd_t qse_awk_sio_cmd_t;

/**
 * The qse_awk_sio_fun_t type defines a source IO function
 */
typedef qse_ssize_t (*qse_awk_sio_fun_t) (
	qse_awk_t*        awk,
	qse_awk_sio_cmd_t cmd, 
	qse_char_t*       data,
	qse_size_t        count
);

/**
 * The qse_awk_rio_cmd_t type defines runtime IO commands.
 */
enum qse_awk_rio_cmd_t
{
	QSE_AWK_RIO_OPEN   = 0,
	QSE_AWK_RIO_CLOSE  = 1,
	QSE_AWK_RIO_READ   = 2,
	QSE_AWK_RIO_WRITE  = 3,
	QSE_AWK_RIO_FLUSH  = 4,
	QSE_AWK_RIO_NEXT   = 5  
};
typedef enum qse_awk_rio_cmd_t qse_awk_rio_cmd_t;

/**
 * The qse_awk_rio_arg_t defines the data passed to a rio function 
 */
struct qse_awk_rio_arg_t 
{
	int type;           /* [IN] console, file, pipe */
	int mode;           /* [IN] read, write, etc */
	qse_char_t* name;   /* [IN] */
	void* handle;       /* [OUT] */

	/* input */
	struct
	{
		qse_char_t buf[2048];
		qse_size_t pos;
		qse_size_t len;
		qse_bool_t eof;
		qse_bool_t eos;
	} in;

	/* output */
	struct
	{
		qse_bool_t eof;
		qse_bool_t eos;
	} out;

	struct qse_awk_rio_arg_t* next;
};
typedef struct qse_awk_rio_arg_t qse_awk_rio_arg_t;

/**
 * The qse_awk_rio_fun_t type defines a runtime IO function
 */
typedef qse_ssize_t (*qse_awk_rio_fun_t) (
	qse_awk_rtx_t*      rtx,
	qse_awk_rio_cmd_t   cmd,
	qse_awk_rio_arg_t*  riod,
	qse_char_t*         data,
	qse_size_t          count
);

/**
 * The qse_awk_prm_t type defines primitive functions
 */
struct qse_awk_prm_t
{
	qse_awk_pow_t     pow;
	qse_awk_sprintf_t sprintf;

#if 0
	/* TODO: accept regular expression handling functions */
	void* (*build) (
		qse_awk_t*        awk,
		const qse_char_t* ptn, 
		qse_size_t        len, 
		int*              errnum
	);

	int (*match) (
		qse_awk_t*         awk,
		void*              code,
		int                option,
		const qse_char_t*  str,
		qse_size_t         len, 
		const qse_char_t** mptr,
		qse_size_t*        mlen, 
		int*               errnum
	);

	void (*free) (
		qse_awk_t* awk,
		void*      code
	);

	qse_bool_t (*isempty) (
		qse_awk_t* awk,
		void*      code
	);
#endif
};
typedef struct qse_awk_prm_t qse_awk_prm_t;

/**
 * The qse_awk_sio_t type defines source script IO.
 */
struct qse_awk_sio_t
{
	qse_awk_sio_fun_t in;
	qse_awk_sio_fun_t out;
};
typedef struct qse_awk_sio_t qse_awk_sio_t;

/**
 * The qse_awk_rio_t type defines a runtime IO set.
 */
struct qse_awk_rio_t
{
	qse_awk_rio_fun_t pipe;
	qse_awk_rio_fun_t file;
	qse_awk_rio_fun_t console;
};
typedef struct qse_awk_rio_t qse_awk_rio_t;

/**
 * The qse_awk_rcb_t type defines runtime callbacks
 */
struct qse_awk_rcb_t
{
	int (*on_loop_enter) (
		qse_awk_rtx_t* rtx, void* data);

	void (*on_loop_exit) (
		qse_awk_rtx_t* rtx, qse_awk_val_t* ret, void* data);

	void (*on_statement) (
		qse_awk_rtx_t* rtx, qse_size_t line, void* data);

	void* data;
};
typedef struct qse_awk_rcb_t qse_awk_rcb_t;

/**
 * The qse_awk_option_t type defines various options to change the behavior
 * of #qse_awk_t.
 */
enum qse_awk_option_t
{ 
	/**
	 * allows undeclared variables and implicit concatenation 
	 **/
	QSE_AWK_IMPLICIT    = (1 << 0),

	/** 
	 * allows explicit variable declaration, the concatenation
	 * operator, a period, and performs the parse-time function check. 
	 */
	QSE_AWK_EXPLICIT    = (1 << 1), 

	/** changes @b ^ from exponentation to bitwise xor */
	QSE_AWK_BXOR        = (1 << 3),

	/** supports shift operators: @b << and @b >> */
	QSE_AWK_SHIFT       = (1 << 4), 

	/** enables the idiv operator: @b // */
	QSE_AWK_IDIV        = (1 << 5), 

	/** supports @b getline and @b print */
	QSE_AWK_RIO         = (1 << 7), 

	/** supports dual direction pipe if QSE_AWK_RIO is on */
	QSE_AWK_RWPIPE      = (1 << 8),

	/** a new line can terminate a statement */
	QSE_AWK_NEWLINE     = (1 << 9),

	/** 
	 * strips off leading and trailing spaces when splitting a record
	 * into fields with a regular expression.
	 *
	 * @code
	 * BEGIN { FS="[:[:space:]]+"; } 
	 * { 
	 *    print "NF=" NF; 
	 *    for (i = 0; i < NF; i++) print i " [" $(i+1) "]";
	 * }
	 * @endcode
	 * " a b c " is split to [a], [b], [c] if #QSE_AWK_STRIPSPACES is on.
	 * Otherwise, it is split to [], [a], [b], [c], [].
	 */
	QSE_AWK_STRIPSPACES = (1 << 11),

	/** enables @b nextofile */
	QSE_AWK_NEXTOFILE   = (1 << 12),

	/** CR + LF by default */
	QSE_AWK_CRLF        = (1 << 13),

	/** enables @b reset */
	QSE_AWK_RESET       = (1 << 14),

	/** allows the assignment of a map value to a variable */
	QSE_AWK_MAPTOVAR    = (1 << 15),

	/** allows @b BEGIN, @b END, pattern-action blocks */
	QSE_AWK_PABLOCK     = (1 << 16),

	/** allows {n,m} in a regular expression. */
	QSE_AWK_REXBOUND    = (1 << 17),

	/** 
	 * performs numeric comparison when a string convertable
	 * to a number is compared with a number or vice versa.
	 *
	 * For an expression (9 > "10.9"),
	 * - 9 is greater if #QSE_AWK_NCMPONSTR is off;
	 * - "10.9" is greater if #QSE_AWK_NCMPONSTR is on
	 */
	QSE_AWK_NCMPONSTR = (1 << 18),

	/** 
	 * makes #qse_awk_t to behave as compatibly as classical AWK
	 * implementations 
	 */
	QSE_AWK_CLASSIC  = QSE_AWK_IMPLICIT | QSE_AWK_RIO | 
	                   QSE_AWK_NEWLINE | QSE_AWK_PABLOCK | 
	                   QSE_AWK_STRIPSPACES
};

/**
 * The qse_awk_errnum_t type defines error codes.
 */
enum qse_awk_errnum_t
{
	QSE_AWK_ENOERR,         /* no error */
	QSE_AWK_EUNKNOWN,       /* unknown error */

	/* common errors */
	QSE_AWK_EINVAL,         /* invalid parameter or data */
	QSE_AWK_ENOMEM,         /* out of memory */
	QSE_AWK_ENOSUP,         /* not supported */
	QSE_AWK_ENOPER,         /* operation not allowed */
	QSE_AWK_ENODEV,         /* no such device */
	QSE_AWK_ENOSPC,         /* no space left on device */
	QSE_AWK_EMFILE,         /* too many open files */
	QSE_AWK_EMLINK,         /* too many links */
	QSE_AWK_EAGAIN,         /* resource temporarily unavailable */
	QSE_AWK_ENOENT,         /* '${1}' not existing */
	QSE_AWK_EEXIST,         /* file or data exists */
	QSE_AWK_EFTBIG,         /* file or data too big */
	QSE_AWK_ETBUSY,         /* system too busy */
	QSE_AWK_EISDIR,         /* is a directory */
	QSE_AWK_RIOERR,         /* i/o error */

	/* mostly parse errors */
	QSE_AWK_EOPEN,          /* cannot open */
	QSE_AWK_EREAD,          /* cannot read */
	QSE_AWK_EWRITE,         /* cannot write */
	QSE_AWK_ECLOSE,         /* cannot close */

	QSE_AWK_EINTERN,        /* internal error */
	QSE_AWK_ERUNTIME,       /* run-time error */
	QSE_AWK_EBLKNST,        /* blocke nested too deeply */
	QSE_AWK_EEXPRNST,       /* expression nested too deeply */

	QSE_AWK_ESINOP,         /* failed to open source input */
	QSE_AWK_ESINCL,         /* failed to close source output */
	QSE_AWK_ESINRD,         /* failed to read source input */

	QSE_AWK_ESOUTOP,        /* failed to open source output */
	QSE_AWK_ESOUTCL,        /* failed to close source output */
	QSE_AWK_ESOUTWR,        /* failed to write source output */

	QSE_AWK_ELXCHR,         /* lexer came accross an wrong character */
	QSE_AWK_ELXDIG,         /* invalid digit */
	QSE_AWK_ELXUNG,         /* lexer failed to unget a character */

	QSE_AWK_EENDSRC,        /* unexpected end of source */
	QSE_AWK_EENDCMT,        /* a comment not closed properly */
	QSE_AWK_EENDSTR,        /* a string not closed with a quote */
	QSE_AWK_EENDREX,        /* unexpected end of a regular expression */
	QSE_AWK_ELBRACE,        /* left brace expected */
	QSE_AWK_ELPAREN,        /* left parenthesis expected */
	QSE_AWK_ERPAREN,        /* right parenthesis expected */
	QSE_AWK_ERBRACK,        /* right bracket expected */
	QSE_AWK_ECOMMA,         /* comma expected */
	QSE_AWK_ESCOLON,        /* semicolon expected */
	QSE_AWK_ECOLON,         /* colon expected */
	QSE_AWK_ESTMEND,        /* statement not ending with a semicolon */
	QSE_AWK_EIN,            /* keyword 'in' is expected */
	QSE_AWK_ENOTVAR,        /* not a variable name after 'in' */
	QSE_AWK_EEXPRES,        /* expression expected */

	QSE_AWK_EFUNCTION,      /* keyword 'function' is expected */
	QSE_AWK_EWHILE,         /* keyword 'while' is expected */
	QSE_AWK_EASSIGN,        /* assignment statement expected */
	QSE_AWK_EIDENT,         /* identifier expected */
	QSE_AWK_EFUNNAME,       /* not a valid function name */
	QSE_AWK_EBLKBEG,        /* BEGIN requires an action block */
	QSE_AWK_EBLKEND,        /* END requires an action block */
	QSE_AWK_EDUPBEG,        /* duplicate BEGIN */
	QSE_AWK_EDUPEND,        /* duplicate END */
	QSE_AWK_EKWRED,         /* keyword redefined */
	QSE_AWK_EFNCRED,        /* intrinsic function redefined */
	QSE_AWK_EFUNRED,        /* function redefined */
	QSE_AWK_EGBLRED,        /* global variable redefined */
	QSE_AWK_EPARRED,        /* parameter redefined */
	QSE_AWK_EVARRED,        /* named variable redefined */
	QSE_AWK_EDUPPAR,        /* duplicate parameter name */
	QSE_AWK_EDUPGBL,        /* duplicate global variable name */
	QSE_AWK_EDUPLCL,        /* duplicate local variable name */
	QSE_AWK_EBADPAR,        /* not a valid parameter name */
	QSE_AWK_EBADVAR,        /* not a valid variable name */
	QSE_AWK_EUNDEF,         /* undefined identifier */
	QSE_AWK_ELVALUE,        /* l-value required */
	QSE_AWK_EGBLTM,         /* too many global variables */
	QSE_AWK_ELCLTM,         /* too many local variables */
	QSE_AWK_EPARTM,         /* too many parameters */
	QSE_AWK_EDELETE,        /* delete not followed by a variable */
	QSE_AWK_ERESET,         /* reset not followed by a variable */
	QSE_AWK_EBREAK,         /* break outside a loop */
	QSE_AWK_ECONTINUE,      /* continue outside a loop */
	QSE_AWK_ENEXTBEG,       /* next illegal in BEGIN block */
	QSE_AWK_ENEXTEND,       /* next illegal in END block */
	QSE_AWK_ENEXTFBEG,      /* nextfile illegal in BEGIN block */
	QSE_AWK_ENEXTFEND,      /* nextfile illegal in END block */
	QSE_AWK_EPRINTFARG,     /* printf not followed by any arguments */
	QSE_AWK_EPREPST,        /* both prefix and postfix increment/decrement 
	                           operator present */

	/* run time error */
	QSE_AWK_EDIVBY0,           /* divide by zero */
	QSE_AWK_EOPERAND,          /* invalid operand */
	QSE_AWK_EPOSIDX,           /* wrong position index */
	QSE_AWK_EARGTF,            /* too few arguments */
	QSE_AWK_EARGTM,            /* too many arguments */
	QSE_AWK_EFUNNONE,          /* function '${1}' not found */
	QSE_AWK_ENOTIDX,           /* variable not indexable */
	QSE_AWK_ENOTDEL,           /* variable not deletable */
	QSE_AWK_ENOTMAP,           /* value not a map */
	QSE_AWK_ENOTMAPIN,         /* right-hand side of 'in' not a map */
	QSE_AWK_ENOTMAPNILIN,      /* right-hand side of 'in' not a map nor nil */
	QSE_AWK_ENOTREF,           /* value not referenceable */
	QSE_AWK_ENOTASS,           /* value not assignable */
	QSE_AWK_EIDXVALASSMAP,     /* indexed value cannot be assigned a map */
	QSE_AWK_EPOSVALASSMAP,     /* a positional cannot be assigned a map */
	QSE_AWK_EMAPTOSCALAR,      /* cannot change a map to a scalar value */
	QSE_AWK_ESCALARTOMAP,      /* cannot change a scalar value to a map */
	QSE_AWK_EMAPNOTALLOWED,    /* a map is not allowed */
	QSE_AWK_EVALTYPE,          /* invalid value type */
	QSE_AWK_ERDELETE,          /* delete called with a wrong target */
	QSE_AWK_ERRESET,           /* reset called with a wrong target */
	QSE_AWK_ERNEXTBEG,         /* next called from BEGIN */
	QSE_AWK_ERNEXTEND,         /* next called from END */
	QSE_AWK_ERNEXTFBEG,        /* nextfile called from BEGIN */
	QSE_AWK_ERNEXTFEND,        /* nextfile called from END */
	QSE_AWK_EFNCUSER,          /* wrong intrinsic function implementation */
	QSE_AWK_EFNCIMPL,          /* intrinsic function handler failed */
	QSE_AWK_EIOUSER,           /* wrong user io handler implementation */
	QSE_AWK_EIONONE,           /* no such io name found */
	QSE_AWK_EIOIMPL,           /* i/o callback returned an error */
	QSE_AWK_EIONMEM,           /* i/o name empty */
	QSE_AWK_EIONMNL,           /* i/o name contains '\0' */
	QSE_AWK_EFMTARG,           /* arguments to format string not sufficient */
	QSE_AWK_EFMTCNV,           /* recursion detected in format conversion */
	QSE_AWK_ECONVFMTCHR,       /* an invalid character found in CONVFMT */
	QSE_AWK_EOFMTCHR,          /* an invalid character found in OFMT */

	/* regular expression error */
	QSE_AWK_EREXRECUR,        /* recursion too deep */
	QSE_AWK_EREXRPAREN,       /* a right parenthesis is expected */
	QSE_AWK_EREXRBRACKET,     /* a right bracket is expected */
	QSE_AWK_EREXRBRACE,       /* a right brace is expected */
	QSE_AWK_EREXUNBALPAREN,   /* unbalanced parenthesis */
	QSE_AWK_EREXINVALBRACE,   /* invalid brace */
	QSE_AWK_EREXCOLON,        /* a colon is expected */
	QSE_AWK_EREXCRANGE,       /* invalid character range */
	QSE_AWK_EREXCCLASS,       /* invalid character class */
	QSE_AWK_EREXBRANGE,       /* invalid boundary range */
	QSE_AWK_EREXEND,          /* unexpected end of the pattern */
	QSE_AWK_EREXGARBAGE,      /* garbage after the pattern */

	/* the number of error numbers, internal use only */
	QSE_AWK_NUMERRNUM 
};

typedef enum qse_awk_errnum_t qse_awk_errnum_t;


struct qse_awk_errinf_t
{
	qse_awk_errnum_t num;
	qse_size_t       lin;
	qse_char_t       msg[256];
};

typedef struct qse_awk_errinf_t qse_awk_errinf_t;

/**
 * The qse_awk_errstr_t type defines a error string getter. It should return 
 * an error formatting string for an error number requested. A new string
 * should contain the same number of positional parameters (${X}) as in the
 * default error formatting string. You can set a new getter into an awk
 * object with the qse_awk_seterrstr() function to customize an error string.
 */
typedef const qse_char_t* (*qse_awk_errstr_t) (
	qse_awk_t*       awk,   /**< an awk object */
	qse_awk_errnum_t num    /**< an error number */
);

/* depth types */
enum qse_awk_depth_t
{
	QSE_AWK_DEPTH_BLOCK_PARSE = (1 << 0),
	QSE_AWK_DEPTH_BLOCK_RUN   = (1 << 1),
	QSE_AWK_DEPTH_EXPR_PARSE  = (1 << 2),
	QSE_AWK_DEPTH_EXPR_RUN    = (1 << 3),
	QSE_AWK_DEPTH_REX_BUILD   = (1 << 4),
	QSE_AWK_DEPTH_REX_MATCH   = (1 << 5)
};

/* rio types */
enum qse_awk_rio_type_t
{
	/* rio types available */
	QSE_AWK_RIO_PIPE,
	QSE_AWK_RIO_FILE,
	QSE_AWK_RIO_CONSOLE,

	/* reserved for internal use only */
	QSE_AWK_RIO_NUM
};

enum qse_awk_rio_mode_t
{
	QSE_AWK_RIO_PIPE_READ      = 0,
	QSE_AWK_RIO_PIPE_WRITE     = 1,
	QSE_AWK_RIO_PIPE_RW        = 2,

	QSE_AWK_RIO_FILE_READ      = 0,
	QSE_AWK_RIO_FILE_WRITE     = 1,
	QSE_AWK_RIO_FILE_APPEND    = 2,

	QSE_AWK_RIO_CONSOLE_READ   = 0,
	QSE_AWK_RIO_CONSOLE_WRITE  = 1
};

enum qse_awk_gbl_id_t
{
	/* this table should match gtab in parse.c.
	 * in addition, qse_awk_rtx_setgbl also counts 
	 * on the order of these values */

	QSE_AWK_GBL_ARGC,
	QSE_AWK_GBL_ARGV,
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
	QSE_AWK_MIN_GBL_ID = QSE_AWK_GBL_ARGC,
	QSE_AWK_MAX_GBL_ID = QSE_AWK_GBL_SUBSEP
};

enum qse_awk_val_type_t
{
	/* the values between QSE_AWK_VAL_NIL and QSE_AWK_VAL_STR inclusive
	 * must be synchronized with an internal table of the __cmp_val 
	 * function in run.c */
	QSE_AWK_VAL_NIL  = 0,
	QSE_AWK_VAL_INT  = 1,
	QSE_AWK_VAL_REAL = 2,
	QSE_AWK_VAL_STR  = 3,

	QSE_AWK_VAL_REX  = 4,
	QSE_AWK_VAL_MAP  = 5,
	QSE_AWK_VAL_REF  = 6
};

enum qse_awk_val_ref_id_t
{
	/* keep these items in the same order as corresponding items
	 * in tree.h */
	QSE_AWK_VAL_REF_NAMED,
	QSE_AWK_VAL_REF_GBL,
	QSE_AWK_VAL_REF_LCL,
	QSE_AWK_VAL_REF_ARG,
	QSE_AWK_VAL_REF_NAMEDIDX,
	QSE_AWK_VAL_REF_GBLIDX,
	QSE_AWK_VAL_REF_LCLIDX,
	QSE_AWK_VAL_REF_ARGIDX,
	QSE_AWK_VAL_REF_POS
};

/**
 * The values defined are used to set the type field of the 
 * qse_awk_rtx_valtostr_out_t structure. The field should be one of the 
 * following values:
 *
 * - QSE_AWK_RTX_VALTOSTR_CPL
 * - QSE_AWK_RTX_VALTOSTR_CPLDUP
 * - QSE_AWK_RTX_VALTOSTR_STRP
 * - QSE_AWK_RTX_VALTOSTR_STRPCAT
 *
 * and it can optionally be ORed with QSE_AWK_RTX_VALTOSTR_PRINT.
 */
enum qse_awk_rtx_valtostr_type_t
{
	QSE_AWK_RTX_VALTOSTR_CPL       = 0x00,
	QSE_AWK_RTX_VALTOSTR_CPLDUP    = 0x01,
	QSE_AWK_RTX_VALTOSTR_STRP      = 0x02,
	QSE_AWK_RTX_VALTOSTR_STRPCAT   = 0x03,
	QSE_AWK_RTX_VALTOSTR_PRINT     = 0x10
};

/**
 * The qse_awk_rtx_valtostr() function converts a value to a string as 
 * indicated in a parameter of the qse_awk_rtx_valtostr_out_t type.
 */
struct qse_awk_rtx_valtostr_out_t
{
	int type;

	union
	{
		qse_xstr_t  cpl;
		qse_xstr_t  cpldup;  /* need to free cpldup.ptr */
		qse_str_t*  strp;
		qse_str_t*  strpcat;
	} u;
};
typedef struct qse_awk_rtx_valtostr_out_t qse_awk_rtx_valtostr_out_t;

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (awk)

/** represents a nil value */
extern qse_awk_val_t* qse_awk_val_nil;

/** represents an empty string  */
extern qse_awk_val_t* qse_awk_val_zls;

/** represents a numeric value -1 */
extern qse_awk_val_t* qse_awk_val_negone;

/** represents a numeric value 0 */
extern qse_awk_val_t* qse_awk_val_zero;

/** represents a numeric value 1 */
extern qse_awk_val_t* qse_awk_val_one;

/**
 * The qse_awk_open() function creates a new qse_awk_t object. The object 
 * created can be passed to other qse_awk_xxx() functions and is valid until 
 * it is destroyed iwth the qse_qse_close() function. The function saves the 
 * memory manager pointer while it copies the contents of the primitive 
 * function structures. Therefore, you should keep the memory manager valid 
 * during the whole life cycle of an qse_awk_t object.
 *
 * @code
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
 * @endcode
 *
 * @return a pointer to a qse_awk_t object on success, QSE_NULL on failure.
 */
qse_awk_t* qse_awk_open ( 
	qse_mmgr_t*    mmgr, /**< a memory manager */
	qse_size_t     xtn,  /**< extension size in bytes */
	qse_awk_prm_t* prm   /**< a pointer to a primitive function structure */
);

/**
 *  The qse_awk_close() function destroys a qse_awk_t object.
 * @return 0 on success, -1 on failure 
 */
int qse_awk_close (
	qse_awk_t* awk /**< an awk object */
);

/**
 * The qse_awk_getprm() function gets primitive functions
 */
qse_awk_prm_t* qse_awk_getprm (
	qse_awk_t* awk
);
/******/

/**
 * The qse_awk_clear() clears the internal state of @a awk. If you want to
 * reuse a qse_awk_t instance that finished being used, you may call 
 * qse_awk_clear() instead of destroying and creating a new
 * #qse_awk_t instance using qse_awk_close() and qse_awk_open().
 *
 * @return 0 on success, -1 on failure
 */
int qse_awk_clear (
	qse_awk_t* awk 
);
/******/

/**
 * The qse_awk_geterrstr() gets an error string getter.
 */
qse_awk_errstr_t qse_awk_geterrstr (
	qse_awk_t*       awk    /**< an awk object */
);

/**
 * The qse_awk_seterrstr() sets an error string getter that is called to
 * compose an error message when its retrieval is requested.
 *
 * Here is an example of changing the formatting string for the #QSE_SED_ECMDNR 
 * error.
 * @code
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
 * @endcode
 */
void qse_awk_seterrstr (
	qse_awk_t*       awk,   /**< an awk object */
	qse_awk_errstr_t errstr /**< an error string getter */
);

int qse_awk_geterrnum (
	qse_awk_t* awk
);

qse_size_t qse_awk_geterrlin (
	qse_awk_t* awk
);

const qse_char_t* qse_awk_geterrmsg (
	qse_awk_t* awk
);

void qse_awk_geterrinf (
	qse_awk_t*        awk,
	qse_awk_errinf_t* errinf
);

void qse_awk_seterrnum (
	qse_awk_t*       awk,
	qse_awk_errnum_t errnum
);

void qse_awk_seterrinf (
	qse_awk_t*              awk, 
	const qse_awk_errinf_t* errinf
);

void qse_awk_geterror (
	qse_awk_t*         awk,
	qse_awk_errnum_t*  errnum, 
	qse_size_t*        errlin,
	const qse_char_t** errmsg
);

void qse_awk_seterror (
	qse_awk_t*        awk,
	qse_awk_errnum_t  errnum,
	qse_size_t        errlin, 
	const qse_cstr_t* errarg
);

int qse_awk_getoption (
	qse_awk_t* awk
);

void qse_awk_setoption (
	qse_awk_t* awk,
	int        opt
);

qse_size_t qse_awk_getmaxdepth (
	qse_awk_t* awk,
	int        type
);

void qse_awk_setmaxdepth (
	qse_awk_t* awk,
	int        types,
	qse_size_t depth
);

int qse_awk_getword (
	qse_awk_t* awk, 
	const qse_char_t* okw,
	qse_size_t olen,
	const qse_char_t** nkw,
	qse_size_t* nlen
);

int qse_awk_unsetword (
	qse_awk_t*        awk,
	const qse_char_t* kw,
	qse_size_t        len
);

void qse_awk_unsetallwords (
	qse_awk_t* awk
);

/**
 * The qse_awk_setword() function enables replacement of a name of a keyword,
 * intrinsic global variables, and intrinsic functions.
 *
 * If @a nkw is QSE_NULL or @a nlen is zero and @a okw is QSE_NULL or 
 * @a olen is zero, it unsets all word replacements; If @a nkw is QSE_NULL or 
 * @a nlen is zero, it unsets the replacement for @a okw and @a olen; If 
 * all of them are valid, it sets the word replace for @a okw and @a olen 
 * to @a nkw and @a nlen.
 *
 * @return 0 on success, -1 on failure
 */
int qse_awk_setword (
	/* the pointer to a qse_awk_t instance */
	qse_awk_t* awk, 
	/* the pointer to an old keyword */
	const qse_char_t* okw, 
	/* the length of the old keyword */
	qse_size_t olen,
	/* the pointer to an new keyword */
	const qse_char_t* nkw, 
	/* the length of the new keyword */
	qse_size_t nlen
);

/****f* AWK/qse_awk_addgbl
 * NAME
 *  qse_awk_addgbl - add an intrinsic global variable.
 * RETURN
 *  The qse_awk_addgbl() function returns the ID of the global variable 
 *  added on success and -1 on failure.
 * SYNOPSIS
 */
int qse_awk_addgbl (
	qse_awk_t*        awk,
	const qse_char_t* name,
	qse_size_t        len
);
/******/

/****f* AWK/qse_awk_delgbl
 * NAME
 *  qse_awk_delgbl - delete an instrinsic global variable. 
 * SYNOPSIS
 */
int qse_awk_delgbl (
	qse_awk_t*        awk,
	const qse_char_t* name,
	qse_size_t        len
);
/******/

/****f* AWK/qse_awk_addfnc
 * NAME
 *  qse_awk_addfnc - add an intrinsic function
 * SYNOPSIS
 */
void* qse_awk_addfnc (
	qse_awk_t*        awk,
	const qse_char_t* name,
	qse_size_t        name_len, 
	int               when_valid,
	qse_size_t        min_args,
	qse_size_t        max_args, 
	const qse_char_t* arg_spec, 
	int (*handler)(qse_awk_rtx_t*,const qse_char_t*,qse_size_t)
);
/******/

/****f* AWK/qse_awk_delfnc
 * NAME
 *  qse_awk_delfnc - delete an intrinsic function
 * SYNOPSIS
 */
int qse_awk_delfnc (
	qse_awk_t*        awk,
	const qse_char_t* name,
	qse_size_t        len
);
/******/

/****f* AWK/qse_awk_clrfnc
 * NAME
 *  qse_awk_clrfnc - delete all intrinsic functions
 * SYNOPSIS
 */
void qse_awk_clrfnc (
	qse_awk_t* awk
);
/*****/


/**
 * The qse_awk_parse() function parses the source script.
 * @return 0 on success, -1 on failure.
 */
int qse_awk_parse (
	qse_awk_t*     awk, /**< an awk object */
	qse_awk_sio_t* sio  /**< source stream I/O handler */
);

/**
 * The qse_awk_alloc() function allocates dynamic memory.
 * @return a pointer to memory space allocated on success, QSE_NULL on failure
 */
void* qse_awk_alloc (
	qse_awk_t* awk,  /**< an awk object */
	qse_size_t size  /**< size of memory to allocate in bytes */
);

/**
 * The qse_awk_free() function frees dynamic memory allocated.
 */
void qse_awk_free (
	qse_awk_t* awk, /**< an awk object */
	void*      ptr  /**< memory space to free */
);

/**
 * The qse_awk_strdup() function is used to duplicate a string using
 * the memory manager used by the associated qse_awk_t instance.
 * The new string should be freed using the qse_awk_free() function.
 *
 * @return a pointer to a new string duplicated of @a s on success, 
 *         QSE_NULL on failure.
 */
qse_char_t* qse_awk_strdup (
	qse_awk_t*        awk, /**< an awk object */
	const qse_char_t* str  /**< a string pointer */
);

/**
 * The qse_awk_strxdup() function is used to duplicate a string whose length
 * is as long as len characters using the memory manager used by the 
 * qse_awk_t instance. The new string should be freed using the qse_awk_free()
 * function.
 *
 * @return a pointer to a new string duplicated of @a s on success, 
 *         QSE_NULL on failure.
 */
qse_char_t* qse_awk_strxdup (
	qse_awk_t*        awk, /**< an awk object */
	const qse_char_t* str, /**< a string pointer */
	qse_size_t        len  /**< the number of character in a string */
);

qse_long_t qse_awk_strxtolong (
	qse_awk_t*         awk,
	const qse_char_t*  str,
	qse_size_t         len,
	int                base,
	const qse_char_t** endptr
);

qse_real_t qse_awk_strxtoreal (
	qse_awk_t*         awk,
	const qse_char_t*  str,
	qse_size_t         len, 
	const qse_char_t** endptr
);

qse_size_t qse_awk_longtostr (
	qse_long_t        value,
	int               radix,
	const qse_char_t* prefix,
	qse_char_t*       buf,
	qse_size_t        size
);

/****f* AWK/qse_awk_rtx_open
 * NAME
 *  qse_awk_rtx_open - create a runtime context
 * SYNOPSIS
 */
qse_awk_rtx_t* qse_awk_rtx_open (
	qse_awk_t*        awk,
	qse_size_t        xtn,
	qse_awk_rio_t*    rio,
	const qse_cstr_t* arg
);
/******/

/****f* AWK/qse_awk_rtx_close
 * NAME
 *  qse_awk_rtx_close - destroy a runtime context
 * SYNOPSIS
 */
void qse_awk_rtx_close (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_loop
 * NAME
 *  qse_awk_rtx_loop - run BEGIN/pattern-action/END blocks
 * DESCRIPTION
 *  The qse_awk_rtx_loop() function executes the BEGIN block, pattern-action
 *  blocks and the END blocks in an AWk program. Multiple invocations of the
 *  function for the lifetime of a runtime context is not desirable.
 * RETURN
 *  The qse_awk_rtx_loop() function returns 0 on success and -1 on failure.
 * EXAMPLE
 *  The example shows typical usage of the function.
 *    rtx = qse_awk_rtx_open (awk, rio, rcb, QSE_NULL, QSE_NULL);
 *    if (rtx != QSE_NULL)
 *    {
 *        qse_awk_rtx_loop (rtx);
 *        qse_awk_rtx_close (rtx);
 *    }
 * SYNOPSIS
 */
int qse_awk_rtx_loop (
	qse_awk_rtx_t* rtx
);
/******/

/**
 * The qse_awk_rtx_call() function invokes an AWK function. However, it is
 * not able to invoke an intrinsic function such as split(). 
 * The #QSE_AWK_PABLOCK option can be turned off to make illegal the BEGIN 
 * block, pattern-action blocks, and the END block.
 *
 * The example shows typical usage of the function.
 * @code
 * rtx = qse_awk_rtx_open (awk, rio, rcb, QSE_NULL, QSE_NULL);
 * if (rtx != QSE_NULL)
 * {
 *     v = qse_awk_rtx_call (rtx, QSE_T("init"), QSE_NULL, 0);
 *     if (v != QSE_NULL) qse_awk_rtx_refdownval (rtx, v);
 *     qse_awk_rtx_call (rtx, QSE_T("fini"), QSE_NULL, 0);
 *     if (v != QSE_NULL) qse_awk_rtx_refdownval (rtx, v);
 *     qse_awk_rtx_close (rtx);
 * }
 * @endcode
 *
 * @return 0 on success, -1 on failure
 */
qse_awk_val_t* qse_awk_rtx_call (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* name,
	qse_awk_val_t**   args,
	qse_size_t        nargs
);

/****f* AWK/qse_awk_stopall
 * NAME
 *  qse_awk_stopall - stop all runtime contexts
 * DESCRIPTION
 *  The qse_awk_stopall() function aborts all active runtime contexts
 *  invoked with the awk parameter.
 * SYNOPSIS
 */
void qse_awk_stopall (
	qse_awk_t* awk
);
/******/

/****f* AWK/qse_awk_shouldstop
 * NAME
 *  qse_awk_shouldstop - test if qse_awk_rtx_stop() is called
 * SYNOPSIS
 */
qse_bool_t qse_awk_rtx_shouldstop (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_stop
 * NAME
 *  qse_awk_rtx_stop - stop a runtime context
 * DESCRIPTION
 *  The qse_awk_rtx_stop() function causes the active qse_awk_run() function to 
 *  be aborted. 
 * SYNOPSIS
 */
void qse_awk_rtx_stop (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_getrcb
 * NAME
 *  qse_awk_rtx_getrcb - get callback
 * SYNOPSIS
 */
qse_awk_rcb_t* qse_awk_rtx_getrcb (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_setrcb
 * NAME
 *  qse_awk_rtx_setrcb - set callback
 * SYNOPSIS
 */
void qse_awk_rtx_setrcb (
	qse_awk_rtx_t* rtx,
	qse_awk_rcb_t* rcb
);
/******/

/****f* AWK/qse_awk_rtx_getnargs 
 * NAME
 *  qse_awk_rtx_getnargs - get the number of arguments passed to qse_awk_run()
 * SYNOPSIS
 */
qse_size_t qse_awk_rtx_getnargs (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_getarg 
 * NAME
 *  qse_awk_rtx_getarg - get an argument passed to qse_awk_run
 * SYNOPSIS
 */
qse_awk_val_t* qse_awk_rtx_getarg (
	qse_awk_rtx_t* rtx,
	qse_size_t     idx
);
/******/

/****f* AWK/qse_awk_rtx_getgbl
 * NAME
 *  qse_awk_rtx_getgbl - gets the value of a global variable
 * INPUTS
 *  * rtx - a runtime context
 *  * id - a global variable ID. It is one of the predefined global 
 *         variable IDs or a value returned by qse_awk_addgbl().
 * RETURN
 *  The pointer to a value is returned. This function never fails
 *  so long as the ID is valid. Otherwise, you may fall into trouble.
 * SYNOPSIS
 */
qse_awk_val_t* qse_awk_rtx_getgbl (
	qse_awk_rtx_t* rtx,
	int            id
);
/******/

/****f* AWK/qse_awk_rtx_setgbl
 * NAME
 *  qse_awk_rtx_setgbl - set the value of a global variable
 * SYNOPSIS
 */
int qse_awk_rtx_setgbl (
	qse_awk_rtx_t* rtx, 
	int            id,
	qse_awk_val_t* val
);
/******/

/****f* AWK/qse_awk_rtx_setretval
 * NAME
 *  qse_awk_rtx_setretval - set the return value
 * DESCRIPTION
 *  The qse_awk_rtx_setretval() sets the return value of a function
 *  when called from within a function handlers. The caller doesn't
 *  have to invoke qse_awk_rtx_refupval() and qse_awk_rtx_refdownval() 
 *  with the value to be passed to qse_awk_rtx_setretval(). 
 *  The qse_awk_rtx_setretval() will update its reference count properly
 *  once the return value is set. 
 * SYNOPSIS
 */
void qse_awk_rtx_setretval (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);
/******/

/****f* AWK/qse_awk_rtx_setfilename
 * NAME
 *  qse_awk_rtx_setfilename - set FILENAME
 * SYNOPSIS
 */
int qse_awk_rtx_setfilename (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str,
	qse_size_t        len
);
/******/

/****f* AWK/qse_awk_rtx_setofilename
 * NAME
 *  qse_awk_rtx_setofilename - set OFILENAME
 * SYNOPSIS
 */
int qse_awk_rtx_setofilename (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str,
	qse_size_t        len
);
/******/

/****f* AWK/qse_awk_rtx_getawk
 * NAME
 *  qse_awk_rtx_getawk - get the owning awk object
 * SYNOPSIS
 */
qse_awk_t* qse_awk_rtx_getawk (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_getmmgr
 * NAME
 *  qse_awk_rtx_getmmgr - get the memory manager of a runtime context
 * SYNOPSIS
 */
qse_mmgr_t* qse_awk_rtx_getmmgr (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_getxtn
 * NAME
 *  qse_awk_rtx_getxtn - get the pointer to extension space
 * SYNOPSIS
 */
void* qse_awk_rtx_getxtn (
	qse_awk_rtx_t* rtx
);
/******/

/**
 * The qse_awk_rtx_getnvmap() gets the map of named variables 
 */
qse_map_t* qse_awk_rtx_getnvmap (
	qse_awk_rtx_t* rtx
);

/**
 * The qse_awk_rtx_geterrnum() function gets an error code of a runtime context
 */
int qse_awk_rtx_geterrnum (
	qse_awk_rtx_t* rtx
);

qse_size_t qse_awk_rtx_geterrlin (
	qse_awk_rtx_t* rtx
);

const qse_char_t* qse_awk_rtx_geterrmsg (
	qse_awk_rtx_t* rtx
);

void qse_awk_rtx_geterrinf (
	qse_awk_rtx_t*    rtx,
	qse_awk_errinf_t* errinf
);

void qse_awk_rtx_geterror (
	qse_awk_rtx_t*     rtx,
	qse_awk_errnum_t*  errnum, 
	qse_size_t*        errlin,
	const qse_char_t** errmsg
);

void qse_awk_rtx_seterrnum (
	qse_awk_rtx_t*   rtx,
	qse_awk_errnum_t errnum
);

void qse_awk_rtx_seterrinf (
	qse_awk_rtx_t*          rtx, 
	const qse_awk_errinf_t* errinf
);

void qse_awk_rtx_seterror (
	qse_awk_rtx_t*    rtx,
	qse_awk_errnum_t  errnum,
	qse_size_t        errlin, 
	const qse_cstr_t* errarg
);

/* record and field functions */
int qse_awk_rtx_clrrec (
	qse_awk_rtx_t* rtx,
	qse_bool_t     skip_inrec_line
);

int qse_awk_rtx_setrec (
	qse_awk_rtx_t*    rtx,
	qse_size_t        idx,
	const qse_char_t* str,
	qse_size_t        len
);

/* value manipulation functions */
qse_awk_val_t* qse_awk_rtx_makeintval (
	qse_awk_rtx_t* rtx,
	qse_long_t     v
);
qse_awk_val_t* qse_awk_rtx_makerealval (
	qse_awk_rtx_t* rtx,
	qse_real_t     v
);

qse_awk_val_t* qse_awk_rtx_makestrval0 (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str
);

qse_awk_val_t* qse_awk_rtx_makestrval (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str,
	qse_size_t        len
);

qse_awk_val_t* qse_awk_rtx_makestrval_nodup (
	qse_awk_rtx_t* rtx,
	qse_char_t*    str,
	qse_size_t     len
);

qse_awk_val_t* qse_awk_rtx_makestrval2 (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str1,
	qse_size_t        len1, 
	const qse_char_t* str2,
	qse_size_t        len2
);

qse_awk_val_t* qse_awk_rtx_makenstrval (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* str,
	qse_size_t        len
);

qse_awk_val_t* qse_awk_rtx_makerexval (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* buf,
	qse_size_t        len,
	void*             code
);

qse_awk_val_t* qse_awk_rtx_makemapval (
	qse_awk_rtx_t* rtx
);

qse_awk_val_t* qse_awk_rtx_makerefval (
	qse_awk_rtx_t*  rtx,
	int             id,
	qse_awk_val_t** adr
);

qse_bool_t qse_awk_rtx_isstaticval (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

/**
 * The qse_awk_rtx_refupval() function increments a reference count of a 
 * value @a val.
 */
void qse_awk_rtx_refupval (
	qse_awk_rtx_t* rtx, /**< a runtime context */
	qse_awk_val_t* val  /**< a value */
);

/**
 * The qse_awk_rtx_refdownval() function decrements a reference count of
 * a value @a val. It destroys the value if it has reached the count of 0.
 */
void qse_awk_rtx_refdownval (
	qse_awk_rtx_t* rtx, /**< a runtime context */
	qse_awk_val_t* val  /**< a value */
);

void qse_awk_rtx_refdownval_nofree (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

qse_bool_t qse_awk_rtx_valtobool (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

/**
 * The qse_awk_rtx_valtostr() function convers a value val to a string as 
 * instructed in the parameter out. Before the call to the function, you 
 * should initialize a variable of the qse_awk_rtx_valtostr_out_t type.
 *
 * The type field is one of the following qse_awk_rtx_valtostr_type_t values:
 *
 * - QSE_AWK_RTX_VALTOSTR_CPL
 * - QSE_AWK_RTX_VALTOSTR_CPLDUP
 * - QSE_AWK_RTX_VALTOSTR_STRP
 * - QSE_AWK_RTX_VALTOSTR_STRPCAT
 *
 * It can optionally be ORed with QSE_AWK_RTX_VALTOSTR_PRINT. The option
 * causes the function to use OFMT for real number conversion. Otherwise,
 * it uses CONVFMT. 
 *
 * You should initialize or free other fields before and after the call 
 * depending on the type field as shown below.
 *  
 * If you have a static buffer, use QSE_AWK_RTX_VALTOSTR_CPL.
 * @code
 * qse_awk_rtx_valtostr_out_t out;
 * qse_char_t buf[100];
 * out.type = QSE_AWK_RTX_VALTOSTR_CPL;
 * out.u.cpl.ptr = buf;
 * out.u.cpl.len = QSE_COUNTOF(buf);
 * if (qse_awk_rtx_valtostr (rtx, v, &out) == QSE_NULL)  goto oops;
 * qse_printf (QSE_T("%.*s\n"), (int)out.u.cpl.len, out.u.cpl.ptr);
 * @endcode
 * 
 * When unsure of the size of the string after conversion, you can use
 * QSE_AWK_RTX_VALTOSTR_CPLDUP. However, you should free the memory block
 * pointed to by the u.cpldup.ptr field after use.
 * @code
 * qse_awk_rtx_valtostr_out_t out;
 * out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
 * if (qse_awk_rtx_valtostr (rtx, v, &out) == QSE_NULL)  goto oops;
 * qse_printf (QSE_T("%.*s\n"), (int)out.u.cpldup.len, out.u.cpldup.ptr);
 * qse_awk_rtx_free (rtx, out.u.cpldup.ptr);
 * @endcode
 *
 * You may like to store the result in a dynamically resizable string.
 * Consider QSE_AWK_RTX_VALTOSTR_STRP.
 * @code
 * qse_awk_rtx_valtostr_out_t out;
 * qse_str_t str;
 * qse_str_init (&str, qse_awk_rtx_getmmgr(rtx), 100);
 * out.type = QSE_AWK_RTX_VALTOSTR_STRP;
 * out.u.strp = str;
 * if (qse_awk_rtx_valtostr (rtx, v, &out) == QSE_NULL)  goto oops;
 * qse_printf (QSE_T("%.*s\n"), 
 *     (int)QSE_STR_LEN(out.u.strp), QSE_STR_PTR(out.u.strp));
 * qse_str_fini (&str);
 * @endcode
 * 
 * If you want to append the converted string to an existing dynamically 
 * resizable string, QSE_AWK_RTX_VALTOSTR_STRPCAT is the answer. The usage is
 * the same as QSE_AWK_RTX_VALTOSTR_STRP except that you have to use the 
 * u.strpcat field instead of the u.strp field.
 *
 * @return the pointer to a string converted on success, QSE_NULL on failure
 */
qse_char_t* qse_awk_rtx_valtostr (
	qse_awk_rtx_t*              rtx, /**< a runtime context */
	qse_awk_val_t*              val, /**< a vlaue */
	qse_awk_rtx_valtostr_out_t* out  /**< a output buffer */
);
/******/

/**
 * The qse_awk_rtx_valtocpldup() function provides a shortcut to the 
 * qse_awk_rtx_valtostr() function with the QSE_AWK_RTX_VALTOSTR_CPLDUP type.
 * It returns the pointer to a string converted from @a val and stores its 
 * length to memory pointed to by @a len. You should free the returned
 * memory block after use. See the code snippet below for a simple usage.
 *
 * @code
 * ptr = qse_awk_rtx_valtocpldup (rtx, v, &len);
 * if (str == QSE_NULL) handle_error();
 * qse_printf (QSE_T("%.*s\n"), (int)len, ptr);
 * qse_awk_rtx_free (rtx, ptr);
 * @endcode
 *
 * @return the pointer to a string converted on success, QSE_NULL on failure
 */
qse_char_t* qse_awk_rtx_valtocpldup (
	qse_awk_rtx_t* rtx, /**< a runtime context */
	qse_awk_val_t* val, /**< a value to convert */
	qse_size_t*    len  /**< result length */
);
/******/

/****f* AWK/qse_awk_rtx_valtonum
 * NAME
 *  qse_awk_rtx_valtonum - convert a value to a number
 * DESCRIPTION
 *  The qse_awk_rtx_valtonum() function converts a value to a number. 
 *  If the value is converted to a long number, it is stored in the memory
 *  pointed to by l and 0 is returned. If the value is converted to a real 
 *  number, it is stored in the memory pointed to by r and 1 is returned.
 * RETURN
 *  The qse_awk_rtx_valtonum() function returns -1 on error, 0 if the converted
 *  number is a long number and 1 if it is a real number.
 * EXAMPLE
 *  The example show how to convert a value to a number and determine
 *  if it is an integer or a floating-point number.
 *    qse_long_t l;
 *    qse_real_t r;
 *    int n;
 *    n = qse_awk_rtx_valtonum (v, &l, &r);
 *    if (n == -1) error ();
 *    else if (n == 0) print_long (l);
 *    else if (n == 1) print_real (r);
 * SYNOPSIS
 */
int qse_awk_rtx_valtonum (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val,
	qse_long_t*    l,
	qse_real_t*    r
);
/******/

/**
 * The qse_awk_rtx_strtonum() function converts a string to a number.
 * A numeric string in the valid decimal, hexadecimal(0x), binary(0b), 
 * octal(0) notation is converted to an integer and it is stored into
 * memory pointed to by @a l; A string containng '.', 'E', or 'e' is 
 * converted to a floating-pointer number and it is stored into memory
 * pointed to by @a r. If @a strict is 0, the function takes up to the last
 * valid character and never fails. If @a strict is non-zero, an invalid 
 * character causes the function to return an error.
 *
 * @return 0 if converted to an integer,
 *         1 if converted to a floating-point number
 *         -1 on error.
 */
int qse_awk_rtx_strtonum (
	qse_awk_rtx_t*    rtx, /**< runtime context */
	int               strict, /**< determines to perform strict check */
	const qse_char_t* ptr, /**< points to a string to convert */
	qse_size_t        len, /**< number of characters in a string */
	qse_long_t*       l,   /**< stores a converted integer */
	qse_real_t*       r    /**< stores a converted floating-poing number */
);

/**
 * The qse_awk_rtx_alloc() function allocats a memory block of @a size bytes
 * using the memory manager associated with a runtime context @a rtx. 
 * @return the pointer to a memory block on success, QSE_NULL on failure.
 */
void* qse_awk_rtx_alloc (
	qse_awk_rtx_t* rtx, /**< a runtime context */
	qse_size_t     size /**< block size in bytes */
);

/**
 * The qse_awk_rtx_free() function frees a memory block pointed to by @a ptr
 * using the memory manager of a runtime ocntext @a rtx.
 */
void qse_awk_rtx_free (
	qse_awk_rtx_t* rtx, /**< a runtime context */
	void*          ptr  /**< a memory block pointer */
);

#ifdef __cplusplus
}
#endif

#endif
