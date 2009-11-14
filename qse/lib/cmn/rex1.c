/*
 * $Id$

{LICENSE HERE}

 */

#include <qse/cmn/rex.h>
#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>
#include "mem.h"

#define GETC(c) do { if getc(c) <= -1) return -1; } while (0)
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
	qse_cint_t c;
	qse_size_t grouplvl;

	qse_rex_node_t* start;
};

typedef struct exec_t exec_t;
struct exec_t
{
	qse_rex_t* rex;

	qse_cstr_t str;
	qse_cstr_t sub;

	qse_lda_t cand[2]; /* candidate arrays */
	int xxx, yyy;
	qse_size_t matched;
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
	qse_rex_node_t* node;
	qse_size_t occ;
	group_t* group;
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

static int getc (comp_t* c)
{
	c->c = (c->ptr < c->end)? *c->ptr++: QSE_CHAR_EOF;
if (c->c == QSE_CHAR_EOF)
qse_printf (QSE_T("getc => <EOF>\n"));
else qse_printf (QSE_T("getc => %c\n"), c->c);
	return 0;
}

static qse_rex_node_t* comp0 (comp_t* c, qse_rex_node_t* ge);

static qse_rex_node_t* comp2 (comp_t* c)
{
	qse_rex_node_t* n;

	switch (c->c)
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

			c->grouplvl++;
			x = comp0 (c, ge);
			if (x == QSE_NULL)
			{
				// freere (ge);
				// freere (n);
				return QSE_NULL;
			}

			if (c->c != QSE_T(')')) 
			{
qse_printf (QSE_T("expecting )\n"));
				// UNBALANCED PAREN.
				// freere (x);
				// freere (n);
				return QSE_NULL;
			}

			c->grouplvl--;
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

		/*
		case QSE_T('['):
			....
		*/

		default:
			/* normal character */
			n = newcharnode (c, c->c);
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

	while (c->c != QSE_T('|') && c->c != QSE_CHAR_EOF && 
	       !(c->grouplvl >= 0 && c->c == QSE_T(')')))
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

	while (c->c == QSE_T('|'))
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
	c.c = QSE_CHAR_EOF;
	c.grouplvl = 0;
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
			//tmp = comp0 (&c, QSE_NULL);
			tmp = comp0 (&c, end);
			if (tmp == QSE_NULL) 
			{
				//freenode (c.start, c.rex->mmgr);
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

static group_t* pushgroup (exec_t* e, group_t* pg, qse_rex_node_t* gn)
{
	group_t* g;
	QSE_ASSERT (gn->id == QSE_REX_NODE_GROUP);

	g = (group_t*) QSE_MMGR_ALLOC (e->rex->mmgr, QSE_SIZEOF(*g));
	if (g == QSE_NULL)
	{
		/* TODO: set error info */
		return QSE_NULL;
	}

	g->node = gn;
	g->occ = 0;
	g->next = pg;

	return g;
}

static int addsimplecand (
	exec_t* e, cand_t* pcand, qse_rex_node_t* node, qse_size_t occ)
{
	QSE_ASSERT (
		node->id == QSE_REX_NODE_CHAR ||
		node->id == QSE_REX_NODE_CHARSET
	);

	cand_t cand;

	cand.node = node;
	cand.occ = occ;
	cand.group = pcand->group;

if (node->id == QSE_REX_NODE_CHAR)
qse_printf (QSE_T("adding %d %c\n"), node->id, node->u.c);
else
qse_printf (QSE_T("adding %d NA\n"), node->id);
		
	if (qse_lda_insert (
		&e->cand[e->xxx],
		QSE_LDA_SIZE(&e->cand[e->xxx]),
		&cand, 1) == (qse_size_t)-1)
	{
		/* TODO: set error code: ENOERR */
		return -1;
	}

	return 0;
}

static int addnextcands (exec_t* e, group_t* group, qse_rex_node_t* cur)
{
	/* skip all NOP nodes */
	while (cur && cur->id == QSE_REX_NODE_NOP) cur = cur->next;

	/* nothing to add */
	if (cur == QSE_NULL) return 0;

	if (cur->id == QSE_REX_NODE_END)
	{
		qse_printf (QSE_T("== ADDING THE END(MATCH) NODE MEANING MATCH FOUND == \n"));
		e->matched++;
	}
	else if (cur->id == QSE_REX_NODE_BRANCH)
	{
	#if 0
		QSE_ASSERT (cur->next == QSE_NULL);
		if (addnextcands (e, group, cur->u.b.left) <= -1) return -1;
		if (addnextcands (e, group, cur->u.b.right) <= -1) return -1;
	#endif
	}
	else if (cur->id == QSE_REX_NODE_GROUP)
	{
		group_t* g = pushgroup (e, group, cur);
		if (g == QSE_NULL) return -1;

		/* add the first node in the group */
		if (addnextcands (e, g, cur->u.g.head) <= -1) return -1;

		if (cur->occ.min <= 0)
		{
			/* if the group node is optional, 
			 * add the next node to the candidate array.
			 * branch case => dup group */
			if (addnextcands (e, group, cur->next) <= -1) return -1;
		}
	}
	else if (cur->id == QSE_REX_NODE_GROUPEND)
	{
		group_t* group;
		qse_rex_node_t* node;

		group = cand->group;
		QSE_ASSERT (group != QSE_NULL);

		node = group->node;
		QSE_ASSERT (node == cur->u.ge.group);

		if (group->occ < node->occ.max)
		{
			/* need to repeat itself */
			group->occ++;
			if (addnextcands (e, cand, node->u.g.head) <= -1) return -1;
		}

		if (group->occ >= node->occ.min)
		{
			/* take the next atom as a candidate.
			 * it is actually a branch case. */

			cand = dupgrouppoppingtop (cand);

			if (addnextcands (e, pg, node->next) <= -1) return -1;
		}
	}
	else
	{
		if (addsimplecand (e, cand, cur, 1) <= -1) return -1;
		if (cur->occ.min <= 0)
		{
			/* if the node is optional,
			 * add the next node to the candidate array */
			if (addnextcands (e, pg, cur->next) <= -1) return -1;
		}
	}

	return 0;
}

static int match (exec_t* e, const qse_char_t* curp)
{
	qse_size_t i;
	qse_char_t curc = *curp;

	for (i = 0; i < QSE_LDA_SIZE(&e->cand[e->yyy]); i++)
	{
		cand_t* cand = QSE_LDA_DPTR(&e->cand[e->yyy],i);
		qse_rex_node_t* node = cand->node;

		if (node->id == QSE_REX_NODE_CHAR)
		{
			if (node->u.c == curc)
			{
				qse_printf (QSE_T("matched %c\n"), node->u.c);

				if (cand->occ < node->occ.max)
				{
					if (addsimplecand (e, cand, node, cand->occ+1) <= -1) return -1;
				}
				if (cand->occ >= node->occ.min)
				{

					if (addnextcands (e, cand, node->next) <= -1) return -1;
				}
			}
		}
		else
		{
			QSE_ASSERT (node->id == QSE_REX_NODE_CHARSET);
			qse_printf (QSE_T("charset not implemented...\n"));
		}
	}

	return 0;
}

static int exec (exec_t* e)
{
	const qse_char_t* ptr = e->sub.ptr;
	const qse_char_t* end = e->sub.ptr + e->sub.len;

	e->matched = 0;
	e->xxx = 0;
	e->yyy = 1;

	/* collect the initial candidates to cand[xxx] */
	qse_lda_clear (&e->cand[e->xxx]); 

	if (addnextcands (e, QSE_NULL, e->rex->code->next) <= -1) return -1;

	while (ptr < end)
	{
		/* kind of swap cand[xxx] and cand[yyy] by swapping indices */
		int tmp = e->xxx;
		e->xxx = e->yyy;
		e->yyy = tmp;

		/* check if there are any next candidates */
		if (QSE_LDA_SIZE(&e->cand[e->yyy]) <= 0)
		{
			/* if none, break */
			break;
		}

		/* clear the array to hold the next candidates */
		qse_lda_clear (&e->cand[e->xxx]); 

qse_printf (QSE_T("MATCHING %c\n"), *ptr);
		if (match (e, ptr) <= -1) return -1;

		ptr++;
	}

	qse_printf (QSE_T("TOTAL MATCHES FOUND... %d\n"), e->matched);
	return 0;
}

static int init_exec_dds (exec_t* e, qse_mmgr_t* mmgr)
{
	/* initializes dynamic data structures */
	if (qse_lda_init (&e->cand[0], mmgr, 100) == QSE_NULL)
	{
		/* TOOD: set error */
		return -1;
	}
	if (qse_lda_init (&e->cand[1], mmgr, 100) == QSE_NULL)
	{
		/* TOOD: set error */
		qse_lda_fini (&e->cand[0]);
		return -1;
	}

	qse_lda_setscale (&e->cand[0], QSE_SIZEOF(cand_t));
	qse_lda_setscale (&e->cand[1], QSE_SIZEOF(cand_t));

	qse_lda_setcopier (&e->cand[0], QSE_LDA_COPIER_INLINE);
	qse_lda_setcopier (&e->cand[1], QSE_LDA_COPIER_INLINE);

	return 0;
}

static void fini_exec_dds (exec_t* e)
{
	qse_lda_fini (&e->cand[1]);
	qse_lda_fini (&e->cand[0]);
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
	e.str.len = len;
	e.sub.ptr = substr;
	e.sub.len = sublen;

	if (init_exec_dds (&e, rex->mmgr) <= -1) return -1;

// TOOD: may have to execute exec in case sublen is 0.
	while (e.sub.len > 0)
	{
		n = exec (&e);
		if (n <= -1) 
		{
			n = -1;
			break;
		}

		if (e.matched > 0) break;

		e.sub.ptr++;
		e.sub.len--;
	}

	fini_exec_dds (&e);

	return n;
}
