/*
 * $Id: tree.h,v 1.67 2006-08-06 15:02:55 bacon Exp $
 */

#ifndef _XP_AWK_TREE_H_
#define _XP_AWK_TREE_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

enum
{
	XP_AWK_NDE_NULL,

	/* statement */
	XP_AWK_NDE_BLK,
	XP_AWK_NDE_IF,
	XP_AWK_NDE_WHILE,
	XP_AWK_NDE_DOWHILE,
	XP_AWK_NDE_FOR,
	XP_AWK_NDE_FOREACH,
	XP_AWK_NDE_BREAK,
	XP_AWK_NDE_CONTINUE,
	XP_AWK_NDE_RETURN,
	XP_AWK_NDE_EXIT,
	XP_AWK_NDE_NEXT,
	XP_AWK_NDE_NEXTFILE,
	XP_AWK_NDE_DELETE,
	XP_AWK_NDE_PRINT,

	/* expression */
	/* if you change the following values including their order,
	 * you should change __eval_func of __eval_expression 
	 * in run.c accordingly */
	XP_AWK_NDE_GRP, 
	XP_AWK_NDE_ASS,
	XP_AWK_NDE_EXP_BIN,
	XP_AWK_NDE_EXP_UNR,
	XP_AWK_NDE_EXP_INCPRE,
	XP_AWK_NDE_EXP_INCPST,
	XP_AWK_NDE_CND,
	XP_AWK_NDE_BFN,
	XP_AWK_NDE_AFN,
	XP_AWK_NDE_INT,
	XP_AWK_NDE_REAL,
	XP_AWK_NDE_STR,
	XP_AWK_NDE_REX,

	/* keep this order for the following items otherwise, you may have 
	 * to change __eval_incpre and __eval_incpst in run.c*/
	XP_AWK_NDE_NAMED,
	XP_AWK_NDE_GLOBAL,
	XP_AWK_NDE_LOCAL,
	XP_AWK_NDE_ARG,
	XP_AWK_NDE_NAMEDIDX,
	XP_AWK_NDE_GLOBALIDX,
	XP_AWK_NDE_LOCALIDX,
	XP_AWK_NDE_ARGIDX,
	/* ---------------------------------- */

	XP_AWK_NDE_POS,
	XP_AWK_NDE_GETLINE,
};

enum
{
	/* the order of these values match 
	 * __in_type_map and __in_opt_map in extio.c */

	XP_AWK_IN_PIPE,
	XP_AWK_IN_COPROC,
	XP_AWK_IN_FILE,
	XP_AWK_IN_CONSOLE
};

enum
{
	/* the order of these values match 
	 * __out_type_map and __out_opt_map in extio.c */

	XP_AWK_OUT_PIPE,
	XP_AWK_OUT_COPROC,
	XP_AWK_OUT_FILE,
	XP_AWK_OUT_FILE_APPEND,
	XP_AWK_OUT_CONSOLE
};

/* afn (awk function defined with the keyword function) */
typedef struct xp_awk_afn_t xp_awk_afn_t;

typedef struct xp_awk_nde_t xp_awk_nde_t;

typedef struct xp_awk_nde_blk_t       xp_awk_nde_blk_t;
typedef struct xp_awk_nde_grp_t       xp_awk_nde_grp_t;
typedef struct xp_awk_nde_ass_t       xp_awk_nde_ass_t;
typedef struct xp_awk_nde_exp_t       xp_awk_nde_exp_t;
typedef struct xp_awk_nde_cnd_t       xp_awk_nde_cnd_t;
typedef struct xp_awk_nde_pos_t       xp_awk_nde_pos_t;
typedef struct xp_awk_nde_int_t       xp_awk_nde_int_t;
typedef struct xp_awk_nde_real_t      xp_awk_nde_real_t;
typedef struct xp_awk_nde_str_t       xp_awk_nde_str_t;
typedef struct xp_awk_nde_rex_t       xp_awk_nde_rex_t;
typedef struct xp_awk_nde_var_t       xp_awk_nde_var_t;
typedef struct xp_awk_nde_call_t      xp_awk_nde_call_t;
typedef struct xp_awk_nde_getline_t   xp_awk_nde_getline_t;

typedef struct xp_awk_nde_if_t        xp_awk_nde_if_t;
typedef struct xp_awk_nde_while_t     xp_awk_nde_while_t;
typedef struct xp_awk_nde_for_t       xp_awk_nde_for_t;
typedef struct xp_awk_nde_foreach_t   xp_awk_nde_foreach_t;
typedef struct xp_awk_nde_break_t     xp_awk_nde_break_t;
typedef struct xp_awk_nde_continue_t  xp_awk_nde_continue_t;
typedef struct xp_awk_nde_return_t    xp_awk_nde_return_t;
typedef struct xp_awk_nde_exit_t      xp_awk_nde_exit_t;
typedef struct xp_awk_nde_next_t      xp_awk_nde_next_t;
typedef struct xp_awk_nde_nextfile_t  xp_awk_nde_nextfile_t;
typedef struct xp_awk_nde_delete_t    xp_awk_nde_delete_t;
typedef struct xp_awk_nde_print_t     xp_awk_nde_print_t;

struct xp_awk_afn_t
{
	xp_char_t* name;
	xp_size_t name_len;
	xp_size_t nargs;
	xp_awk_nde_t* body;
};

#define XP_AWK_NDE_HDR \
	int type; \
	xp_awk_nde_t* next

struct xp_awk_nde_t
{
	XP_AWK_NDE_HDR;
};

/* XP_AWK_NDE_BLK - block statement including top-level blocks */
struct xp_awk_nde_blk_t
{
	XP_AWK_NDE_HDR;
	xp_size_t nlocals;
	xp_awk_nde_t* body;
};

