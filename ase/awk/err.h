/*
 * $Id: err.h,v 1.2 2006-03-04 15:54:37 bacon Exp $
 */

#ifndef _XP_AWK_ERR_H_
#define _XP_AWK_ERR_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

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
	XP_AWK_ELBRACE,     /* left brace expected */
	XP_AWK_ELPAREN,     /* left parenthesis expected */
	XP_AWK_ERPAREN,     /* right parenthesis expected */
	XP_AWK_ERBRACK,     /* right bracket expected */
	XP_AWK_ECOMMA,      /* comma expected */
	XP_AWK_ESEMICOLON,  /* semicolon expected */
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
	XP_AWK_EUNDEF       /* undefined identifier */
};

#endif
