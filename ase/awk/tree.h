/*
 * $Id: tree.h,v 1.76 2006-10-22 12:39:30 bacon Exp $
 */

#ifndef _SSE_AWK_TREE_H_
#define _SSE_AWK_TREE_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

enum
{
	SSE_AWK_NDE_NULL,

	/* statement */
	SSE_AWK_NDE_BLK,
	SSE_AWK_NDE_IF,
	SSE_AWK_NDE_WHILE,
	SSE_AWK_NDE_DOWHILE,
	SSE_AWK_NDE_FOR,
	SSE_AWK_NDE_FOREACH,
	SSE_AWK_NDE_BREAK,
	SSE_AWK_NDE_CONTINUE,
	SSE_AWK_NDE_RETURN,
	SSE_AWK_NDE_EXIT,
	SSE_AWK_NDE_NEXT,
	SSE_AWK_NDE_NEXTFILE,
	SSE_AWK_NDE_DELETE,
	SSE_AWK_NDE_PRINT,

	/* expression */
	/* if you change the following values including their order,
	 * you should change __eval_func of __eval_expression 
	 * in run.c accordingly */
	SSE_AWK_NDE_GRP, 
	SSE_AWK_NDE_ASS,
	SSE_AWK_NDE_EXP_BIN,
	SSE_AWK_NDE_EXP_UNR,
	SSE_AWK_NDE_EXP_INCPRE,
	SSE_AWK_NDE_EXP_INCPST,
	SSE_AWK_NDE_CND,
	SSE_AWK_NDE_BFN,
	SSE_AWK_NDE_AFN,
	SSE_AWK_NDE_INT,
	SSE_AWK_NDE_REAL,
	SSE_AWK_NDE_STR,
	SSE_AWK_NDE_REX,

	/* keep this order for the following items otherwise, you may have 
	 * to change __eval_incpre and __eval_incpst in run.c as well as
	 * SSE_AWK_VAL_REF_XXX in val.h */
	SSE_AWK_NDE_NAMED,
	SSE_AWK_NDE_GLOBAL,
	SSE_AWK_NDE_LOCAL,
	SSE_AWK_NDE_ARG,
	SSE_AWK_NDE_NAMEDIDX,
	SSE_AWK_NDE_GLOBALIDX,
	SSE_AWK_NDE_LOCALIDX,
	SSE_AWK_NDE_ARGIDX,
	SSE_AWK_NDE_POS,
	/* ---------------------------------- */

	SSE_AWK_NDE_GETLINE,
};

enum
{
	/* the order of these values match 
	 * __in_type_map and __in_opt_map in extio.c */

	SSE_AWK_IN_PIPE,
	SSE_AWK_IN_COPROC,
	SSE_AWK_IN_FILE,
	SSE_AWK_IN_CONSOLE
};

enum
{
	/* the order of these values match 
	 * __out_type_map and __out_opt_map in extio.c */

	SSE_AWK_OUT_PIPE,
	SSE_AWK_OUT_COPROC,
	SSE_AWK_OUT_FILE,
	SSE_AWK_OUT_FILE_APPEND,
	SSE_AWK_OUT_CONSOLE
};

/* afn (awk function defined with the keyword function) */
typedef struct sse_awk_afn_t sse_awk_afn_t;

typedef struct sse_awk_nde_t sse_awk_nde_t;

typedef struct sse_awk_nde_blk_t       sse_awk_nde_blk_t;
typedef struct sse_awk_nde_grp_t       sse_awk_nde_grp_t;
typedef struct sse_awk_nde_ass_t       sse_awk_nde_ass_t;
typedef struct sse_awk_nde_exp_t       sse_awk_nde_exp_t;
typedef struct sse_awk_nde_cnd_t       sse_awk_nde_cnd_t;
typedef struct sse_awk_nde_pos_t       sse_awk_nde_pos_t;
typedef struct sse_awk_nde_int_t       sse_awk_nde_int_t;
typedef struct sse_awk_nde_real_t      sse_awk_nde_real_t;
typedef struct sse_awk_nde_str_t       sse_awk_nde_str_t;
typedef struct sse_awk_nde_rex_t       sse_awk_nde_rex_t;
typedef struct sse_awk_nde_var_t       sse_awk_nde_var_t;
typedef struct sse_awk_nde_call_t      sse_awk_nde_call_t;
typedef struct sse_awk_nde_getline_t   sse_awk_nde_getline_t;

