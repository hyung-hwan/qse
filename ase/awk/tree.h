/*
 * $Id: tree.h,v 1.77 2006-10-24 04:10:12 bacon Exp $
 */

#ifndef _ASE_AWK_TREE_H_
#define _ASE_AWK_TREE_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

enum
{
	ASE_AWK_NDE_NULL,

	/* statement */
	ASE_AWK_NDE_BLK,
	ASE_AWK_NDE_IF,
	ASE_AWK_NDE_WHILE,
	ASE_AWK_NDE_DOWHILE,
	ASE_AWK_NDE_FOR,
	ASE_AWK_NDE_FOREACH,
	ASE_AWK_NDE_BREAK,
	ASE_AWK_NDE_CONTINUE,
	ASE_AWK_NDE_RETURN,
	ASE_AWK_NDE_EXIT,
	ASE_AWK_NDE_NEXT,
	ASE_AWK_NDE_NEXTFILE,
	ASE_AWK_NDE_DELETE,
	ASE_AWK_NDE_PRINT,

	/* expression */
	/* if you change the following values including their order,
	 * you should change __eval_func of __eval_expression 
	 * in run.c accordingly */
	ASE_AWK_NDE_GRP, 
	ASE_AWK_NDE_ASS,
	ASE_AWK_NDE_EXP_BIN,
	ASE_AWK_NDE_EXP_UNR,
	ASE_AWK_NDE_EXP_INCPRE,
	ASE_AWK_NDE_EXP_INCPST,
	ASE_AWK_NDE_CND,
	ASE_AWK_NDE_BFN,
	ASE_AWK_NDE_AFN,
	ASE_AWK_NDE_INT,
	ASE_AWK_NDE_REAL,
	ASE_AWK_NDE_STR,
	ASE_AWK_NDE_REX,

	/* keep this order for the following items otherwise, you may have 
	 * to change __eval_incpre and __eval_incpst in run.c as well as
	 * ASE_AWK_VAL_REF_XXX in val.h */
	ASE_AWK_NDE_NAMED,
	ASE_AWK_NDE_GLOBAL,
	ASE_AWK_NDE_LOCAL,
	ASE_AWK_NDE_ARG,
	ASE_AWK_NDE_NAMEDIDX,
	ASE_AWK_NDE_GLOBALIDX,
	ASE_AWK_NDE_LOCALIDX,
	ASE_AWK_NDE_ARGIDX,
	ASE_AWK_NDE_POS,
	/* ---------------------------------- */

	ASE_AWK_NDE_GETLINE,
};

enum
{
	/* the order of these values match 
	 * __in_type_map and __in_opt_map in extio.c */

	ASE_AWK_IN_PIPE,
	ASE_AWK_IN_COPROC,
	ASE_AWK_IN_FILE,
	ASE_AWK_IN_CONSOLE
};

enum
{
	/* the order of these values match 
	 * __out_type_map and __out_opt_map in extio.c */

	ASE_AWK_OUT_PIPE,
	ASE_AWK_OUT_COPROC,
	ASE_AWK_OUT_FILE,
	ASE_AWK_OUT_FILE_APPEND,
	ASE_AWK_OUT_CONSOLE
};

/* afn (awk function defined with the keyword function) */
typedef struct ase_awk_afn_t ase_awk_afn_t;

typedef struct ase_awk_nde_t ase_awk_nde_t;

typedef struct ase_awk_nde_blk_t       ase_awk_nde_blk_t;
typedef struct ase_awk_nde_grp_t       ase_awk_nde_grp_t;
typedef struct ase_awk_nde_ass_t       ase_awk_nde_ass_t;
typedef struct ase_awk_nde_exp_t       ase_awk_nde_exp_t;
typedef struct ase_awk_nde_cnd_t       ase_awk_nde_cnd_t;
typedef struct ase_awk_nde_pos_t       ase_awk_nde_pos_t;
typedef struct ase_awk_nde_int_t       ase_awk_nde_int_t;
typedef struct ase_awk_nde_real_t      ase_awk_nde_real_t;
typedef struct ase_awk_nde_str_t       ase_awk_nde_str_t;
typedef struct ase_awk_nde_rex_t       ase_awk_nde_rex_t;
typedef struct ase_awk_nde_var_t       ase_awk_nde_var_t;
typedef struct ase_awk_nde_call_t      ase_awk_nde_call_t;
typedef struct ase_awk_nde_getline_t   ase_awk_nde_getline_t;

