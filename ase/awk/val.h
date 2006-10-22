/*
 * $Id: val.h,v 1.47 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_VAL_H_
#define _SSE_AWK_VAL_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

enum
{
	/* the values between SSE_AWK_VAL_NIL and SSE_AWK_VAL_STR inclusive
	 * must be synchronized with an internal table of the __cmp_val 
	 * function in run.c */
	SSE_AWK_VAL_NIL  = 0,
	SSE_AWK_VAL_INT  = 1,
	SSE_AWK_VAL_REAL = 2,
	SSE_AWK_VAL_STR  = 3,

	SSE_AWK_VAL_REX  = 4,
	SSE_AWK_VAL_MAP  = 5,
	SSE_AWK_VAL_REF  = 6
};

enum
{
	/* keep these items in the same order as corresponding items
	 * in tree.h */
	SSE_AWK_VAL_REF_NAMED,
	SSE_AWK_VAL_REF_GLOBAL,
	SSE_AWK_VAL_REF_LOCAL,
	SSE_AWK_VAL_REF_ARG,
	SSE_AWK_VAL_REF_NAMEDIDX,
	SSE_AWK_VAL_REF_GLOBALIDX,
	SSE_AWK_VAL_REF_LOCALIDX,
	SSE_AWK_VAL_REF_ARGIDX,
	SSE_AWK_VAL_REF_POS
};

enum
{
	SSE_AWK_VALTOSTR_CLEAR = (1 << 0),
	SSE_AWK_VALTOSTR_PRINT = (1 << 1)
};

typedef struct sse_awk_val_nil_t  sse_awk_val_nil_t;
typedef struct sse_awk_val_int_t  sse_awk_val_int_t;
typedef struct sse_awk_val_real_t sse_awk_val_real_t;
typedef struct sse_awk_val_str_t  sse_awk_val_str_t;
typedef struct sse_awk_val_rex_t  sse_awk_val_rex_t;
typedef struct sse_awk_val_map_t  sse_awk_val_map_t;
typedef struct sse_awk_val_ref_t  sse_awk_val_ref_t;

#if SSE_SIZEOF_INT == 2
#define SSE_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 13
#else
#define SSE_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 29
#endif

struct sse_awk_val_t
{
	SSE_AWK_VAL_HDR;	
};

/* SSE_AWK_VAL_NIL */
struct sse_awk_val_nil_t
{
	SSE_AWK_VAL_HDR;
};

/* SSE_AWK_VAL_INT */
struct sse_awk_val_int_t
{
	SSE_AWK_VAL_HDR;
	sse_long_t val;
	sse_awk_nde_int_t* nde;
};

/* SSE_AWK_VAL_REAL */
struct sse_awk_val_real_t
{
	SSE_AWK_VAL_HDR;
	sse_real_t val;
	sse_awk_nde_real_t* nde;
};

/* SSE_AWK_VAL_STR */
struct sse_awk_val_str_t
{
	SSE_AWK_VAL_HDR;
	sse_char_t* buf;
	sse_size_t  len;
};

/* SSE_AWK_VAL_REX */
struct sse_awk_val_rex_t
{
	SSE_AWK_VAL_HDR;
	sse_char_t* buf;
	sse_size_t  len;
	void*      code;
};

/* SSE_AWK_VAL_MAP */
struct sse_awk_val_map_t
{
	SSE_AWK_VAL_HDR;

	/* TODO: make val_map to array if the indices used are all 
	 *       integers switch to map dynamically once the 
	 *       non-integral index is seen.
	 */
	sse_awk_map_t* map; 
};

/* SSE_AWK_VAL_REF */
struct sse_awk_val_ref_t
{
	SSE_AWK_VAL_HDR;

	int id;
	/* if id is SSE_AWK_VAL_REF_POS, adr holds an index of the 
	 * positionalvariable. Otherwise, adr points to the value 
	 * directly. */
	sse_awk_val_t** adr;
};

#ifdef __cplusplus
extern "C" {
#endif

extern sse_awk_val_t* sse_awk_val_nil;
extern sse_awk_val_t* sse_awk_val_zls;
extern sse_awk_val_t* sse_awk_val_zero;
extern sse_awk_val_t* sse_awk_val_one;

sse_awk_val_t* sse_awk_makeintval (sse_awk_run_t* run, sse_long_t v);
sse_awk_val_t* sse_awk_makerealval (sse_awk_run_t* run, sse_real_t v);

sse_awk_val_t* sse_awk_makestrval0 (sse_awk_run_t* run, const sse_char_t* str);
sse_awk_val_t* sse_awk_makestrval (
	sse_awk_run_t* run, const sse_char_t* str, sse_size_t len);
sse_awk_val_t* sse_awk_makestrval2 (
	sse_awk_run_t* run,
	const sse_char_t* str1, sse_size_t len1, 
	const sse_char_t* str2, sse_size_t len2);

sse_awk_val_t* sse_awk_makerexval (
	sse_awk_run_t* run, const sse_char_t* buf, sse_size_t len, void* code);
sse_awk_val_t* sse_awk_makemapval (sse_awk_run_t* run);
sse_awk_val_t* sse_awk_makerefval (
	sse_awk_run_t* run, int id, sse_awk_val_t** adr);

sse_bool_t sse_awk_isbuiltinval (sse_awk_val_t* val);

void sse_awk_freeval (sse_awk_run_t* run, sse_awk_val_t* val, sse_bool_t cache);
void sse_awk_refupval (sse_awk_val_t* val);
void sse_awk_refdownval (sse_awk_run_t* run, sse_awk_val_t* val);
void sse_awk_refdownval_nofree (sse_awk_run_t* run, sse_awk_val_t* val);

sse_bool_t sse_awk_valtobool (
	sse_awk_run_t* run, sse_awk_val_t* val);

sse_char_t* sse_awk_valtostr (
	sse_awk_run_t* run, sse_awk_val_t* val, 
	int opt, sse_awk_str_t* buf, sse_size_t* len);

int sse_awk_valtonum (
	sse_awk_run_t* run, sse_awk_val_t* v, sse_long_t* l, sse_real_t* r);

void sse_awk_printval (sse_awk_val_t* val);

#ifdef __cplusplus
}
#endif

#endif
