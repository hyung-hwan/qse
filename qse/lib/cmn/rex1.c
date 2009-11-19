/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/rex.h>
#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>
#include "mem.h"

#define OCC_MAX QSE_TYPE_MAX(qse_size_t)

struct qse_rex_t
{
	QSE_DEFINE_COMMON_FIELDS (rex)

	int errnum;
	qse_rex_node_t* code;
};

typedef struct comp_t comp_t;
struct comp_t
{
	qse_rex_t* rex;

	qse_cstr_t re;

	const qse_char_t* ptr;
	const qse_char_t* end;

	struct
	{
		enum
		{
			CT_NORMAL,
			CT_SPECIAL
		} type;
		qse_cint_t value;
		int escaped;
	} c;

	qse_size_t gdepth; /* group depth */
	qse_rex_node_t* start;
};

typedef struct exec_t exec_t;
struct exec_t
{
	qse_rex_t* rex;

	struct
	{
		const qse_char_t* ptr;
		const qse_char_t* end;
	} str;

	struct
	{
		const qse_char_t* ptr;
		const qse_char_t* end;
	} sub;

	struct
	{
		int active;
		int pending;
		qse_lda_t set[2]; /* candidate arrays */
	} cand;

	qse_size_t nmatches;
	const qse_char_t* matchend; /* 1 character past the match end */
};

typedef struct pair_t pair_t;
struct pair_t
{
	qse_rex_node_t* head;
	qse_rex_node_t* tail;
};

typedef struct group_t group_t;
struct group_t
{
	qse_rex_node_t* node;
	qse_size_t occ;
	group_t* next;
};

typedef struct cand_t cand_t;
struct cand_t
{
	qse_rex_node_t*   node;
	qse_size_t        occ;

	/* the stack of groups that this candidate belongs to. 
	 * it is in the singliy linked list form */
	group_t*          group;

	/* match pointer. the number of character advancement 
	 * differs across various node types. BOL and EOL don't advance to
	 * the next character on match while ANYCHAR and CHAR do on match.
	 * therefore, the match pointer is managed per candidate basis. */
	const qse_char_t* mptr; 
};

qse_rex_t* qse_rex_open (qse_mmgr_t* mmgr, qse_size_t xtn, void* code)
{
	qse_rex_t* rex;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	rex = (qse_rex_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_rex_t) + xtn);
	if (rex == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (rex, 0, QSE_SIZEOF(*rex));
	rex->mmgr = mmgr;
	/* TODO: must duplicate? */
	rex->code = code;

	return rex;
}

static void freenode (qse_rex_node_t* node, qse_mmgr_t* mmgr)
{
	if (node->id == QSE_REX_NODE_CHARSET)
	{
		// TODO: 
		QSE_MMGR_FREE (mmgr, node->u.cs);
	}

	QSE_MMGR_FREE (mmgr, node);
}

static void freeallnodes (qse_rex_node_t* start)
{
	qse_rex_node_t* x, * y;
	qse_mmgr_t* mmgr;

	QSE_ASSERT (start->id == QSE_REX_NODE_START);

	mmgr = start->u.s.mmgr;
	x = start->u.s.link;
	while (x != QSE_NULL)
	{
		y = x->link;
		freenode (x, mmgr);
		x = y;
	}

	QSE_MMGR_FREE (mmgr, start);
}

void qse_rex_close (qse_rex_t* rex)
{
	if (rex->code != QSE_NULL) freeallnodes (rex->code);
	QSE_MMGR_FREE (rex->mmgr, rex);
}

static qse_rex_node_t* newnode (comp_t* c, qse_rex_node_id_t id)
{
	qse_rex_node_t* node;

	node = (qse_rex_node_t*) 
		QSE_MMGR_ALLOC (c->rex->mmgr, QSE_SIZEOF(qse_rex_node_t));
	if (node == QSE_NULL) 
	{
// TODO set error code
		return QSE_NULL;
	}

	QSE_MEMSET (node, 0, QSE_SIZEOF(*node));
	node->id = id;

	if (c->start != QSE_NULL) 
	{
		QSE_ASSERT (c->start->id == QSE_REX_NODE_START);
		node->link = c->start->u.s.link;
		c->start->u.s.link = node;
	}

	return node;
}

