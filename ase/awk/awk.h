/* 
 * $Id: awk.h,v 1.4 2005-11-14 15:23:53 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#include <xp/types.h>
#include <xp/macros.h>
#include <xp/bas/string.h>

enum
{
	XP_AWK_ENOERR,
	XP_AWK_ENOMEM, /* out of memory */
	XP_AWK_ESRCOP,
	XP_AWK_ESRCCL,
	XP_AWK_ESRCDT, /* error in reading source */
	XP_AWK_ELXCHR, /* lexer came accross an wrong character */
	XP_AWK_ELXUNG  /* lexer failed to unget a character */
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
	/* io functions */
	xp_awk_io_t source_func;
	xp_awk_io_t input_func;
	xp_awk_io_t output_func;

	void* source_arg;
	void* input_arg;
	void* output_arg;

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
	xp_bool_t __malloced;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_t* xp_awk_open (xp_awk_t* awk);
int xp_awk_close (xp_awk_t* awk);

int xp_awk_attach_source (xp_awk_t* awk, xp_awk_io_t source, void* source_arg);
int xp_awk_detach_source (xp_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
