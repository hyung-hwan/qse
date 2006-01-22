/*
 * $Id: tree.h,v 1.15 2006-01-22 15:11:17 bacon Exp $
 */

#ifndef _XP_AWK_TREE_H_
#define _XP_AWK_TREE_H_

enum
{
	XP_AWK_NODE_NULL,
	XP_AWK_NODE_BLOCK,
	XP_AWK_NODE_BREAK,
	XP_AWK_NODE_CONTINUE,
	XP_AWK_NODE_RETURN,
	XP_AWK_NODE_EXIT,
	XP_AWK_NODE_NEXT,
	XP_AWK_NODE_NEXTFILE,
	XP_AWK_NODE_ASSIGN,
	XP_AWK_NODE_BINARY,
	XP_AWK_NODE_UNARY,
	XP_AWK_NODE_STR,
	XP_AWK_NODE_NUM,
	XP_AWK_NODE_VAR,
	XP_AWK_NODE_IDX,
	XP_AWK_NODE_CALL,
	XP_AWK_NODE_IF,
	XP_AWK_NODE_WHILE,
	XP_AWK_NODE_DOWHILE,
	XP_AWK_NODE_FOR
};

typedef struct xp_awk_node_t xp_awk_node_t;
typedef struct xp_awk_node_block_t xp_awk_node_block_t;
typedef struct xp_awk_node_assign_t xp_awk_node_assign_t;
typedef struct xp_awk_node_expr_t xp_awk_node_expr_t;
typedef struct xp_awk_node_term_t xp_awk_node_term_t;
typedef struct xp_awk_node_idx_t xp_awk_node_idx_t;
typedef struct xp_awk_node_call_t xp_awk_node_call_t;
typedef struct xp_awk_node_if_t xp_awk_node_if_t;
typedef struct xp_awk_node_while_t xp_awk_node_while_t;
typedef struct xp_awk_node_for_t xp_awk_node_for_t;


struct xp_awk_func_t
{
	xp_char_t* name;
	xp_size_t nargs;
	xp_awk_node_t* body;
};

#define XP_AWK_NODE_HDR \
	int type; \
	xp_awk_node_t* next

struct xp_awk_node_t
{
	XP_AWK_NODE_HDR;
};

struct xp_awk_node_block_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* body;
};

struct xp_awk_node_assign_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* left;
	xp_awk_node_t* right;
};

struct xp_awk_node_expr_t
{
	XP_AWK_NODE_HDR;
	int opcode;
	xp_awk_node_t* left;
	xp_awk_node_t* right;
};

struct xp_awk_node_term_t
{
	XP_AWK_NODE_HDR;
	xp_char_t* value;
};

struct xp_awk_node_idx_t
{
	XP_AWK_NODE_HDR;
	xp_char_t* name;
	xp_awk_node_t* idx;
};

struct xp_awk_node_call_t
{
	XP_AWK_NODE_HDR; /* XP_AWK_NODE_CALL */
	xp_char_t* name;
	xp_awk_node_t* args;
};

struct xp_awk_node_if_t
{
	XP_AWK_NODE_HDR; /* XP_AWK_NODE_IF */
	xp_awk_node_t* test;
	xp_awk_node_t* then_part;
	xp_awk_node_t* else_part; /* optional */
};

struct xp_awk_node_while_t
{
	XP_AWK_NODE_HDR; /* XP_AWK_NODE_WHILE, XP_AWK_NODE_DOWHILE */
	xp_awk_node_t* test;
	xp_awk_node_t* body;
};

struct xp_awk_node_for_t
{
	XP_AWK_NODE_HDR; /* XP_AWK_NODE_FOR */
	xp_awk_node_t* init; /* optional */
	xp_awk_node_t* test; /* optional */
	xp_awk_node_t* incr; /* optional */
	xp_awk_node_t* body;
};

#ifdef __cplusplus
extern "C" {
#endif

void xp_awk_prnpt (xp_awk_node_t* tree);
void xp_awk_clrpt (xp_awk_node_t* tree);

#ifdef __cplusplus
}
#endif

#endif