/* XP_AWK_NDE_GRP - expression group */
struct xp_awk_nde_grp_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* body;
};

/* XP_AWK_NDE_ASS - assignment */
struct xp_awk_nde_ass_t
{
	XP_AWK_NDE_HDR;
	int opcode;
	xp_awk_nde_t* left;
	xp_awk_nde_t* right;
};

/* XP_AWK_NDE_EXP_BIN, XP_AWK_NDE_EXP_UNR, 
 * XP_AWK_NDE_EXP_INCPRE, XP_AW_NDE_EXP_INCPST */
struct xp_awk_nde_exp_t
{
	XP_AWK_NDE_HDR;
	int opcode;
	xp_awk_nde_t* left;
	xp_awk_nde_t* right; /* XP_NULL for UNR, INCPRE, INCPST */
};

/* XP_AWK_NDE_CND */
struct xp_awk_nde_cnd_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* test;
	xp_awk_nde_t* left;
	xp_awk_nde_t* right;
};

/* XP_AWK_NDE_POS - positional - $1, $2, $x, etc */
struct xp_awk_nde_pos_t  
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* val;	
};

/* XP_AWK_NDE_INT */
struct xp_awk_nde_int_t
{
	XP_AWK_NDE_HDR;
	xp_long_t val;
};

/* XP_AWK_NDE_REAL */
struct xp_awk_nde_real_t
{
	XP_AWK_NDE_HDR;
	xp_real_t val;
};

/* XP_AWK_NDE_STR */
struct xp_awk_nde_str_t
{
	XP_AWK_NDE_HDR;
	xp_char_t* buf;
	xp_size_t  len;
};

/* XP_AWK_NDE_REX */
struct xp_awk_nde_rex_t
{
	XP_AWK_NDE_HDR;
	xp_char_t* buf;
	xp_size_t  len;
	void*      code;
};

/* XP_AWK_NDE_NAMED, XP_AWK_NDE_GLOBAL, 
 * XP_AWK_NDE_LOCAL, XP_AWK_NDE_ARG 
 * XP_AWK_NDE_NAMEDIDX, XP_AWK_NDE_GLOBALIDX, 
 * XP_AWK_NDE_LOCALIDX, XP_AWK_NDE_ARGIDX */
struct xp_awk_nde_var_t
{
	XP_AWK_NDE_HDR;
	struct 
	{
		xp_char_t* name;
		xp_size_t name_len;
		xp_size_t idxa;
	} id;
	xp_awk_nde_t* idx; /* XP_NULL for non-XXXXIDX */
};

/* XP_AWK_NDE_BFN, XP_AWK_NDE_AFN */
struct xp_awk_nde_call_t
{
	XP_AWK_NDE_HDR;
	union
	{
		struct
		{
			xp_char_t* name;
			xp_size_t name_len;
		} afn;

		/* minimum information of a built-in function 
		 * needed during run-time. */
		struct
		{
			const xp_char_t* name;
			xp_size_t name_len;
			xp_size_t min_args;
			xp_size_t max_args;
			int (*handler) (void* run);
		} bfn;
		/* xp_awk_bfn_t* bfn; */
	} what;
	xp_awk_nde_t* args;
	xp_size_t nargs;
};

/* XP_AWK_NDE_GETLINE */
struct xp_awk_nde_getline_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* var;
	int in_type; /* XP_AWK_GETLINE_XXX */
	xp_awk_nde_t* in;
};

/* XP_AWK_NDE_IF */
struct xp_awk_nde_if_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* test;
	xp_awk_nde_t* then_part;
	xp_awk_nde_t* else_part; /* optional */
};

/* XP_AWK_NDE_WHILE, XP_AWK_NDE_DOWHILE */
struct xp_awk_nde_while_t
{
	XP_AWK_NDE_HDR; 
	xp_awk_nde_t* test;
	xp_awk_nde_t* body;
};

/* XP_AWK_NDE_FOR */
struct xp_awk_nde_for_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* init; /* optional */
	xp_awk_nde_t* test; /* optional */
	xp_awk_nde_t* incr; /* optional */
	xp_awk_nde_t* body;
};

/* XP_AWK_NDE_FOREACH */
struct xp_awk_nde_foreach_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* test;
	xp_awk_nde_t* body;
};

/* XP_AWK_NDE_BREAK */
struct xp_awk_nde_break_t
{
	XP_AWK_NDE_HDR;
};

/* XP_AWK_NDE_CONTINUE */
struct xp_awk_nde_continue_t
{
	XP_AWK_NDE_HDR;
};

/* XP_AWK_NDE_RETURN */
struct xp_awk_nde_return_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* val; /* optional (no return code if XP_NULL) */	
};

/* XP_AWK_NDE_EXIT */
struct xp_awk_nde_exit_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* val; /* optional (no exit code if XP_NULL) */
};

/* XP_AWK_NDE_NEXT */
struct xp_awk_nde_next_t
{
	XP_AWK_NDE_HDR;
};

/* XP_AWK_NDE_NEXTFILE */
struct xp_awk_nde_nextfile_t
{
	XP_AWK_NDE_HDR;
};

/* XP_AWK_NDE_DELETE */
struct xp_awk_nde_delete_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* var;
};

/* XP_AWK_NDE_PRINT */
struct xp_awk_nde_print_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* args;
	int out_type; /* XP_AWK_OUT_XXX */
	xp_awk_nde_t* out;
};

#ifdef __cplusplus
extern "C" {
#endif

int xp_awk_prnpt (xp_awk_t* awk, xp_awk_nde_t* tree);
int xp_awk_prnptnpt (xp_awk_t* awk, xp_awk_nde_t* tree);

void xp_awk_clrpt (xp_awk_nde_t* tree);

#ifdef __cplusplus
}
#endif

#endif
