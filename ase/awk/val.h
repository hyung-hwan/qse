/*
 * $Id: val.h,v 1.1 2006-03-03 11:45:45 bacon Exp $
 */

#ifndef _XP_AWK_VAL_H_
#define _XP_AWK_VAL_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

typedef struct xp_awk_val_t      xp_awk_val_t;
typedef struct xp_awk_val_int_t  xp_awk_val_int_t;
typedef struct xp_awk_val_real_t xp_awk_val_real_t;
typedef struct xp_awk_val_str_t  xp_awk_val_str_t;

#define XP_AWK_VAL_HDR \
	int type

struct xp_awk_val_t
{
	XP_AWK_VAL_HDR;	
};

struct xp_awk_val_int_t
{
	XP_AWK_VAL_HDR;
	xp_long_t val;
};

struct xp_awk_val_real_t
{
	XP_AWK_VAL_HDR;
	xp_real_t val;
};

struct xp_awk_val_str_t
{
	XP_AWK_VAL_HDR;
	xp_char_t* buf;
	xp_size_t  len;
};

#endif
