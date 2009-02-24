/*
 * $Id: awk.h 79 2009-02-24 03:57:28Z hyunghwan.chung $
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

/****o* AWK/Interpreter
 * DESCRIPTION
 *  The library includes an AWK interpreter that can be embedded into other
 *  applications or can run stand-alone.
 *
 *  #include <qse/awk/awk.h>
 ******
 */

/****t* AWK/qse_awk_t
 * NAME
 *  qse_awk_t - define an AWK type
 * SYNOPSIS
 */
typedef struct qse_awk_t     qse_awk_t;
/******/

/****t* AWK/qse_awk_rtx_t
 * NAME
 *  qse_awk_rtx_t - define an AWK runtime context type
 * SYNOPSIS
 */
typedef struct qse_awk_rtx_t qse_awk_rtx_t; /* (R)untime con(T)e(X)t */
/******/

/* this is not a value. it is just a value holder */
typedef struct qse_awk_val_chunk_t qse_awk_val_chunk_t;

#if QSE_SIZEOF_INT == 2
#	define QSE_AWK_VAL_HDR \
		unsigned int type: 3; \
		unsigned int ref: 13
#else
#	define QSE_AWK_VAL_HDR \
		unsigned int type: 3; \
		unsigned int ref: 29
#endif

#define QSE_AWK_VAL_TYPE(x) ((x)->type)

/****s* AWK/qse_awk_val_t
 * NAME
 *  qse_awk_val_t - define an abstract value type
 * SYNOPSIS
 */
struct qse_awk_val_t
{
	QSE_AWK_VAL_HDR;	
};
typedef struct qse_awk_val_t qse_awk_val_t;
/******/

/****s* AWK/qse_awk_val_nil_t
 * NAME
 *  qse_awk_val_nil_t - define a nil value type
 * DESCRIPTION
 *  The type field is QSE_AWK_VAL_NIL.
 * SYNOPSIS
 */
struct qse_awk_val_nil_t
{
	QSE_AWK_VAL_HDR;
};
typedef struct qse_awk_val_nil_t  qse_awk_val_nil_t;
/******/

/****s* AWK/qse_awk_val_int_t
 * NAME
 *  qse_awk_val_int_t - define an integer number type
 * DESCRIPTION
 *  The type field is QSE_AWK_VAL_INT.
 * SYNOPSIS
 */
struct qse_awk_val_int_t
{
	QSE_AWK_VAL_HDR;
	qse_long_t val;
	void*      nde;
};
typedef struct qse_awk_val_int_t qse_awk_val_int_t;
/******/

/****s* AWK/qse_awk_val_real_t
 * NAME
 *  qse_awk_val_real_t - define a floating-point number type
 * DESCRIPTION
 *  The type field is QSE_AWK_VAL_REAL.
 * SYNOPSIS
 */
struct qse_awk_val_real_t
{
	QSE_AWK_VAL_HDR;
	qse_real_t val;
	void*      nde;
};
typedef struct qse_awk_val_real_t qse_awk_val_real_t;
/******/

/****s* AWK/qse_awk_val_str_t
 * NAME
 *  qse_awk_val_str_t - define a string type
 * DESCRIPTION
 *  The type field is QSE_AWK_VAL_STR.
 * SYNOPSIS
 */
struct qse_awk_val_str_t
{
	QSE_AWK_VAL_HDR;
	qse_char_t* ptr;
	qse_size_t  len;
};
typedef struct qse_awk_val_str_t  qse_awk_val_str_t;
/******/

/****s* AWK/qse_awk_val_rex_t
 * NAME
 *  qse_awk_val_rex_t - define a regular expression type
 * DESCRIPTION
 *  The type field is QSE_AWK_VAL_REX.
 * SYNOPSIS
 */
struct qse_awk_val_rex_t
{
	QSE_AWK_VAL_HDR;
	qse_char_t* ptr;
	qse_size_t  len;
	void*       code;
};
typedef struct qse_awk_val_rex_t  qse_awk_val_rex_t;
/******/

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