typedef struct sse_awk_nde_if_t        sse_awk_nde_if_t;
typedef struct sse_awk_nde_while_t     sse_awk_nde_while_t;
typedef struct sse_awk_nde_for_t       sse_awk_nde_for_t;
typedef struct sse_awk_nde_foreach_t   sse_awk_nde_foreach_t;
typedef struct sse_awk_nde_break_t     sse_awk_nde_break_t;
typedef struct sse_awk_nde_continue_t  sse_awk_nde_continue_t;
typedef struct sse_awk_nde_return_t    sse_awk_nde_return_t;
typedef struct sse_awk_nde_exit_t      sse_awk_nde_exit_t;
typedef struct sse_awk_nde_next_t      sse_awk_nde_next_t;
typedef struct sse_awk_nde_nextfile_t  sse_awk_nde_nextfile_t;
typedef struct sse_awk_nde_delete_t    sse_awk_nde_delete_t;
typedef struct sse_awk_nde_print_t     sse_awk_nde_print_t;

struct sse_awk_afn_t
{
	sse_char_t* name;
	sse_size_t name_len;
	sse_size_t nargs;
	sse_awk_nde_t* body;
};

#define SSE_AWK_NDE_HDR \
	int type; \
	sse_awk_nde_t* next

struct sse_awk_nde_t
{
	SSE_AWK_NDE_HDR;
};

/* SSE_AWK_NDE_BLK - block statement including top-level blocks */
struct sse_awk_nde_blk_t
{
	SSE_AWK_NDE_HDR;
	sse_size_t nlocals;
	sse_awk_nde_t* body;
};

/* SSE_AWK_NDE_GRP - expression group */
struct sse_awk_nde_grp_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* body;
};

/* SSE_AWK_NDE_ASS - assignment */
struct sse_awk_nde_ass_t
{
	SSE_AWK_NDE_HDR;
	int opcode;
	sse_awk_nde_t* left;
	sse_awk_nde_t* right;
};

/* SSE_AWK_NDE_EXP_BIN, SSE_AWK_NDE_EXP_UNR, 
 * SSE_AWK_NDE_EXP_INCPRE, SSE_AW_NDE_EXP_INCPST */
struct sse_awk_nde_exp_t
{
	SSE_AWK_NDE_HDR;
	int opcode;
	sse_awk_nde_t* left;
	sse_awk_nde_t* right; /* SSE_NULL for UNR, INCPRE, INCPST */
};

/* SSE_AWK_NDE_CND */
struct sse_awk_nde_cnd_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* test;
	sse_awk_nde_t* left;
	sse_awk_nde_t* right;
};

/* SSE_AWK_NDE_POS - positional - $1, $2, $x, etc */
struct sse_awk_nde_pos_t  
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* val;	
};

/* SSE_AWK_NDE_INT */
struct sse_awk_nde_int_t
{
	SSE_AWK_NDE_HDR;
	sse_long_t val;
	sse_char_t* str; 
	sse_size_t  len;
};

/* SSE_AWK_NDE_REAL */
struct sse_awk_nde_real_t
{
	SSE_AWK_NDE_HDR;
	sse_real_t val;
	sse_char_t* str;
	sse_size_t  len;
};

/* SSE_AWK_NDE_STR */
struct sse_awk_nde_str_t
{
	SSE_AWK_NDE_HDR;
	sse_char_t* buf;
	sse_size_t  len;
};

/* SSE_AWK_NDE_REX */
struct sse_awk_nde_rex_t
{
	SSE_AWK_NDE_HDR;
	sse_char_t* buf;
	sse_size_t  len;
	void*      code;
};

