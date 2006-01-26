/*
 * $Id: run.c,v 1.1 2006-01-26 15:35:20 bacon Exp $
 */

#include <xp/awk/awk.h>
#ifndef __STAND_ALONE
#include <xp/bas/assert.h>
#endif

static int __run_block (xp_awk_t* awk, xp_awk_node_t* node);
static int __run_statement (xp_awk_t* awk, xp_awk_node_t* node);

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
	return -1;
}

static int __run_statement (xp_awk_t* awk, xp_awk_node_t* node)
{
	switch (node->type) {
	case XP_AWK_NODE_BLOCK:
		if (__run_block(awk, node) == -1) return -1;
		break;
	case XP_AWK_NODE_BREAK:
		break;
	case XP_AWK_NODE_CONTINUE:
		break;
	}

	return 0;
}
