/*
 * $Id: tree.h,v 1.23 2006-03-03 11:45:45 bacon Exp $
 */

#ifndef _XP_AWK_TREE_H_
#define _XP_AWK_TREE_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/aw/awk.h> instead
#endif

enum
{
	XP_AWK_NDE_NULL,
	XP_AWK_NDE_BLOCK,
	XP_AWK_NDE_BREAK,
	XP_AWK_NDE_CONTINUE,
	XP_AWK_NDE_RETURN,
	XP_AWK_NDE_EXIT,
	XP_AWK_NDE_NEXT,
	XP_AWK_NDE_NEXTFILE,
	XP_AWK_NDE_ASS,
	XP_AWK_NDE_BINARY,
	XP_AWK_NDE_UNARY,
	XP_AWK_NDE_STR,
	XP_AWK_NDE_NUM,
	XP_AWK_NDE_NAMED,
	XP_AWK_NDE_NAMEDIDX,
	XP_AWK_NDE_GLOBAL,
	XP_AWK_NDE_GLOBALIDX,
	XP_AWK_NDE_LOCAL,
	XP_AWK_NDE_LOCALIDX,
	XP_AWK_NDE_ARG,
	XP_AWK_NDE_ARGIDX,
	XP_AWK_NDE_POS,
	XP_AWK_NDE_CALL,
	XP_AWK_NDE_IF,
	XP_AWK_NDE_WHILE,
	XP_AWK_NDE_DOWHILE,
	XP_AWK_NDE_FOR
};

typedef struct xp_awk_func_t xp_awk_func_t;

typedef struct xp_awk_nde_t       xp_awk_nde_t;
typedef struct xp_awk_nde_sgv_t   xp_awk_nde_sgv_t;
typedef struct xp_awk_nde_block_t xp_awk_nde_block_t;
typedef struct xp_awk_nde_ass_t   xp_awk_nde_ass_t;
typedef struct xp_awk_nde_expr_t  xp_awk_nde_expr_t;
typedef struct xp_awk_nde_term_t  xp_awk_nde_term_t;
typedef struct xp_awk_nde_var_t   xp_awk_nde_var_t;
typedef struct xp_awk_nde_idx_t   xp_awk_nde_idx_t;
typedef struct xp_awk_nde_pos_t   xp_awk_nde_pos_t;
typedef struct xp_awk_nde_call_t  xp_awk_nde_call_t;
typedef struct xp_awk_nde_if_t    xp_awk_nde_if_t;
typedef struct xp_awk_nde_while_t xp_awk_nde_while_t;
typedef struct xp_awk_nde_for_t   xp_awk_nde_for_t;

struct xp_awk_func_t
{
	xp_char_t* name;
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

/* XP_AWK_NDE_RETURN, XP_AWK_NDE_EXIT, XP_AWK_NDE_POS */
struct xp_awk_nde_sgv_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* value;	
};

struct xp_awk_nde_block_t
{
	XP_AWK_NDE_HDR;
	xp_size_t nlocals;
	xp_awk_nde_t* body;
};

struct xp_awk_nde_ass_t
{
	XP_AWK_NDE_HDR;
	xp_awk_nde_t* left;
	xp_awk_nde_t* right;
};

struct xp_awk_nde_expr_t
{
	XP_AWK_NDE_HDR;
	int opcode;
	xp_awk_nde_t* left;
	xp_awk_nde_t* right;
};

struct xp_awk_nde_term_t
{
	XP_AWK_NDE_HDR;
	xp_char_t* value;
};

struct xp_awk_nde_var_t
{
	XP_AWK_NDE_HDR;
	struct // union
	{
		xp_char_t* name;
		xp_size_t idxa;
	} id;
};

struct xp_awk_nde_idx_t
{
	XP_AWK_NDE_HDR;
	struct // union
	{
		xp_char_t* name;
		xp_size_t idxa;
	} id;
	xp_awk_nde_t* idx;
};


struct xp_awk_nde_call_t
{
	XP_AWK_NDE_HDR; /* XP_AWK_NDE_CALL */
	xp_char_t* name;
	xp_awk_nde_t* args;
};

struct xp_awk_nde_if_t
{
	XP_AWK_NDE_HDR; /* XP_AWK_NDE_IF */
	xp_awk_nde_t* test;
	xp_awk_nde_t* then_part;
	xp_awk_nde_t* else_part; /* optional */
};

struct xp_awk_nde_while_t
{
	XP_AWK_NDE_HDR; /* XP_AWK_NDE_WHILE, XP_AWK_NDE_DOWHILE */
	xp_awk_nde_t* test;
	xp_awk_nde_t* body;
};

struct xp_awk_nde_for_t
{
	XP_AWK_NDE_HDR; /* XP_AWK_NDE_FOR */
	xp_awk_nde_t* init; /* optional */
	xp_awk_nde_t* test; /* optional */
	xp_awk_nde_t* incr; /* optional */
	xp_awk_nde_t* body;
};

#ifdef __cplusplus
extern "C" {
#endif

void xp_awk_prnpt (xp_awk_nde_t* tree);
void xp_awk_clrpt (xp_awk_nde_t* tree);

#ifdef __cplusplus
}
#endif

#endif
