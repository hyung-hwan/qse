/* 
 * $Id: awk.h,v 1.143 2006-11-18 15:36:56 bacon Exp $
 */

#ifndef _ASE_AWK_AWK_H_
#define _ASE_AWK_AWK_H_

#include <ase/types.h>
#include <ase/macros.h>

typedef struct ase_awk_t ase_awk_t;
typedef struct ase_awk_run_t ase_awk_run_t;
typedef struct ase_awk_val_t ase_awk_val_t;
typedef struct ase_awk_extio_t ase_awk_extio_t;

typedef struct ase_awk_syscas_t ase_awk_syscas_t;
typedef struct ase_awk_srcios_t ase_awk_srcios_t;
typedef struct ase_awk_runios_t ase_awk_runios_t;
typedef struct ase_awk_runcbs_t ase_awk_runcbs_t;
typedef struct ase_awk_runarg_t ase_awk_runarg_t;

typedef void (*ase_awk_lk_t) (ase_awk_t* awk, void* arg);
typedef ase_ssize_t (*ase_awk_io_t) (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

struct ase_awk_extio_t 
{
	ase_awk_run_t* run; /* [IN] */
	int type;          /* [IN] console, file, coproc, pipe */
	int mode;          /* [IN] read, write, etc */
	ase_char_t* name;   /* [IN] */
	void* custom_data; /* [IN] */

	void* handle;      /* [OUT] */

	/* input buffer */
	struct
	{
		ase_char_t buf[2048];
		ase_size_t pos;
		ase_size_t len;
		ase_bool_t eof;
	} in;

	ase_awk_extio_t* next;
};

struct ase_awk_syscas_t
{
	/* memory */
	void* (*malloc) (ase_size_t n, void* custom_data);
	void* (*realloc) (void* ptr, ase_size_t n, void* custom_data);
	void  (*free) (void* ptr, void* custom_data);

	/* thread lock */
	ase_awk_lk_t lock;
	ase_awk_lk_t unlock;

	/* character class */
	ase_bool_t (*is_upper)  (ase_cint_t c);
	ase_bool_t (*is_lower)  (ase_cint_t c);
	ase_bool_t (*is_alpha)  (ase_cint_t c);
	ase_bool_t (*is_digit)  (ase_cint_t c);
	ase_bool_t (*is_xdigit) (ase_cint_t c);
	ase_bool_t (*is_alnum)  (ase_cint_t c);
	ase_bool_t (*is_space)  (ase_cint_t c);
	ase_bool_t (*is_print)  (ase_cint_t c);
	ase_bool_t (*is_graph)  (ase_cint_t c);
	ase_bool_t (*is_cntrl)  (ase_cint_t c);
	ase_bool_t (*is_punct)  (ase_cint_t c);
	ase_cint_t (*to_upper)  (ase_cint_t c);
	ase_cint_t (*to_lower)  (ase_cint_t c);

	/* utilities */
	void* (*memcpy) (void* dst, const void* src, ase_size_t n);
	void* (*memset) (void* dst, int val, ase_size_t n);
	ase_real_t (*pow) (ase_real_t x, ase_real_t y);

	int (*sprintf) (ase_char_t* buf, ase_size_t size, ase_char_t* fmt, ...);
	void (*aprintf) (ase_char_t* fmt, ...); /* assertion */
	void (*dprintf) (ase_char_t* fmt, ...); /* debug */
	void (*abort) (void);

	void* custom_data;
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
	void (*on_start) (ase_awk_t* awk, void* handle, void* arg);
	void (*on_end) (ase_awk_t* awk, void* handle, int errnum, void* arg);
	void* custom_data;
};

struct ase_awk_runarg_t
{
	const ase_char_t* ptr;
	ase_size_t len;
};

/* io function commands */
enum 
{
	ASE_AWK_IO_OPEN   = 0,
	ASE_AWK_IO_CLOSE  = 1,
	ASE_AWK_IO_READ   = 2,
	ASE_AWK_IO_WRITE  = 3,
	ASE_AWK_IO_FLUSH  = 4,
	ASE_AWK_IO_NEXT   = 5  
};

enum
{
	ASE_AWK_IO_PIPE_READ      = 0,
	ASE_AWK_IO_PIPE_WRITE     = 1,

	ASE_AWK_IO_FILE_READ      = 0,
	ASE_AWK_IO_FILE_WRITE     = 1,
	ASE_AWK_IO_FILE_APPEND    = 2,

	ASE_AWK_IO_CONSOLE_READ   = 0,
	ASE_AWK_IO_CONSOLE_WRITE  = 1
};

/* various options */
enum 
{ 
	/* allow undeclared variables */
	ASE_AWK_IMPLICIT    = (1 << 0),

	/* variable requires explicit declaration */
	ASE_AWK_EXPLICIT    = (1 << 1), 

	/* a function name should not coincide to be a variable name */
	ASE_AWK_UNIQUE      = (1 << 2),

	/* allow variable shading */
	ASE_AWK_SHADING     = (1 << 3), 

	/* support shift operators */
	ASE_AWK_SHIFT       = (1 << 4), 

	/* support comments by a hash sign */
	ASE_AWK_HASHSIGN    = (1 << 5), 

	/* support comments by double slashes */
	ASE_AWK_DBLSLASHES  = (1 << 6), 

	/* support string concatenation in tokenization.
	 * this option can change the behavior of a certain construct.
	 * getline < "abc" ".def" is treated as if it is getline < "abc.def" 
	 * when this option is on. If this option is off, the same expression
	 * is treated as if it is (getline < "abc") ".def". */
	ASE_AWK_STRCONCAT   = (1 << 7), 

	/* support getline and print */
	ASE_AWK_EXTIO       = (1 << 8), 

	/* support blockless patterns */
	ASE_AWK_BLOCKLESS   = (1 << 9), 

 	/* execution starts from main */
	ASE_AWK_RUNMAIN     = (1 << 10),

	/* use 1 as the start index for string operations */
	ASE_AWK_STRINDEXONE = (1 << 11),

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
	ASE_AWK_STRIPSPACES = (1 << 12),

	/* a newline terminates a statement */
	ASE_AWK_NEWLINE = (1 << 13)
};

/* error code */
enum 
{
	ASE_AWK_ENOERR,         /* no error */
	ASE_AWK_ENOMEM,         /* out of memory */
	ASE_AWK_EINVAL,         /* invalid parameter */
	ASE_AWK_ERUNTIME,       /* run-time error */
	ASE_AWK_ERUNNING,       /* there are running instances */
	ASE_AWK_ETOOMANYRUNS,   /* too many running instances */
	ASE_AWK_ERECURSION,     /* recursion too deep */

	ASE_AWK_ESRCINOPEN,
	ASE_AWK_ESRCINCLOSE,
	ASE_AWK_ESRCINREAD, 

	ASE_AWK_ESRCOUTOPEN,
	ASE_AWK_ESRCOUTCLOSE,
	ASE_AWK_ESRCOUTWRITE,

	ASE_AWK_ECONINOPEN,
	ASE_AWK_ECONINCLOSE,
	ASE_AWK_ECONINNEXT,
	ASE_AWK_ECONINDATA, 

	ASE_AWK_ECONOUTOPEN,
	ASE_AWK_ECONOUTCLOSE,
	ASE_AWK_ECONOUTNEXT,
	ASE_AWK_ECONOUTDATA,

	ASE_AWK_ELXCHR,         /* lexer came accross an wrong character */
	ASE_AWK_ELXUNG,         /* lexer failed to unget a character */

	ASE_AWK_EENDSRC,        /* unexpected end of source */
	ASE_AWK_EENDCOMMENT,    /* unexpected end of a comment */
	ASE_AWK_EENDSTR,        /* unexpected end of a string */
	ASE_AWK_EENDREX,        /* unexpected end of a regular expression */
	ASE_AWK_ELBRACE,        /* left brace expected */
	ASE_AWK_ELPAREN,        /* left parenthesis expected */
	ASE_AWK_ERPAREN,        /* right parenthesis expected */
	ASE_AWK_ERBRACK,        /* right bracket expected */
	ASE_AWK_ECOMMA,         /* comma expected */
	ASE_AWK_ESEMICOLON,     /* semicolon expected */
	ASE_AWK_ECOLON,         /* colon expected */
	ASE_AWK_EIN,            /* keyword 'in' is expected */
	ASE_AWK_ENOTVAR,        /* not a variable name after 'in' */
	ASE_AWK_EEXPRESSION,    /* expression expected */

	ASE_AWK_EWHILE,         /* keyword 'while' is expected */
	ASE_AWK_EASSIGNMENT,    /* assignment statement expected */
	ASE_AWK_EIDENT,         /* identifier expected */
	ASE_AWK_EBEGINBLOCK,    /* BEGIN requires an action block */
	ASE_AWK_EENDBLOCK,      /* END requires an action block */
	ASE_AWK_EDUPBEGIN,      /* duplicate BEGIN */
	ASE_AWK_EDUPEND,        /* duplicate END */
	ASE_AWK_EDUPFUNC,       /* duplicate function name */
	ASE_AWK_EDUPPARAM,      /* duplicate parameter name */
	ASE_AWK_EDUPVAR,        /* duplicate variable name */
	ASE_AWK_EDUPNAME,       /* duplicate name - function, variable, etc */
	ASE_AWK_EUNDEF,         /* undefined identifier */
	ASE_AWK_ELVALUE,        /* l-value required */
	ASE_AWK_ETOOFEWARGS,    /* too few arguments */
	ASE_AWK_ETOOMANYARGS,   /* too many arguments */
	ASE_AWK_ETOOMANYGLOBALS, /* too many global variables */
	ASE_AWK_ETOOMANYLOCALS, /* too many local variables */
	ASE_AWK_ETOOMANYPARAMS, /* too many parameters */
	ASE_AWK_EBREAK,         /* break outside a loop */
	ASE_AWK_ECONTINUE,      /* continue outside a loop */
	ASE_AWK_ENEXT,          /* next illegal in BEGIN or END block */
	ASE_AWK_ENEXTFILE,      /* nextfile illegal in BEGIN or END block */
	ASE_AWK_EGETLINE,       /* getline expected */
	ASE_AWK_EPRINTFARG,     /* printf must have one or more arguments */

	/* run time error */
	ASE_AWK_EINTERNAL,         /* internal error */
	ASE_AWK_EDIVBYZERO,        /* divide by zero */
	ASE_AWK_EOPERAND,          /* invalid operand */
	ASE_AWK_EPOSIDX,           /* wrong position index */
	ASE_AWK_ENOSUCHFUNC,       /* no such function */
	ASE_AWK_ENOTASSIGNABLE,    /* value not assignable */
	ASE_AWK_ENOTINDEXABLE,     /* not indexable variable */
	ASE_AWK_ENOTDELETABLE,     /* not deletable variable */
	ASE_AWK_ENOTREFERENCEABLE, /* not referenceable value */
	ASE_AWK_EIDXVALASSMAP,     /* indexed value cannot be assigned a map */
	ASE_AWK_EPOSVALASSMAP,     /* a positional cannot be assigned a map */
	ASE_AWK_EMAPTOSCALAR,      /* cannot change a map to a scalar value */
	ASE_AWK_ESCALARTOMAP,      /* cannot change a scalar value to a map */
	ASE_AWK_EMAPNOTALLOWED,    /* a map is not allowed */
	ASE_AWK_EVALTYPE,          /* wrong value type */
	ASE_AWK_EPIPE,             /* pipe operation error */
	ASE_AWK_ENEXTCALL,         /* next called from BEGIN or END */
	ASE_AWK_ENEXTFILECALL,     /* nextfile called from BEGIN or END */
	ASE_AWK_EIOIMPL,           /* wrong implementation of user io handler */
	ASE_AWK_ENOSUCHIO,         /* no such io name found */
	ASE_AWK_EIOHANDLER,        /* io handler has returned an error */
	ASE_AWK_EFMTARG,           /* arguments to format string not sufficient */
	ASE_AWK_EFMTCONV,          /* recursion detected in format conversion */
	ASE_AWK_ECONVFMTCHAR,      /* an invalid character found in CONVFMT */
	ASE_AWK_EOFMTCHAR,         /* an invalid character found in OFMT */

	/* regular expression error */
	ASE_AWK_EREXRPAREN,       /* a right parenthesis is expected */
	ASE_AWK_EREXRBRACKET,     /* a right bracket is expected */
	ASE_AWK_EREXRBRACE,       /* a right brace is expected */
	ASE_AWK_EREXCOLON,        /* a colon is expected */
	ASE_AWK_EREXCRANGE,       /* invalid character range */
	ASE_AWK_EREXCCLASS,       /* invalid character class */
	ASE_AWK_EREXBRANGE,       /* invalid boundary range */
	ASE_AWK_EREXEND,          /* unexpected end of the pattern */
	ASE_AWK_EREXGARBAGE       /* garbage after the pattern */
};

/* extio types */
enum
{
	/* extio types available */
	ASE_AWK_EXTIO_PIPE,
	ASE_AWK_EXTIO_COPROC,
	ASE_AWK_EXTIO_FILE,
	ASE_AWK_EXTIO_CONSOLE,

	/* reserved for internal use only */
	ASE_AWK_EXTIO_NUM
};

/* assertion statement */
#ifdef NDEBUG
	#define ASE_AWK_ASSERT(awk,expr) ((void)0)
	#define ASE_AWK_ASSERTX(awk,expr,desc) ((void)0)
#else
	#define ASE_AWK_ASSERT(awk,expr) (void)((expr) || \
		(ase_awk_assertfail (awk, ASE_T(#expr), ASE_NULL, ASE_T(__FILE__), __LINE__), 0))
	#define ASE_AWK_ASSERTX(awk,expr,desc) (void)((expr) || \
		(ase_awk_assertfail (awk, ASE_T(#expr), ASE_T(desc), ASE_T(__FILE__), __LINE__), 0))
#endif

#ifdef __cplusplus
extern "C" {
#endif

ase_awk_t* ase_awk_open (const ase_awk_syscas_t* syscas);
int ase_awk_close (ase_awk_t* awk);
int ase_awk_clear (ase_awk_t* awk);

int ase_awk_geterrnum (ase_awk_t* awk);
ase_size_t ase_awk_getsrcline (ase_awk_t* awk);

int ase_awk_getopt (ase_awk_t* awk);
void ase_awk_setopt (ase_awk_t* awk, int opt);

int ase_awk_parse (ase_awk_t* awk, ase_awk_srcios_t* srcios);

/*
 * ase_awk_run return 0 on success and -1 on failure, generally speaking.
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
int ase_awk_run (ase_awk_t* awk, 
	ase_awk_runios_t* runios, 
	ase_awk_runcbs_t* runcbs, 
	ase_awk_runarg_t* runarg);

int ase_awk_stop (ase_awk_t* awk, ase_awk_run_t* run);
void ase_awk_stopall (ase_awk_t* awk);

/* functions to access internal stack structure */
ase_size_t ase_awk_getnargs (ase_awk_run_t* run);
ase_awk_val_t* ase_awk_getarg (ase_awk_run_t* run, ase_size_t idx);
ase_awk_val_t* ase_awk_getglobal (ase_awk_run_t* run, ase_size_t idx);
int ase_awk_setglobal (ase_awk_run_t* run, ase_size_t idx, ase_awk_val_t* val);
void ase_awk_setretval (ase_awk_run_t* run, ase_awk_val_t* val);

int ase_awk_setconsolename (
	ase_awk_run_t* run, const ase_char_t* name, ase_size_t len);

int ase_awk_getrunerrnum (ase_awk_run_t* run);
void ase_awk_setrunerrnum (ase_awk_run_t* run, int errnum);

/* record and field functions */
int ase_awk_clrrec (ase_awk_run_t* run, ase_bool_t skip_inrec_line);
int ase_awk_setrec (ase_awk_run_t* run, ase_size_t idx, const ase_char_t* str, ase_size_t len);

/* utility functions exported by awk.h */
ase_long_t ase_awk_strxtolong (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len,
	int base, const ase_char_t** endptr);
ase_real_t ase_awk_strxtoreal (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len, 
	const ase_char_t** endptr);

ase_size_t ase_awk_longtostr (
	ase_long_t value, int radix, const ase_char_t* prefix,
	ase_char_t* buf, ase_size_t size);

/* string functions exported by awk.h */
ase_char_t* ase_awk_strdup (
	ase_awk_t* awk, const ase_char_t* str);
ase_char_t* ase_awk_strxdup (
	ase_awk_t* awk, const ase_char_t* str, ase_size_t len);
ase_char_t* ase_awk_strxdup2 (
	ase_awk_t* awk,
	const ase_char_t* str1, ase_size_t len1,
	const ase_char_t* str2, ase_size_t len2);

ase_size_t ase_awk_strlen (const ase_char_t* str);
ase_size_t ase_awk_strcpy (ase_char_t* buf, const ase_char_t* str);
ase_size_t ase_awk_strncpy (ase_char_t* buf, const ase_char_t* str, ase_size_t len);
int ase_awk_strcmp (const ase_char_t* s1, const ase_char_t* s2);

int ase_awk_strxncmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2);

int ase_awk_strxncasecmp (
	ase_awk_t* awk,
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2);

ase_char_t* ase_awk_strxnstr (
	const ase_char_t* str, ase_size_t strsz, 
	const ase_char_t* sub, ase_size_t subsz);

/* abort function for assertion. use ASE_AWK_ASSERT instead */
int ase_awk_assertfail (ase_awk_t* awk, 
	const ase_char_t* expr, const ase_char_t* desc, 
	const ase_char_t* file, int line);

/* utility functions to convert an error number ot a string */
const ase_char_t* ase_awk_geterrstr (int errnum);

#ifdef __cplusplus
}
#endif

#endif