typedef struct qse_awk_prm_t qse_awk_prm_t;
typedef struct qse_awk_sio_t qse_awk_sio_t;
typedef struct qse_awk_rio_t qse_awk_rio_t;
typedef struct qse_awk_riod_t qse_awk_riod_t;
typedef struct qse_awk_rcb_t qse_awk_rcb_t;

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

typedef qse_bool_t (*qse_awk_isccls_t) (
	qse_awk_t*    awk,
	qse_cint_t    c,
	qse_ccls_id_t type
);

typedef qse_cint_t (*qse_awk_toccls_t) (
	qse_awk_t*    awk,
	qse_cint_t    c,
	qse_ccls_id_t type
);

enum qse_awk_sio_cmd_t
{
	QSE_AWK_SIO_OPEN   = 0,
	QSE_AWK_SIO_CLOSE  = 1,
	QSE_AWK_SIO_READ   = 2,
	QSE_AWK_SIO_WRITE  = 3
};

typedef enum qse_awk_sio_cmd_t qse_awk_sio_cmd_t;

/****t* AWK/qse_awk_siof_t
 * NAME
 *  qse_awk_siof_t - define a source IO function
 * SYNOPSIS
 */
typedef qse_ssize_t (*qse_awk_siof_t) (
	qse_awk_t*        awk,
	qse_awk_sio_cmd_t cmd, 
	qse_char_t*       data,
	qse_size_t        count
);
/*****/

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

/****f* AWK/qse_awk_riof_t
 * NAME
 *  qse_awk_riof_t - define a runtime IO function
 * SYNOPSIS
 */
typedef qse_ssize_t (*qse_awk_riof_t) (
	qse_awk_rtx_t*    rtx,
	qse_awk_rio_cmd_t cmd, 
	qse_awk_riod_t*   riod,
	qse_char_t*       data,
	qse_size_t        count
);
/******/

/****f* AWK/qse_awk_riod_t
 * NAME
 *  qse_awk_riod_f - define the data passed to a rio function 
 * SYNOPSIS
 */
struct qse_awk_riod_t 
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

	qse_awk_riod_t* next;
};
/******/

struct qse_awk_prm_t
{
	qse_awk_pow_t     pow;
	qse_awk_sprintf_t sprintf;
	qse_awk_isccls_t  isccls;
	qse_awk_toccls_t  toccls;

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
		int* errnum
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

struct qse_awk_sio_t
{
	qse_awk_siof_t in;
	qse_awk_siof_t out;
};

struct qse_awk_rio_t
{
	qse_awk_riof_t pipe;
	qse_awk_riof_t file;
	qse_awk_riof_t console;
};

struct qse_awk_rcb_t
{
	int (*on_enter) (
		qse_awk_rtx_t* rtx, void* data);

	void (*on_statement) (
		qse_awk_rtx_t* rtx, qse_size_t line, void* data);

	void (*on_exit) (
		qse_awk_rtx_t* rtx, qse_awk_val_t* ret, void* data);

	void* data;
};

/* various options */
enum qse_awk_option_t
{ 
	/* allow undeclared variables and implicit concatenation */
	QSE_AWK_IMPLICIT    = (1 << 0),

	/* allow explicit variable declaration, the concatenation
	 * operator(.), and a parse-time function check. */
	QSE_AWK_EXPLICIT    = (1 << 1), 

	/* change ^ from exponentation to bitwise xor */
	QSE_AWK_BXOR        = (1 << 3),

	/* support shift operators */
	QSE_AWK_SHIFT       = (1 << 4), 

	/* enable the idiv operator (double slashes) */
	QSE_AWK_IDIV        = (1 << 5), 

	/* support getline and print */
	QSE_AWK_RIO       = (1 << 7), 

	/* support dual direction pipe. QSE_AWK_RIO must be on */
	QSE_AWK_RWPIPE      = (1 << 8),

	/* can terminate a statement with a new line */
	QSE_AWK_NEWLINE     = (1 << 9),

	/* strip off leading and trailing spaces when splitting a record
	 * into fields with a regular expression.
	 *
	 * Consider the following program.
	 *  BEGIN { FS="[:[:space:]]+"; } 
	 *  { 
	 *  	print "NF=" NF; 
	 *  	for (i = 0; i < NF; i++) print i " [" $(i+1) "]";
	 *  }
	 *
	 * The program splits " a b c " into [a], [b], [c] when this
	 * option is on while into [], [a], [b], [c], [] when it is off.
	 */
	QSE_AWK_STRIPSPACES = (1 << 11),

