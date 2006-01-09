/*
 * $Id: tree.h,v 1.2 2006-01-09 12:51:47 bacon Exp $
 */

#ifndef _XP_AWK_TREE_H_
#define _XP_AWK_TREE_H_

/*
enum TokenType
{
	IF, END, ID, NUM, READ, WRITE, UNTIL, ....
}


enum NodeKind { statement, expression };
enum StatKind { if, repeat, assign, read, write };
enum ExpKind  { op, const, id };
enum ExpType  {  void, integer, boolean };

struct treenode
{
	treenode* child[3]; // 
	treenode* sibling; // <---- next statement...

	int lineno;
	NodeKind node_kind;	
	union {
		statkind s;
		expkind e;
	} kind;
	union {
		TokenType Op;
		int val;
		char* name;
	} attr;


	exptype type; <- for type checking...
};

struct node_t
{
	int type;

};
*/

enum
{
	XP_AWK_NODE_BLOCK,
	XP_AWK_NODE_BREAK,
	XP_AWK_NODE_CONTINUE
};

typedef struct xp_awk_node_t xp_awk_node_t;
typedef struct xp_awk_node_block_t xp_awk_node_block_t;
typedef struct xp_awk_node_if_t xp_awk_node_if_t;

#define XP_AWK_NODE_HDR \
	int type; \
	xp_awk_node_t* sbls

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

struct xp_awk_node_if_t
{
	XP_AWK_NODE_HDR;
	xp_awk_node_t* cond;
	xp_awk_node_t* if_part;
	xp_awk_node_t* else_part;
};

#endif
