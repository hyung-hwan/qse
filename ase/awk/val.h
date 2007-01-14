/*
 * $Id: val.h,v 1.58 2007-01-14 15:07:22 bacon Exp $
 */

#ifndef _ASE_AWK_VAL_H_
#define _ASE_AWK_VAL_H_

#ifndef _ASE_AWK_AWK_H_
#error Include <ase/awk/awk.h> first
#endif

#include <ase/awk/str.h>
#include <ase/awk/map.h>

enum ase_awk_val_type_t
{
	/* the values between ASE_AWK_VAL_NIL and ASE_AWK_VAL_STR inclusive
	 * must be synchronized with an internal table of the __cmp_val 
	 * function in run.c */
	ASE_AWK_VAL_NIL  = 0,
	ASE_AWK_VAL_INT  = 1,
	ASE_AWK_VAL_REAL = 2,
	ASE_AWK_VAL_STR  = 3,

	ASE_AWK_VAL_REX  = 4,
	ASE_AWK_VAL_MAP  = 5,
	ASE_AWK_VAL_REF  = 6
};

enum ase_awk_val_ref_id_t
{
	/* keep these items in the same order as corresponding items
	 * in tree.h */
	ASE_AWK_VAL_REF_NAMED,
	ASE_AWK_VAL_REF_GLOBAL,
	ASE_AWK_VAL_REF_LOCAL,
	ASE_AWK_VAL_REF_ARG,
	ASE_AWK_VAL_REF_NAMEDIDX,
	ASE_AWK_VAL_REF_GLOBALIDX,
	ASE_AWK_VAL_REF_LOCALIDX,
	ASE_AWK_VAL_REF_ARGIDX,
	ASE_AWK_VAL_REF_POS
};

enum ase_awk_valtostr_opt_t
{
	ASE_AWK_VALTOSTR_CLEAR = (1 << 0),
	ASE_AWK_VALTOSTR_PRINT = (1 << 1)
};

typedef struct ase_awk_val_nil_t  ase_awk_val_nil_t;
typedef struct ase_awk_val_int_t  ase_awk_val_int_t;
typedef struct ase_awk_val_real_t ase_awk_val_real_t;
typedef struct ase_awk_val_str_t  ase_awk_val_str_t;
typedef struct ase_awk_val_rex_t  ase_awk_val_rex_t;
typedef struct ase_awk_val_map_t  ase_awk_val_map_t;
typedef struct ase_awk_val_ref_t  ase_awk_val_ref_t;

#if ASE_SIZEOF_INT == 2
#define ASE_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 13
#else
#define ASE_AWK_VAL_HDR \
	unsigned int type: 3; \
	unsigned int ref: 29
#endif

#ifndef ASE_AWK_NDE_INT_DEFINED
#define ASE_AWK_NDE_INT_DEFINED
typedef struct ase_awk_nde_int_t       ase_awk_nde_int_t;
#endif

#ifndef ASE_AWK_NDE_REAL_DEFINED
#define ASE_AWK_NDE_REAL_DEFINED
typedef struct ase_awk_nde_real_t      ase_awk_nde_real_t;
#endif


struct ase_awk_val_t
{
	ASE_AWK_VAL_HDR;	
};

/* ASE_AWK_VAL_NIL */
struct ase_awk_val_nil_t
{
	ASE_AWK_VAL_HDR;
};

/* ASE_AWK_VAL_INT */
struct ase_awk_val_int_t
{
	ASE_AWK_VAL_HDR;
	ase_long_t val;
	ase_awk_nde_int_t* nde;
};

/* ASE_AWK_VAL_REAL */
struct ase_awk_val_real_t
{
	ASE_AWK_VAL_HDR;
	ase_real_t val;
	ase_awk_nde_real_t* nde;
};

/* ASE_AWK_VAL_STR */
struct ase_awk_val_str_t
{
	ASE_AWK_VAL_HDR;
	ase_char_t* buf;
	ase_size_t  len;
};

/* ASE_AWK_VAL_REX */
struct ase_awk_val_rex_t
{
	ASE_AWK_VAL_HDR;
	ase_char_t* buf;
	ase_size_t  len;
	void*      code;
};

/* ASE_AWK_VAL_MAP */
struct ase_awk_val_map_t
{
	ASE_AWK_VAL_HDR;

	/* TODO: make val_map to array if the indices used are all 
	 *       integers switch to map dynamically once the 
	 *       non-integral index is seen.
	 */
	ase_awk_map_t* map; 
};

/* ASE_AWK_VAL_REF */
struct ase_awk_val_ref_t
{
	ASE_AWK_VAL_HDR;

	int id;
	/* if id is ASE_AWK_VAL_REF_POS, adr holds an index of the 
	 * positional variable. Otherwise, adr points to the value 
	 * directly. */
	ase_awk_val_t** adr;
};

#ifdef __cplusplus
extern "C" {
#endif

extern ase_awk_val_t* ase_awk_val_nil;
extern ase_awk_val_t* ase_awk_val_zls;
extern ase_awk_val_t* ase_awk_val_nl;
extern ase_awk_val_t* ase_awk_val_negone;
extern ase_awk_val_t* ase_awk_val_zero;
extern ase_awk_val_t* ase_awk_val_one;

ase_awk_val_t* ase_awk_makeintval (ase_awk_run_t* run, ase_long_t v);
ase_awk_val_t* ase_awk_makerealval (ase_awk_run_t* run, ase_real_t v);

ase_awk_val_t* ase_awk_makestrval0 (
	ase_awk_run_t* run, const ase_char_t* str);
ase_awk_val_t* ase_awk_makestrval (
	ase_awk_run_t* run, const ase_char_t* str, ase_size_t len);
ase_awk_val_t* ase_awk_makestrval_nodup (
	ase_awk_run_t* run, ase_char_t* str, ase_size_t len);
ase_awk_val_t* ase_awk_makestrval2 (
	ase_awk_run_t* run,
	const ase_char_t* str1, ase_size_t len1, 
	const ase_char_t* str2, ase_size_t len2);

ase_awk_val_t* ase_awk_makerexval (
	ase_awk_run_t* run, const ase_char_t* buf, ase_size_t len, void* code);
ase_awk_val_t* ase_awk_makemapval (ase_awk_run_t* run);
ase_awk_val_t* ase_awk_makerefval (
	ase_awk_run_t* run, int id, ase_awk_val_t** adr);

ase_bool_t ase_awk_isbuiltinval (ase_awk_val_t* val);

void ase_awk_freeval (ase_awk_run_t* run, ase_awk_val_t* val, ase_bool_t cache);
void ase_awk_refupval (ase_awk_run_t* run, ase_awk_val_t* val);
void ase_awk_refdownval (ase_awk_run_t* run, ase_awk_val_t* val);
void ase_awk_refdownval_nofree (ase_awk_run_t* run, ase_awk_val_t* val);

ase_bool_t ase_awk_valtobool (
	ase_awk_run_t* run, ase_awk_val_t* val);

ase_char_t* ase_awk_valtostr (
	ase_awk_run_t* run, ase_awk_val_t* val, 
	int opt, ase_awk_str_t* buf, ase_size_t* len);

int ase_awk_valtonum (
	ase_awk_run_t* run, ase_awk_val_t* v, ase_long_t* l, ase_real_t* r);
int ase_awk_strtonum (
	ase_awk_run_t* run, const ase_char_t* ptr, ase_size_t len, 
	ase_long_t* l, ase_real_t* r);

void ase_awk_dprintval (ase_awk_run_t* run, ase_awk_val_t* val);

#ifdef __cplusplus
}
#endif

#endif
