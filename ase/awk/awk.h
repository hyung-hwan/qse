/* 
 * $Id: awk.h,v 1.2 2005-11-06 12:01:29 bacon Exp $
 */

#ifndef _XP_AWK_AWK_H_
#define _XP_AWK_AWK_H_

#include <xp/types.h>
#include <xp/macros.h>

enum
{
	XP_AWK_ENOERR
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
	xp_awk_io_t input_func;
	xp_awk_io_t output_func;
	void* input_arg;
	void* output_arg;

	/* housekeeping */
	int errnum;
	xp_bool_t __malloced;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_t* xp_awk_open (xp_awk_t* awk);
int xp_awk_close (xp_awk_t* awk);

#ifdef __cplusplus
}
#endif

#endif
