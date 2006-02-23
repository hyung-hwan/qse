/*
 * $Id: run.c,v 1.2 2006-02-23 15:37:34 bacon Exp $
 */

#include <xp/awk/awk.h>
#ifndef __STAND_ALONE
#include <xp/bas/assert.h>
#endif

static int __run_block (xp_awk_t* awk, xp_awk_node_t* node);
static int __run_statement (xp_awk_t* awk, xp_awk_node_t* node);
static int __run_assignment (xp_awk_t* awk, xp_awk_node_assign_t* node);
static int __eval_expr (xp_awk_t* awk, xp_awk_node_t* node);

int xp_awk_run (xp_awk_t* awk)
{
	if (awk->tree.begin != XP_NULL) {
		if (__run_block(awk, awk->tree.begin) == -1) return -1;
	}

	if (awk->tree.end != XP_NULL) {
		if (__run_block(awk, awk->tree.end) == -1) return -1;
	}

	return 0;
}

static int __run_block (xp_awk_t* awk, xp_awk_node_t* node)
{
	xp_assert (node->type == XP_AWK_NODE_BLOCK);

	xp_awk_node_t* p = node;

	while (p != XP_NULL) {

		if (__run_statement (awk, p) == -1) return -1;
		p = p->next;
	}
	
	return 0;
}

static int __run_statement (xp_awk_t* awk, xp_awk_node_t* node)
{
	switch (node->type) {
	case XP_AWK_NODE_NULL:
		/* do nothing */
		break;

	case XP_AWK_NODE_BLOCK:
		if (__run_block(awk, node) == -1) return -1;
		break;

	case XP_AWK_NODE_IF:
		break;
	case XP_AWK_NODE_WHILE:
		break;
	case XP_AWK_NODE_DOWHILE:
		break;
	case XP_AWK_NODE_FOR:
		break;

	case XP_AWK_NODE_BREAK:
		break;
	case XP_AWK_NODE_CONTINUE:
		break;

	case XP_AWK_NODE_RETURN:
		break;

	case XP_AWK_NODE_EXIT:
		break;

	case XP_AWK_NODE_NEXT:
		break;

	case XP_AWK_NODE_NEXTFILE:
		break;

	case XP_AWK_NODE_ASSIGN:
		if (__run_assignment (awk, node) == -1) return -1;
		break;

	case XP_AWK_NODE_NUM:
		break;
	}

	return 0;
}

static int __run_assignment (xp_awk_t* awk, xp_awk_node_assign_t* node)
{
	if (node->type == XP_AWK_NODE_NAMED) {
		xp_awk_node_t* right = __eval_expr (awk, node->right);
		if (right == NULL) return -1;

		if (xp_awk_hash_insert(awk->run.named, right) == -1) {
			awk->errnum = XP_AWK_ENOMEM;
			return -1;
		}
	}
	else if (node->type == XP_AWK_NODE_GLOBAL) {
	}
	else if (node->type == XP_AWK_NODE_LOCAL) {
	}
	else if (node->type == XP_AWK_NODE_ARG) {
	}

	else if (node->type == XP_AWK_NODE_NAMEIDX) {
	}
	else if (node->type == XP_AWK_NODE_GLOBALIDX) {
	}
	else if (node->type == XP_AWK_NODE_LOCALIDX) {
	}
	else if (node->type == XP_AWK_NODE_ARGIDX) {
	}

	esle {
		awk->errnum = XP_AWK_EINTERNAL;
		return -1;
	}

	return 0;
}

static int __eval_expr (xp_awk_t* awk, xp_awk_node_t* node)
{
	return -1;
}
