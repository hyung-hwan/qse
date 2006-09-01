/* 
 * $Id: awk.h,v 1.105 2006-09-01 03:44:16 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_awk_t xp_awk_t;
typedef struct xp_awk_val_t xp_awk_val_t;
typedef struct xp_awk_extio_t xp_awk_extio_t;

typedef struct xp_awk_syscas_t xp_awk_syscas_t;
typedef struct xp_awk_srcios_t xp_awk_srcios_t;
typedef struct xp_awk_runios_t xp_awk_runios_t;
typedef struct xp_awk_runcbs_t xp_awk_runcbs_t;

typedef void (*xp_awk_lk_t) (xp_awk_t* awk, void* arg);
typedef xp_ssize_t (*xp_awk_io_t) (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

struct xp_awk_extio_t 
{
	int type;         /* [IN] console, file, coproc, pipe */
	int mode;         /* [IN] read, write, etc */
	xp_char_t* name;  /* [IN] */
	void* handle;     /* [OUT] */

	/* input buffer */
	struct
	{
		xp_char_t buf[2048];
		xp_size_t pos;
		xp_size_t len;
		xp_bool_t eof;
	} in;

	xp_awk_extio_t* next;
};

/*
struct xp_awk_thrlks_t
{
	xp_awk_lk_t lock;
	xp_awk_lk_t unlock;
	void* custom_data;
};
*/
struct xp_awk_syscas_t
{
	/* memory */
	void* (*malloc) (xp_size_t n, void* custom_data);
	void* (*realloc) (void* ptr, xp_size_t n, void* custom_data);
	void  (*free) (void* ptr, void* custom_data);

	/* thread lock */
	xp_awk_lk_t lock;
	xp_awk_lk_t unlock;

	void* custom_data;
};

struct xp_awk_srcios_t
{
	xp_awk_io_t in;
	xp_awk_io_t out;
	void* custom_data;
};

struct xp_awk_runios_t
{
	xp_awk_io_t pipe;
	xp_awk_io_t coproc;
	xp_awk_io_t file;
	xp_awk_io_t console;
};

struct xp_awk_runcbs_t
{
	void (*on_start) (xp_awk_t* awk, void* handle, void* arg);
	void (*on_end) (xp_awk_t* awk, void* handle, int errnum, void* arg);
	void* custom_data;
};


/* io function commands */
enum 
{
	XP_AWK_IO_OPEN   = 0,
	XP_AWK_IO_CLOSE  = 1,
	XP_AWK_IO_READ   = 2,
	XP_AWK_IO_WRITE  = 3,
	XP_AWK_IO_FLUSH  = 4,
	XP_AWK_IO_NEXT   = 5  
};

enum
{
	XP_AWK_IO_PIPE_READ      = 0,
	XP_AWK_IO_PIPE_WRITE     = 1,

	XP_AWK_IO_FILE_READ      = 0,
	XP_AWK_IO_FILE_WRITE     = 1,
	XP_AWK_IO_FILE_APPEND    = 2,

	XP_AWK_IO_CONSOLE_READ   = 0,
	XP_AWK_IO_CONSOLE_WRITE  = 1
};

/* various options */
enum
{ 
	/* allow undeclared variables */
	XP_AWK_IMPLICIT    = (1 << 0),

	/* variable requires explicit declaration */
	XP_AWK_EXPLICIT    = (1 << 1), 

	/* a function name should not coincide to be a variable name */
	XP_AWK_UNIQUE      = (1 << 2),

	/* allow variable shading */
	XP_AWK_SHADING     = (1 << 3), 

	/* support shift operators */
	XP_AWK_SHIFT       = (1 << 4), 

	/* support comments by a hash sign */
	XP_AWK_HASHSIGN    = (1 << 5), 

	/* support comments by double slashes */
	XP_AWK_DBLSLASHES  = (1 << 6), 

	/* support string concatenation in tokenization.
	 * this option can change the behavior of a certain construct.
	 * getline < "abc" ".def" is treated as if it is getline < "abc.def" 
	 * when this option is on. If this option is off, the same expression
	 * is treated as if it is (getline < "abc") ".def". */
	XP_AWK_STRCONCAT   = (1 << 7), 

	/* support getline and print */
	XP_AWK_EXTIO       = (1 << 8), 

	/* support blockless patterns */
	XP_AWK_BLOCKLESS   = (1 << 9), 

 	/* execution starts from main */
	XP_AWK_RUNMAIN     = (1 << 10),

	/* use 1 as the start index for string operations */
	XP_AWK_STRINDEXONE = (1 << 11)
};

