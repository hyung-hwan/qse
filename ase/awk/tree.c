/*
 * $Id: tree.c,v 1.1 2006-01-14 14:09:52 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#include <xp/bas/stdio.h>

static xp_char_t __binop_char[] =
{
	XP_CHAR('+'),
	XP_CHAR('-'),
	XP_CHAR('*'),
	XP_CHAR('/'),
	XP_CHAR('%')
};

static void __print_tabs (int depth);
static int __print_expr_node (xp_awk_node_t* node);
static int __print_expr_node_list (xp_awk_node_t* tree);
static void __print_statements (xp_awk_node_t* tree, int depth);

static void __print_tabs (int depth)
{
	int i;
	for (i = 0; i < depth; i++) xp_printf (XP_TEXT("\t"));
}

static int __print_expr_node (xp_awk_node_t* node)
{
	switch (node->type) {
	case XP_AWK_NODE_ASSIGN:
		xp_printf (XP_TEXT("%s = "), ((xp_awk_node_assign_t*)node)->left);
		if (__print_expr_node (((xp_awk_node_assign_t*)node)->right) == -1) return -1;
		xp_assert ((((xp_awk_node_assign_t*)node)->right)->next == XP_NULL);
		break;

	case XP_AWK_NODE_BINARY:
		xp_printf (XP_TEXT("("));
		if (__print_expr_node (((xp_awk_node_expr_t*)node)->left) == -1) return -1;
		xp_assert ((((xp_awk_node_expr_t*)node)->left)->next == XP_NULL);
		xp_printf (XP_TEXT(" %c "), __binop_char[((xp_awk_node_expr_t*)node)->opcode]);
		if (__print_expr_node (((xp_awk_node_expr_t*)node)->right) == -1) return -1;
		xp_assert ((((xp_awk_node_expr_t*)node)->right)->next == XP_NULL);
		xp_printf (XP_TEXT(")"));
		break;

	case XP_AWK_NODE_UNARY:
// TODO:
		xp_printf (XP_TEXT("unary basic expression\n"));
		break;

	case XP_AWK_NODE_STR:
		xp_printf (XP_TEXT("\"%s\""), ((xp_awk_node_term_t*)node)->value);
		break;

	case XP_AWK_NODE_NUM:
		xp_printf (XP_TEXT("%s"), ((xp_awk_node_term_t*)node)->value);
		break;

	case XP_AWK_NODE_VAR:
		xp_printf (XP_TEXT("%s"), ((xp_awk_node_term_t*)node)->value);
		break;

	case XP_AWK_NODE_CALL:
		xp_printf (XP_TEXT("%s ("), ((xp_awk_node_call_t*)node)->name);
		if (__print_expr_node_list (((xp_awk_node_call_t*)node)->args) == -1) return -1;
		xp_printf (XP_TEXT(")"));
		break;

	default:
		return -1;
	}

	return 0;
}

static int __print_expr_node_list (xp_awk_node_t* tree)
{
	xp_awk_node_t* p = tree;

	while (p != XP_NULL) {
		if (__print_expr_node (p) == -1) return -1;
		p = p->next;
		if (p != XP_NULL) xp_printf (XP_TEXT(","));
	}

	return 0;
}

static void __print_statements (xp_awk_node_t* tree, int depth)
{
	xp_awk_node_t* p = tree;

	while (p != XP_NULL) {

		switch (p->type) {
		case XP_AWK_NODE_BLOCK:
			__print_tabs (depth);
			xp_printf (XP_TEXT("{\n"));
			__print_statements (((xp_awk_node_block_t*)p)->body, depth + 1);	
			__print_tabs (depth);
			xp_printf (XP_TEXT("}\n"));
			break;

		case XP_AWK_NODE_BREAK:
			__print_tabs (depth);
			xp_printf (XP_TEXT("break;\n"));
			break;

		case XP_AWK_NODE_CONTINUE:
			__print_tabs (depth);
			xp_printf (XP_TEXT("continue;\n"));
			break;

		case XP_AWK_NODE_RETURN:
			__print_tabs (depth);
			xp_printf (XP_TEXT("return "));
			xp_assert (((xp_awk_node_block_t*)p)->body->next == XP_NULL);
			if (__print_expr_node(((xp_awk_node_block_t*)p)->body) == 0) {
				xp_printf (XP_TEXT(";\n"));
			}
			else {
				xp_awk_node_block_t* x = (xp_awk_node_block_t*)p;
				xp_printf (XP_TEXT("***INTERNAL ERROR: unknown node type - %d\n"), x->type);
			}
			break;

		case XP_AWK_NODE_EXIT:
			__print_tabs (depth);
			xp_printf (XP_TEXT("exit "));
			xp_assert (((xp_awk_node_block_t*)p)->body->next == XP_NULL);
			if (__print_expr_node(((xp_awk_node_block_t*)p)->body) == 0) {
				xp_printf (XP_TEXT(";\n"));
			}
			else {
				xp_awk_node_block_t* x = (xp_awk_node_block_t*)p;
				xp_printf (XP_TEXT("***INTERNAL ERROR: unknown node type - %d\n"), x->type);
			}
			break;

		default:
			__print_tabs (depth);
			if (__print_expr_node(p) == 0) {
				xp_printf (XP_TEXT(";\n"));
			}
			else {
				xp_printf (XP_TEXT("***INTERNAL ERROR: unknown type - %d\n"), p->type);
			}
		}

		p = p->next;
	}
}

void xp_awk_prnpt (xp_awk_node_t* tree)
{
	__print_statements (tree, 0);
}

void xp_awk_clrpt (xp_awk_node_t* tree)
{
	xp_awk_node_t* p = tree;
	xp_awk_node_t* next;

	while (p != XP_NULL) {
		next = p->next;

		switch (p->type) {
		case XP_AWK_NODE_BLOCK:
			xp_awk_clrpt (((xp_awk_node_block_t*)p)->body);
			xp_free (p);
			break;

		case XP_AWK_NODE_BREAK:
		case XP_AWK_NODE_CONTINUE:
			xp_free (p);
			break;
		
		case XP_AWK_NODE_RETURN:
		case XP_AWK_NODE_EXIT:
			xp_awk_clrpt (((xp_awk_node_block_t*)p)->body);
			xp_free (p);
			break;

		case XP_AWK_NODE_ASSIGN:
			xp_free (((xp_awk_node_assign_t*)p)->left);
			xp_awk_clrpt (((xp_awk_node_assign_t*)p)->right);
			xp_free (p);
			break;

		case XP_AWK_NODE_BINARY:
			xp_assert ((((xp_awk_node_expr_t*)p)->left)->next == XP_NULL);
			xp_assert ((((xp_awk_node_expr_t*)p)->right)->next == XP_NULL);

			xp_awk_clrpt (((xp_awk_node_expr_t*)p)->left);
			xp_awk_clrpt (((xp_awk_node_expr_t*)p)->right);
			xp_free (p);
			break;

		case XP_AWK_NODE_UNARY:
// TODO: clear unary expression...
			xp_free (p);
			break;

		case XP_AWK_NODE_STR:
		case XP_AWK_NODE_NUM:
		case XP_AWK_NODE_VAR:
			xp_free (((xp_awk_node_term_t*)p)->value);
			xp_free (p);
			break;

		case XP_AWK_NODE_CALL:
			xp_free (((xp_awk_node_call_t*)p)->name);
			xp_awk_clrpt (((xp_awk_node_call_t*)p)->args);
			xp_free (p);
			break;

		default:
			xp_assert (XP_TEXT("shoud not happen") == XP_TEXT(" here"));
		}

		p = next;
	}
}
