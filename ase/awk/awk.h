/* 
 * $Id: awk.h,v 1.31 2006-03-03 11:45:45 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#ifdef __STAND_ALONE
#include <xp/awk/sa.h>
#else
#include <xp/types.h>
#include <xp/macros.h>
#include <xp/bas/str.h>
#endif

#include <xp/awk/tree.h>
#include <xp/awk/tab.h>
#include <xp/awk/map.h>
#include <xp/awk/val.h>

enum
{
	XP_AWK_ENOERR,
	XP_AWK_ENOMEM, /* out of memory */

	XP_AWK_ESRCOP,
	XP_AWK_ESRCCL,
	XP_AWK_ESRCDT, /* error in reading source */

	XP_AWK_ELXCHR, /* lexer came accross an wrong character */
	XP_AWK_ELXUNG, /* lexer failed to unget a character */

	XP_AWK_EENDSRC, /* unexpected end of source */
	XP_AWK_ELBRACE, /* left brace expected */
	XP_AWK_ELPAREN, /* left parenthesis expected */
	XP_AWK_ERPAREN, /* right parenthesis expected */
	XP_AWK_ERBRACK, /* right bracket expected */
	XP_AWK_ECOMMA,  /* comma expected */
	XP_AWK_ESEMICOLON, /* semicolon expected */
	XP_AWK_EEXPR,   /* expression expected */

	XP_AWK_EWHILE,  /* keyword 'while' is expected */
	XP_AWK_EASSIGNMENT, /* assignment statement expected */
	XP_AWK_EIDENT,  /* identifier expected */
	XP_AWK_EDUPBEGIN, /* duplicate BEGIN */
	XP_AWK_EDUPEND,   /* duplicate END */
	XP_AWK_EDUPFUNC,  /* duplicate function name */
	XP_AWK_EDUPPARAM, /* duplicate parameter name */
	XP_AWK_EDUPVAR,   /* duplicate variable name */
	XP_AWK_EDUPNAME,  /* duplicate name - function, variable, etc */
	XP_AWK_EUNDEF     /* undefined identifier */
};

/*
 * TYPE: xp_awk_t
 */
typedef struct xp_awk_t xp_awk_t;
typedef struct xp_awk_chain_t xp_awk_chain_t;

/*
 * TYPE: xp_awk_io_t
 */
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
	XP_AWK_SHIFT    = (1 << 4)  /* support shift operators */
};

struct xp_awk_t
{
	/* options */
	struct
	{
		int parse;
		int run;	
	} opt;

	/* io functions */
	xp_awk_io_t src_func;
	xp_awk_io_t in_func;
	xp_awk_io_t out_func;

	void* src_arg;
	void* in_arg;
	void* out_arg;

	/* parse tree */
	struct 
	{
		xp_size_t nglobals;
		xp_awk_map_t funcs;
		xp_awk_nde_t* begin;
		xp_awk_nde_t* end;
		xp_awk_chain_t* chain;
		xp_awk_chain_t* chain_tail;
	} tree;

	/* temporary information that the parser needs */
	struct
	{
		xp_awk_tab_t globals;
		xp_awk_tab_t locals;
		xp_awk_tab_t params;
		xp_size_t nlocals_max;
	} parse;

	/* run-time data structure */
	struct
	{
		xp_awk_map_t named;
	} run;

	/* source buffer management */
	struct 
	{
		xp_cint_t curc;
		xp_cint_t ungotc[5];
		xp_size_t ungotc_count;
	} lex;

	/* token */
	struct 
	{
		int       type;
		xp_str_t  name;
	} token;

	/* housekeeping */
	int errnum;
	xp_bool_t __dynamic;
};

struct xp_awk_chain_t
{
	xp_awk_nde_t* pattern;
	xp_awk_nde_t* action;
	xp_awk_chain_t* next;	
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTION: xp_awk_open
 */
xp_awk_t* xp_awk_open (xp_awk_t* awk);

/*
 * FUNCTION: xp_awk_close
 */
int xp_awk_close (xp_awk_t* awk);


int xp_awk_geterrnum (xp_awk_t* awk);
const xp_char_t* xp_awk_geterrstr (xp_awk_t* awk);

/*
 * FUNCTION: xp_awk_clear
 */
void xp_awk_clear (xp_awk_t* awk);

/*
 * FUNCTION: xp_awk_attsrc
 */
int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg);

/*
 * FUNCTION: xp_awk_detsrc
 */
int xp_awk_detsrc (xp_awk_t* awk);

int xp_awk_attin (xp_awk_t* awk, xp_awk_io_t in, void* arg);
int xp_awk_detin (xp_awk_t* awk);

int xp_awk_attout (xp_awk_t* awk, xp_awk_io_t out, void* arg);
int xp_awk_detout (xp_awk_t* awk);

int xp_awk_parse (xp_awk_t* awk);
int xp_awk_run (xp_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
