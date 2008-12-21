/*
 * $Id: awk.h 501 2008-12-17 08:39:15Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_AWK_AWK_H_
#define _QSE_AWK_AWK_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/map.h>
#include <qse/cmn/str.h>

/****o* qse.awk/awk interpreter
 * DESCRIPTION
 *  The library includes an AWK interpreter that can be embedded into other
 *  applications or can run stand-alone.
 *
 *  #include <qse/awk/awk.h>
 ******
 */

typedef struct qse_awk_t qse_awk_t;
typedef struct qse_awk_run_t qse_awk_run_t;
typedef struct qse_awk_val_t qse_awk_val_t;
typedef struct qse_awk_extio_t qse_awk_extio_t;

typedef struct qse_awk_prmfns_t qse_awk_prmfns_t;
typedef struct qse_awk_srcios_t qse_awk_srcios_t;
typedef struct qse_awk_runios_t qse_awk_runios_t;
typedef struct qse_awk_runcbs_t qse_awk_runcbs_t;
typedef struct qse_awk_runarg_t qse_awk_runarg_t;
typedef struct qse_awk_rexfns_t qse_awk_rexfns_t;

typedef qse_real_t (*qse_awk_pow_t) (void* data, qse_real_t x, qse_real_t y);
typedef int (*qse_awk_sprintf_t) (
	void* data, qse_char_t* buf, qse_size_t size, 
	const qse_char_t* fmt, ...);

typedef qse_ssize_t (*qse_awk_io_t) (
	int cmd, void* arg, qse_char_t* data, qse_size_t count);

struct qse_awk_extio_t 
{
	qse_awk_run_t* run; /* [IN] */
	int type;           /* [IN] console, file, coproc, pipe */
	int mode;           /* [IN] read, write, etc */
	qse_char_t* name;   /* [IN] */
	void* data;         /* [IN] */
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

	qse_awk_extio_t* next;
};

struct qse_awk_prmfns_t
{
	qse_awk_pow_t     pow;         /* required */
	qse_awk_sprintf_t sprintf;     /* required */

	/* user-defined data passed to the functions above */
	void*             data; /* optional */
};

struct qse_awk_srcios_t
{
	qse_awk_io_t in;
	qse_awk_io_t out;
	void* data;
};

struct qse_awk_runios_t
{
	qse_awk_io_t pipe;
	qse_awk_io_t coproc;
	qse_awk_io_t file;
	qse_awk_io_t console;
	void* data;
};

struct qse_awk_runcbs_t
{
	void (*on_start) (
		qse_awk_run_t* run, void* data);

	void (*on_statement) (
		qse_awk_run_t* run, qse_size_t line, void* data);

	void (*on_return) (
		qse_awk_run_t* run, qse_awk_val_t* ret, void* data);

	void (*on_end) (
		qse_awk_run_t* run, int errnum, void* data);

	void* data;
};

struct qse_awk_runarg_t
{
	qse_char_t* ptr;
	qse_size_t len;
};

struct qse_awk_rexfns_t
{
	void* (*build) (
		qse_awk_t* awk, const qse_char_t* ptn, 
		qse_size_t len, int* errnum);

	int (*match) (
		qse_awk_t* awk, void* code, int option,
		const qse_char_t* str, qse_size_t len, 
		const qse_char_t** mptr, qse_size_t* mlen, 
		int* errnum);

	void (*free) (qse_awk_t* awk, void* code);

	qse_bool_t (*isempty) (qse_awk_t* awk, void* code);
};

/* io function commands */
enum qse_awk_iocmd_t
{
	QSE_AWK_IO_OPEN   = 0,
	QSE_AWK_IO_CLOSE  = 1,
	QSE_AWK_IO_READ   = 2,
	QSE_AWK_IO_WRITE  = 3,
	QSE_AWK_IO_FLUSH  = 4,
	QSE_AWK_IO_NEXT   = 5  
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
	QSE_AWK_EXTIO       = (1 << 7), 

	/* support co-process - NOT IMPLEMENTED YET */
	QSE_AWK_COPROC      = (1 << 8),