typedef struct ase_awk_nde_if_t        ase_awk_nde_if_t;
typedef struct ase_awk_nde_while_t     ase_awk_nde_while_t;
typedef struct ase_awk_nde_for_t       ase_awk_nde_for_t;
typedef struct ase_awk_nde_foreach_t   ase_awk_nde_foreach_t;
typedef struct ase_awk_nde_break_t     ase_awk_nde_break_t;
typedef struct ase_awk_nde_continue_t  ase_awk_nde_continue_t;
typedef struct ase_awk_nde_return_t    ase_awk_nde_return_t;
typedef struct ase_awk_nde_exit_t      ase_awk_nde_exit_t;
typedef struct ase_awk_nde_next_t      ase_awk_nde_next_t;
typedef struct ase_awk_nde_nextfile_t  ase_awk_nde_nextfile_t;
typedef struct ase_awk_nde_delete_t    ase_awk_nde_delete_t;
typedef struct ase_awk_nde_print_t     ase_awk_nde_print_t;

struct ase_awk_afn_t
{
	ase_char_t* name;
	ase_size_t name_len;
	ase_size_t nargs;
	ase_awk_nde_t* body;
};

#define ASE_AWK_NDE_HDR \
	int type; \
	ase_awk_nde_t* next

struct ase_awk_nde_t
{
	ASE_AWK_NDE_HDR;
};

/* ASE_AWK_NDE_BLK - block statement including top-level blocks */
struct ase_awk_nde_blk_t
{
	ASE_AWK_NDE_HDR;
	ase_size_t nlocals;
	ase_awk_nde_t* body;
};

/* ASE_AWK_NDE_GRP - expression group */
struct ase_awk_nde_grp_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* body;
};

/* ASE_AWK_NDE_ASS - assignment */
struct ase_awk_nde_ass_t
{
	ASE_AWK_NDE_HDR;
	int opcode;
	ase_awk_nde_t* left;
	ase_awk_nde_t* right;
};

/* ASE_AWK_NDE_EXP_BIN, ASE_AWK_NDE_EXP_UNR, 
 * ASE_AWK_NDE_EXP_INCPRE, ASE_AW_NDE_EXP_INCPST */
struct ase_awk_nde_exp_t
{
	ASE_AWK_NDE_HDR;
	int opcode;
	ase_awk_nde_t* left;
	ase_awk_nde_t* right; /* ASE_NULL for UNR, INCPRE, INCPST */
};

/* ASE_AWK_NDE_CND */
struct ase_awk_nde_cnd_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* test;
	ase_awk_nde_t* left;
	ase_awk_nde_t* right;
};

/* ASE_AWK_NDE_POS - positional - $1, $2, $x, etc */
struct ase_awk_nde_pos_t  
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* val;	
};

/* ASE_AWK_NDE_INT */
struct ase_awk_nde_int_t
{
	ASE_AWK_NDE_HDR;
	ase_long_t val;
	ase_char_t* str; 
	ase_size_t  len;
};

/* ASE_AWK_NDE_REAL */
struct ase_awk_nde_real_t
{
	ASE_AWK_NDE_HDR;
	ase_real_t val;
	ase_char_t* str;
	ase_size_t  len;
};

/* ASE_AWK_NDE_STR */
struct ase_awk_nde_str_t
{
	ASE_AWK_NDE_HDR;
	ase_char_t* buf;
	ase_size_t  len;
};

/* ASE_AWK_NDE_REX */
struct ase_awk_nde_rex_t
{
	ASE_AWK_NDE_HDR;
	ase_char_t* buf;
	ase_size_t  len;
	void*      code;
};

