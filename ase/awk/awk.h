/* 
 * $Id: awk.h,v 1.39 2006-03-31 16:35:37 bacon Exp $
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
	XP_AWK_SHIFT    = (1 << 4)  /* support shift operators */
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

int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg);
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