	/* can terminate a statement with a new line */
	QSE_AWK_NEWLINE     = (1 << 9),

	/* use 1 as the start index for string operations and ARGV */
	QSE_AWK_BASEONE     = (1 << 10),

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

	/* pass the arguments to the main function */
	QSE_AWK_ARGSTOMAIN  = (1 << 14),

	/* enable the non-standard keyword reset */
	QSE_AWK_RESET       = (1 << 15),

	/* allows the assignment of a map value to a variable */
	QSE_AWK_MAPTOVAR    = (1 << 16),

	/* allows BEGIN, END, pattern-action blocks */
	QSE_AWK_PABLOCK     = (1 << 17)
};

/* error code */
enum qse_awk_errnum_t
{
	QSE_AWK_ENOERR,         /* no error */
	QSE_AWK_ECUSTOM,        /* custom error */

	QSE_AWK_EINVAL,         /* invalid parameter or data */
	QSE_AWK_ENOMEM,         /* out of memory */
	QSE_AWK_ENOSUP,         /* not supported */
	QSE_AWK_ENOPER,         /* operation not allowed */
	QSE_AWK_ENODEV,         /* no such device */
	QSE_AWK_ENOSPC,         /* no space left on device */
	QSE_AWK_EMFILE,         /* too many open files */
	QSE_AWK_EMLINK,         /* too many links */
	QSE_AWK_EAGAIN,         /* resource temporarily unavailable */
	QSE_AWK_ENOENT,         /* "'%.*s' not existing */
	QSE_AWK_EEXIST,         /* file or data exists */
	QSE_AWK_EFTBIG,         /* file or data too big */
	QSE_AWK_ETBUSY,         /* system too busy */
	QSE_AWK_EISDIR,         /* is a directory */
	QSE_AWK_EIOERR,         /* i/o error */

	QSE_AWK_EOPEN,          /* cannot open */
	QSE_AWK_EREAD,          /* cannot read */
	QSE_AWK_EWRITE,         /* cannot write */
	QSE_AWK_ECLOSE,         /* cannot close */

	QSE_AWK_EINTERN,        /* internal error */
	QSE_AWK_ERUNTIME,       /* run-time error */
	QSE_AWK_EBLKNST,        /* blocke nested too deeply */
	QSE_AWK_EEXPRNST,       /* expression nested too deeply */

	QSE_AWK_ESINOP,
	QSE_AWK_ESINCL,
	QSE_AWK_ESINRD, 

	QSE_AWK_ESOUTOP,
	QSE_AWK_ESOUTCL,
	QSE_AWK_ESOUTWR,

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

	QSE_AWK_EFUNC,          /* keyword 'func' is expected */
	QSE_AWK_EWHILE,         /* keyword 'while' is expected */
	QSE_AWK_EASSIGN,        /* assignment statement expected */
	QSE_AWK_EIDENT,         /* identifier expected */
	QSE_AWK_EFNNAME,        /* not a valid function name */
	QSE_AWK_EBLKBEG,        /* BEGIN requires an action block */
	QSE_AWK_EBLKEND,        /* END requires an action block */
	QSE_AWK_EDUPBEG,        /* duplicate BEGIN */
	QSE_AWK_EDUPEND,        /* duplicate END */
	QSE_AWK_EBFNRED,        /* intrinsic function redefined */
	QSE_AWK_EAFNRED,        /* function redefined */
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
	QSE_AWK_EGLNCPS,        /* coprocess not supported by getline */

	/* run time error */
	QSE_AWK_EDIVBY0,           /* divide by zero */
	QSE_AWK_EOPERAND,          /* invalid operand */
	QSE_AWK_EPOSIDX,           /* wrong position index */
	QSE_AWK_EARGTF,            /* too few arguments */
	QSE_AWK_EARGTM,            /* too many arguments */
	QSE_AWK_EFNNONE,           /* "function '%.*s' not found" */
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
	QSE_AWK_EBFNUSER,          /* wrong intrinsic function implementation */
	QSE_AWK_EBFNIMPL,          /* intrinsic function handler failed */
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

/* extio types */
enum qse_awk_extio_type_t
{
	/* extio types available */
	QSE_AWK_EXTIO_PIPE,
	QSE_AWK_EXTIO_COPROC,
	QSE_AWK_EXTIO_FILE,
	QSE_AWK_EXTIO_CONSOLE,