/* error code */
enum
{
	XP_AWK_ENOERR,         /* no error */
	XP_AWK_ENOMEM,         /* out of memory */
	XP_AWK_EINVAL,         /* invalid parameter */
	XP_AWK_ERUNTIME,       /* run-time error */
	XP_AWK_ERUNNING,       /* there are running instances */
	XP_AWK_ETOOMANYRUNS,   /* too many running instances */
	XP_AWK_ERECURSION,     /* recursion too deep */

	XP_AWK_ESRCINOPEN,
	XP_AWK_ESRCINCLOSE,
	XP_AWK_ESRCINREAD, 

	XP_AWK_ESRCOUTOPEN,
	XP_AWK_ESRCOUTCLOSE,
	XP_AWK_ESRCOUTWRITE,

	XP_AWK_ECONINOPEN,
	XP_AWK_ECONINCLOSE,
	XP_AWK_ECONINNEXT,
	XP_AWK_ECONINDATA, 

	XP_AWK_ECONOUTOPEN,
	XP_AWK_ECONOUTCLOSE,
	XP_AWK_ECONOUTNEXT,
	XP_AWK_ECONOUTDATA,

	XP_AWK_ELXCHR,         /* lexer came accross an wrong character */
	XP_AWK_ELXUNG,         /* lexer failed to unget a character */

	XP_AWK_EENDSRC,        /* unexpected end of source */
	XP_AWK_EENDCOMMENT,    /* unexpected end of a comment */
	XP_AWK_EENDSTR,        /* unexpected end of a string */
	XP_AWK_EENDREX,        /* unexpected end of a regular expression */
	XP_AWK_ELBRACE,        /* left brace expected */
	XP_AWK_ELPAREN,        /* left parenthesis expected */
	XP_AWK_ERPAREN,        /* right parenthesis expected */
	XP_AWK_ERBRACK,        /* right bracket expected */
	XP_AWK_ECOMMA,         /* comma expected */
	XP_AWK_ESEMICOLON,     /* semicolon expected */
	XP_AWK_ECOLON,         /* colon expected */
	XP_AWK_EIN,            /* keyword 'in' is expected */
	XP_AWK_ENOTVAR,        /* not a variable name after 'in' */
	XP_AWK_EEXPRESSION,    /* expression expected */

	XP_AWK_EWHILE,         /* keyword 'while' is expected */
	XP_AWK_EASSIGNMENT,    /* assignment statement expected */
	XP_AWK_EIDENT,         /* identifier expected */
	XP_AWK_EBEGINBLOCK,    /* BEGIN requires an action block */
	XP_AWK_EENDBLOCK,      /* END requires an action block */
	XP_AWK_EDUPBEGIN,      /* duplicate BEGIN */
	XP_AWK_EDUPEND,        /* duplicate END */
	XP_AWK_EDUPFUNC,       /* duplicate function name */
	XP_AWK_EDUPPARAM,      /* duplicate parameter name */
	XP_AWK_EDUPVAR,        /* duplicate variable name */
	XP_AWK_EDUPNAME,       /* duplicate name - function, variable, etc */
	XP_AWK_EUNDEF,         /* undefined identifier */
	XP_AWK_ELVALUE,        /* l-value required */
	XP_AWK_ETOOFEWARGS,    /* too few arguments */
	XP_AWK_ETOOMANYARGS,   /* too many arguments */
	XP_AWK_ETOOMANYGLOBALS, /* too many global variables */
	XP_AWK_ETOOMANYLOCALS, /* too many local variables */
	XP_AWK_ETOOMANYPARAMS, /* too many parameters */
	XP_AWK_EBREAK,         /* break outside a loop */
	XP_AWK_ECONTINUE,      /* continue outside a loop */
	XP_AWK_ENEXT,          /* next illegal in BEGIN or END block */
	XP_AWK_ENEXTFILE,      /* nextfile illegal in BEGIN or END block */
	XP_AWK_EGETLINE,       /* getline expected */

	/* run time error */
	XP_AWK_EDIVBYZERO,        /* divide by zero */
	XP_AWK_EOPERAND,          /* invalid operand */
	XP_AWK_EPOSIDX,           /* wrong position index */
	XP_AWK_ENOSUCHFUNC,       /* no such function */
	XP_AWK_ENOTASSIGNABLE,    /* value not assignable */
	XP_AWK_ENOTINDEXABLE,     /* not indexable variable */
	XP_AWK_ENOTDELETABLE,     /* not deletable variable */
	XP_AWK_ENOTREFERENCEABLE, /* not referenceable value */
	XP_AWK_EIDXVALASSMAP,     /* indexed value cannot be assigned a map */
	XP_AWK_EMAPTOSCALAR,      /* cannot change a map to a scalar value */
	XP_AWK_ESCALARTOMAP,      /* cannot change a scalar value to a map */
	XP_AWK_EVALTYPE,          /* wrong value type */
	XP_AWK_EPIPE,             /* pipe operation error */
	XP_AWK_ENEXTCALL,         /* next called from BEGIN or END */
	XP_AWK_ENEXTFILECALL,     /* nextfile called from BEGIN or END */
	XP_AWK_EIOIMPL,           /* wrong implementation of user io handler */
	XP_AWK_ENOSUCHIO,         /* no such io name found */
	XP_AWK_EIOHANDLER,        /* io handler has returned an error */
	XP_AWK_EINTERNAL,         /* internal error */