static qse_rex_node_t* newstartnode (comp_t* c)
{
	qse_rex_node_t* n = newnode (c, QSE_REX_NODE_START);
	if (n != QSE_NULL) 
	{
		n->u.s.mmgr = c->rex->mmgr;
		n->u.s.link = QSE_NULL;
	}
	return n;
}

static qse_rex_node_t* newendnode (comp_t* c)
{
	return newnode (c, QSE_REX_NODE_END);
}

static qse_rex_node_t* newnopnode (comp_t* c)
{
	return newnode (c, QSE_REX_NODE_NOP);
}

static qse_rex_node_t* newgroupnode (comp_t* c, qse_rex_node_t* head)
{
	qse_rex_node_t* n = newnode (c, QSE_REX_NODE_GROUP);
	if (n != QSE_NULL) n->u.g.head = head;
	return n;
}

static qse_rex_node_t* newgroupendnode (comp_t* c, qse_rex_node_t* group)
{
	qse_rex_node_t* n = newnode (c, QSE_REX_NODE_GROUPEND);
	if (n != QSE_NULL) n->u.ge.group = group;
	return n;
}

static qse_rex_node_t* newcharnode (comp_t* c, qse_char_t ch)
{
	qse_rex_node_t* n = newnode (c, QSE_REX_NODE_CHAR);
	if (n != QSE_NULL) n->u.c = ch;
	return n;
}

static qse_rex_node_t* newbranchnode (
	comp_t* c, qse_rex_node_t* left, qse_rex_node_t* right)
{
	qse_rex_node_t* n = newnode (c, QSE_REX_NODE_BRANCH);
	if (n != QSE_NULL)
	{
		n->u.b.left = left; 
		n->u.b.right = right;
	}
	return n;
}

#define CHECK_END(builder) \
	do { \
		if (builder->ptr >= builder->ptn.end) \
		{ \
			builder->errnum = QSE_REX_EEND; \
			return -1; \
		} \
	} while(0)

#define IS_HEX(c) \
	((c >= QSE_T('0') && c <= QSE_T('9')) || \
	 (c >= QSE_T('A') && c <= QSE_T('F')) || \
	 (c >= QSE_T('a') && c <= QSE_T('f')))

#define HEX_TO_NUM(c) \
	((c >= QSE_T('0') && c <= QSE_T('9'))? c-QSE_T('0'):  \
	 (c >= QSE_T('A') && c <= QSE_T('F'))? c-QSE_T('A')+10: \
	                                       c-QSE_T('a')+10)