	/* reserved for internal use only */
	QSE_AWK_EXTIO_NUM
};

enum qse_awk_extio_mode_t
{
	QSE_AWK_EXTIO_PIPE_READ      = 0,
	QSE_AWK_EXTIO_PIPE_WRITE     = 1,

	/*
	QSE_AWK_EXTIO_COPROC_READ    = 0,
	QSE_AWK_EXTIO_COPROC_WRITE   = 1,
	QSE_AWK_EXTIO_COPROC_RDWR    = 2,
	*/

	QSE_AWK_EXTIO_FILE_READ      = 0,
	QSE_AWK_EXTIO_FILE_WRITE     = 1,
	QSE_AWK_EXTIO_FILE_APPEND    = 2,

	QSE_AWK_EXTIO_CONSOLE_READ   = 0,
	QSE_AWK_EXTIO_CONSOLE_WRITE  = 1
};

enum qse_awk_global_id_t
{
	/* this table should match gtab in parse.c.
	 * in addition, qse_awk_setglobal also counts 
	 * on the order of these values */

	QSE_AWK_GLOBAL_ARGC,
	QSE_AWK_GLOBAL_ARGV,
	QSE_AWK_GLOBAL_CONVFMT,
	QSE_AWK_GLOBAL_FILENAME,
	QSE_AWK_GLOBAL_FNR,
	QSE_AWK_GLOBAL_FS,
	QSE_AWK_GLOBAL_IGNORECASE,
	QSE_AWK_GLOBAL_NF,
	QSE_AWK_GLOBAL_NR,
	QSE_AWK_GLOBAL_OFILENAME,
	QSE_AWK_GLOBAL_OFMT,
	QSE_AWK_GLOBAL_OFS,
	QSE_AWK_GLOBAL_ORS,
	QSE_AWK_GLOBAL_RLENGTH,
	QSE_AWK_GLOBAL_RS,
	QSE_AWK_GLOBAL_RSTART,
	QSE_AWK_GLOBAL_SUBSEP,

