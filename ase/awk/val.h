/*
 * $Id: val.h,v 1.34 2006-07-26 15:00:00 bacon Exp $
 */

#ifndef _XP_AWK_VAL_H_
#define _XP_AWK_VAL_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

enum
{
	XP_AWK_VAL_NIL  = 0,
	XP_AWK_VAL_INT  = 1,
	XP_AWK_VAL_REAL = 2,
	XP_AWK_VAL_STR  = 3,
	XP_AWK_VAL_REX  = 4,
	XP_AWK_VAL_MAP  = 5
};

typedef struct xp_awk_val_nil_t  xp_awk_val_nil_t;
typedef struct xp_awk_val_int_t  xp_awk_val_int_t;
typedef struct xp_awk_val_real_t xp_awk_val_real_t;
typedef struct xp_awk_val_str_t  xp_awk_val_str_t;
typedef struct xp_awk_val_rex_t  xp_awk_val_rex_t;
typedef struct xp_awk_val_map_t  xp_awk_val_map_t;

#if XP_SIZEOF_INT == 2
#define XP_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 13
#else
#define XP_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 29
#endif

struct xp_awk_val_t
{
	XP_AWK_VAL_HDR;	
};

/* XP_AWK_VAL_NIL */
struct xp_awk_val_nil_t
{
	XP_AWK_VAL_HDR;
};

/* XP_AWK_VAL_INT */
struct xp_awk_val_int_t
{
	XP_AWK_VAL_HDR;
	xp_long_t val;
};

/* XP_AWK_VAL_REAL */
struct xp_awk_val_real_t
{
	XP_AWK_VAL_HDR;
	xp_real_t val;
};

/* XP_AWK_VAL_STR */
struct xp_awk_val_str_t
{
	XP_AWK_VAL_HDR;
	xp_char_t* buf;
	xp_size_t  len;
};

/* XP_AWK_VAL_REX */
struct xp_awk_val_rex_t
{
	XP_AWK_VAL_HDR;
	xp_char_t* buf;
	xp_size_t  len;
	void*      code;
};

/* XP_AWK_VAL_MAP */
struct xp_awk_val_map_t
{
	XP_AWK_VAL_HDR;

	/* TODO: make val_map to array if the indices used are all integers
	 *       switch to map dynamically once the non-integral index is seen 
	 */
	xp_awk_map_t* map; 
};

#ifdef __cplusplus
extern "C" {
#endif

extern xp_awk_val_t* xp_awk_val_nil;
extern xp_awk_val_t* xp_awk_val_zls;
extern xp_awk_val_t* xp_awk_val_zero;
extern xp_awk_val_t* xp_awk_val_one;

xp_awk_val_t* xp_awk_makeintval (xp_awk_run_t* run, xp_long_t v);
xp_awk_val_t* xp_awk_makerealval (xp_awk_run_t* run, xp_real_t v);
xp_awk_val_t* xp_awk_makestrval0 (const xp_char_t* str);
xp_awk_val_t* xp_awk_makestrval (const xp_char_t* str, xp_size_t len);
xp_awk_val_t* xp_awk_makestrval2 (
	const xp_char_t* str1, xp_size_t len1, 
	const xp_char_t* str2, xp_size_t len2);
xp_awk_val_t* xp_awk_makerexval (
	const xp_char_t* buf, xp_size_t len, void* code);
xp_awk_val_t* xp_awk_makemapval (xp_awk_run_t* run);

xp_bool_t xp_awk_isbuiltinval (xp_awk_val_t* val);

void xp_awk_freeval (xp_awk_run_t* run, xp_awk_val_t* val, xp_bool_t cache);
void xp_awk_refupval (xp_awk_val_t* val);
void xp_awk_refdownval (xp_awk_run_t* run, xp_awk_val_t* val);
void xp_awk_refdownval_nofree (xp_awk_run_t* run, xp_awk_val_t* val);

xp_bool_t xp_awk_valtobool (xp_awk_val_t* val);
xp_char_t* xp_awk_valtostr (
	xp_awk_val_t* val, int* errnum, xp_str_t* buf, xp_size_t* len);
int xp_awk_valtonum (xp_awk_val_t* v, xp_long_t* l, xp_real_t* r);

void xp_awk_printval (xp_awk_val_t* val);

#ifdef __cplusplus
}
#endif

#endif