	/* enable the nextoutfile keyword */
	QSE_AWK_NEXTOFILE   = (1 << 12),

	/* cr + lf by default */
	QSE_AWK_CRLF        = (1 << 13),

	/* enable the non-standard keyword reset */
	QSE_AWK_RESET       = (1 << 14),

	/* allows the assignment of a map value to a variable */
	QSE_AWK_MAPTOVAR    = (1 << 15),

	/* allows BEGIN, END, pattern-action blocks */
	QSE_AWK_PABLOCK     = (1 << 16),

	/* option aggregtes */
	QSE_AWK_CLASSIC  = QSE_AWK_IMPLICIT | QSE_AWK_RIO | 
	                   QSE_AWK_NEWLINE | QSE_AWK_PABLOCK
};

/****e* AWK/qse_awk_errnum_t
 * NAME
 *  qse_awk_errnum_t - define an error code
 * SYNOPSIS
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
	QSE_AWK_EVALTYPE,          /* wrong value type */
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
	QSE_AWK_EREXUNBALPAR,     /* unbalanced parenthesis */
	QSE_AWK_EREXCOLON,        /* a colon is expected */
	QSE_AWK_EREXCRANGE,       /* invalid character range */
	QSE_AWK_EREXCCLASS,       /* invalid character class */
	QSE_AWK_EREXBRANGE,       /* invalid boundary range */
	QSE_AWK_EREXEND,          /* unexpected end of the pattern */
	QSE_AWK_EREXGARBAGE,      /* garbage after the pattern */

	/* the number of error numbers, internal use only */
	QSE_AWK_NUMERRNUM 
};
/******/

typedef enum qse_awk_errnum_t qse_awk_errnum_t;

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

enum qse_awk_rtx_valtostr_opt_t
{
	QSE_AWK_RTX_VALTOSTR_CLEAR = (1 << 0),
	QSE_AWK_RTX_VALTOSTR_FIXED = (1 << 1), /* this overrides CLEAR */
	QSE_AWK_RTX_VALTOSTR_PRINT = (1 << 2)
};

#if 0
/* TODO: change qse_awk_valtostr() according to the following structure... */
struct qse_awk_valtostr_out_t
{
	enum
	{
		QSE_AWK_RTX_VALTOSTR_CP
		QSE_AWK_RTX_VALTOSTR_CPL
		QSE_AWK_RTX_VALTOSTR_STRP
	} type;

	union
	{
		qse_char_t* cp;
		qse_xstr_t  cpl;
		qse_str_t*  strp;
	} u;
};
typedef struct qse_awk_valtostr_out_t qse_awk_valtostr_out_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** represents the nil value */
extern qse_awk_val_t* qse_awk_val_nil;

/** represents an empty string  */
extern qse_awk_val_t* qse_awk_val_zls;

/** represents a numeric value -1 */
extern qse_awk_val_t* qse_awk_val_negone;

/** represents a numeric value 0 */
extern qse_awk_val_t* qse_awk_val_zero;

/** represents a numeric value 1 */
extern qse_awk_val_t* qse_awk_val_one;

/****f* AWK/qse_awk_open
 * NAME
 *  qse_awk_open - create an awk object
 * DESCRIPTION
 *  The qse_awk_open() function creates a new qse_awk_t object.
 *  The instance created can be passed to other qse_awk_xxx() functions and
 *  is valid until it is successfully destroyed using the qse_qse_close() 
 *  function. The function save the memory manager pointer while it copies
 *  the contents of the primitive function structures. Therefore, you should
 *  keep the memory manager valid during the whole life cycle of an qse_awk_t
 *  object.
 *
 *    qse_awk_t* dummy()
 *    {
 *       qse_mmgr_t mmgr;
 *       qse_awk_prm_t prm;
 *       return qse_awk_open (
 *          &mmgr, // NOT OK because the contents of mmgr is 
 *                 // invalidated when dummy() returns. 
 *          0, 
 *          &prm   // OK 
 *       );
 *    }
 *
 * RETURN
 *  The qse_awk_open() function returns the pointer to a qse_awk_t object 
 *  on success and QSE_NULL on failure.
 * SYNOPSIS
 */
