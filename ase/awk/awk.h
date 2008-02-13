/* 
 * $Id: awk.h,v 1.26 2007/11/10 15:21:40 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_AWK_AWK_H_
#define _ASE_AWK_AWK_H_

/** 
 * @file awk.h
 * @brief Primary header file for the engine
 *  
 * This file defines most of the data types and functions required to embed
 * the interpreter engine.
 */

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>
#include <ase/cmn/map.h>

typedef struct ase_awk_t ase_awk_t;
typedef struct ase_awk_run_t ase_awk_run_t;
typedef struct ase_awk_val_t ase_awk_val_t;
typedef struct ase_awk_extio_t ase_awk_extio_t;

typedef struct ase_awk_prmfns_t ase_awk_prmfns_t;
typedef struct ase_awk_srcios_t ase_awk_srcios_t;
typedef struct ase_awk_runios_t ase_awk_runios_t;
typedef struct ase_awk_runcbs_t ase_awk_runcbs_t;
typedef struct ase_awk_runarg_t ase_awk_runarg_t;
typedef struct ase_awk_rexfns_t ase_awk_rexfns_t;

typedef ase_real_t (*ase_awk_pow_t) (void* custom, ase_real_t x, ase_real_t y);
typedef int (*ase_awk_sprintf_t) (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...);
typedef void (*ase_awk_dprintf_t) (void* custom, const ase_char_t* fmt, ...); 

