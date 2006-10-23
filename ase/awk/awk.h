/* 
 * $Id: awk.h,v 1.132 2006-10-23 14:44:42 bacon Exp $
 */

#ifndef _SSE_AWK_AWK_H_
#define _SSE_AWK_AWK_H_

#include <sse/types.h>
#include <sse/macros.h>

typedef struct sse_awk_t sse_awk_t;
typedef struct sse_awk_run_t sse_awk_run_t;
typedef struct sse_awk_val_t sse_awk_val_t;
typedef struct sse_awk_extio_t sse_awk_extio_t;

typedef struct sse_awk_syscas_t sse_awk_syscas_t;
typedef struct sse_awk_srcios_t sse_awk_srcios_t;
typedef struct sse_awk_runios_t sse_awk_runios_t;
typedef struct sse_awk_runcbs_t sse_awk_runcbs_t;
typedef struct sse_awk_runarg_t sse_awk_runarg_t;

typedef void (*sse_awk_lk_t) (sse_awk_t* awk, void* arg);
typedef sse_ssize_t (*sse_awk_io_t) (
	int cmd, void* arg, sse_char_t* data, sse_size_t count);

struct sse_awk_extio_t 
{
	sse_awk_run_t* run; /* [IN] */
	int type;          /* [IN] console, file, coproc, pipe */
	int mode;          /* [IN] read, write, etc */
	sse_char_t* name;   /* [IN] */
	void* custom_data; /* [IN] */

	void* handle;      /* [OUT] */

	/* input buffer */
	struct
	{
		sse_char_t buf[2048];
		sse_size_t pos;
		sse_size_t len;
		sse_bool_t eof;
	} in;

	sse_awk_extio_t* next;
};

struct sse_awk_syscas_t
{
	/* memory */
	void* (*malloc) (sse_size_t n, void* custom_data);
	void* (*realloc) (void* ptr, sse_size_t n, void* custom_data);
	void  (*free) (void* ptr, void* custom_data);

	/* thread lock */
	sse_awk_lk_t lock;
	sse_awk_lk_t unlock;

	/* character class */
	sse_bool_t (*is_upper)  (sse_cint_t c);
	sse_bool_t (*is_lower)  (sse_cint_t c);
	sse_bool_t (*is_alpha)  (sse_cint_t c);
	sse_bool_t (*is_digit)  (sse_cint_t c);
	sse_bool_t (*is_xdigit) (sse_cint_t c);
	sse_bool_t (*is_alnum)  (sse_cint_t c);
	sse_bool_t (*is_space)  (sse_cint_t c);
	sse_bool_t (*is_print)  (sse_cint_t c);
	sse_bool_t (*is_graph)  (sse_cint_t c);
	sse_bool_t (*is_cntrl)  (sse_cint_t c);
	sse_bool_t (*is_punct)  (sse_cint_t c);
	sse_cint_t (*to_upper)  (sse_cint_t c);
	sse_cint_t (*to_lower)  (sse_cint_t c);

	/* utilities */
	void* (*memcpy)  (void* dst, const void* src, sse_size_t n);
	void* (*memset)  (void* dst, int val, sse_size_t n);

	int (*sprintf) (sse_char_t* buf, sse_size_t size, sse_char_t* fmt, ...);
	int (*dprintf) (sse_char_t* fmt, ...);
	void (*abort) (void);

	void* custom_data;
};

struct sse_awk_srcios_t
{
	sse_awk_io_t in;
	sse_awk_io_t out;
	void* custom_data;
};

struct sse_awk_runios_t
{
	sse_awk_io_t pipe;
	sse_awk_io_t coproc;
	sse_awk_io_t file;
	sse_awk_io_t console;
	void* custom_data;
};

struct sse_awk_runcbs_t
{
	void (*on_start) (sse_awk_t* awk, void* handle, void* arg);
	void (*on_end) (sse_awk_t* awk, void* handle, int errnum, void* arg);
	void* custom_data;
};

struct sse_awk_runarg_t
{
	const sse_char_t* ptr;
	sse_size_t len;
};

/* io function commands */
enum 
{
	SSE_AWK_IO_OPEN   = 0,
	SSE_AWK_IO_CLOSE  = 1,
	SSE_AWK_IO_READ   = 2,
	SSE_AWK_IO_WRITE  = 3,
	SSE_AWK_IO_FLUSH  = 4,
	SSE_AWK_IO_NEXT   = 5  
};

enum
{
	SSE_AWK_IO_PIPE_READ      = 0,
	SSE_AWK_IO_PIPE_WRITE     = 1,