qse_awk_t* qse_awk_open ( 
	qse_mmgr_t*     mmgr  /* a memory manager */,
	qse_size_t      xtn   /* the size of extension in bytes */,
	qse_awk_prm_t*  prm   /* primitive functoins */
);
/******/

/****f* AWK/qse_awk_close 
 * NAME
 *  qse_awk_close - destroy an awk object
 * DESCRIPTION
 *  A qse_awk_t instance must be destroyed using the qse_awk_close() function
 *  when finished being used. The instance passed is not valid any more once 
 *  the function returns success.
 * RETURN
 *  0 on success, -1 on failure 
 * SYNOPSIS
 */
int qse_awk_close (
	qse_awk_t* awk
);
/******/

/****f* AWK/qse_awk_getmmgr
 * NAME
 *  qse_awk_getmmgr - get the memory manager 
 * DESCRIPTION
 *  The qse_awk_getmmgr() function returns the pointer to the memory manager.
 * SYNOPSIS
 */
qse_mmgr_t* qse_awk_getmmgr (
	qse_awk_t* awk 
);
/******/

/****f* AWK/qse_awk_setmmgr
 * NAME
 *  qse_awk_setmmgr - set the extension
 * DESCRIPTION
 *  The qse_awk_setmmgr() specify the memory manager to use. As the memory 
 *  manager is specified into qse_awk_open(), you are not encouraged to change
 *  it by calling this function. Doing so may cause a lot of problems.
 * SYNOPSIS
 */
void qse_awk_setmmgr (
	qse_awk_t*  awk,
	qse_mmgr_t* mmgr
);
/******/

/****f* AWK/qse_awk_getxtn
 * NAME
 *  qse_awk_getxtn - get the extension
 * DESCRIPTION
 *  The extension area is allocated in the qse_awk_open() function when it is 
 *  given a positive extension size. The pointer to the beginning of the area
 *  can be acquired using the qse_awk_getxtn() function and be utilized 
 *  for various purposes.
 * SYNOPSIS
 */
void* qse_awk_getxtn (
	qse_awk_t* awk  /* an awk object */
);
/******/

/****f* AWK/qse_awk_getprm
 * NAME
 *  qse_awk_getprm - get primitive functions
 * SYNOPSIS
 */
qse_awk_prm_t* qse_awk_getprm (
	qse_awk_t* awk
);
/******/

/****f* AWK/qse_awk_getccls
 * NAME
 *  qse_awk_getccls - get the character classifier
 * DESCRIPTION
 *  The qse_awk_getccls() function returns the character classifier composed
 *  from the primitive functions in a call to qse_awk_open(). The data field
 *  is set to the awk object. The classifier returned is valid while the 
 *  associated awk object is alive.
 * SYNOPSIS
 */
qse_ccls_t* qse_awk_getccls (
	qse_awk_t* awk
);
/******/

/****f* AWK/qse_awk_clear
 * NAME
 *  qse_awk_clear - clear a qse_awk_t object
 * DESCRIPTION
 *  If you want to reuse a qse_awk_t instance that finished being used,
 *  you may call qse_awk_close instead of destroying and creating a new
 *  qse_awk_t instance using qse_awk_close() and qse_awk_open().
 * RETURN
 *  0 on success, -1 on failure
 * SYNOPSIS
 */
int qse_awk_clear (
	qse_awk_t* awk 
);
/******/

/****f* AWK/qse_awk_geterrstr
 * NAME
 *  qse_awk_geterrstr - get a format string for an error code
 * DESCRIPTION
 *  The qse_awk_geterrstr() function returns a pointer to a format string for
 *  the error number num.
 * SYNOPSIS
 */
const qse_char_t* qse_awk_geterrstr (
	qse_awk_t*       awk,
	qse_awk_errnum_t num
);
/******/

/****f* AWK/qse_awk_seterrstr
 * NAME
 *  qse_awk_geterrstr - set a format string for an error
 * DESCRIPTION
 *  The qse_awk_seterrstr() function sets a format string for an error. The 
 *  format string is used to compose an actual error message to be returned
 *  by qse_awk_geterrmsg() and qse_awk_geterror().
 * SYNOPSIS
 */
