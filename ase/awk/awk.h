/* 
 * $Id: awk.h,v 1.48 2006-04-14 10:56:42 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#include <xp/types.h>
#include <xp/macros.h>

typedef struct xp_awk_t xp_awk_t;

typedef xp_ssize_t (*xp_awk_io_t) (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

/* io function commands */
enum 
{
	XP_AWK_IO_OPEN,
	XP_AWK_IO_CLOSE,
	XP_AWK_IO_DATA
};

/* parse options */
enum
{
	XP_AWK_IMPLICIT = (1 << 0), /* allow undeclared variables */
	XP_AWK_EXPLICIT = (1 << 1), /* variable requires explicit declaration */
	XP_AWK_UNIQUE   = (1 << 2), /* a function name should not coincide to be a variable name */
	XP_AWK_SHADING  = (1 << 3), /* allow variable shading */
	XP_AWK_SHIFT    = (1 << 4), /* support shift operators */
	XP_AWK_HASHSIGN = (1 << 5)  /* support comments by a hash sign */
};

/* run options */
enum
{
	XP_AWK_RUNMAIN  = (1 << 0)  /* execution starts from main */
};

/* error code */
enum
{
	XP_AWK_ENOERR,      /* no error */
	XP_AWK_ENOMEM,      /* out of memory */

	XP_AWK_ESRCOP,
	XP_AWK_ESRCCL,
	XP_AWK_ESRCDT,      /* error in reading source */

	XP_AWK_ELXCHR,      /* lexer came accross an wrong character */
	XP_AWK_ELXUNG,      /* lexer failed to unget a character */

	XP_AWK_EENDSRC,     /* unexpected end of source */
	XP_AWK_EENDSTR,     /* unexpected end of a string */
	XP_AWK_ELBRACE,     /* left brace expected */
	XP_AWK_ELPAREN,     /* left parenthesis expected */
	XP_AWK_ERPAREN,     /* right parenthesis expected */
	XP_AWK_ERBRACK,     /* right bracket expected */
	XP_AWK_ECOMMA,      /* comma expected */
	XP_AWK_ESEMICOLON,  /* semicolon expected */
	XP_AWK_ECOLON,      /* colon expected */
	XP_AWK_EEXPRESSION, /* expression expected */

	XP_AWK_EWHILE,      /* keyword 'while' is expected */
	XP_AWK_EASSIGNMENT, /* assignment statement expected */
	XP_AWK_EIDENT,      /* identifier expected */
	XP_AWK_EDUPBEGIN,   /* duplicate BEGIN */
	XP_AWK_EDUPEND,     /* duplicate END */
	XP_AWK_EDUPFUNC,    /* duplicate function name */
	XP_AWK_EDUPPARAM,   /* duplicate parameter name */
	XP_AWK_EDUPVAR,     /* duplicate variable name */
	XP_AWK_EDUPNAME,    /* duplicate name - function, variable, etc */
	XP_AWK_EUNDEF,      /* undefined identifier */
	XP_AWK_ELVALUE,     /* l-value required */

	/* run time error */
	XP_AWK_EDIVBYZERO,  /* divide by zero */
	XP_AWK_EOPERAND,    /* invalid operand */
	XP_AWK_ENOSUCHFUNC, /* no such function */
	XP_AWK_EINTERNAL    /* internal error */
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_t* xp_awk_open (void);
int xp_awk_close (xp_awk_t* awk);

int xp_awk_geterrnum (xp_awk_t* awk);
const xp_char_t* xp_awk_geterrstr (xp_awk_t* awk);

void xp_awk_clear (xp_awk_t* awk);
void xp_awk_setparseopt (xp_awk_t* awk, int opt);
void xp_awk_setrunopt (xp_awk_t* awk, int opt);

int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg);
int xp_awk_detsrc (xp_awk_t* awk);
int xp_awk_attin (xp_awk_t* awk, xp_awk_io_t in, void* arg);
int xp_awk_detin (xp_awk_t* awk);
int xp_awk_attout (xp_awk_t* awk, xp_awk_io_t out, void* arg);
int xp_awk_detout (xp_awk_t* awk);

int xp_awk_parse (xp_awk_t* awk);
int xp_awk_run (xp_awk_t* awk);

/* utility functions exported by awk.h */
xp_long_t xp_awk_strtolong (const xp_char_t* str, int base);
xp_real_t xp_awk_strtoreal (const xp_char_t* str);

#ifdef __cplusplus
}
#endif

#endif