static int getc (comp_t* com)
{
	if (com->ptr >= com->end)
	{
		com->c.type = CT_NORMAL;
		com->c.value = QSE_CHAR_EOF;
		com->c.escaped = 0;
		return 0;
	}

	com->c.type = CT_NORMAL;
	com->c.value = *com->ptr++;
	com->c.escaped = QSE_FALSE;

	if (com->c.value == QSE_T('\\'))
	{	       
		qse_char_t c;

		CHECK_END (builder);
		c = *com->ptr++;

		if (c == QSE_T('n')) c = QSE_T('\n');
		else if (c == QSE_T('r')) c = QSE_T('\r');
		else if (c == QSE_T('t')) c = QSE_T('\t');
		else if (c == QSE_T('f')) c = QSE_T('\f');
		else if (c == QSE_T('b')) c = QSE_T('\b');
		else if (c == QSE_T('v')) c = QSE_T('\v');
		else if (c == QSE_T('a')) c = QSE_T('\a');
		else if (c >= QSE_T('0') && c <= QSE_T('7')) 
		{
			qse_char_t cx;

			c = c - QSE_T('0');

			CHECK_END (builder);
			cx = *com->ptr++;
			if (cx >= QSE_T('0') && cx <= QSE_T('7'))
			{
				c = c * 8 + cx - QSE_T('0');

				CHECK_END (builder);
				cx = *com->ptr++;
				if (cx >= QSE_T('0') && cx <= QSE_T('7'))
				{
					c = c * 8 + cx - QSE_T('0');
				}
			}
		}
		else if (c == QSE_T('x')) 
		{
			qse_char_t cx;

			CHECK_END (builder);
			cx = *com->ptr++;
			if (IS_HEX(cx))
			{
				c = HEX_TO_NUM(cx);

				CHECK_END (builder);
				cx = *com->ptr++;
				if (IS_HEX(cx))
				{
					c = c * 16 + HEX_TO_NUM(cx);
				}
			}
		}
	#ifdef QSE_CHAR_IS_WCHAR
		else if (c == QSE_T('u') && QSE_SIZEOF(qse_char_t) >= 2) 
		{
			qse_char_t cx;

			CHECK_END (builder);
			cx = *com->ptr++;
			if (IS_HEX(cx))
			{
				qse_size_t i;

				c = HEX_TO_NUM(cx);

				for (i = 0; i < 3; i++)
				{
					CHECK_END (builder);
					cx = *com->ptr++;

					if (!IS_HEX(cx)) break;
					c = c * 16 + HEX_TO_NUM(cx);
				}
			}
		}
		else if (c == QSE_T('U') && QSE_SIZEOF(qse_char_t) >= 4) 
		{
			qse_char_t cx;

			CHECK_END (builder);
			cx = *com->ptr++;
			if (IS_HEX(cx))
			{
				qse_size_t i;

				c = HEX_TO_NUM(cx);

				for (i = 0; i < 7; i++)
				{
					CHECK_END (builder);
					cx = *com->ptr++;

					if (!IS_HEX(cx)) break;
					c = c * 16 + HEX_TO_NUM(cx);
				}
			}
		}
	#endif

		com->c.value = c;
		com->c.escaped = QSE_TRUE;

		return 0;
	}
	else
	{
		if (level == LEVEL_TOP)
		{
			if (com->c.value == QSE_T('[') ||
			    com->c.value == QSE_T('|') ||
			    com->c.value == QSE_T('^') ||
			    com->c.value == QSE_T('$') ||
			    (!(com->option & QSE_REX_BUILD_NOBOUND) &&
			     com->c.value == QSE_T('{')) ||
			    com->c.value == QSE_T('+') ||
			    com->c.value == QSE_T('?') ||
			    com->c.value == QSE_T('*') ||
			    com->c.value == QSE_T('.') ||
			    com->c.value == QSE_T('(') ||
			    com->c.value == QSE_T(')')) 
			{
				com->c.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_CHARSET)
		{
			if (com->c.value == QSE_T(']')) 
			{
				com->c.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_RANGE)
		{
			if (com->c.value == QSE_T(',') ||
			    com->c.value == QSE_T('}')) 
			{
				com->c.type = CT_SPECIAL;
			}
		}
	}
#if 0
	com->c = (com->ptr < com->end)? *com->ptr++: QSE_CHAR_EOF;
if (com->c == QSE_CHAR_EOF)
qse_printf (QSE_T("getc => <EOF>\n"));
else qse_printf (QSE_T("getc => %c\n"), com->c);
#endif
	return 0;
}

static qse_rex_node_t* comp0 (comp_t* c, qse_rex_node_t* ge);

static qse_rex_node_t* comp2 (comp_t* c)
{
	qse_rex_node_t* n;

	switch (c->c.value)
	{
		case QSE_T('('):
		{
			qse_rex_node_t* x, * ge;
			
			n = newgroupnode (c, QSE_NULL);
			if (n == QSE_NULL) return QSE_NULL;

			ge = newgroupendnode (c, n);
			if (ge == QSE_NULL) 
			{
				// free n
				return QSE_NULL;
			}

			if (getc(c) <= -1)
			{
				// freere (ge);
				// freere (n);
				return QSE_NULL;
			}

			c->gdepth++;
			x = comp0 (c, ge);
			if (x == QSE_NULL)
			{
				// freere (ge);
				// freere (n);
				return QSE_NULL;
			}

			if (c->c.value != QSE_T(')')) 
			{
qse_printf (QSE_T("expecting )\n"));
				// UNBALANCED PAREN.
				// freere (x);
				// freere (n);
				return QSE_NULL;
			}

			c->gdepth--;
			if (getc(c) <= -1)
			{
				// freere (x);
				// freere (n);
				return QSE_NULL;
			}

			n->u.g.head = x;
			break;
		}
			
		case QSE_T('.'):
			n = newnode (c, QSE_REX_NODE_ANYCHAR);
			if (n == QSE_NULL) return QSE_NULL;
			if (getc(c) <= -1)
			{
				// TODO: error handling..
				return QSE_NULL;
			}
			break;

		case QSE_T('^'):
			n = newnode (c, QSE_REX_NODE_BOL);
			if (n == QSE_NULL) return QSE_NULL;
			if (getc(c) <= -1)
			{
				// TODO: error handling..
				return QSE_NULL;
			}
			break;

		case QSE_T('$'):
			n = newnode (c, QSE_REX_NODE_EOL);
			if (n == QSE_NULL) return QSE_NULL;
			if (getc(c) <= -1)
			{
				// TODO: error handling..
				return QSE_NULL;
			}
			break;


		/*
		case QSE_T('['):
			....
		*/

		default:
			/* normal character */
			n = newcharnode (c, c->c.value);
			if (n == QSE_NULL) return QSE_NULL;
			if (getc(c) <= -1)
			{
				// TODO: error handling..
				return QSE_NULL;
			}
			break;
	}

	/* handle the occurrence specifier, if any */
	switch (c->c)
	{
		case QSE_T('?'):
			n->occ.min = 0;
			n->occ.max = 1;
			if (getc(c) <= -1)
			{
				// TODO: error handling..
				//free n
				return QSE_NULL;
			}
			break;
			
		case QSE_T('*'):
			n->occ.min = 0;
			n->occ.max = OCC_MAX;
			if (getc(c) <= -1)
			{
				// TODO: error handling..
				//free n
				return QSE_NULL;
			}
			break;

		case QSE_T('+'):
			n->occ.min = 1;
			n->occ.max = OCC_MAX;
			if (getc(c) <= -1)
			{
				// TODO: error handling..
				//free n
				return QSE_NULL;
			}
			break;

		/*
		case QSE_T('{'):
			 // TODO --------------
			break;
		*/

		default:
			n->occ.min = 1;
			n->occ.max = 1;
	}

	return n;
}

static qse_rex_node_t* comp1 (comp_t* c, pair_t* pair)
{
	pair->head = newnopnode (c);
	if (pair->head == QSE_NULL) return QSE_NULL;

	pair->tail = pair->head;

	while (c->c.value != QSE_T('|') && c->c.value != QSE_CHAR_EOF && 
	       !(c->gdepth >= 0 && c->c.value == QSE_T(')')))
	{
		qse_rex_node_t* tmp = comp2 (c);
		if (tmp == QSE_NULL) 
		{
			/* TODO: free all nodes... from head down to tail... */
			return QSE_NULL;
		}

		pair->tail->next = tmp;
		pair->tail = tmp;
	}

	return pair->head;
}

static qse_rex_node_t* comp0 (comp_t* c, qse_rex_node_t* ge)
{
	qse_rex_node_t* left, * right, * tmp;
	pair_t xpair;

	left = comp1 (c, &xpair);
	if (left == QSE_NULL) return QSE_NULL;
	xpair.tail->next = ge;

	while (c->c.value == QSE_T('|'))
	{
		if (getc (c) <= -1) 
		{
			//freere (left);
			return QSE_NULL;
		}
		
		right = comp1 (c, &xpair);
		if (right == QSE_NULL)
		{
			//freere (l and r);
			return QSE_NULL;
		}
		xpair.tail->next = ge;

		tmp = newbranchnode (c, left, right);
		if (tmp == QSE_NULL)
		{
			//freere (left and right);
			return QSE_NULL;
		}

		left = tmp;
	} 

	return left;
}

qse_rex_node_t* qse_rex_comp (
	qse_rex_t* rex, const qse_char_t* ptr, qse_size_t len)
{
	comp_t c;

	if (rex->code != QSE_NULL)
	{
		freeallnodes (rex->code);
		rex->code = QSE_NULL;
	}

	c.rex = rex;
	c.re.ptr = ptr;
	c.re.len = len;	

	c.ptr = ptr;
	c.end = ptr + len;

	c.c.value = QSE_CHAR_EOF;

	c.gdepth = 0;
	c.start = QSE_NULL;

	if (getc(&c) <= -1) return QSE_NULL;

	c.start = newstartnode (&c);
	if (c.start != QSE_NULL)
	{
		qse_rex_node_t* end;
		end = newendnode (&c);
		if (end == QSE_NULL)
		{
			freenode (c.start, c.rex->mmgr);
			c.start = QSE_NULL;
		}
		else
		{
			qse_rex_node_t* tmp;
			/*tmp = comp0 (&c, QSE_NULL);*/
			tmp = comp0 (&c, end);
			if (tmp == QSE_NULL) 
			{
				/*freenode (c.start, c.rex->mmgr);*/
				freeallnodes (c.start);
				c.start = QSE_NULL;
			}
			else 
			{
qse_printf (QSE_T("start has tmp...\n"));
				c.start->next = tmp;
			}
		}
	}

	rex->code = c.start;
	return rex->code;
}

static group_t* dupgroups (exec_t* e, group_t* g)
{
	group_t* yg, * xg = QSE_NULL;

	QSE_ASSERT (g != QSE_NULL);

	if (g->next != QSE_NULL) 
	{
		/* TODO: make it non recursive or 
		 *       implement stack overflow protection */
		xg = dupgroups (e, g->next);
		if (xg == QSE_NULL) return QSE_NULL;
	}

	yg = (group_t*) QSE_MMGR_ALLOC (e->rex->mmgr, QSE_SIZEOF(*g));
	if (yg == QSE_NULL)
	{
		/* TODO: freegroups (xg); */
		/* TODO: set error info */
		return QSE_NULL;
	}

	QSE_MEMCPY (yg, g, QSE_SIZEOF(*yg));
	yg->next = xg;

	return yg;
}

static void freegroup (exec_t* e, group_t* group)
{
	QSE_ASSERT (group != QSE_NULL);
	QSE_MMGR_FREE (e->rex->mmgr, group);
}

static void freegroups (exec_t* e, group_t* group)
{
	group_t* next;

	while (group != QSE_NULL)
	{
		next = group->next;
		freegroup (e, group);
		group = next;
	}
}

static group_t* pushgroup (exec_t* e, group_t* group, qse_rex_node_t* newgn)
{
	group_t* newg;

	QSE_ASSERT (newgn->id == QSE_REX_NODE_GROUP);

	newg = (group_t*) QSE_MMGR_ALLOC (e->rex->mmgr, QSE_SIZEOF(*newg));
	if (newg == QSE_NULL)
	{
		/* TODO: set error info */
		return QSE_NULL;
	}

	newg->node = newgn;
	newg->occ = 0;
	newg->next = group;

	return newg;
}

static group_t* pushgroupdup (exec_t* e, group_t* pg, qse_rex_node_t* gn)
{
	group_t* gs = QSE_NULL;

	/* duplicate the group stack if necessary */
	if (pg != QSE_NULL)
	{
		gs = dupgroups (e, pg);
		if (gs == QSE_NULL) return QSE_NULL;
	}

	/* and push a new group to the stack */
	return pushgroup (e, gs, gn);
}

static int addsimplecand (
	exec_t* e, group_t* group, qse_rex_node_t* node, 
	qse_size_t occ, const qse_char_t* mptr)
{
	QSE_ASSERT (
		node->id == QSE_REX_NODE_BOL ||
		node->id == QSE_REX_NODE_EOL ||
		node->id == QSE_REX_NODE_ANYCHAR ||
		node->id == QSE_REX_NODE_CHAR ||
		node->id == QSE_REX_NODE_CHARSET
	);

	cand_t cand;

	cand.node = node;
	cand.occ = occ;
	cand.group = group;
	cand.mptr = mptr;

/*if (node->id == QSE_REX_NODE_CHAR)
qse_printf (QSE_T("adding %d %c\n"), node->id, node->u.c);
else
qse_printf (QSE_T("adding %d NA\n"), node->id);*/
		
	if (qse_lda_insert (
		&e->cand.set[e->cand.pending],
		QSE_LDA_SIZE(&e->cand.set[e->cand.pending]),
		&cand, 1) == (qse_size_t)-1)
	{
		/* TODO: set error code: ENOERR */
		return -1;
	}

	return 0;
}

static int addcands (
	exec_t* e, group_t* group, qse_rex_node_t* prevnode,
	qse_rex_node_t* candnode, const qse_char_t* mptr)
{
	/* skip all NOP nodes */
	while (candnode != QSE_NULL && candnode->id == QSE_REX_NODE_NOP) 
		candnode = candnode->next;

	/* nothing to add */
	if (candnode == QSE_NULL) return 0;

	if (candnode->id == QSE_REX_NODE_END)
	{
		qse_printf (QSE_T("== ADDING THE END(MATCH) NODE MEANING MATCH FOUND == \n"));
		if (e->matchend == QSE_NULL || mptr >= e->matchend)
			e->matchend = mptr;
		e->nmatches++;
	}
	else if (candnode->id == QSE_REX_NODE_BRANCH)
	{
		group_t* groupdup;

		QSE_ASSERT (candnode->next == QSE_NULL);

		groupdup = dupgroups (e, group);
		if (groupdup == QSE_NULL) return -1;

		if (addcands (e, group, prevnode, candnode->u.b.left, mptr) <= -1) return -1;
		if (addcands (e, groupdup, prevnode, candnode->u.b.right, mptr) <= -1) return -1;
	}
	else if (candnode->id == QSE_REX_NODE_GROUP)
	{
		group_t* groupdup;

		if (candnode->occ.min <= 0)
		{
			/* if the group node is optional, 
			 * add the next node to the candidate array. */
			if (addcands (e, group, prevnode, candnode->next, mptr) <= -1) return -1;
		}

		/* push the candnoderent group node (candnode) to the group
		 * stack duplicated. */
		groupdup = pushgroupdup (e, group, candnode);
		if (groupdup == QSE_NULL) return -1;

		/* add the first node in the group */
		if (addcands (e, groupdup, candnode, candnode->u.g.head, mptr) <= -1) return -1;

	}
	else if (candnode->id == QSE_REX_NODE_GROUPEND)
	{
		qse_rex_node_t* node;
		qse_size_t occ;

		QSE_ASSERTX (group != QSE_NULL, 
			"GROUPEND reached must be paired up with a GROUP");

		if (prevnode != candnode) 
		/*if (prevnode == QSE_NULL || prevnode->id != QSE_REX_NODE_GROUPEND)*/
		{
			group->occ++;

			occ = group->occ;
			node = group->node;
			QSE_ASSERT (node == candnode->u.ge.group);
	
			if (occ >= node->occ.min)
			{
				group_t* gx = group->next;
	
				/* take the next atom as a candidate.
				 * it is actually a branch case. move on. */
	
				if (occ < node->occ.max)
				{
					/* check if the group will be repeated.
					 * if so, duplicate the group stack excluding
					 * the top. it goes along a different path and
					 * hence requires a duplicated group stack. */
					if (group->next != QSE_NULL)
					{
						gx = dupgroups (e, group->next);
						if (gx == QSE_NULL) return -1;
					}
				}
	
				if (addcands (e, gx, candnode, node->next, mptr) <= -1) return -1;
			}
	
			if (occ < node->occ.max)
			{
				/* need to repeat itself. */
				if (addcands (e, group, candnode, node->u.g.head, mptr) <= -1) return -1;
			}
		}
	}
	else
	{
		group_t* gx = group;

		if (candnode->occ.min <= 0)
		{
			/* if the node is optional,
			 * add the next node to the candidate array  */
			if (addcands (e, group, prevnode, candnode->next, mptr) <= -1) return -1;

			if (group != QSE_NULL)
			{
				gx = dupgroups (e, group);
				if (gx == QSE_NULL) return -1;
			}
		}

		if (addsimplecand (e, gx, candnode, 1, mptr) <= -1) return -1;
	}

	return 0;
}

static int match (exec_t* e)
{
	qse_size_t i;

	QSE_ASSERT (QSE_LDA_SIZE(&e->cand.set[e->cand.active]) > 0);

	for (i = 0; i < QSE_LDA_SIZE(&e->cand.set[e->cand.active]); i++)
	{
		cand_t* cand = QSE_LDA_DPTR(&e->cand.set[e->cand.active],i);
		qse_rex_node_t* node = cand->node;
		const qse_char_t* nmptr = QSE_NULL;

		switch (node->id)
		{
			case QSE_REX_NODE_BOL:
				if (cand->mptr == e->str.ptr) nmptr = cand->mptr;
				break;

			case QSE_REX_NODE_EOL:
				if (cand->mptr >= e->str.end) nmptr = cand->mptr;
				break;

			case QSE_REX_NODE_ANYCHAR:
				if (cand->mptr < e->sub.end) nmptr = cand->mptr + 1;
				break;

			case QSE_REX_NODE_CHAR:	
				if (cand->mptr < e->sub.end && node->u.c == *cand->mptr) nmptr = cand->mptr + 1;
					//qse_printf (QSE_T("matched %c\n"), node->u.c);
				break;

			case QSE_REX_NODE_CHARSET:
				qse_printf (QSE_T("charset not implemented...\n"));
				break;

			default:
				// TODO: set error code -> internal error. this should not happen
				return -1;
		}

		if (nmptr != QSE_NULL)
		{
			if (cand->occ >= node->occ.min)
			{
				group_t* gx = cand->group;
				if (cand->occ < node->occ.max)
				{
					if (cand->group != QSE_NULL)
					{
						gx = dupgroups (e, cand->group);
						if (gx == QSE_NULL) return -1;
					}
				}
	
				/* move on to the next candidate */
				if (addcands (e, gx, node, node->next, nmptr) <= -1) return -1;
			}
			if (cand->occ < node->occ.max)
			{
				/* repeat itself more */
				if (addsimplecand (e, cand->group, node, cand->occ+1, nmptr) <= -1) return -1;
			}
		}
	}

	return 0;
}

static int exec (exec_t* e)
{
	int n;

	e->nmatches = 0;
	e->matchend = QSE_NULL;

	e->cand.pending = 0;
	e->cand.active = 1;

	/* empty the pending set to collect the initial candidates */
	qse_lda_clear (&e->cand.set[e->cand.pending]); 

	/* the first node must be the START node */
	QSE_ASSERT (e->rex->code->id == QSE_REX_NODE_START);

	/* addcands() collects a set of candidates into the pending set */
	n = addcands (
		e,                  /* execution structure */
		QSE_NULL,           /* doesn't belong to any groups yet */
		e->rex->code,       /* dummy previous node, the start node */
		e->rex->code->next, /* start from the second node */
		e->sub.ptr          /* current match pointer */
	);
	if (n <= -1) return -1;

	do
	{
		/* kind of swap the next set and the current set by swapping indices */
		int tmp = e->cand.pending;
		e->cand.pending = e->cand.active;
		e->cand.active = tmp;

		/* check if there are any next candidates */
		if (QSE_LDA_SIZE(&e->cand.set[e->cand.active]) <= 0)
		{
			/* if no more candidates, break */
			break;
		}

{
int i;
qse_printf (QSE_T("SET="));
for (i = 0; i < QSE_LDA_SIZE(&e->cand.set[e->cand.active]); i++)
{
	cand_t* cand = QSE_LDA_DPTR(&e->cand.set[e->cand.active],i);
	qse_rex_node_t* node = cand->node;

	if (node->id == QSE_REX_NODE_CHAR)
		qse_printf (QSE_T("%c "), node->u.c);
	else if (node->id == QSE_REX_NODE_ANYCHAR)
		qse_printf (QSE_T(". "), node->u.c);
	else if (node->id == QSE_REX_NODE_BOL)
		qse_printf (QSE_T("^ "));
	else if (node->id == QSE_REX_NODE_EOL)
		qse_printf (QSE_T("$ "));
}
qse_printf (QSE_T("\n"));
}

		/* clear the array to hold the next candidates */
		qse_lda_clear (&e->cand.set[e->cand.pending]); 

		if (match (e) <= -1) return -1;
	}
	while (1);

if (e->nmatches > 0)
{
	qse_printf (QSE_T("MATCH: %d [%.*s]\n"), 
		(int)(e->matchend - e->sub.ptr), 
		(int)(e->matchend - e->sub.ptr), e->sub.ptr);
}

	qse_printf (QSE_T("TOTAL MATCHES FOUND... %d\n"), e->nmatches);
	return 0;
}

static int init_exec_dds (exec_t* e, qse_mmgr_t* mmgr)
{
	/* initializes dynamic data structures */
	if (qse_lda_init (&e->cand.set[0], mmgr, 100) == QSE_NULL)
	{
		/* TOOD: set error */
		return -1;
	}
	if (qse_lda_init (&e->cand.set[1], mmgr, 100) == QSE_NULL)
	{
		/* TOOD: set error */
		qse_lda_fini (&e->cand.set[0]);
		return -1;
	}

	qse_lda_setscale (&e->cand.set[0], QSE_SIZEOF(cand_t));
	qse_lda_setscale (&e->cand.set[1], QSE_SIZEOF(cand_t));

	qse_lda_setcopier (&e->cand.set[0], QSE_LDA_COPIER_INLINE);
	qse_lda_setcopier (&e->cand.set[1], QSE_LDA_COPIER_INLINE);

	return 0;
}

static void fini_exec_dds (exec_t* e)
{
	qse_lda_fini (&e->cand.set[1]);
	qse_lda_fini (&e->cand.set[0]);
}

int qse_rex_exec (qse_rex_t* rex, 
	const qse_char_t* str, qse_size_t len,
	const qse_char_t* substr, qse_size_t sublen)
{
	exec_t e;
	int n = 0;

	if (rex->code == QSE_NULL)
	{
		//* TODO:  set error code: no regular expression compiled.
		return -1;
	}

	QSE_MEMSET (&e, 0, QSE_SIZEOF(e));
	e.rex = rex;
	e.str.ptr = str;
	e.str.end = str + len;
	e.sub.ptr = substr;
	e.sub.end = substr + sublen;

	if (init_exec_dds (&e, rex->mmgr) <= -1) return -1;

	while (e.sub.ptr <= e.sub.end)
	{
		n = exec (&e);
		if (n <= -1) 
		{
			n = -1;
			break;
		}

		if (e.nmatches > 0) break;

		e.sub.ptr++;
	}

	fini_exec_dds (&e);

	return n;
}