int qse_awk_seterrstr (
	qse_awk_t*        awk,
	qse_awk_errnum_t  num,
	const qse_char_t* str
);
/******/

int qse_awk_geterrnum (
	qse_awk_t* awk
);

qse_size_t qse_awk_geterrlin (
	qse_awk_t* awk
);

const qse_char_t* qse_awk_geterrmsg (
	qse_awk_t* awk
);

void qse_awk_seterrnum (
	qse_awk_t* awk,
	int        errnum
);

void qse_awk_seterrmsg (
	qse_awk_t*        awk, 
	int               errnum, 
	qse_size_t        errlin,
	const qse_char_t* errmsg
);

void qse_awk_geterror (
	qse_awk_t*          awk,
	int*               errnum, 
	qse_size_t*        errlin,
	const qse_char_t** errmsg
);

void qse_awk_seterror (
	qse_awk_t*        awk,
	int               errnum,
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

/*
 * NAME:
 *  enable replacement of a name of a keyword, intrinsic global variables, 
 *  and intrinsic functions.
 *
 * DESCRIPTION:
 *  If nkw is QSE_NULL or nlen is zero and okw is QSE_NULL or olen is zero,
 *  it unsets all word replacements. If nkw is QSE_NULL or nlen is zero,
 *  it unsets the replacement for okw and olen. If all of them are valid,
 *  it sets the word replace for okw and olen to nkw and nlen.
 *
 * RETURN: 0 on success, -1 on failure
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


/****f* AWK/qse_awk_parse
 * NAME
 *  qse_awk_parse - parse source code
 * SYNOPSIS
 */
int qse_awk_parse (
	qse_awk_t*     awk,
	qse_awk_sio_t* sio
);
/******/

/****f* AWK/qse_awk_alloc
 * NAME 
 *  qse_awk_alloc - allocate dynamic memory
 * RETURN
 *   the pointer to the memory space allocated on success, QSE_NULL on failure
 * SYNOPSIS
 */
void* qse_awk_alloc (
	qse_awk_t* awk /* the pointer to a qse_awk_t instance */,
	qse_size_t size /* the size of memory to allocate in bytes */
);
/******/

/****f* AWK/qse_awk_free
 * NAME 
 *  qse_awk_free - free dynamic memory
 * SYNOPSIS
 */
void qse_awk_free (
	qse_awk_t* awk /* the pointer to a qse_awk_t instance */,
	void*      ptr /* the pointer to the memory space to free */
);
/******/

/****f* AWK/qse_awk_strdup
 * NAME 
 *  qse_awk_strdup - duplicate a null-terminated string
 * DESCRIPTION
 *  The qse_awk_strdup() function is used to duplicate a string using
 *  the memory manager used by the associated qse_awk_t instance.
 *  The new string should be freed using the qse_awk_free() function.
 * RETURN
 *  The qse_awk_strdup() function returns the pointer to a new string which
 *  is a duplicate of the string s. It returns QSE_NULL on failure.
 * SYNOPSIS
 */
qse_char_t* qse_awk_strdup (
	qse_awk_t*        awk /* the pointer to a qse_awk_t instance */,
	const qse_char_t* str /* the pointer to a string */
);
/******/

/****f* AWK/qse_awk_strxdup 
 * NAME 
 *  qse_awk_strxdup - duplicate a length-delimited string
 * DESCRIPTION
 *  The qse_awk_strxdup() function is used to duplicate a string whose length
 *  is as long as len characters using the memory manager used by the 
 *  qse_awk_t instance. The new string should be freed using the qse_awk_free()
 *  function.
 * RETURN
 *  The qse_awk_strxdup() function returns the pointer to a new string which 
 *  is a duplicate of the string s on success. It returns QSE_NULL on failure.
 * SYNOPSIS
 */
qse_char_t* qse_awk_strxdup (
	qse_awk_t*        awk,
	const qse_char_t* str,
	qse_size_t        len
);
/******/

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

/****f* AWK/qse_awk_rtx_call
 * NAME
 *  qse_awk_rtx_call - call a function
 * DESCRIPTION
 *  The qse_awk_rtx_call() function invokes an AWK function. However, it is
 *  not able to invoke an intrinsic function such as split(). 
 *  The QSE_AWK_PABLOCK option can be turned off to make illegal the BEGIN 
 *  block, pattern-action blocks, and the END block.
 * RETURN
 *  The qse_awk_rtx_call() function returns 0 on success and -1 on failure.
 * EXAMPLE
 *  The example shows typical usage of the function.
 *    rtx = qse_awk_rtx_open (awk, rio, rcb, QSE_NULL, QSE_NULL);
 *    if (rtx != QSE_NULL)
 *    {
 *        v = qse_awk_rtx_call (rtx, QSE_T("init"), QSE_NULL, 0);
 *        if (v != QSE_NULL) qse_awk_rtx_refdownval (rtx, v);
 *        qse_awk_rtx_call (rtx, QSE_T("fini"), QSE_NULL, 0);
 *        if (v != QSE_NULL) qse_awk_rtx_refdownval (rtx, v);
 *        qse_awk_rtx_close (rtx);
 *    }
 * SYNOPSIS
 */
qse_awk_val_t* qse_awk_rtx_call (
	qse_awk_rtx_t*    rtx,
	const qse_char_t* name,
	qse_awk_val_t**   args,
	qse_size_t        nargs
);
/******/

/****f* AWK/qse_awk_stopall
 * NAME
 *  qse_awk_stopall - stop all runtime contexts
 * DESCRIPTION
 *  The qse_awk_stopall() function aborts all active qse_awk_run() functions
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

/****f* AWK/qse_awk_rtx_getnvmap
 * NAME
 *  qse_awk_rtx_getnvmap - get the map of named variables 
 * SYNOPSIS
 */
qse_map_t* qse_awk_rtx_getnvmap (
	qse_awk_rtx_t* rtx
);
/******/

/****f* AWK/qse_awk_rtx_geterrnum
 * NAME
 *  qse_awk_rtx_geterrnum - get an error code of a runtime context
 * SYNOPSIS
 */
int qse_awk_rtx_geterrnum (
	qse_awk_rtx_t* rtx
);
/******/

qse_size_t qse_awk_rtx_geterrlin (
	qse_awk_rtx_t* rtx
);

const qse_char_t* qse_awk_rtx_geterrmsg (
	qse_awk_rtx_t* rtx
);

void qse_awk_rtx_seterrnum (
	qse_awk_rtx_t* rtx,
	int            errnum
);

void qse_awk_rtx_seterrmsg (
	qse_awk_rtx_t*    rtx, 
	int               errnum,
	qse_size_t        errlin,
	const qse_char_t* errmsg
);

void qse_awk_rtx_geterror (
	qse_awk_rtx_t*     rtx,
	int*               errnum, 
	qse_size_t*        errlin,
	const qse_char_t** errmsg
);

void qse_awk_rtx_seterror (
	qse_awk_rtx_t*    rtx,
	int               errnum,
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

void qse_awk_rtx_refupval (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

void qse_awk_rtx_refdownval (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

void qse_awk_rtx_refdownval_nofree (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

qse_bool_t qse_awk_rtx_valtobool (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val
);

qse_char_t* qse_awk_rtx_valtostr (
	qse_awk_rtx_t* rtx,
	qse_awk_val_t* val, 
	int            opt, 
	qse_str_t*     buf,
	qse_size_t*    len
);

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
	qse_awk_val_t* v   /* the value to convert to a number */,
	qse_long_t*    l   /* a pointer to a long number */, 
	qse_real_t*    r   /* a pointer to a qse_real_t */
);
/******/

/****f* AWK/qse_awk_rtx_strtonum
 * NAME
 *  qse_awk_rtx_strtonum - convert a string to a number
 * SYNOPSIS
 */
int qse_awk_rtx_strtonum (
	qse_awk_rtx_t*    run,
	const qse_char_t* ptr,
	qse_size_t        len, 
	qse_long_t*       l, 
	qse_real_t*       r
);
/******/

#ifdef __cplusplus
}
#endif

#endif