/* SSE_AWK_NDE_NAMED, SSE_AWK_NDE_GLOBAL, 
 * SSE_AWK_NDE_LOCAL, SSE_AWK_NDE_ARG 
 * SSE_AWK_NDE_NAMEDIDX, SSE_AWK_NDE_GLOBALIDX, 
 * SSE_AWK_NDE_LOCALIDX, SSE_AWK_NDE_ARGIDX */
struct sse_awk_nde_var_t
{
	SSE_AWK_NDE_HDR;
	struct 
	{
		sse_char_t* name;
		sse_size_t  name_len;
		sse_size_t  idxa;
	} id;
	sse_awk_nde_t* idx; /* SSE_NULL for non-XXXXIDX */
};

/* SSE_AWK_NDE_BFN, SSE_AWK_NDE_AFN */
struct sse_awk_nde_call_t
{
	SSE_AWK_NDE_HDR;
	union
	{
		struct
		{
			sse_char_t* name;
			sse_size_t name_len;
		} afn;

		/* minimum information of a built-in function 
		 * needed during run-time. */
		struct
		{
			const sse_char_t* name;
			sse_size_t name_len;
			sse_size_t min_args;
			sse_size_t max_args;
			const sse_char_t* arg_spec;
			int (*handler) (sse_awk_run_t* awk);
		} bfn;
		/* sse_awk_bfn_t* bfn; */
	} what;
	sse_awk_nde_t* args;
	sse_size_t nargs;
};

/* SSE_AWK_NDE_GETLINE */
struct sse_awk_nde_getline_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* var;
	int in_type; /* SSE_AWK_GETLINE_XXX */
	sse_awk_nde_t* in;
};

/* SSE_AWK_NDE_IF */
struct sse_awk_nde_if_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* test;
	sse_awk_nde_t* then_part;
	sse_awk_nde_t* else_part; /* optional */
};

/* SSE_AWK_NDE_WHILE, SSE_AWK_NDE_DOWHILE */
struct sse_awk_nde_while_t
{
	SSE_AWK_NDE_HDR; 
	sse_awk_nde_t* test;
	sse_awk_nde_t* body;
};

/* SSE_AWK_NDE_FOR */
struct sse_awk_nde_for_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* init; /* optional */
	sse_awk_nde_t* test; /* optional */
	sse_awk_nde_t* incr; /* optional */
	sse_awk_nde_t* body;
};

/* SSE_AWK_NDE_FOREACH */
struct sse_awk_nde_foreach_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* test;
	sse_awk_nde_t* body;
};

/* SSE_AWK_NDE_BREAK */
struct sse_awk_nde_break_t
{
	SSE_AWK_NDE_HDR;
};

/* SSE_AWK_NDE_CONTINUE */
struct sse_awk_nde_continue_t
{
	SSE_AWK_NDE_HDR;
};

/* SSE_AWK_NDE_RETURN */
struct sse_awk_nde_return_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* val; /* optional (no return code if SSE_NULL) */	
};

/* SSE_AWK_NDE_EXIT */
struct sse_awk_nde_exit_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* val; /* optional (no exit code if SSE_NULL) */
};

/* SSE_AWK_NDE_NEXT */
struct sse_awk_nde_next_t
{
	SSE_AWK_NDE_HDR;
};

/* SSE_AWK_NDE_NEXTFILE */
struct sse_awk_nde_nextfile_t
{
	SSE_AWK_NDE_HDR;
};

/* SSE_AWK_NDE_DELETE */
struct sse_awk_nde_delete_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* var;
};

/* SSE_AWK_NDE_PRINT */
struct sse_awk_nde_print_t
{
	SSE_AWK_NDE_HDR;
	sse_awk_nde_t* args;
	int out_type; /* SSE_AWK_OUT_XXX */
	sse_awk_nde_t* out;
};

#ifdef __cplusplus
extern "C" {
#endif

int sse_awk_prnpt (sse_awk_t* awk, sse_awk_nde_t* tree);
int sse_awk_prnptnpt (sse_awk_t* awk, sse_awk_nde_t* tree);

void sse_awk_clrpt (sse_awk_t* awk, sse_awk_nde_t* tree);

#ifdef __cplusplus
}
#endif

#endif
