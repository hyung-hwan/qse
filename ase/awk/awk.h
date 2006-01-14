/* 
 * $Id: awk.h,v 1.12 2006-01-14 16:09:57 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#include <xp/types.h>
#include <xp/macros.h>
#include <xp/bas/str.h>
#include <xp/awk/tree.h>

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
	XP_AWK_ECOMMA,  /* comma expected */
	XP_AWK_ESEMICOLON, /* semicolon expected */
	XP_AWK_EEXPR    /* expression expected */
};

/*
 * TYPE: xp_awk_t
 */
typedef struct xp_awk_t xp_awk_t;

/*
 * TYPE: xp_awk_io_t
 */
typedef xp_ssize_t (*xp_awk_io_t) (
	int cmd, void* arg, xp_char_t* data, xp_size_t count);

enum 
{
	XP_AWK_IO_OPEN,
	XP_AWK_IO_CLOSE,
	XP_AWK_IO_DATA
};

struct xp_awk_t
{
	/* parse tree */
	xp_awk_node_t* tree;

	/* io functions */
	xp_awk_io_t src_func;
	xp_awk_io_t inp_func;
	xp_awk_io_t outp_func;

	void* src_arg;
	void* inp_arg;
	void* outp_arg;

	/* source buffer management */
	struct {
		xp_cint_t curc;
		xp_cint_t ungotc[5];
		xp_size_t ungotc_count;
	} lex;

	/* token */
	struct {
		int       type;
		xp_str_t  name;
	} token;

	/* housekeeping */
	int errnum;
	xp_bool_t __dynamic;
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

/*
 * FUNCTION: xp_awk_attsrc
 */
int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg);

/*
 * FUNCTION: xp_awk_detsrc
 */
int xp_awk_detsrc (xp_awk_t* awk);

int xp_awk_attinp (xp_awk_t* awk, xp_awk_io_t inp, void* arg);
int xp_awk_detinp (xp_awk_t* awk);

int xp_awk_attoutp (xp_awk_t* awk, xp_awk_io_t outp, void* arg);
int xp_awk_detoutp (xp_awk_t* awk);

int xp_awk_parse (xp_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