	/* regular expression error */
	XP_AWK_EREXRPAREN,       /* a right parenthesis is expected */
	XP_AWK_EREXRBRACKET,     /* a right bracket is expected */
	XP_AWK_EREXRBRACE,       /* a right brace is expected */
	XP_AWK_EREXCOLON,        /* a colon is expected */
	XP_AWK_EREXCRANGE,       /* invalid character range */
	XP_AWK_EREXCCLASS,       /* invalid character class */
	XP_AWK_EREXBRANGE,       /* invalid boundary range */
	XP_AWK_EREXEND,          /* unexpected end of the pattern */
	XP_AWK_EREXGARBAGE       /* garbage after the pattern */
};

/* extio types */
enum
{
	/* extio types available */
	XP_AWK_EXTIO_PIPE,
	XP_AWK_EXTIO_COPROC,
	XP_AWK_EXTIO_FILE,
	XP_AWK_EXTIO_CONSOLE,

	/* reserved for internal use only */
	XP_AWK_EXTIO_NUM
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_t* xp_awk_open (xp_awk_syscas_t* syscas);
int xp_awk_close (xp_awk_t* awk);
int xp_awk_clear (xp_awk_t* awk);

int xp_awk_geterrnum (xp_awk_t* awk);
xp_size_t xp_awk_getsrcline (xp_awk_t* awk);

int xp_awk_getopt (xp_awk_t* awk);
void xp_awk_setopt (xp_awk_t* awk, int opt);

int xp_awk_parse (xp_awk_t* awk, xp_awk_srcios_t* srcios);

/*
 * xp_awk_run return 0 on success and -1 on failure, generally speaking.
 *  A runtime context is required for it to start running the program.
 *  Once the runtime context is created, the program starts to run.
 *  The context creation failure is reported by the return value -1 of
 *  this function. however, the runtime error after the context creation
 *  is reported differently depending on the use of the callback.
 *  When no callback is specified (i.e. runcbs is XP_NULL), xp_awk_run
 *  returns -1 on an error and awk->errnum is set accordingly.
 *  However, if a callback is specified (i.e. runcbs is not XP_NULL),
 *  xp_awk_run returns 0 on both success and failure. Instead, the 
 *  on_end handler of the callback is triggered with the relevant 
 *  error number. The third parameter to on_end denotes this error number.
 */
int xp_awk_run (xp_awk_t* awk, 
	xp_awk_runios_t* runios, xp_awk_runcbs_t* runcbs);

int xp_awk_stop (xp_awk_t* awk, void* run);
void xp_awk_stopall (xp_awk_t* awk);
int xp_awk_getrunerrnum (xp_awk_t* awk, void* run, int* errnum);

/* functions to access internal stack structure */
xp_size_t xp_awk_getnargs (void* run);
xp_awk_val_t* xp_awk_getarg (void* run, xp_size_t idx);
xp_awk_val_t* xp_awk_getglobal (void* run, xp_size_t idx);
int xp_awk_setglobal (void* run, xp_size_t idx, xp_awk_val_t* val);
void xp_awk_seterrnum (void* run, int errnum);
void xp_awk_setretval (void* run, xp_awk_val_t* val);

/* utility functions exported by awk.h */
xp_long_t xp_awk_strtolong (
	const xp_char_t* str, int base, const xp_char_t** endptr);
xp_real_t xp_awk_strtoreal (const xp_char_t* str);

/* string functions exported by awk.h */
xp_char_t* xp_awk_strdup (
	xp_awk_t* awk, const xp_char_t* str);
xp_char_t* xp_awk_strxdup (
	xp_awk_t* awk, const xp_char_t* str, xp_size_t len);
xp_char_t* xp_awk_strxdup2 (
	xp_awk_t* awk,
	const xp_char_t* str1, xp_size_t len1,
	const xp_char_t* str2, xp_size_t len2);

/* utility functions to convert an error number ot a string */
const xp_char_t* xp_awk_geterrstr (int errnum);

#ifdef __cplusplus
}
#endif

#endif
