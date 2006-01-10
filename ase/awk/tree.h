/*
 * $Id: tree.h,v 1.3 2006-01-10 13:57:54 bacon Exp $
 */

#ifndef _XP_AWK_TREE_H_
#define _XP_AWK_TREE_H_

enum
{
	XP_AWK_NODE_BLOCK,
	XP_AWK_NODE_BREAK,
	XP_AWK_NODE_CONTINUE
};

typedef struct xp_awk_node_t xp_awk_node_t;
typedef struct xp_awk_node_block_t xp_awk_node_block_t;
typedef struct xp_awk_node_expr_t xp_awk_node_expr_t;
typedef struct xp_awk_node_term_t xp_awk_node_term_t;
typedef struct xp_awk_node_if_t xp_awk_node_if_t;
typedef struct xp_awk_node_while_t xp_awk_node_while_t;
typedef struct xp_awk_node_do_t xp_awk_node_do_t;

#define XP_AWK_NODE_HDR \
	int type; \
	xp_awk_node_t* next

struct xp_awk_node_t
{
	XP_AWK_NODE_HDR;
};

/*
struct xp_awk_node_plain_t
{
	XP_AWK_NODE_HDR;
};
*/

struct xp_awk_node_block_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* body;
};

struct xp_awk_node_expr_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* left;
	xp_awk_node_t* right;
};

struct xp_awk_node_term_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* value;
};

struct xp_awk_node_if_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* test;
	xp_awk_node_t* then_part;
	xp_awk_node_t* else_part;
};

struct xp_awk_node_while_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* test;
	xp_awk_node_t* body;
};

struct xp_awk_node_do_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* body;
	xp_awk_node_t* test;
};

#endif