	SSE_AWK_IO_FILE_READ      = 0,
	SSE_AWK_IO_FILE_WRITE     = 1,
	SSE_AWK_IO_FILE_APPEND    = 2,

	SSE_AWK_IO_CONSOLE_READ   = 0,
	SSE_AWK_IO_CONSOLE_WRITE  = 1
};

/* various options */
enum 
{ 
	/* allow undeclared variables */
	SSE_AWK_IMPLICIT    = (1 << 0),

	/* variable requires explicit declaration */
	SSE_AWK_EXPLICIT    = (1 << 1), 

	/* a function name should not coincide to be a variable name */
	SSE_AWK_UNIQUE      = (1 << 2),

	/* allow variable shading */
	SSE_AWK_SHADING     = (1 << 3), 

	/* support shift operators */
	SSE_AWK_SHIFT       = (1 << 4), 

	/* support comments by a hash sign */
	SSE_AWK_HASHSIGN    = (1 << 5), 

	/* support comments by double slashes */
	SSE_AWK_DBLSLASHES  = (1 << 6), 

	/* support string concatenation in tokenization.
	 * this option can change the behavior of a certain construct.
	 * getline < "abc" ".def" is treated as if it is getline < "abc.def" 
	 * when this option is on. If this option is off, the same expression
	 * is treated as if it is (getline < "abc") ".def". */
	SSE_AWK_STRCONCAT   = (1 << 7), 

	/* support getline and print */
	SSE_AWK_EXTIO       = (1 << 8), 

	/* support blockless patterns */
	SSE_AWK_BLOCKLESS   = (1 << 9), 

 	/* execution starts from main */
	SSE_AWK_RUNMAIN     = (1 << 10),

	/* use 1 as the start index for string operations */
	SSE_AWK_STRINDEXONE = (1 << 11),

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
	SSE_AWK_STRIPSPACES = (1 << 12),

	/* a newline terminates a statement */
	SSE_AWK_NEWLINE = (1 << 13)
};

/* error code */
enum 
{
	SSE_AWK_ENOERR,         /* no error */
	SSE_AWK_ENOMEM,         /* out of memory */
	SSE_AWK_EINVAL,         /* invalid parameter */
	SSE_AWK_ERUNTIME,       /* run-time error */
	SSE_AWK_ERUNNING,       /* there are running instances */
	SSE_AWK_ETOOMANYRUNS,   /* too many running instances */
	SSE_AWK_ERECURSION,     /* recursion too deep */

	SSE_AWK_ESRCINOPEN,
	SSE_AWK_ESRCINCLOSE,
	SSE_AWK_ESRCINREAD, 

	SSE_AWK_ESRCOUTOPEN,
	SSE_AWK_ESRCOUTCLOSE,
	SSE_AWK_ESRCOUTWRITE,

	SSE_AWK_ECONINOPEN,
	SSE_AWK_ECONINCLOSE,
	SSE_AWK_ECONINNEXT,
	SSE_AWK_ECONINDATA, 

	SSE_AWK_ECONOUTOPEN,
	SSE_AWK_ECONOUTCLOSE,
	SSE_AWK_ECONOUTNEXT,
	SSE_AWK_ECONOUTDATA,

	SSE_AWK_ELXCHR,         /* lexer came accross an wrong character */
	SSE_AWK_ELXUNG,         /* lexer failed to unget a character */

	SSE_AWK_EENDSRC,        /* unexpected end of source */
	SSE_AWK_EENDCOMMENT,    /* unexpected end of a comment */
	SSE_AWK_EENDSTR,        /* unexpected end of a string */
	SSE_AWK_EENDREX,        /* unexpected end of a regular expression */
	SSE_AWK_ELBRACE,        /* left brace expected */
	SSE_AWK_ELPAREN,        /* left parenthesis expected */
	SSE_AWK_ERPAREN,        /* right parenthesis expected */
	SSE_AWK_ERBRACK,        /* right bracket expected */
	SSE_AWK_ECOMMA,         /* comma expected */
	SSE_AWK_ESEMICOLON,     /* semicolon expected */
	SSE_AWK_ECOLON,         /* colon expected */
	SSE_AWK_EIN,            /* keyword 'in' is expected */
	SSE_AWK_ENOTVAR,        /* not a variable name after 'in' */
	SSE_AWK_EEXPRESSION,    /* expression expected */

