/*
 * $Id: val.h,v 1.39 2006-08-31 16:00:20 bacon Exp $
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
	XP_AWK_VAL_MAP  = 5,
	XP_AWK_VAL_REF  = 6
};

enum
{
	/* keep these items in the same order as corresponding items
	 * in tree.h */
	XP_AWK_VAL_REF_NAMED,
	XP_AWK_VAL_REF_GLOBAL,
	XP_AWK_VAL_REF_LOCAL,
	XP_AWK_VAL_REF_ARG,
	XP_AWK_VAL_REF_NAMEDIDX,
	XP_AWK_VAL_REF_GLOBALIDX,
	XP_AWK_VAL_REF_LOCALIDX,
	XP_AWK_VAL_REF_ARGIDX
};

typedef struct xp_awk_val_nil_t  xp_awk_val_nil_t;
typedef struct xp_awk_val_int_t  xp_awk_val_int_t;
typedef struct xp_awk_val_real_t xp_awk_val_real_t;
typedef struct xp_awk_val_str_t  xp_awk_val_str_t;
typedef struct xp_awk_val_rex_t  xp_awk_val_rex_t;
typedef struct xp_awk_val_map_t  xp_awk_val_map_t;
typedef struct xp_awk_val_ref_t  xp_awk_val_ref_t;

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

/* XP_AWK_VAL_REF */
struct xp_awk_val_ref_t
{
	XP_AWK_VAL_HDR;

	int id;
	xp_awk_val_t** adr;
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

xp_awk_val_t* xp_awk_makestrval0 (xp_awk_run_t* run, const xp_char_t* str);
xp_awk_val_t* xp_awk_makestrval (
	xp_awk_run_t* run, const xp_char_t* str, xp_size_t len);
xp_awk_val_t* xp_awk_makestrval2 (
	xp_awk_run_t* run,
	const xp_char_t* str1, xp_size_t len1, 
	const xp_char_t* str2, xp_size_t len2);

xp_awk_val_t* xp_awk_makerexval (
	xp_awk_run_t* run, const xp_char_t* buf, xp_size_t len, void* code);
xp_awk_val_t* xp_awk_makemapval (xp_awk_run_t* run);
xp_awk_val_t* xp_awk_makerefval (
	xp_awk_run_t* run, int id, xp_awk_val_t** adr);

xp_bool_t xp_awk_isbuiltinval (xp_awk_val_t* val);

void xp_awk_freeval (xp_awk_run_t* run, xp_awk_val_t* val, xp_bool_t cache);
void xp_awk_refupval (xp_awk_val_t* val);
void xp_awk_refdownval (xp_awk_run_t* run, xp_awk_val_t* val);
void xp_awk_refdownval_nofree (xp_awk_run_t* run, xp_awk_val_t* val);

xp_bool_t xp_awk_valtobool (xp_awk_val_t* val);
xp_char_t* xp_awk_valtostr (
	xp_awk_run_t* run, xp_awk_val_t* val, 
	xp_bool_t clear_buf, xp_awk_str_t* buf, xp_size_t* len);
int xp_awk_valtonum (xp_awk_val_t* v, xp_long_t* l, xp_real_t* r);

void xp_awk_printval (xp_awk_val_t* val);

#ifdef __cplusplus
}
#endif

#endif