typedef ase_ssize_t (*ase_awk_io_t) (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

struct ase_awk_extio_t 
{
	ase_awk_run_t* run; /* [IN] */
	int type;           /* [IN] console, file, coproc, pipe */
	int mode;           /* [IN] read, write, etc */
	ase_char_t* name;   /* [IN] */
	void* custom_data;  /* [IN] */
	void* handle;       /* [OUT] */

	/* input */
	struct
	{
		ase_char_t buf[2048];
		ase_size_t pos;
		ase_size_t len;
		ase_bool_t eof;
		ase_bool_t eos;
	} in;

	/* output */
	struct
	{
		ase_bool_t eof;
		ase_bool_t eos;
	} out;

	ase_awk_extio_t* next;
};

struct ase_awk_prmfns_t
{
	ase_mmgr_t mmgr;
	ase_ccls_t ccls;

	struct
	{
		/* utilities */
		ase_awk_pow_t     pow;         /* required */
		ase_awk_sprintf_t sprintf;     /* required */
		ase_awk_dprintf_t dprintf;     /* required in the debug mode */

		/* user-defined data passed to the functions above */
		void*             custom_data; /* optional */
	} misc;
};

struct ase_awk_srcios_t
{
	ase_awk_io_t in;
	ase_awk_io_t out;
	void* custom_data;
};

struct ase_awk_runios_t
{
	ase_awk_io_t pipe;
	ase_awk_io_t coproc;
	ase_awk_io_t file;
	ase_awk_io_t console;
	void* custom_data;
};

struct ase_awk_runcbs_t
{
	void (*on_start) (
		ase_awk_run_t* run, void* custom_data);

	void (*on_statement) (
		ase_awk_run_t* run, ase_size_t line, void* custom_data);

	void (*on_return) (
		ase_awk_run_t* run, ase_awk_val_t* ret, void* custom_data);

	void (*on_end) (
		ase_awk_run_t* run, int errnum, void* custom_data);

	void* custom_data;
};

struct ase_awk_runarg_t
{
	ase_char_t* ptr;
	ase_size_t len;
};

struct ase_awk_rexfns_t
{
	void* (*build) (
		ase_awk_t* awk, const ase_char_t* ptn, 
		ase_size_t len, int* errnum);

	int (*match) (
		ase_awk_t* awk, void* code, int option,
		const ase_char_t* str, ase_size_t len, 
		const ase_char_t** mptr, ase_size_t* mlen, 
		int* errnum);

	void (*free) (ase_awk_t* awk, void* code);

	ase_bool_t (*isempty) (ase_awk_t* awk, void* code);
};

/* io function commands */
enum ase_awk_iocmd_t
{
	ASE_AWK_IO_OPEN   = 0,
	ASE_AWK_IO_CLOSE  = 1,
	ASE_AWK_IO_READ   = 2,
	ASE_AWK_IO_WRITE  = 3,
	ASE_AWK_IO_FLUSH  = 4,
	ASE_AWK_IO_NEXT   = 5  
};

/* various options */
enum ase_awk_option_t
{ 
	/* allow undeclared variables and implicit concatenation */
	ASE_AWK_IMPLICIT    = (1 << 0),

	/* allow explicit variable declaration, the concatenation
	 * operator(.), and a parse-time function check. */
	ASE_AWK_EXPLICIT    = (1 << 1), 

	/* a function name should not coincide to be a variable name */
	/*ASE_AWK_UNIQUEFN    = (1 << 2),*/

	/* allow variable shading */
	/*ASE_AWK_SHADING     = (1 << 3),*/

	/* support shift operators */
	ASE_AWK_SHIFT       = (1 << 4), 

	/* enable the idiv operator (double slashes) */
	ASE_AWK_IDIV        = (1 << 5), 

	/* support string concatenation in tokenization.
	 * this option can change the behavior of a certain construct.
	 * getline < "abc" ".def" is treated as if it is getline < "abc.def" 
	 * when this option is on. If this option is off, the same expression
	 * is treated as if it is (getline < "abc") ".def". */
	ASE_AWK_STRCONCAT   = (1 << 6), 

	/* support getline and print */
	ASE_AWK_EXTIO       = (1 << 7), 

	/* support co-process - NOT IMPLEMENTED YET */
	ASE_AWK_COPROC      = (1 << 8),

	/* support blockless patterns */
	ASE_AWK_BLOCKLESS   = (1 << 9), 

	/* use 1 as the start index for string operations and ARGV */
	ASE_AWK_BASEONE     = (1 << 10),

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
	ASE_AWK_STRIPSPACES = (1 << 11),

	/* enable the nextoutfile keyword */
	ASE_AWK_NEXTOFILE   = (1 << 12),

	/* cr + lf by default */
	ASE_AWK_CRLF        = (1 << 13),

	/* pass the arguments to the main function */
	ASE_AWK_ARGSTOMAIN  = (1 << 14),

	/* enable the non-standard keyworkd reset */
	ASE_AWK_RESET       = (1 << 15),

	/* allows the assignment of a map value to a variable */
	ASE_AWK_MAPTOVAR    = (1 << 16),

	/* allows BEGIN, END, pattern-action blocks */
	ASE_AWK_PABLOCK     = (1 << 17)
};

/* error code */
enum ase_awk_errnum_t
{
	ASE_AWK_ENOERR,         /* no error */
	ASE_AWK_ECUSTOM,        /* custom error */

	ASE_AWK_EINVAL,         /* invalid parameter or data */
	ASE_AWK_ENOMEM,         /* out of memory */
	ASE_AWK_ENOSUP,         /* not supported */
	ASE_AWK_ENOPER,         /* operation not allowed */
	ASE_AWK_ENODEV,         /* no such device */
	ASE_AWK_ENOSPC,         /* no space left on device */
	ASE_AWK_EMFILE,         /* too many open files */
	ASE_AWK_EMLINK,         /* too many links */
	ASE_AWK_EAGAIN,         /* resource temporarily unavailable */
	ASE_AWK_ENOENT,         /* "'%.*s' not existing */
	ASE_AWK_EEXIST,         /* file or data exists */
	ASE_AWK_EFTBIG,         /* file or data too big */
	ASE_AWK_ETBUSY,         /* system too busy */
	ASE_AWK_EISDIR,         /* is a directory */
	ASE_AWK_EIOERR,         /* i/o error */

	ASE_AWK_EOPEN,          /* cannot open */
	ASE_AWK_EREAD,          /* cannot read */
	ASE_AWK_EWRITE,         /* cannot write */
	ASE_AWK_ECLOSE,         /* cannot close */

	ASE_AWK_EINTERN,        /* internal error */
	ASE_AWK_ERUNTIME,       /* run-time error */
	ASE_AWK_EBLKNST,        /* blocke nested too deeply */
	ASE_AWK_EEXPRNST,       /* expression nested too deeply */

	ASE_AWK_ESINOP,
	ASE_AWK_ESINCL,
	ASE_AWK_ESINRD, 

	ASE_AWK_ESOUTOP,
	ASE_AWK_ESOUTCL,
	ASE_AWK_ESOUTWR,

	ASE_AWK_ELXCHR,         /* lexer came accross an wrong character */
	ASE_AWK_ELXDIG,         /* invalid digit */
	ASE_AWK_ELXUNG,         /* lexer failed to unget a character */

	ASE_AWK_EENDSRC,        /* unexpected end of source */
	ASE_AWK_EENDCMT,        /* a comment not closed properly */
	ASE_AWK_EENDSTR,        /* a string not closed with a quote */
	ASE_AWK_EENDREX,        /* unexpected end of a regular expression */
	ASE_AWK_ELBRACE,        /* left brace expected */
	ASE_AWK_ELPAREN,        /* left parenthesis expected */
	ASE_AWK_ERPAREN,        /* right parenthesis expected */
	ASE_AWK_ERBRACK,        /* right bracket expected */
	ASE_AWK_ECOMMA,         /* comma expected */
	ASE_AWK_ESCOLON,        /* semicolon expected */
	ASE_AWK_ECOLON,         /* colon expected */
	ASE_AWK_ESTMEND,        /* statement not ending with a semicolon */
	ASE_AWK_EIN,            /* keyword 'in' is expected */
	ASE_AWK_ENOTVAR,        /* not a variable name after 'in' */
	ASE_AWK_EEXPRES,        /* expression expected */

	ASE_AWK_EFUNC,          /* keyword 'func' is expected */
	ASE_AWK_EWHILE,         /* keyword 'while' is expected */
	ASE_AWK_EASSIGN,        /* assignment statement expected */
	ASE_AWK_EIDENT,         /* identifier expected */
	ASE_AWK_EFNNAME,        /* not a valid function name */
	ASE_AWK_EBLKBEG,        /* BEGIN requires an action block */
	ASE_AWK_EBLKEND,        /* END requires an action block */
	ASE_AWK_EDUPBEG,        /* duplicate BEGIN */
	ASE_AWK_EDUPEND,        /* duplicate END */
	ASE_AWK_EBFNRED,        /* intrinsic function redefined */
	ASE_AWK_EAFNRED,        /* function redefined */
	ASE_AWK_EGBLRED,        /* global variable redefined */
	ASE_AWK_EPARRED,        /* parameter redefined */
	ASE_AWK_EVARRED,        /* named variable redefined */
	ASE_AWK_EDUPPAR,        /* duplicate parameter name */
	ASE_AWK_EDUPGBL,        /* duplicate global variable name */
	ASE_AWK_EDUPLCL,        /* duplicate local variable name */
	ASE_AWK_EBADPAR,        /* not a valid parameter name */
	ASE_AWK_EBADVAR,        /* not a valid variable name */
	ASE_AWK_EUNDEF,         /* undefined identifier */
	ASE_AWK_ELVALUE,        /* l-value required */
	ASE_AWK_EGBLTM,         /* too many global variables */
	ASE_AWK_ELCLTM,         /* too many local variables */
	ASE_AWK_EPARTM,         /* too many parameters */
	ASE_AWK_EDELETE,        /* delete not followed by a variable */
	ASE_AWK_ERESET,         /* reset not followed by a variable */
	ASE_AWK_EBREAK,         /* break outside a loop */
	ASE_AWK_ECONTINUE,      /* continue outside a loop */
	ASE_AWK_ENEXTBEG,       /* next illegal in BEGIN block */
	ASE_AWK_ENEXTEND,       /* next illegal in END block */
	ASE_AWK_ENEXTFBEG,      /* nextfile illegal in BEGIN block */
	ASE_AWK_ENEXTFEND,      /* nextfile illegal in END block */
	ASE_AWK_EPRINTFARG,     /* printf not followed by any arguments */
	ASE_AWK_EPREPST,        /* both prefix and postfix increment/decrement 
	                           operator present */
	ASE_AWK_EGLNCPS,        /* coprocess not supported by getline */

	/* run time error */
	ASE_AWK_EDIVBY0,           /* divide by zero */
	ASE_AWK_EOPERAND,          /* invalid operand */
	ASE_AWK_EPOSIDX,           /* wrong position index */
	ASE_AWK_EARGTF,            /* too few arguments */
	ASE_AWK_EARGTM,            /* too many arguments */
	ASE_AWK_EFNNONE,           /* "function '%.*s' not found" */
	ASE_AWK_ENOTIDX,           /* variable not indexable */
	ASE_AWK_ENOTDEL,           /* variable not deletable */
	ASE_AWK_ENOTMAP,           /* value not a map */
	ASE_AWK_ENOTMAPIN,         /* right-hand side of 'in' not a map */
	ASE_AWK_ENOTMAPNILIN,      /* right-hand side of 'in' not a map nor nil */
	ASE_AWK_ENOTREF,           /* value not referenceable */
	ASE_AWK_ENOTASS,           /* value not assignable */
	ASE_AWK_EIDXVALASSMAP,     /* indexed value cannot be assigned a map */
	ASE_AWK_EPOSVALASSMAP,     /* a positional cannot be assigned a map */
	ASE_AWK_EMAPTOSCALAR,      /* cannot change a map to a scalar value */
	ASE_AWK_ESCALARTOMAP,      /* cannot change a scalar value to a map */
	ASE_AWK_EMAPNOTALLOWED,    /* a map is not allowed */
	ASE_AWK_EVALTYPE,          /* wrong value type */
	ASE_AWK_ERDELETE,          /* delete called with a wrong target */
	ASE_AWK_ERRESET,           /* reset called with a wrong target */
	ASE_AWK_ERNEXTBEG,         /* next called from BEGIN */
	ASE_AWK_ERNEXTEND,         /* next called from END */
	ASE_AWK_ERNEXTFBEG,        /* nextfile called from BEGIN */
	ASE_AWK_ERNEXTFEND,        /* nextfile called from END */
	ASE_AWK_EBFNUSER,          /* wrong intrinsic function implementation */
	ASE_AWK_EBFNIMPL,          /* intrinsic function handler failed */
	ASE_AWK_EIOUSER,           /* wrong user io handler implementation */
	ASE_AWK_EIONONE,           /* no such io name found */
	ASE_AWK_EIOIMPL,           /* i/o callback returned an error */
	ASE_AWK_EIONMEM,           /* i/o name empty */
	ASE_AWK_EIONMNL,           /* i/o name contains '\0' */
	ASE_AWK_EFMTARG,           /* arguments to format string not sufficient */
	ASE_AWK_EFMTCNV,           /* recursion detected in format conversion */
	ASE_AWK_ECONVFMTCHR,       /* an invalid character found in CONVFMT */
	ASE_AWK_EOFMTCHR,          /* an invalid character found in OFMT */

	/* regular expression error */
	ASE_AWK_EREXRECUR,        /* recursion too deep */
	ASE_AWK_EREXRPAREN,       /* a right parenthesis is expected */
	ASE_AWK_EREXRBRACKET,     /* a right bracket is expected */
	ASE_AWK_EREXRBRACE,       /* a right brace is expected */
	ASE_AWK_EREXUNBALPAR,     /* unbalanced parenthesis */
	ASE_AWK_EREXCOLON,        /* a colon is expected */
	ASE_AWK_EREXCRANGE,       /* invalid character range */
	ASE_AWK_EREXCCLASS,       /* invalid character class */
	ASE_AWK_EREXBRANGE,       /* invalid boundary range */
	ASE_AWK_EREXEND,          /* unexpected end of the pattern */
	ASE_AWK_EREXGARBAGE,      /* garbage after the pattern */

	/* the number of error numbers, internal use only */
	ASE_AWK_NUMERRNUM 
};

/* depth types */
enum ase_awk_depth_t
{
	ASE_AWK_DEPTH_BLOCK_PARSE = (1 << 0),
	ASE_AWK_DEPTH_BLOCK_RUN   = (1 << 1),
	ASE_AWK_DEPTH_EXPR_PARSE  = (1 << 2),
	ASE_AWK_DEPTH_EXPR_RUN    = (1 << 3),
	ASE_AWK_DEPTH_REX_BUILD   = (1 << 4),
	ASE_AWK_DEPTH_REX_MATCH   = (1 << 5)
};

/* extio types */
enum ase_awk_extio_type_t
{
	/* extio types available */
	ASE_AWK_EXTIO_PIPE,
	ASE_AWK_EXTIO_COPROC,
	ASE_AWK_EXTIO_FILE,
	ASE_AWK_EXTIO_CONSOLE,

	/* reserved for internal use only */
	ASE_AWK_EXTIO_NUM
};

enum ase_awk_extio_mode_t
{
	ASE_AWK_EXTIO_PIPE_READ      = 0,
	ASE_AWK_EXTIO_PIPE_WRITE     = 1,

	/*
	ASE_AWK_EXTIO_COPROC_READ    = 0,
	ASE_AWK_EXTIO_COPROC_WRITE   = 1,
	ASE_AWK_EXTIO_COPROC_RDWR    = 2,
	*/

	ASE_AWK_EXTIO_FILE_READ      = 0,
	ASE_AWK_EXTIO_FILE_WRITE     = 1,
	ASE_AWK_EXTIO_FILE_APPEND    = 2,

	ASE_AWK_EXTIO_CONSOLE_READ   = 0,
	ASE_AWK_EXTIO_CONSOLE_WRITE  = 1
};

enum ase_awk_global_id_t
{
	/* this table should match gtab in parse.c.
	 * in addition, ase_awk_setglobal also counts 
	 * on the order of these values */

	ASE_AWK_GLOBAL_ARGC,
	ASE_AWK_GLOBAL_ARGV,
	ASE_AWK_GLOBAL_CONVFMT,
	ASE_AWK_GLOBAL_FILENAME,
	ASE_AWK_GLOBAL_FNR,
	ASE_AWK_GLOBAL_FS,
	ASE_AWK_GLOBAL_IGNORECASE,
	ASE_AWK_GLOBAL_NF,
	ASE_AWK_GLOBAL_NR,
	ASE_AWK_GLOBAL_OFILENAME,
	ASE_AWK_GLOBAL_OFMT,
	ASE_AWK_GLOBAL_OFS,
	ASE_AWK_GLOBAL_ORS,
	ASE_AWK_GLOBAL_RLENGTH,
	ASE_AWK_GLOBAL_RS,
	ASE_AWK_GLOBAL_RSTART,
	ASE_AWK_GLOBAL_SUBSEP,

	/* these are not not the actual IDs and are used internally only 
	 * Make sure you update these values properly if you add more 
	 * ID definitions, however */
	ASE_AWK_MIN_GLOBAL_ID = ASE_AWK_GLOBAL_ARGC,
	ASE_AWK_MAX_GLOBAL_ID = ASE_AWK_GLOBAL_SUBSEP
};

#ifdef __cplusplus
extern "C" {
#endif

ase_awk_t* ase_awk_open (const ase_awk_prmfns_t* prmfns, void* custom_data);
int ase_awk_close (ase_awk_t* awk);
int ase_awk_clear (ase_awk_t* awk);

ase_mmgr_t* ase_awk_getmmgr (ase_awk_t* awk);
void* ase_awk_getcustomdata (ase_awk_t* awk);

const ase_char_t* ase_awk_geterrstr (ase_awk_t* awk, int num);
int ase_awk_seterrstr (ase_awk_t* awk, int num, const ase_char_t* str);

int ase_awk_geterrnum (ase_awk_t* awk);
ase_size_t ase_awk_geterrlin (ase_awk_t* awk);
const ase_char_t* ase_awk_geterrmsg (ase_awk_t* awk);

void ase_awk_seterrnum (ase_awk_t* awk, int errnum);
void ase_awk_seterrmsg (ase_awk_t* awk, 
	int errnum, ase_size_t errlin, const ase_char_t* errmsg);

void ase_awk_geterror (
	ase_awk_t* awk, int* errnum, 
	ase_size_t* errlin, const ase_char_t** errmsg);

void ase_awk_seterror (
	ase_awk_t* awk, int errnum, ase_size_t errlin, 
	const ase_cstr_t* errarg, ase_size_t argcnt);

int ase_awk_getoption (ase_awk_t* awk);
void ase_awk_setoption (ase_awk_t* awk, int opt);

ase_size_t ase_awk_getmaxdepth (ase_awk_t* awk, int type);
void ase_awk_setmaxdepth (ase_awk_t* awk, int types, ase_size_t depth);

int ase_awk_getword (ase_awk_t* awk, 
	const ase_char_t* okw, ase_size_t olen,
	const ase_char_t** nkw, ase_size_t* nlen);
/**
 * Enables replacement of a name of a keyword, intrinsic global variables, 
 * and intrinsic functions.
 *
 * If nkw is ASE_NULL or nlen is zero and okw is ASE_NULL or olen is zero,
 * it unsets all word replacements. If nkw is ASE_NULL or nlen is zero,
 * it unsets the replacement for okw and olen. If all of them are valid,
 * it sets the word replace for okw and olen to nkw and nlen.
 *
 * @return 
 * 	On success, 0 is returned.
 * 	On failure, -1 is returned.
 */
int ase_awk_setword (ase_awk_t* awk, 
	const ase_char_t* okw, ase_size_t olen,
	const ase_char_t* nkw, ase_size_t nlen);

/**
 * Sets the customized regular processing routine.
 *
 * @return 
 * 	On success, 0 is returned.
 * 	On failure, -1 is returned.
 */
int ase_awk_setrexfns (ase_awk_t* awk, ase_awk_rexfns_t* rexfns);

/**
 * Adds an intrinsic global variable.
 *
 * @return 
 * 	On success, the ID of the global variable added is returned.
 * 	On failure, -1 is returned.
 */
int ase_awk_addglobal (ase_awk_t* awk, const ase_char_t* name, ase_size_t len);

/**
 * Deletes a instrinsic global variable. 
 *
 * @return 
 * 	On success, 0 is returned.
 * 	On failure, -1 is returned.
 */
int ase_awk_delglobal (ase_awk_t* awk, const ase_char_t* name, ase_size_t len);

/**
 * Parses the source code
 *
 * @return 
 * 	On success, 0 is returned.
 * 	On failure, -1 is returned.
 */
int ase_awk_parse (ase_awk_t* awk, ase_awk_srcios_t* srcios);

/**
 * Executes a parsed program.
 *
 * ase_awk_run returns 0 on success and -1 on failure, generally speaking.
 *  A runtime context is required for it to start running the program.
 *  Once the runtime context is created, the program starts to run.
 *  The context creation failure is reported by the return value -1 of
 *  this function. however, the runtime error after the context creation
 *  is reported differently depending on the use of the callback.
 *  When no callback is specified (i.e. runcbs is ASE_NULL), ase_awk_run
 *  returns -1 on an error and awk->errnum is set accordingly.
 *  However, if a callback is specified (i.e. runcbs is not ASE_NULL),
 *  ase_awk_run returns 0 on both success and failure. Instead, the 
 *  on_end handler of the callback is triggered with the relevant 
 *  error number. The third parameter to on_end denotes this error number.
 */
int ase_awk_run (
	ase_awk_t* awk, const ase_char_t* main,
	ase_awk_runios_t* runios, ase_awk_runcbs_t* runcbs, 
	ase_awk_runarg_t* runarg, void* custom_data);

void ase_awk_stop (ase_awk_run_t* run);
void ase_awk_stopall (ase_awk_t* awk);

ase_bool_t ase_awk_isstop (ase_awk_run_t* run);


/** 
 * Gets the number of arguments passed to ase_awk_run 
 */
ase_size_t ase_awk_getnargs (ase_awk_run_t* run);

/** 
 * Gets an argument passed to ase_awk_run
 */
ase_awk_val_t* ase_awk_getarg (ase_awk_run_t* run, ase_size_t idx);

/**
 * Gets the value of a global variable.
 *
 * @param run A run-time context
 * @param id The ID to a global variable. 
 * 	This value correspondsto the predefined global variable IDs or 
 * 	the value returned by ase_awk_addglobal.
 * @return
 * 	The pointer to a value is returned. This function never fails
 * 	so long as id is valid. Otherwise, you may fall into trouble.
 */
ase_awk_val_t* ase_awk_getglobal (ase_awk_run_t* run, int id);
int ase_awk_setglobal (ase_awk_run_t* run, int id, ase_awk_val_t* val);

/**
 * Sets the return value of a function from within a function handler.
 *
 * @param run A run-time context
 * @param val A pointer to the value to set.
 * 	ase_awk_refupval and ase_awk_refdownval are not needed because
 * 	ase_awk_setretval never fails and it updates the reference count
 * 	of the value properly.
 */
void ase_awk_setretval (ase_awk_run_t* run, ase_awk_val_t* val);

int ase_awk_setfilename (
	ase_awk_run_t* run, const ase_char_t* name, ase_size_t len);
int ase_awk_setofilename (
	ase_awk_run_t* run, const ase_char_t* name, ase_size_t len);

ase_awk_t* ase_awk_getrunawk (ase_awk_run_t* awk);
void* ase_awk_getruncustomdata (ase_awk_run_t* awk);
ase_map_t* ase_awk_getrunnamedvarmap (ase_awk_run_t* awk);

/* functions to manipulate the run-time error */
int ase_awk_getrunerrnum (ase_awk_run_t* run);
ase_size_t ase_awk_getrunerrlin (ase_awk_run_t* run);
const ase_char_t* ase_awk_getrunerrmsg (ase_awk_run_t* run);
void ase_awk_setrunerrnum (ase_awk_run_t* run, int errnum);
void ase_awk_setrunerrmsg (ase_awk_run_t* run, 
	int errnum, ase_size_t errlin, const ase_char_t* errmsg);

void ase_awk_getrunerror (
	ase_awk_run_t* run, int* errnum, 
	ase_size_t* errlin, const ase_char_t** errmsg);

void ase_awk_setrunerror (
	ase_awk_run_t* run, int errnum, ase_size_t errlin, 
	const ase_cstr_t* errarg, ase_size_t argcnt);

/* functions to manipulate intrinsic functions */
void* ase_awk_addfunc (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t name_len, 
	int when_valid, ase_size_t min_args, ase_size_t max_args, 
	const ase_char_t* arg_spec, 
	int (*handler)(ase_awk_run_t*,const ase_char_t*,ase_size_t));

int ase_awk_delfunc (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t name_len);

void ase_awk_clrbfn (ase_awk_t* awk);

/* record and field functions */
int ase_awk_clrrec (ase_awk_run_t* run, ase_bool_t skip_inrec_line);
int ase_awk_setrec (ase_awk_run_t* run, ase_size_t idx, const ase_char_t* str, ase_size_t len);

/* utility functions exported by awk.h */
void* ase_awk_malloc (ase_awk_t* awk, ase_size_t size);
void ase_awk_free (ase_awk_t* awk, void* ptr);

ase_char_t* ase_awk_strxdup (
	ase_awk_t* awk, const ase_char_t* ptr, ase_size_t len);

ase_long_t ase_awk_strxtolong (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len,
	int base, const ase_char_t** endptr);
ase_real_t ase_awk_strxtoreal (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len, 
	const ase_char_t** endptr);

ase_size_t ase_awk_longtostr (
	ase_long_t value, int radix, const ase_char_t* prefix,
	ase_char_t* buf, ase_size_t size);

#ifdef __cplusplus
}
#endif

#endif
