/* 
 * $Id: awk.h,v 1.84 2006-07-30 15:53:42 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_awk_t xp_awk_t;
typedef struct xp_awk_val_t xp_awk_val_t;
typedef struct xp_awk_extio_t xp_awk_extio_t;
typedef struct xp_awk_rex_t xp_awk_rex_t;

typedef xp_ssize_t (*xp_awk_io_t) (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

struct xp_awk_extio_t 
{
	int type; /* console, file, coproc, pipe */
	int mode; /* read, write, etc */
	xp_char_t* name;

	void* handle;

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

/* io function commands */
enum 
{
	XP_AWK_IO_OPEN   = 0,
	XP_AWK_IO_CLOSE  = 1,
	XP_AWK_IO_READ   = 2,
	XP_AWK_IO_WRITE  = 3,
	XP_AWK_IO_NEXT   = 4  
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

/* parse options */
enum
{ 
	/* allow undeclared variables */
	XP_AWK_IMPLICIT   = (1 << 0),

	/* variable requires explicit declaration */
	XP_AWK_EXPLICIT   = (1 << 1), 

	/* a function name should not coincide to be a variable name */
	XP_AWK_UNIQUE     = (1 << 2),

	/* allow variable shading */
	XP_AWK_SHADING    = (1 << 3), 

	/* support shift operators */
	XP_AWK_SHIFT      = (1 << 4), 

	/* support comments by a hash sign */
	XP_AWK_HASHSIGN   = (1 << 5), 

	/* support comments by double slashes */
	XP_AWK_DBLSLASHES = (1 << 6), 

	/* support string concatenation in tokenization.
	 * this option can change the behavior of a certain construct.
	 * getline < "abc" ".def" is treated as if it is getline < "abc.def" 
	 * when this option is on. If this option is off, the same expression
	 * is treated as if it is (getline < "abc") ".def". */
	XP_AWK_STRCONCAT  = (1 << 7), 

	/* support getline and print */
	XP_AWK_EXTIO      = (1 << 8) 
};

/* run options */
enum
{
	XP_AWK_RUNMAIN  = (1 << 0)  /* execution starts from main */
};

/* error code */
enum
{
	XP_AWK_ENOERR,         /* no error */
	XP_AWK_ENOMEM,         /* out of memory */
	XP_AWK_EINVAL,         /* invalid parameter */

	XP_AWK_ENOSRCIO,       /* no source io handler set */
	XP_AWK_ESRCINOPEN,
	XP_AWK_ESRCINCLOSE,
	XP_AWK_ESRCINNEXT,
	XP_AWK_ESRCINDATA,     /* error in reading source */

	XP_AWK_ETXTINOPEN,
	XP_AWK_ETXTINCLOSE,
	XP_AWK_ETXTINNEXT,
	XP_AWK_ETXTINDATA,     /* error in reading text */

	XP_AWK_ELXCHR,         /* lexer came accross an wrong character */
	XP_AWK_ELXUNG,         /* lexer failed to unget a character */

	XP_AWK_EENDSRC,        /* unexpected end of source */
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
	XP_AWK_EGETLINE,       /* getline expected */
	XP_AWK_EREXBUILD,      /* cannot build regexp */
	XP_AWK_EREXMATCH,      /* an error occurred in matching regexp */

	/* run time error */
	XP_AWK_EDIVBYZERO,       /* divide by zero */
	XP_AWK_EOPERAND,         /* invalid operand */
	XP_AWK_EPOSIDX,          /* wrong position index */
	XP_AWK_ENOSUCHFUNC,      /* no such function */
	XP_AWK_ENOTASSIGNABLE,   /* value not assignable */
	XP_AWK_ENOTINDEXABLE,    /* not indexable variable */
	XP_AWK_ENOTDELETABLE,    /* not deletable variable */
	XP_AWK_ENOTSCALARIZABLE, /* not scalarizable variable */
	XP_AWK_EVALTYPE,         /* wrong value type */
	XP_AWK_EPIPE,            /* pipe operation error */
	XP_AWK_EIOIMPL,          /* wrong implementation of user io handler */
	XP_AWK_EINTERNAL         /* internal error */
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

xp_awk_t* xp_awk_open (void);
int xp_awk_close (xp_awk_t* awk);

int xp_awk_geterrnum (xp_awk_t* awk);
const xp_char_t* xp_awk_geterrstr (xp_awk_t* awk);
int xp_awk_getsuberrnum (xp_awk_t* awk);
const xp_char_t* xp_awk_getsuberrstr (xp_awk_t* awk);

void xp_awk_clear (xp_awk_t* awk);
void xp_awk_setparseopt (xp_awk_t* awk, int opt);
void xp_awk_setrunopt (xp_awk_t* awk, int opt);

int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg);
int xp_awk_detsrc (xp_awk_t* awk);
xp_size_t xp_awk_getsrcline (xp_awk_t* awk);

int xp_awk_setextio (xp_awk_t* awk, int id, xp_awk_io_t handler, void* arg);
int xp_awk_parse (xp_awk_t* awk);
int xp_awk_run (xp_awk_t* awk);

/* functions to access internal stack structure */
xp_size_t xp_awk_getnargs (void* run);
xp_awk_val_t* xp_awk_getarg (void* run, xp_size_t idx);
void xp_awk_setretval (void* run, xp_awk_val_t* val);
void xp_awk_setglobal (void* run, xp_size_t idx, xp_awk_val_t* val);
void xp_awk_seterrnum (void* run, int errnum);

/* utility functions exported by awk.h */
xp_long_t xp_awk_strtolong (
	const xp_char_t* str, int base, const xp_char_t** endptr);
xp_real_t xp_awk_strtoreal (const xp_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