/* ASE_AWK_NDE_NAMED, ASE_AWK_NDE_GLOBAL, 
 * ASE_AWK_NDE_LOCAL, ASE_AWK_NDE_ARG 
 * ASE_AWK_NDE_NAMEDIDX, ASE_AWK_NDE_GLOBALIDX, 
 * ASE_AWK_NDE_LOCALIDX, ASE_AWK_NDE_ARGIDX */
struct ase_awk_nde_var_t
{
	ASE_AWK_NDE_HDR;
	struct 
	{
		ase_char_t* name;
		ase_size_t  name_len;
		ase_size_t  idxa;
	} id;
	ase_awk_nde_t* idx; /* ASE_NULL for non-XXXXIDX */
};

/* ASE_AWK_NDE_BFN, ASE_AWK_NDE_AFN */
struct ase_awk_nde_call_t
{
	ASE_AWK_NDE_HDR;
	union
	{
		struct
		{
			ase_char_t* name;
			ase_size_t name_len;
		} afn;

		/* minimum information of a built-in function 
		 * needed during run-time. */
		struct
		{
			const ase_char_t* name;
			ase_size_t name_len;
			ase_size_t min_args;
			ase_size_t max_args;
			const ase_char_t* arg_spec;
			int (*handler) (ase_awk_run_t* awk);
		} bfn;
		/* ase_awk_bfn_t* bfn; */
	} what;
	ase_awk_nde_t* args;
	ase_size_t nargs;
};

/* ASE_AWK_NDE_GETLINE */
struct ase_awk_nde_getline_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* var;
	int in_type; /* ASE_AWK_GETLINE_XXX */
	ase_awk_nde_t* in;
};

/* ASE_AWK_NDE_IF */
struct ase_awk_nde_if_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* test;
	ase_awk_nde_t* then_part;
	ase_awk_nde_t* else_part; /* optional */
};

/* ASE_AWK_NDE_WHILE, ASE_AWK_NDE_DOWHILE */
struct ase_awk_nde_while_t
{
	ASE_AWK_NDE_HDR; 
	ase_awk_nde_t* test;
	ase_awk_nde_t* body;
};

/* ASE_AWK_NDE_FOR */
struct ase_awk_nde_for_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* init; /* optional */
	ase_awk_nde_t* test; /* optional */
	ase_awk_nde_t* incr; /* optional */
	ase_awk_nde_t* body;
};

/* ASE_AWK_NDE_FOREACH */
struct ase_awk_nde_foreach_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* test;
	ase_awk_nde_t* body;
};

/* ASE_AWK_NDE_BREAK */
struct ase_awk_nde_break_t
{
	ASE_AWK_NDE_HDR;
};

/* ASE_AWK_NDE_CONTINUE */
struct ase_awk_nde_continue_t
{
	ASE_AWK_NDE_HDR;
};

/* ASE_AWK_NDE_RETURN */
struct ase_awk_nde_return_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* val; /* optional (no return code if ASE_NULL) */	
};

/* ASE_AWK_NDE_EXIT */
struct ase_awk_nde_exit_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* val; /* optional (no exit code if ASE_NULL) */
};

/* ASE_AWK_NDE_NEXT */
struct ase_awk_nde_next_t
{
	ASE_AWK_NDE_HDR;
};

/* ASE_AWK_NDE_NEXTFILE */
struct ase_awk_nde_nextfile_t
{
	ASE_AWK_NDE_HDR;
};

/* ASE_AWK_NDE_DELETE */
struct ase_awk_nde_delete_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* var;
};

/* ASE_AWK_NDE_PRINT */
struct ase_awk_nde_print_t
{
	ASE_AWK_NDE_HDR;
	ase_awk_nde_t* args;
	int out_type; /* ASE_AWK_OUT_XXX */
	ase_awk_nde_t* out;
};

#ifdef __cplusplus
extern "C" {
#endif

int ase_awk_prnpt (ase_awk_t* awk, ase_awk_nde_t* tree);
int ase_awk_prnptnpt (ase_awk_t* awk, ase_awk_nde_t* tree);

void ase_awk_clrpt (ase_awk_t* awk, ase_awk_nde_t* tree);

#ifdef __cplusplus
}
#endif

#endif