	SSE_AWK_EWHILE,         /* keyword 'while' is expected */
	SSE_AWK_EASSIGNMENT,    /* assignment statement expected */
	SSE_AWK_EIDENT,         /* identifier expected */
	SSE_AWK_EBEGINBLOCK,    /* BEGIN requires an action block */
	SSE_AWK_EENDBLOCK,      /* END requires an action block */
	SSE_AWK_EDUPBEGIN,      /* duplicate BEGIN */
	SSE_AWK_EDUPEND,        /* duplicate END */
	SSE_AWK_EDUPFUNC,       /* duplicate function name */
	SSE_AWK_EDUPPARAM,      /* duplicate parameter name */
	SSE_AWK_EDUPVAR,        /* duplicate variable name */
	SSE_AWK_EDUPNAME,       /* duplicate name - function, variable, etc */
	SSE_AWK_EUNDEF,         /* undefined identifier */
	SSE_AWK_ELVALUE,        /* l-value required */
	SSE_AWK_ETOOFEWARGS,    /* too few arguments */
	SSE_AWK_ETOOMANYARGS,   /* too many arguments */
	SSE_AWK_ETOOMANYGLOBALS, /* too many global variables */
	SSE_AWK_ETOOMANYLOCALS, /* too many local variables */
	SSE_AWK_ETOOMANYPARAMS, /* too many parameters */
	SSE_AWK_EBREAK,         /* break outside a loop */
	SSE_AWK_ECONTINUE,      /* continue outside a loop */
	SSE_AWK_ENEXT,          /* next illegal in BEGIN or END block */
	SSE_AWK_ENEXTFILE,      /* nextfile illegal in BEGIN or END block */
	SSE_AWK_EGETLINE,       /* getline expected */

	/* run time error */
	SSE_AWK_EDIVBYZERO,        /* divide by zero */
	SSE_AWK_EOPERAND,          /* invalid operand */
	SSE_AWK_EPOSIDX,           /* wrong position index */
	SSE_AWK_ENOSUCHFUNC,       /* no such function */
	SSE_AWK_ENOTASSIGNABLE,    /* value not assignable */
	SSE_AWK_ENOTINDEXABLE,     /* not indexable variable */
	SSE_AWK_ENOTDELETABLE,     /* not deletable variable */
	SSE_AWK_ENOTREFERENCEABLE, /* not referenceable value */
	SSE_AWK_EIDXVALASSMAP,     /* indexed value cannot be assigned a map */
	SSE_AWK_EPOSVALASSMAP,     /* a positional cannot be assigned a map */
	SSE_AWK_EMAPTOSCALAR,      /* cannot change a map to a scalar value */
	SSE_AWK_ESCALARTOMAP,      /* cannot change a scalar value to a map */
	SSE_AWK_EMAPNOTALLOWED,    /* a map is not allowed */
	SSE_AWK_EVALTYPE,          /* wrong value type */
	SSE_AWK_EPIPE,             /* pipe operation error */
	SSE_AWK_ENEXTCALL,         /* next called from BEGIN or END */
	SSE_AWK_ENEXTFILECALL,     /* nextfile called from BEGIN or END */
	SSE_AWK_EIOIMPL,           /* wrong implementation of user io handler */
	SSE_AWK_ENOSUCHIO,         /* no such io name found */
	SSE_AWK_EIOHANDLER,        /* io handler has returned an error */
	SSE_AWK_EINTERNAL,         /* internal error */

	/* regular expression error */
	SSE_AWK_EREXRPAREN,       /* a right parenthesis is expected */
	SSE_AWK_EREXRBRACKET,     /* a right bracket is expected */
	SSE_AWK_EREXRBRACE,       /* a right brace is expected */
	SSE_AWK_EREXCOLON,        /* a colon is expected */
	SSE_AWK_EREXCRANGE,       /* invalid character range */
	SSE_AWK_EREXCCLASS,       /* invalid character class */
	SSE_AWK_EREXBRANGE,       /* invalid boundary range */
	SSE_AWK_EREXEND,          /* unexpected end of the pattern */
	SSE_AWK_EREXGARBAGE       /* garbage after the pattern */
};

/* extio types */
enum
{
	/* extio types available */
	SSE_AWK_EXTIO_PIPE,
	SSE_AWK_EXTIO_COPROC,
	SSE_AWK_EXTIO_FILE,
	SSE_AWK_EXTIO_CONSOLE,

	/* reserved for internal use only */
	SSE_AWK_EXTIO_NUM
};