	/* these are not not the actual IDs and are used internally only 
	 * Make sure you update these values properly if you add more 
	 * ID definitions, however */
	QSE_AWK_MIN_GLOBAL_ID = QSE_AWK_GLOBAL_ARGC,
	QSE_AWK_MAX_GLOBAL_ID = QSE_AWK_GLOBAL_SUBSEP
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
	QSE_AWK_VAL_REF_GLOBAL,
	QSE_AWK_VAL_REF_LOCAL,
	QSE_AWK_VAL_REF_ARG,
	QSE_AWK_VAL_REF_NAMEDIDX,
	QSE_AWK_VAL_REF_GLOBALIDX,
	QSE_AWK_VAL_REF_LOCALIDX,
	QSE_AWK_VAL_REF_ARGIDX,
	QSE_AWK_VAL_REF_POS
};

enum qse_awk_valtostr_opt_t
{
	QSE_AWK_VALTOSTR_CLEAR = (1 << 0),
	QSE_AWK_VALTOSTR_FIXED = (1 << 1),/* this overrides CLEAR */
	QSE_AWK_VALTOSTR_PRINT = (1 << 2)
};

enum qse_awk_parse_ist_t
{
	QSE_AWK_PARSE_FILES = 0,
	QSE_AWK_PARSE_STRING = 1
};

typedef struct qse_awk_val_nil_t  qse_awk_val_nil_t;
typedef struct qse_awk_val_int_t  qse_awk_val_int_t;
typedef struct qse_awk_val_real_t qse_awk_val_real_t;
typedef struct qse_awk_val_str_t  qse_awk_val_str_t;
typedef struct qse_awk_val_rex_t  qse_awk_val_rex_t;
typedef struct qse_awk_val_map_t  qse_awk_val_map_t;
typedef struct qse_awk_val_ref_t  qse_awk_val_ref_t;

/* this is not a value. it is just a value holder */
typedef struct qse_awk_val_chunk_t qse_awk_val_chunk_t;

#if QSE_SIZEOF_INT == 2
#define QSE_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 13
#else
#define QSE_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 29
#endif

#define QSE_AWK_VAL_TYPE(x) ((x)->type)

struct qse_awk_val_t
{
	QSE_AWK_VAL_HDR;	
};

/* QSE_AWK_VAL_NIL */
struct qse_awk_val_nil_t
{
	QSE_AWK_VAL_HDR;
};

/* QSE_AWK_VAL_INT */
struct qse_awk_val_int_t
{
	QSE_AWK_VAL_HDR;
	qse_long_t val;
	void* nde;
};

/* QSE_AWK_VAL_REAL */
struct qse_awk_val_real_t
{
	QSE_AWK_VAL_HDR;
	qse_real_t val;
	void* nde;
};

/* QSE_AWK_VAL_STR */
struct qse_awk_val_str_t
{
	QSE_AWK_VAL_HDR;
	qse_char_t* buf;
	qse_size_t  len;
};

/* QSE_AWK_VAL_REX */
struct qse_awk_val_rex_t
{
	QSE_AWK_VAL_HDR;
	qse_char_t* buf;
	qse_size_t  len;
	void*      code;
};

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

/****f* qse.awk/qse_awk_open
 * NAME
 *  qse_awk_open - create an awk object
 * 
 * DESCRIPTION
 *  The qse_awk_open() function creates a new qse_awk_t instance.
 *  The instance created can be passed to other qse_awk_xxx() functions and
 *  is valid until it is successfully destroyed using the qse_qse_close() 
 *  function.
 *
 * RETURN
 *  The qse_awk_open() function returns the pointer to an qse_awk_t instance 
 *  on success and QSE_NULL on failure.
 *
 * SYNOPSIS
 */
qse_awk_t* qse_awk_open ( 
	qse_mmgr_t* mmgr    /* a memory manager */,
	qse_size_t  xtnsize /* size of extension area in bytes */
);
/******/

/****f* qse.awk/qse_awk_close 
 * NAME
 *  qse_awk_close - destroy an awk object
 *
 * An qse_awk_t instance should be destroyed using the qse_awk_close() function
 * when finished being used. The instance passed is not valid any more once 
 * the function returns success.
 *
 * RETURN
 *  0 on success, -1 on failure 
 * 
 * SYNOPSIS
 */
int qse_awk_close (
	qse_awk_t* awk  /* an awk object */
);
/******/

/****f* qse.awk/qse_awk_getmmgr
 * NAME
 *  qse_awk_getmmgr - get the memory manager 
 *
 * DESCRIPTION
 *  The qse_awk_getmmgr() function returns the pointer to the memory manager.
 *
 * SYNOPSIS
 */
qse_mmgr_t* qse_awk_getmmgr (
	qse_awk_t* awk  /* an awk object */
);
/******/

void qse_awk_setmmgr (
	qse_awk_t* awk,
	qse_mmgr_t* mmgr
);

/****f* qse.awk/qse_awk_getxtn
 * NAME
 *  qse_awk_getxtn - get the extension
 *
 * DESCRIPTION
 *  The extension area is allocated in the qse_awk_open() function when it is 
 *  given a positive extension size. The pointer to the beginning of the area
 *  can be acquired using the qse_awk_getxtn() function and be utilized 
 *  for various purposes.
 *
 * SYNOPSIS
 */
void* qse_awk_getxtn (
	qse_awk_t* awk  /* an awk object */
);
/******/

qse_ccls_t* qse_awk_getccls (
	qse_awk_t* awk
);

/*
 * set the character classfier
 */
void qse_awk_setccls (
	/* the pointer to an qse_awk_t instance */
	qse_awk_t* awk, 
	/* the pointer to a character classfiler */
	qse_ccls_t* ccls
);

qse_awk_prmfns_t* qse_awk_getprmfns (
	qse_awk_t* awk
);

/*
 * set primitive functions
 */
void qse_awk_setprmfns (
	/* the pointer to an qse_awk_t instance */
	qse_awk_t* awk, 
	/* the pointer to a primitive function structure */
	qse_awk_prmfns_t* prmfns
);

/*
 * clear an qse_awk_t instance
 *
 * If you want to reuse an qse_awk_t instance that finished being used,
 * you may call qse_awk_close instead of destroying and creating a new
 * qse_awk_t instance using qse_awk_close() and qse_awk_open().
 *
 * RETURN 0 on success, -1 on failure
 */
int qse_awk_clear (
	/* the pointer to an qse_awk_t instance */
	qse_awk_t* awk 
);

const qse_char_t* qse_awk_geterrstr (qse_awk_t* awk, int num);
int qse_awk_seterrstr (qse_awk_t* awk, int num, const qse_char_t* str);

int qse_awk_geterrnum (qse_awk_t* awk);
qse_size_t qse_awk_geterrlin (qse_awk_t* awk);
const qse_char_t* qse_awk_geterrmsg (qse_awk_t* awk);

void qse_awk_seterrnum (qse_awk_t* awk, int errnum);
void qse_awk_seterrmsg (qse_awk_t* awk, 
	int errnum, qse_size_t errlin, const qse_char_t* errmsg);

void qse_awk_geterror (
	qse_awk_t* awk, int* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg);

void qse_awk_seterror (
	qse_awk_t* awk, int errnum, qse_size_t errlin, 
	const qse_cstr_t* errarg, qse_size_t argcnt);

int qse_awk_getoption (qse_awk_t* awk);
void qse_awk_setoption (qse_awk_t* awk, int opt);

qse_size_t qse_awk_getmaxdepth (qse_awk_t* awk, int type);
void qse_awk_setmaxdepth (qse_awk_t* awk, int types, qse_size_t depth);

int qse_awk_getword (
	qse_awk_t* awk, 
	const qse_char_t* okw,
	qse_size_t olen,
	const qse_char_t** nkw,
	qse_size_t* nlen
);

int qse_awk_unsetword (
	qse_awk_t* awk,
	const qse_char_t* kw,
	qse_size_t len
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
 * RETURNS: 0 on success, -1 on failure
 */
int qse_awk_setword (
	/* the pointer to an qse_awk_t instance */
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

/*
 * set the customized regular processing routine. (TODO:  NOT YET IMPLEMENTED)
 *
 * RETURNS 0 on success, -1 on failure
 */
int qse_awk_setrexfns (qse_awk_t* awk, qse_awk_rexfns_t* rexfns);

/****f* qse.awk/qse_awk_addglobal
 * NAME
 *  qse_awk_addglobal - add an intrinsic global variable.
 *
 * RETURN
 *  On success, the ID of the global variable added is returned.
 *  On failure, -1 is returned.
 *
 * SYNOPSIS
 */
int qse_awk_addglobal (
	qse_awk_t*        awk,
	const qse_char_t* name,
	qse_size_t        len
);
/******/

/****f* qse.awk/qse_awk_delglobal
 * NAME
 *  qse_awk_delglobal - delete an instrinsic global variable. 
 *
 * SYNOPSIS
 */
int qse_awk_delglobal (
	qse_awk_t*        awk,
	const qse_char_t* name,
	qse_size_t        len
);
/******/

/****f* qse.awk/qse_awk_parse
 * NAME
 *  qse_awk_parse - parse source code
 *
 * SYNOPSIS
 */
int qse_awk_parse (
	qse_awk_t*        awk,
	qse_awk_srcios_t* srcios
);
/******/


/****f* qse.awk/qse_awk_opensimple
 * NAME
 *  qse_awk_opensimple - create an awk object
 *
 * SYNOPSIS
 */
qse_awk_t* qse_awk_opensimple (
	qse_size_t xtnsize /* size of extension area in bytes */
);
/******/

/****f* qse.awk/qse_awk_parsesimple
 * NAME
 *  qse_awk_parsesimple - parse source code
 *
 * SYNOPSIS
 */
int qse_awk_parsesimple (
	qse_awk_t*        awk,
	const void*       isp /* source file names or source string */,
	int               ist /* QSE_AWK_PARSE_FILES, QSE_AWK_PARSE_STRING */,
	const qse_char_t* osf /* an output source file name */
);
/******/

/****f* qse.awk/qse_awk_runsimple
 * NAME
 *  qse_awk_runsimple - run a parsed program
 *
 * SYNOPSIS
 */
int qse_awk_runsimple (
	qse_awk_t*        awk,
	qse_char_t**      icf /* input console files */,
	qse_awk_runcbs_t* cbs /* callbacks */
);
/******/

/**
 * Executes a parsed program.
 *
 * qse_awk_run returns 0 on success and -1 on failure, generally speaking.
 *  A runtime context is required for it to start running the program.
 *  Once the runtime context is created, the program starts to run.
 *  The context creation failure is reported by the return value -1 of
 *  this function. however, the runtime error after the context creation
 *  is reported differently depending on the use of the callback.
 *  When no callback is specified (i.e. runcbs is QSE_NULL), qse_awk_run
 *  returns -1 on an error and awk->errnum is set accordingly.
 *  However, if a callback is specified (i.e. runcbs is not QSE_NULL),
 *  qse_awk_run returns 0 on both success and failure. Instead, the 
 *  on_end handler of the callback is triggered with the relevant 
 *  error number. The third parameter to on_end denotes this error number.
 */
int qse_awk_run (
	qse_awk_t* awk, const qse_char_t* main,
	qse_awk_runios_t* runios, qse_awk_runcbs_t* runcbs, 
	qse_awk_runarg_t* runarg, void* data);

void qse_awk_stop (qse_awk_run_t* run);
void qse_awk_stopall (qse_awk_t* awk);

qse_bool_t qse_awk_isstop (qse_awk_run_t* run);


/** 
 * Gets the number of arguments passed to qse_awk_run 
 */
qse_size_t qse_awk_getnargs (qse_awk_run_t* run);

/** 
 * Gets an argument passed to qse_awk_run
 */
qse_awk_val_t* qse_awk_getarg (qse_awk_run_t* run, qse_size_t idx);

/****f* qse.awk/qse_awk_getglobal
 * NAME
 *  qse_awk_getglobal - gets the value of a global variable
 *
 * PARAMETERS
 *  id - A global variable id. An ID is one of the predefined global 
 *       variable IDs or the value returned by qse_awk_addglobal().
 *
 * RETURN
 *  The pointer to a value is returned. This function never fails
 *  so long as id is valid. Otherwise, you may fall into trouble.
 */
qse_awk_val_t* qse_awk_getglobal (
	qse_awk_run_t* run,
	int            id
);
/******/

int qse_awk_setglobal (
	qse_awk_run_t* run, 
	int            id,
	qse_awk_val_t* val
);

/****f* qse.awk/qse_awk_setretval
 * NAME
 *  qse_awk_setretval - set the return value
 *
 * DESCRIPTION
 *  The qse_awk_setretval() sets the return value of a function
 *  when called from within a function handlers. The caller doesn't
 *  have to invoke qse_awk_refupval() and qse_awk_refdownval() 
 *  with the value to be passed to qse_awk_setretval(). 
 *  The qse_awk_setretval() will update its reference count properly
 *  once the return value is set. 
 */
void qse_awk_setretval (
	qse_awk_run_t* run,
	qse_awk_val_t* val
);

int qse_awk_setfilename (
	qse_awk_run_t* run, const qse_char_t* name, qse_size_t len);
int qse_awk_setofilename (
	qse_awk_run_t* run, const qse_char_t* name, qse_size_t len);


/****f* qse.awk/qse_awk_getrunawk
 * NAME
 *  qse_awk_getrunawk - get the owning awk object
 *
 * SYNOPSIS
 */
qse_awk_t* qse_awk_getrunawk (
	qse_awk_run_t* run
);
/******/

/****f* qse.awk/qse_awk_getrunmmgr
 * NAME
 *  qse_awk_getrunmmgr - get the memory manager of a run object
 * 
 * SYNOPSIS
 */
qse_mmgr_t* qse_awk_getrunmmgr (
	qse_awk_run_t* run
);
/******/

/****f* qse.awk/qse_awk_getrundata
 * NAME
 *  qse_awk_getrundata - get the user-specified data for a run object
 * 
 * SYNOPSIS
 */
void* qse_awk_getrundata (
	qse_awk_run_t* run
);
/******/

/****f* qse.awk/qse_awk_getrunnvmap
 * NAME
 *  qse_awk_getrunnvmap - get the map of named variables 
 * 
 * SYNOPSIS
 */
qse_map_t* qse_awk_getrunnvmap (
	qse_awk_run_t* run
);
/******/

/* functions to manipulate the run-time error */
int qse_awk_getrunerrnum (
	qse_awk_run_t* run
);
qse_size_t qse_awk_getrunerrlin (
	qse_awk_run_t* run
);
const qse_char_t* qse_awk_getrunerrmsg (
	qse_awk_run_t* run
);
void qse_awk_setrunerrnum (
	qse_awk_run_t* run,
	int errnum
);
void qse_awk_setrunerrmsg (
	qse_awk_run_t* run, 
	int errnum,
	qse_size_t errlin,
	const qse_char_t* errmsg
);

void qse_awk_getrunerror (
	qse_awk_run_t* run, int* errnum, 
	qse_size_t* errlin, const qse_char_t** errmsg);

void qse_awk_setrunerror (
	qse_awk_run_t* run, int errnum, qse_size_t errlin, 
	const qse_cstr_t* errarg, qse_size_t argcnt);

/* functions to manipulate intrinsic functions */
void* qse_awk_addfunc (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t name_len, 
	int when_valid, qse_size_t min_args, qse_size_t max_args, 
	const qse_char_t* arg_spec, 
	int (*handler)(qse_awk_run_t*,const qse_char_t*,qse_size_t));

int qse_awk_delfunc (
	qse_awk_t* awk, const qse_char_t* name, qse_size_t name_len);

void qse_awk_clrbfn (qse_awk_t* awk);

/* record and field functions */
int qse_awk_clrrec (qse_awk_run_t* run, qse_bool_t skip_inrec_line);
int qse_awk_setrec (qse_awk_run_t* run, qse_size_t idx, const qse_char_t* str, qse_size_t len);

/* utility functions exported by awk.h */

/*
 * NAME allocate dynamic memory
 *
 * DESCRIPTION
 *
 *
 * RETURNS
 *   the pointer to the memory area allocated on success, QSE_NULL on failure
 */
void* qse_awk_alloc (
	/* the pointer to an qse_awk_t instance */
	qse_awk_t* awk, 
	/* the size of memory to allocate in bytes */
	qse_size_t size
);

/*
 * NAME free dynamic memory
 *
 * DESCRIPTION
 */
void qse_awk_free (
	/* the pointer to an qse_awk_t instance */
	qse_awk_t* awk, 
	/* the pointer to the memory area to free */
	void* ptr
);

/*
 * NAME duplicate a string
 *
 * DESCRIPTION
 *  The qse_awk_strdup() function is used to duplicate a string using
 *  the memory manager used by the associated qse_awk_t instance.
 *  The new string should be freed using the qse_awk_free() function.
 *
 * RETURNS
 *  The pointer to a new string which is a duplicate of the string s.
 */
qse_char_t* qse_awk_strdup (
	/* the pointer to an qse_awk_t instance */
	qse_awk_t* awk, 
	/* the pointer to a string */
	const qse_char_t* s
);

/*
 * NAME duplicate a string of a length given
 *
 * DESCRIPTION
 *  The qse_awk_strdup() function is used to duplicate a string whose length
 *  is as long as l characters using the memory manager used by the associated
 *  qse_awk_t instance. The new string should be freed using the qse_awk_free()
 *  function.
 *
 * RETURNS
 *  The pointer to a new string which is a duplicate of the string s.
 */
qse_char_t* qse_awk_strxdup (qse_awk_t* awk, const qse_char_t* s, qse_size_t l);

qse_long_t qse_awk_strxtolong (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len,
	int base, const qse_char_t** endptr);
qse_real_t qse_awk_strxtoreal (
	qse_awk_t* awk, const qse_char_t* str, qse_size_t len, 
	const qse_char_t** endptr);

qse_size_t qse_awk_longtostr (
	qse_long_t value, int radix, const qse_char_t* prefix,
	qse_char_t* buf, qse_size_t size);

/* value manipulation functions */
qse_awk_val_t* qse_awk_makeintval (qse_awk_run_t* run, qse_long_t v);
qse_awk_val_t* qse_awk_makerealval (qse_awk_run_t* run, qse_real_t v);

qse_awk_val_t* qse_awk_makestrval0 (
	qse_awk_run_t* run, const qse_char_t* str);
qse_awk_val_t* qse_awk_makestrval (
	qse_awk_run_t* run, const qse_char_t* str, qse_size_t len);
qse_awk_val_t* qse_awk_makestrval_nodup (
	qse_awk_run_t* run, qse_char_t* str, qse_size_t len);
qse_awk_val_t* qse_awk_makestrval2 (
	qse_awk_run_t* run,
	const qse_char_t* str1, qse_size_t len1, 
	const qse_char_t* str2, qse_size_t len2);

qse_awk_val_t* qse_awk_makerexval (
	qse_awk_run_t* run, const qse_char_t* buf, qse_size_t len, void* code);
qse_awk_val_t* qse_awk_makemapval (qse_awk_run_t* run);
qse_awk_val_t* qse_awk_makerefval (
	qse_awk_run_t* run, int id, qse_awk_val_t** adr);

qse_bool_t qse_awk_isstaticval (qse_awk_val_t* val);

void qse_awk_freeval (qse_awk_run_t* run, qse_awk_val_t* val, qse_bool_t cache);

void qse_awk_refupval (qse_awk_run_t* run, qse_awk_val_t* val);
void qse_awk_refdownval (qse_awk_run_t* run, qse_awk_val_t* val);
void qse_awk_refdownval_nofree (qse_awk_run_t* run, qse_awk_val_t* val);

void qse_awk_freevalchunk (qse_awk_run_t* run, qse_awk_val_chunk_t* chunk);

qse_bool_t qse_awk_valtobool (
	qse_awk_run_t* run,
	qse_awk_val_t* val
);

qse_char_t* qse_awk_valtostr (
	qse_awk_run_t* run,
	qse_awk_val_t* val, 
	int opt, 
	qse_str_t* buf,
	qse_size_t* len
);

/****f* qse.awk/qse_awk_valtonum
 * NAME
 *  qse_awk_valtonum - convert a value to a number
 *
 * DESCRIPTION
 *  The qse_awk_valtonum() function converts a value to a number. 
 *  If the value is converted to a long number, it is stored in the memory
 *  pointed to by l and 0 is returned. If the value is converted to a real 
 *  number, it is stored in the memory pointed to by r and 1 is returned.
 * 
 * RETURN
 *  The qse_awk_valtonum() function returns -1 on error, 0 if the converted
 *  number is a long number and 1 if it is a real number.
 *
 * EXAMPLES
 *  qse_long_t l;
 *  qse_real_t r;
 *  int n;
 *
 *  n = qse_awk_valtonum (v, &l, &r);
 *  if (n == -1) error ();
 *  else if (n == 0) do_long (l);
 *  else if (n == 1) do_real (r);
 *
 * SYNOPSIS
 */
int qse_awk_valtonum (
	qse_awk_run_t* run,
	qse_awk_val_t* v   /* the value to convert to a number */,
	qse_long_t*    l   /* a pointer to a long number */, 
	qse_real_t*    r   /* a pointer to a qse_real_t */
);
/******/

/****f* qse.awk/qse_awk_strtonum
 * NAME
 *  qse_awk_strtonum - convert a string to a number
 *
 * SYNOPSIS
 */
int qse_awk_strtonum (
	qse_awk_run_t*    run,
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