#ifdef __cplusplus
extern "C" {
#endif

sse_awk_t* sse_awk_open (const sse_awk_syscas_t* syscas);
int sse_awk_close (sse_awk_t* awk);
int sse_awk_clear (sse_awk_t* awk);

int sse_awk_geterrnum (sse_awk_t* awk);
sse_size_t sse_awk_getsrcline (sse_awk_t* awk);

int sse_awk_getopt (sse_awk_t* awk);
void sse_awk_setopt (sse_awk_t* awk, int opt);

int sse_awk_parse (sse_awk_t* awk, sse_awk_srcios_t* srcios);

/*
 * sse_awk_run return 0 on success and -1 on failure, generally speaking.
 *  A runtime context is required for it to start running the program.
 *  Once the runtime context is created, the program starts to run.
 *  The context creation failure is reported by the return value -1 of
 *  this function. however, the runtime error after the context creation
 *  is reported differently depending on the use of the callback.
 *  When no callback is specified (i.e. runcbs is SSE_NULL), sse_awk_run
 *  returns -1 on an error and awk->errnum is set accordingly.
 *  However, if a callback is specified (i.e. runcbs is not SSE_NULL),
 *  sse_awk_run returns 0 on both success and failure. Instead, the 
 *  on_end handler of the callback is triggered with the relevant 
 *  error number. The third parameter to on_end denotes this error number.
 */
int sse_awk_run (sse_awk_t* awk, 
	sse_awk_runios_t* runios, 
	sse_awk_runcbs_t* runcbs, 
	sse_awk_runarg_t* runarg);

int sse_awk_stop (sse_awk_t* awk, sse_awk_run_t* run);
void sse_awk_stopall (sse_awk_t* awk);

/* functions to access internal stack structure */
sse_size_t sse_awk_getnargs (sse_awk_run_t* run);
sse_awk_val_t* sse_awk_getarg (sse_awk_run_t* run, sse_size_t idx);
sse_awk_val_t* sse_awk_getglobal (sse_awk_run_t* run, sse_size_t idx);
int sse_awk_setglobal (sse_awk_run_t* run, sse_size_t idx, sse_awk_val_t* val);
void sse_awk_setretval (sse_awk_run_t* run, sse_awk_val_t* val);

int sse_awk_setconsolename (
	sse_awk_run_t* run, const sse_char_t* name, sse_size_t len);

int sse_awk_getrunerrnum (sse_awk_run_t* run);
void sse_awk_setrunerrnum (sse_awk_run_t* run, int errnum);

/* record and field functions */
int sse_awk_clrrec (sse_awk_run_t* run, sse_bool_t skip_inrec_line);
int sse_awk_setrec (sse_awk_run_t* run, sse_size_t idx, const sse_char_t* str, sse_size_t len);

/* utility functions exported by awk.h */
sse_long_t sse_awk_strxtolong (
	sse_awk_t* awk, const sse_char_t* str, sse_size_t len,
	int base, const sse_char_t** endptr);
sse_real_t sse_awk_strxtoreal (
	sse_awk_t* awk, const sse_char_t* str, sse_size_t len, 
	const sse_char_t** endptr);

sse_size_t sse_awk_longtostr (
	sse_long_t value, int radix, const sse_char_t* prefix,
	sse_char_t* buf, sse_size_t size);

/* string functions exported by awk.h */
sse_char_t* sse_awk_strdup (
	sse_awk_t* awk, const sse_char_t* str);
sse_char_t* sse_awk_strxdup (
	sse_awk_t* awk, const sse_char_t* str, sse_size_t len);
sse_char_t* sse_awk_strxdup2 (
	sse_awk_t* awk,
	const sse_char_t* str1, sse_size_t len1,
	const sse_char_t* str2, sse_size_t len2);

sse_size_t sse_awk_strlen (const sse_char_t* str);
sse_size_t sse_awk_strcpy (sse_char_t* buf, const sse_char_t* str);
sse_size_t sse_awk_strncpy (sse_char_t* buf, const sse_char_t* str, sse_size_t len);
int sse_awk_strcmp (const sse_char_t* s1, const sse_char_t* s2);

int sse_awk_strxncmp (
	const sse_char_t* s1, sse_size_t len1, 
	const sse_char_t* s2, sse_size_t len2);

int sse_awk_strxncasecmp (
	sse_awk_t* awk,
	const sse_char_t* s1, sse_size_t len1, 
	const sse_char_t* s2, sse_size_t len2);

sse_char_t* sse_awk_strxnstr (
	const sse_char_t* str, sse_size_t strsz, 
	const sse_char_t* sub, sse_size_t subsz);

/* utility functions to convert an error number ot a string */
const sse_char_t* sse_awk_geterrstr (int errnum);

#ifdef __cplusplus
}
#endif

#endif
