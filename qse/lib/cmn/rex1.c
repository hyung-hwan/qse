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
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/lda.h>
#include "mem.h"

#define OCC_MAX QSE_TYPE_MAX(qse_size_t)

typedef struct comp_t comp_t;
struct comp_t
{
	qse_rex_t* rex;

	qse_cstr_t re;

	const qse_char_t* ptr;
	const qse_char_t* end;

	struct
	{
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

/* The group_t type defines a structure to maintain the nested
 * traces of subgroups. The actual traces are maintained in a stack
 * of sinlgly linked group_t elements. The head element acts
 * as a management element where the occ field is a reference count
 * and the node field is QSE_NULL always 
 */
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

	/* occurrence */
	qse_size_t        occ;

	/* the stack of groups that this candidate belongs to. 
	 * it is in the singliy linked list form */
	group_t*          group;

	/* match pointer. the number of character advancement 
	 * differs across various node types. BOL and EOL don't advance to
	 * the next character on match while ANY and CHAR do on match.
	 * therefore, the match pointer is managed per candidate basis. */
	const qse_char_t* mptr; 
};

qse_rex_t* qse_rex_open (qse_mmgr_t* mmgr, qse_size_t xtn, qse_rex_node_t* code)
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

	/* note that passing a compiled expression to qse_rex_open() 
	 * is to delegate it to this rex object. when this rex object
	 * is closed, the code delegated is destroyed. */
	if (code && code->id != QSE_REX_NODE_START)
	{
		/* check if the first node is the start node */
		QSE_MMGR_FREE (mmgr, rex);	
		return QSE_NULL;
	}

	rex->code = code;
	return rex;
}

static void freenode (qse_rex_node_t* node, qse_mmgr_t* mmgr)
{
	if (node->id == QSE_REX_NODE_CSET)
	{
		if (node->u.cset.member != QSE_NULL) 
			qse_str_close (node->u.cset.member);
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

int qse_rex_getoption (qse_rex_t* rex)
{
	return rex->option;
}

void qse_rex_setoption (qse_rex_t* rex, int opts)
{
	rex->option = opts;
}

qse_rex_errnum_t qse_rex_geterrnum (qse_rex_t* rex)
{
	return rex->errnum;
}

const qse_char_t* qse_rex_geterrmsg (qse_rex_t* rex)
{
	static const qse_char_t* errstr[] = 
	{
		QSE_T("no error"),
		QSE_T("no sufficient memory available"),
		QSE_T("no expression compiled"),
		QSE_T("recursion too deep"),
		QSE_T("right parenthesis expected"),
		QSE_T("right bracket expected"),
		QSE_T("right brace expected"),
		QSE_T("colon expected"),
		QSE_T("invalid character range"),
		QSE_T("invalid character class"),
		QSE_T("invalid occurrence bound"),
		QSE_T("special character at wrong position"),
		QSE_T("premature expression end")
	};
	
	return (rex->errnum >= 0 && rex->errnum < QSE_COUNTOF(errstr))?
		errstr[rex->errnum]: QSE_T("unknown error");
}

static qse_rex_node_t* newnode (comp_t* c, qse_rex_node_id_t id)
{
	qse_rex_node_t* node;

	node = (qse_rex_node_t*) 
		QSE_MMGR_ALLOC (c->rex->mmgr, QSE_SIZEOF(qse_rex_node_t));
	if (node == QSE_NULL) 
	{
		c->rex->errnum = QSE_REX_ENOMEM;
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

static qse_rex_node_t* newgroupnode (comp_t* c)
{
	return newnode (c, QSE_REX_NODE_GROUP);
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

#define CHECK_END(com) \
	do { \
		if (com->ptr >= com->end) \
		{ \
			com->rex->errnum = QSE_REX_EPREEND; \
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

#define IS_SPE(com,ch) ((com)->c.value == (ch) && !(com)->c.escaped)
#define IS_ESC(com) ((com)->c.escaped)
#define IS_EOF(com) ((com)->c.value == QSE_CHAR_EOF)

#define getc_noesc(c) getc(c,1)
#define getc_esc(c)   getc(c,0)

static int getc (comp_t* com, int noesc)
{
	qse_char_t c;

	if (com->ptr >= com->end)
	{
		com->c.value = QSE_CHAR_EOF;
		com->c.escaped = 0;
		return 0;
	}

	com->c.value = *com->ptr++;
	com->c.escaped = 0;

	if (noesc || com->c.value != QSE_T('\\')) return 0;


	CHECK_END (com);
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

		CHECK_END (com);
		cx = *com->ptr++;
		if (cx >= QSE_T('0') && cx <= QSE_T('7'))
		{
			c = c * 8 + cx - QSE_T('0');

			CHECK_END (com);
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

		CHECK_END (com);
		cx = *com->ptr++;
		if (IS_HEX(cx))
		{
			c = HEX_TO_NUM(cx);

			CHECK_END (com);
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

		CHECK_END (com);
		cx = *com->ptr++;
		if (IS_HEX(cx))
		{
			qse_size_t i;

			c = HEX_TO_NUM(cx);

			for (i = 0; i < 3; i++)
			{
				CHECK_END (com);
				cx = *com->ptr++;

				if (!IS_HEX(cx)) break;
				c = c * 16 + HEX_TO_NUM(cx);
			}
		}
	}
	else if (c == QSE_T('U') && QSE_SIZEOF(qse_char_t) >= 4) 
	{
		qse_char_t cx;

		CHECK_END (com);
		cx = *com->ptr++;
		if (IS_HEX(cx))
		{
			qse_size_t i;

			c = HEX_TO_NUM(cx);

			for (i = 0; i < 7; i++)
			{
				CHECK_END (com);
				cx = *com->ptr++;

				if (!IS_HEX(cx)) break;
				c = c * 16 + HEX_TO_NUM(cx);
			}
		}
	}
#endif

	com->c.value = c;
	com->c.escaped = QSE_TRUE;

#if 0
	com->c = (com->ptr < com->end)? *com->ptr++: QSE_CHAR_EOF;
if (com->c == QSE_CHAR_EOF)
qse_printf (QSE_T("getc => <EOF>\n"));
else qse_printf (QSE_T("getc => %c\n"), com->c);
#endif
	return 0;
}

struct ccinfo_t
{
	qse_cstr_t name;
	int (*func) (qse_char_t c);
}; 

#define ISBLANK(c) ((c) == QSE_T(' ') || (c) == QSE_T('\t'))
static int cc_isalnum (qse_char_t c) { return QSE_ISALNUM (c); }
static int cc_isalpha (qse_char_t c) { return QSE_ISALPHA (c); }
static int cc_isblank (qse_char_t c) { return ISBLANK(c); }
static int cc_iscntrl (qse_char_t c) { return QSE_ISCNTRL (c); }
static int cc_isdigit (qse_char_t c) { return QSE_ISDIGIT (c); }
static int cc_isgraph (qse_char_t c) { return QSE_ISGRAPH (c); }
static int cc_islower (qse_char_t c) { return QSE_ISLOWER (c); }
static int cc_isprint (qse_char_t c) { return QSE_ISPRINT (c); }
static int cc_ispunct (qse_char_t c) { return QSE_ISPUNCT (c); }
static int cc_isspace (qse_char_t c) { return QSE_ISSPACE (c); }
static int cc_isupper (qse_char_t c) { return QSE_ISUPPER (c); }
static int cc_isxdigit (qse_char_t c) { return QSE_ISXDIGIT (c); }

static struct ccinfo_t ccinfo[] =
{
	{ { QSE_T("alnum"),  5 }, cc_isalnum },
	{ { QSE_T("alpha"),  5 }, cc_isalpha },
	{ { QSE_T("blank"),  5 }, cc_isblank },
	{ { QSE_T("cntrl"),  5 }, cc_iscntrl },
	{ { QSE_T("digit"),  5 }, cc_isdigit },
	{ { QSE_T("graph"),  5 }, cc_isgraph },
	{ { QSE_T("lower"),  5 }, cc_islower },
	{ { QSE_T("print"),  5 }, cc_isprint },
	{ { QSE_T("punct"),  5 }, cc_ispunct },
	{ { QSE_T("space"),  5 }, cc_isspace },
	{ { QSE_T("upper"),  5 }, cc_isupper },
	{ { QSE_T("xdigit"), 6 }, cc_isxdigit },

	/*
	{ { QSE_T("arabic"),   6 }, cc_isarabic },
	{ { QSE_T("chinese"),  7 }, cc_ischinese },
	{ { QSE_T("english"),  7 }, cc_isenglish },
	{ { QSE_T("japanese"), 8 }, cc_isjapanese },
	{ { QSE_T("korean"),   6 }, cc_iskorean }, 
	{ { QSE_T("thai"),     4 }, cc_isthai }, 
	*/

	{ { QSE_NULL,          0 }, QSE_NULL }
};

static int charclass (comp_t* com)
{
	const struct ccinfo_t* ccp = ccinfo;
	qse_size_t len = com->end - com->ptr;

	while (ccp->name.ptr != QSE_NULL)
	{
		if (qse_strxbeg(com->ptr,len,ccp->name.ptr) != QSE_NULL) break;
		ccp++;
	}

	if (ccp->name.ptr == QSE_NULL)
	{
		/* wrong class name */
		com->rex->errnum = QSE_REX_ECCLASS;
		return -1;
	}

	com->ptr += ccp->name.len;

	if (getc_noesc(com) <= -1) return -1;
	if (com->c.value != QSE_T(':'))
	{
		com->rex->errnum = QSE_REX_ECCLASS;
		return -1;
	}

	if (getc_noesc(com) <= -1) return -1;
	if (com->c.value != QSE_T(']'))
	{
		com->rex->errnum = QSE_REX_ERBRACK;	
		return -1;
	}

	if (getc_esc(com) <= -1) return -1;
	return (int)(ccp - ccinfo);
}

#define ADD_CSET_CODE(com,node,code,len) \
	do { if (add_cset_code(com,node,code,len) <= -1) return -1; } while(0)

static int add_cset_code (
	comp_t* com, qse_rex_node_t* node, const qse_char_t* c, qse_size_t l)
{
	if (qse_str_ncat(node->u.cset.member,c,l) == (qse_size_t)-1)
	{
		com->rex->errnum = QSE_REX_ENOMEM;
		return -1;
	}
	return 0;
}

static int charset (comp_t* com, qse_rex_node_t* node)
{
	QSE_ASSERT (node->id == QSE_REX_NODE_CSET);
	QSE_ASSERT (node->u.cset.negated == 0);
	QSE_ASSERT (node->u.cset.member	== QSE_NULL);

	if (IS_SPE(com,QSE_T('^')))
	{
		/* negate an expression */
		node->u.cset.negated = 1;
		if (getc_noesc(com) <= -1) return -1;
	}


	/* initialize the member array */
	node->u.cset.member = qse_str_open (com->rex->mmgr, 0, 64);
	if (node->u.cset.member == QSE_NULL)
	{
		com->rex->errnum = QSE_REX_ENOMEM;
		return -1;
	}

	/* if ] is the first character or the second character following ^,
	 * it is treated literally */

	do
	{
		int x1, x2;
		qse_char_t c1, c2;

		x1 = com->c.escaped;
		c1 = com->c.value;
		if (c1 == QSE_CHAR_EOF)
		{
			com->rex->errnum = QSE_REX_EPREEND;
			return -1;
		}

		if (getc_esc(com) <= -1) return -1;
		x2 = com->c.escaped;
		c2 = com->c.value;

		if (!x1 && c1 == QSE_T('[') && 
		    !x2 && c2 == QSE_T(':'))
		{
			int n;
			qse_char_t tmp[2];

			/* begins with [: 
			 * don't read in the next character as charclass() 
			 * matches a class name differently from other routines.
			 * if (getc_noesc(com) <= -1) return -1;
			 */
			if ((n = charclass(com)) <= -1) return -1;
			
			QSE_ASSERT (n < QSE_TYPE_MAX(qse_char_t));

			tmp[0] = QSE_REX_CSET_CLASS;
			tmp[1] = n;
			ADD_CSET_CODE (com, node, tmp, QSE_COUNTOF(tmp));
		}
		else if (!x2 && c2 == QSE_T('-'))
		{
			if (getc_esc(com) <= -1) return -1;
			if (IS_SPE(com, QSE_T(']')))
			{
				qse_char_t tmp[4];

				/* '-' is the last character in the set.
				 * treat it literally */

				tmp[0] = QSE_REX_CSET_CHAR;
				tmp[1] = c1;
				tmp[2] = QSE_REX_CSET_CHAR;
				tmp[3] = c2;

				ADD_CSET_CODE (com, node, tmp, QSE_COUNTOF(tmp));
				break;
			}

			if (c1 > com->c.value)
			{
				/* range end must be >= range start */
				com->rex->errnum = QSE_REX_ECRANGE;
				return -1;
			}
			else if (c1 == com->c.value)
			{
				/* if two chars in the range are the same,
				 * treat it as a single character */
				qse_char_t tmp[2];
				tmp[0] = QSE_REX_CSET_CHAR;
				tmp[1] = c1;
				ADD_CSET_CODE (com, node, tmp, QSE_COUNTOF(tmp));
			}
			else
			{
				qse_char_t tmp[3];
				tmp[0] = QSE_REX_CSET_RANGE;
				tmp[1] = c1;
				tmp[2] = com->c.value;
				ADD_CSET_CODE (com, node, tmp, QSE_COUNTOF(tmp));
			}
			
			if (getc_esc(com) <= -1) return -1;
		}
		else
		{
			qse_char_t tmp[2];
			tmp[0] = QSE_REX_CSET_CHAR;
			tmp[1] = c1;
			ADD_CSET_CODE (com, node, tmp, QSE_COUNTOF(tmp));
		}
	}
	while (!IS_SPE(com,QSE_T(']')));

	if (getc_esc(com) <= -1) return -1;
	return 0;
}

static int occbound (comp_t* com, qse_rex_node_t* n)
{
	qse_size_t bound;

	bound = 0;
	while (com->c.value >= QSE_T('0') && com->c.value <= QSE_T('9'))
	{
		bound = bound * 10 + com->c.value - QSE_T('0');
		if (getc_noesc(com) <= -1) return -1;
	}

	n->occ.min = bound;

	if (com->c.value == QSE_T(',')) 
	{
		if (getc_noesc(com) <= -1) return -1;

		if (com->c.value >= QSE_T('0') && com->c.value <= QSE_T('9'))
		{
			bound = 0;

			do
			{
				bound = bound * 10 + com->c.value - QSE_T('0');
				if (getc_noesc(com) <= -1) return -1;
			}
			while (com->c.value >= QSE_T('0') && 
			       com->c.value <= QSE_T('9'));

			n->occ.max = bound;
		}
		else n->occ.max = OCC_MAX;
	}
	else n->occ.max = n->occ.min;

	if (n->occ.min > n->occ.min)
	{
		/* invalid occurrences range */
		com->rex->errnum = QSE_REX_EBOUND;
		return -1;
	}

	if (com->c.value != QSE_T('}'))
	{
		com->rex->errnum = QSE_REX_ERBRACE;
		return -1;
	}

	if (getc_esc(com) <= -1) return -1;
	return 0;
}

static qse_rex_node_t* comp0 (comp_t* com, qse_rex_node_t* ge);

static qse_rex_node_t* comp2 (comp_t* com)
{
	qse_rex_node_t* n;

	if (!IS_ESC(com))
	{
		switch (com->c.value)
		{
			case QSE_T('('):
			{
				/* enter a subgroup */

				qse_rex_node_t* x, * ge;

				n = newgroupnode (com);
				if (n == QSE_NULL) return QSE_NULL;

				ge = newgroupendnode (com, n);
				if (ge == QSE_NULL) return QSE_NULL;

				if (getc_esc(com) <= -1) return QSE_NULL;

				com->gdepth++;
				/* pass the GROUPEND node so that the
				 * last node in the subgroup links to
				 * this GROUPEND node. */
				x = comp0 (com, ge);
				if (x == QSE_NULL) return QSE_NULL;

				if (!IS_SPE(com,QSE_T(')')))
				{
					com->rex->errnum = QSE_REX_ERPAREN;
					return QSE_NULL;
				}

				com->gdepth--;
				if (getc_esc(com) <= -1) return QSE_NULL;

				n->u.g.head = x;
				n->u.g.end = ge;
				break;
			}
			
			case QSE_T('.'):
				n = newnode (com, QSE_REX_NODE_ANY);
				if (n == QSE_NULL) return QSE_NULL;
				if (getc_esc(com) <= -1) return QSE_NULL;
				break;

			case QSE_T('^'):
				n = newnode (com, QSE_REX_NODE_BOL);
				if (n == QSE_NULL) return QSE_NULL;
				if (getc_esc(com) <= -1) return QSE_NULL;
				break;
	
			case QSE_T('$'):
				n = newnode (com, QSE_REX_NODE_EOL);
				if (n == QSE_NULL) return QSE_NULL;
				if (getc_esc(com) <= -1) return QSE_NULL;
				break;

			case QSE_T('['):
				n = newnode (com, QSE_REX_NODE_CSET);
				if (n == QSE_NULL) return QSE_NULL;
				if (getc_esc(com) <= -1) return QSE_NULL;
				if (charset(com, n) <= -1) return QSE_NULL;
				break;

			default:
				if (com->rex->option & QSE_REX_STRICT)
				{
					qse_char_t spc[] = QSE_T(")?*+{");

					if (com->rex->option & QSE_REX_NOBOUND)
						spc[4] = QSE_T('\0');

					if (qse_strchr (spc, com->c.value) != QSE_NULL)
					{
						com->rex->errnum = QSE_REX_ESPCAWP;
						return QSE_NULL;
					}
				}

				goto normal_char;
		}
	}
	else
	{
	normal_char:
		/* normal character */
		n = newcharnode (com, com->c.value);
		if (n == QSE_NULL) return QSE_NULL;
		if (getc_esc(com) <= -1) return QSE_NULL;
	}

	n->occ.min = 1;
	n->occ.max = 1;

	if (!IS_ESC(com))
	{
		/* handle the occurrence specifier, if any */

		switch (com->c.value)
		{
			case QSE_T('?'):
				n->occ.min = 0;
				n->occ.max = 1;
				if (getc_esc(com) <= -1) return QSE_NULL;
				break;
			
			case QSE_T('*'):
				n->occ.min = 0;
				n->occ.max = OCC_MAX;
				if (getc_esc(com) <= -1) return QSE_NULL;
				break;

			case QSE_T('+'):
				n->occ.min = 1;
				n->occ.max = OCC_MAX;
				if (getc_esc(com) <= -1) return QSE_NULL;
				break;

			case QSE_T('{'):
				if (!(com->rex->option & QSE_REX_NOBOUND))
				{
					if (getc_noesc(com) <= -1) 
						return QSE_NULL;
					if (occbound(com,n) <= -1) 
						return QSE_NULL;
				}
				break;
		}
	}

	return n;
}

/* compile a list of atoms at the outermost level and/or
 * within a subgroup */
static qse_rex_node_t* comp1 (comp_t* c, pair_t* pair)
{
	pair->head = newnopnode (c);
	if (pair->head == QSE_NULL) return QSE_NULL;

#ifdef DONOT_SKIP_NOP
	pair->head->occ.min = 1;
	pair->head->occ.max = 1;
#endif

	pair->tail = pair->head;

	while (!IS_SPE(c,QSE_T('|')) && !IS_EOF(c) &&
	       !(c->gdepth > 0 && IS_SPE(c,QSE_T(')'))))
	{
		qse_rex_node_t* tmp = comp2 (c);
		if (tmp == QSE_NULL) return QSE_NULL;

		if (tmp->id == QSE_REX_NODE_GROUP)
		{
			/* simple optimization to remove an empty group */
			qse_rex_node_t* gg = tmp->u.g.head;
			while (gg->id == QSE_REX_NODE_NOP) gg = gg->next;
			if (gg->id == QSE_REX_NODE_GROUPEND)
			{
				/* exclude an empty subgroup */
				continue;
			}
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

	while (IS_SPE(c,QSE_T('|')))
	{
		if (getc_esc(c) <= -1) return QSE_NULL;
		
		right = comp1 (c, &xpair);
		if (right == QSE_NULL) return QSE_NULL;

		xpair.tail->next = ge;

		tmp = newbranchnode (c, left, right);
		if (tmp == QSE_NULL) return QSE_NULL;

		left = tmp;
	} 

	return left;
}

qse_rex_node_t* qse_rex_comp (
	qse_rex_t* rex, const qse_char_t* ptr, qse_size_t len)
{
	comp_t c;
	qse_rex_node_t* end, * body;

	c.rex = rex;
	c.re.ptr = ptr;
	c.re.len = len;	

	c.ptr = ptr;
	c.end = ptr + len;

	c.c.value = QSE_CHAR_EOF;

	c.gdepth = 0;
	c.start = QSE_NULL;

	/* read the first character */
	if (getc_esc(&c) <= -1) return QSE_NULL;

	c.start = newstartnode (&c);
	if (c.start == QSE_NULL) return QSE_NULL;

	end = newendnode (&c);
	if (end == QSE_NULL)
	{
		freenode (c.start, c.rex->mmgr);
		return QSE_NULL;
	}

	body = comp0 (&c, end);
	if (body == QSE_NULL) 
	{
		freeallnodes (c.start);
		return QSE_NULL;
	}

	c.start->next = body;
	if (rex->code != QSE_NULL) freeallnodes (rex->code);
	rex->code = c.start;

	return rex->code;
}

static void freegroupstackmembers (group_t* gs, qse_mmgr_t* mmgr)
{
	while (gs != QSE_NULL)
	{
		group_t* next = gs->next;
		QSE_MMGR_FREE (mmgr, gs);
		gs = next;
	}
}

static void freegroupstack (group_t* gs, qse_mmgr_t* mmgr)
{
	QSE_ASSERT (gs != QSE_NULL);
	QSE_ASSERTX (gs->node == QSE_NULL, 
		"The head of a group stack must point to QSE_NULL for "
		"management purpose.");

	freegroupstackmembers (gs, mmgr);
}


static void refupgroupstack (group_t* gs)
{
	if (gs != QSE_NULL)
	{
		QSE_ASSERTX (gs->node == QSE_NULL, 
			"The head of a group stack must point to QSE_NULL for "
			"management purpose.");
		gs->occ++;
	}
}

static void refdowngroupstack (group_t* gs, qse_mmgr_t* mmgr)
{
	if (gs != QSE_NULL)
	{
		QSE_ASSERTX (gs->node == QSE_NULL, 
			"The head of a group stack must point to QSE_NULL for "
			"management purpose.");
		if (--gs->occ <= 0)
		{
			freegroupstack (gs, mmgr);
		}
	}
}

static group_t* dupgroupstackmembers (exec_t* e, group_t* g)
{
	group_t* yg, * xg = QSE_NULL;

	QSE_ASSERT (g != QSE_NULL);

	if (g->next != QSE_NULL) 
	{
		/* TODO: make it non recursive or 
		 *       implement stack overflow protection */
		xg = dupgroupstackmembers (e, g->next);
		if (xg == QSE_NULL) return QSE_NULL;
	}

	yg = (group_t*) QSE_MMGR_ALLOC (e->rex->mmgr, QSE_SIZEOF(*yg));
	if (yg == QSE_NULL)
	{
		if (xg != QSE_NULL) freegroupstack (xg, e->rex->mmgr);
		e->rex->errnum = QSE_REX_ENOMEM;
		return QSE_NULL;
	}

	QSE_MEMCPY (yg, g, QSE_SIZEOF(*yg));
	yg->next = xg;

	return yg;
}

static group_t* dupgroupstack (exec_t* e, group_t* gs)
{
	group_t* head;

	QSE_ASSERT (gs != QSE_NULL);
	QSE_ASSERTX (gs->node == QSE_NULL, 
		"The head of a group stack must point to QSE_NULL for "
		"management purpose.");

	head = dupgroupstackmembers (e, gs);
	if (head == QSE_NULL) return QSE_NULL;

	QSE_ASSERTX (
		head->node == QSE_NULL && 
		head->node == gs->node && 
		head->occ == gs->occ,
		"The duplicated stack head must not be corrupted"
	);

	/* reset the reference count of a duplicated stack */
	head->occ = 0;
	return head;
}

/* push 'gn' to the group stack 'gs'.
 * if dup is non-zero, the group stack is duplicated and 'gn' is pushed to 
 * its top */
static group_t* __groupstackpush (
	exec_t* e, group_t* gs, qse_rex_node_t* gn, int dup)
{
	group_t* head, * elem;

	QSE_ASSERT (gn->id == QSE_REX_NODE_GROUP);

	if (gs == QSE_NULL)
	{
		/* gn is the first group pushed. no stack yet.
		 * create the head to store management info. */
		head = (group_t*) QSE_MMGR_ALLOC (e->rex->mmgr, QSE_SIZEOF(*head));
		if (head == QSE_NULL)
		{
			e->rex->errnum = QSE_REX_ENOMEM;
			return QSE_NULL;
		}

		/* the head does not point to any group node. */
		head->node = QSE_NULL;
		/* the occ field is used for reference counting. 
		 * refupgroupstack and refdowngroupstack update it. */
		head->occ = 0;
		/* the head links to the first actual group */
		head->next = QSE_NULL;	
	}
	else 
	{
		if (dup)
		{
			/* duplicate existing stack */
			head = dupgroupstack (e, gs);
			if (head == QSE_NULL) return QSE_NULL;
		}
		else
		{
			head = gs;
		}
	}

	/* create a new stack element */
	elem = (group_t*) QSE_MMGR_ALLOC (e->rex->mmgr, QSE_SIZEOF(*elem));
	if (elem == QSE_NULL)
	{
		/* rollback */
		if (gs == QSE_NULL) 
			QSE_MMGR_FREE (e->rex->mmgr, head);
		else if (dup) 
			freegroupstack (head, e->rex->mmgr);

		e->rex->errnum = QSE_REX_ENOMEM;
		return QSE_NULL;
	}

	/* initialize the element */
	elem->node = gn;
	elem->occ = 0;

	/* make it the top */
	elem->next = head->next;
	head->next = elem;

	return head;
}

#define dupgroupstackpush(e,gs,gn) __groupstackpush(e,gs,gn,1)
#define groupstackpush(e,gs,gn) __groupstackpush(e,gs,gn,0)

/* duplidate a group stack excluding the top data element */
static group_t* dupgroupstackpop (exec_t* e, group_t* gs)
{
	group_t* dupg, * head;

	QSE_ASSERT (gs != QSE_NULL);
	QSE_ASSERTX (gs->node == QSE_NULL, 
		"The head of a group stack must point to QSE_NULL for "
		"management purpose.");
	QSE_ASSERTX (gs->next != QSE_NULL && gs->next->next != QSE_NULL, 
		"dupgroupstackpop() needs at least two data elements");

	dupg = dupgroupstackmembers (e, gs->next->next);
	if (dupg == QSE_NULL) return QSE_NULL;

	head = (group_t*) QSE_MMGR_ALLOC (e->rex->mmgr, QSE_SIZEOF(*head));
	if (head == QSE_NULL)
	{
		if (dupg != QSE_NULL) freegroupstackmembers (dupg, e->rex->mmgr);
		e->rex->errnum = QSE_REX_ENOMEM;
		return QSE_NULL;
	}

	head->node = QSE_NULL;
	head->occ = 0;
	head->next = dupg;

	return head;
}

static group_t* groupstackpop (exec_t* e, group_t* gs)
{
	group_t* top;

	QSE_ASSERT (gs != QSE_NULL);
	QSE_ASSERTX (gs->node == QSE_NULL, 
		"The head of a group stack must point to QSE_NULL for "
		"management purpose.");
	QSE_ASSERTX (gs->next != QSE_NULL && gs->next->next != QSE_NULL, 
		"groupstackpop() needs at least two data elements");


	top = gs->next;
	gs->next = top->next;

	QSE_MMGR_FREE (e->rex->mmgr, top);	
	return gs;
}

static int addsimplecand (
	exec_t* e, group_t* group, qse_rex_node_t* node, 
	qse_size_t occ, const qse_char_t* mptr)
{
	QSE_ASSERT (
		node->id == QSE_REX_NODE_NOP ||
		node->id == QSE_REX_NODE_BOL ||
		node->id == QSE_REX_NODE_EOL ||
		node->id == QSE_REX_NODE_ANY ||
		node->id == QSE_REX_NODE_CHAR ||
		node->id == QSE_REX_NODE_CSET
	);

	cand_t cand;

	cand.node = node;
	cand.occ = occ;
	cand.group = group;
	cand.mptr = mptr;

/*if (node->id == QSE_REX_NODE_CHAR)
qse_printf (QSE_T("adding %d %c\n"), node->id, node->u.c);
else
qse_printf (QSE_T("adding %d NA\n"), node->id);
*/
		
	if (qse_lda_search (
		&e->cand.set[e->cand.pending],
		0, &cand, 1) != QSE_LDA_NIL)
	{
		/* exclude any existing entries in the array.
		 * see comp_cand() for the equality test used.
		 * note this linear search may be a performance bottle neck	
		 * if the arrary grows large. not so sure if it should be
		 * switched to a different data structure such as a hash table.
		 * the problem is that most practical regular expressions
		 * won't have many candidates for a particular match point.
		 * so i'm a bit skeptical about data struct switching.
		 */
		return 0;
	}

	if (qse_lda_insert (
		&e->cand.set[e->cand.pending],
		QSE_LDA_SIZE(&e->cand.set[e->cand.pending]),
		&cand, 1) == QSE_LDA_NIL)
	{
		e->rex->errnum = QSE_REX_ENOMEM;
		return -1;
	}

	/* the reference must be decremented by the freeer */
	refupgroupstack (group);
	return 0;
}

/* addcands() function add a candicate from candnode.
 * if candnode is not a simple node, it traverses further
 * until it reaches a simple node. prevnode is the last
 * GROUPEND node visited during traversal. If no GROUPEND
 * is visited yet, it can be any starting node */
static int addcands (
	exec_t* e, group_t* group, qse_rex_node_t* prevnode,
	qse_rex_node_t* candnode, const qse_char_t* mptr)
{
warpback:

#ifndef DONOT_SKIP_NOP
	/* skip all NOP nodes */
	while (candnode != QSE_NULL && candnode->id == QSE_REX_NODE_NOP) 
		candnode = candnode->next;
#endif

	/* nothing to add */
	if (candnode == QSE_NULL) return 0;

	switch (candnode->id)
	{
		case QSE_REX_NODE_END:
		{
/*qse_printf (QSE_T("== ADDING THE END(MATCH) NODE MEANING MATCH FOUND == \n"));*/
			if (e->matchend == QSE_NULL || mptr >= e->matchend)
				e->matchend = mptr;
			e->nmatches++;
			break;
		}

		case QSE_REX_NODE_BRANCH:
		{
			group_t* gx = group;
			int n;

			QSE_ASSERTX (candnode->next == QSE_NULL, 
				"The current implementation does not link "
				"nodes to a branch node via the next field."
				"follow the left and the right field instead");

			if (group != QSE_NULL)
			{
				gx = dupgroupstack (e, group);
				if (gx == QSE_NULL) return -1;
			}

			refupgroupstack (group);
			refupgroupstack (gx);

			n = addcands (e, group, 
				prevnode, candnode->u.b.left, mptr);
			if (n >= 0) 
			{
				n = addcands (e, gx, 
					prevnode, candnode->u.b.right, mptr);
			}

			refdowngroupstack (gx, e->rex->mmgr);
			refdowngroupstack (group, e->rex->mmgr);

			if (n <= -1) return -1;
			break;
		}

		case QSE_REX_NODE_GROUP:
		{
			int n;
			qse_rex_node_t* front;

/*qse_printf (QSE_T("GROUP %p PREV %p\n"), candnode, prevnode);*/

			/* skip all NOP nodes */
			front = candnode->u.g.head;
			while (front->id == QSE_REX_NODE_NOP) 
				front = front->next;
			if (front->id == QSE_REX_NODE_GROUPEND) 
			{
				/* if GROUPEND is reached, the group
				 * is empty. jump to the next node
				 * regardless of its occurrence. 
				 * however, this will never be reached 
				 * as it has been removed in comp() */
				candnode = candnode->next;
				goto warpback;
			}

			if (candnode->occ.max > 0)
			{
				/* add the first node in a subgroup
				 * as a candidate */

				group_t* gx;

				/* push the current group node (candnode) to 
				 * the group stack. if candnode->next is 
				 * added to the candidate array, which means
				 * the group stack has already been used to 
				 * a different path, the group stack is
				 * duplicated for this path. */

				gx = (candnode->occ.min <= 0)?
					dupgroupstackpush (e, group, candnode):
					groupstackpush (e, group, candnode);
				if (gx == QSE_NULL) return -1;

				/* add the first node in the group to 
				 * the candidate array */
				refupgroupstack (gx);
				n = addcands (e, gx, prevnode, front, mptr); 
				refdowngroupstack (gx, e->rex->mmgr);

				if (n <= -1) return -1;
			}
			
			if (candnode->occ.min <= 0)
			{
				/* if the group node is optional, 
				 * add the next node to the candidate array. */
	
			/* BEGIN avoid recursion */
			#if 0
				refupgroupstack (group);
				n = addcands (e, group, prevnode, candnode->next, mptr);
				refdowngroupstack (group, e->rex->mmgr);
				if (n <= -1) return -1;
			#endif
				candnode = candnode->next;
				goto warpback;
			/* END avoid recursion */
			}

			break;
		}

		case QSE_REX_NODE_GROUPEND:
		{
			int n;
			group_t* top;
			qse_rex_node_t* node;
			qse_size_t occ;

/*qse_printf (QSE_T("GROUPEND %p PREV %p\n"), candnode, prevnode);*/
			QSE_ASSERTX (
				group != QSE_NULL && group->next != QSE_NULL, 
				"GROUPEND must be paired up with GROUP");

			if (prevnode == candnode) 
			{
				/* consider a pattern like (x*)*.
				 * when GROUPEND is reached, an 'if' block below
				 * tries to add the first node(node->u.g.head) 
				 * in the group again. however, it('x') is optional, 
				 * a possible path reach GROUPEND directly without 
				 * adding a candidate. this check is needed to 
				 * avoid the infinite loop, which otherwise is not
				 * avoidable. */
				break;
			}
	
			top = group->next;
			top->occ++;

			occ = top->occ;
			node = top->node;
			QSE_ASSERTX (node == candnode->u.ge.group, 
				"The GROUP node in the group stack must be the one "
				"pairing up with the GROUPEND node."
			);
	
			if (occ >= node->occ.min)
			{
				group_t* gx;
	
				/* the lower bound has been met. 
				 * for a pattern (abc){3,4}, 'abc' has been 
				 * repeated 3 times. in this case, the next 
				 * node can be added to the candiate array.
				 * it is actually a branch case. move on. */
	
				if (top->next == QSE_NULL)
				{
					/* only one element in the stack.
					 * falls back to QSE_NULL regardless 
					 * of the need to reuse it */
					gx = QSE_NULL;
				}
				else if (occ < node->occ.max)
				{
					/* check if the group will be repeated.
					 * if so, duplicate the group stack 
					 * excluding the top. it goes along a 
					 * different path and hence requires  
					 * duplication. */

					gx = dupgroupstackpop (e, group);
					if (gx == QSE_NULL) return -1;
				}
				else
				{
					/* reuse the group stack. pop the top 
					 * data element off the stack */

					gx = groupstackpop (e, group);

					/* this function always succeeds and
					 * returns the same head */
					QSE_ASSERT (gx == group);
				}
	
				refupgroupstack (gx);
				if (prevnode != QSE_NULL && prevnode->id == QSE_REX_NODE_GROUPEND)
				{
					n = addcands (e, gx, prevnode, node->next, mptr);
				}
				else
				{
					n = addcands (e, gx, candnode, node->next, mptr);
				}
				refdowngroupstack (gx, e->rex->mmgr);
				if (n <= -1) return -1;
			}

			if (occ < node->occ.max)
			{
				/* repeat itself. */

			/* BEGIN avoid recursion */
			#if 0
				refupgroupstack (group);
				n = addcands (e, group, prevnode, node->u.g.head, mptr);
				refdowngroupstack (group, e->rex->mmgr);
				if (n <= -1) return -1;
			#endif

				prevnode = candnode;
				candnode = node->u.g.head;
				goto warpback;
			/* END avoid recursion */
			}

			break;
		}

		default:
		{
			int n;

			if (candnode->occ.max > 0)
			{
				group_t* gx;
	
				/* if the upper bound is greater than 0, 
				 * this node is added to the candidate array */

				if (group != QSE_NULL && candnode->occ.min <= 0)
				{
					/* if a group stack exists(group != QSE_NULL)
					 * for this path  and it has been
					 * used for a different path above, 
					 * duplicate the group stack */
					gx = dupgroupstack (e, group);
					if (gx == QSE_NULL) return -1;
				}
				else gx = group;
	
				refupgroupstack (gx);
				n = addsimplecand (e, gx, candnode, 1, mptr);
				refdowngroupstack (gx, e->rex->mmgr);

				if (n <= -1) return -1;
			}

			if (candnode->occ.min <= 0)
			{
				/* if the node is optional,
				 * add the next node to the candidate array  */

			/* BEGIN avoid recursion */
			#if 0
				refupgroupstack (group);
				n = addcands (e, group, prevnode, candnode->next, mptr);
				refdowngroupstack (group, e->rex->mmgr);
				if (n <= -1) return -1;
			#endif
				candnode = candnode->next;
				goto warpback;
			/* END avoid recursion */
			}


			break;
		}
	}

	return 0;
}

static int charset_matched (exec_t* e, qse_rex_node_t* node, qse_char_t c)
{
	const qse_char_t* ptr, * end;
	int matched = 0;

	QSE_ASSERT (node->u.cset.member != QSE_NULL);

	ptr = QSE_STR_PTR (node->u.cset.member);
	end = ptr + QSE_STR_LEN (node->u.cset.member);

	while (ptr < end && !matched)
	{
		switch (*ptr)
		{
			case QSE_REX_CSET_CHAR:
			{
				if (c == *++ptr) matched = !0;
				break;
			}
				
			case QSE_REX_CSET_RANGE:
			{
				qse_char_t c1, c2;
				c1 = *++ptr;
				c2 = *++ptr;
				if (c >= c1 && c <= c2) matched = !0;
				break;
			}
				
			case QSE_REX_CSET_CLASS:
			{
				qse_char_t c1;

				c1 = *++ptr;
				QSE_ASSERT (c1 < QSE_COUNTOF(ccinfo));
				if (ccinfo[c1].func(c)) matched = !0;
			
				break;
			}

			default:
			{
				QSE_ASSERTX (0, 
					"SHOUL NEVER HAPPEN - membership code "
					"for a character set must be one of "
					"QSE_REX_CSET_CHAR, "
					"QSE_REX_CSET_RANGE, "
					"QSE_REX_CSET_CLASS");

				/* return no match if this part is reached.
				 * however, something is totally wrong if it
				 * happens. */
				return 0;
			}
		}

		ptr++;
	}

	if (node->u.cset.negated) matched = !matched;
	return matched;
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
#ifdef DONOT_SKIP_NOP
			case QSE_REX_NODE_NOP:
				nmptr = cand->mptr;
				break;
#endif

			case QSE_REX_NODE_BOL:
				if (cand->mptr == e->str.ptr) 
				{
					/* the next match pointer remains 
					 * the same as ^ matches a position,
					 * not a character. */
					nmptr = cand->mptr;
				}
				break;

			case QSE_REX_NODE_EOL:
				if (cand->mptr >= e->str.end) 
				{
					/* the next match pointer remains 
					 * the same as $ matches a position,
					 * not a character. */
					nmptr = cand->mptr;
				}
				break;

			case QSE_REX_NODE_ANY:
				if (cand->mptr < e->sub.end) 
				{
					/* advance the match pointer to the
					 * next chracter.*/
					nmptr = cand->mptr + 1;
				}
				break;

			case QSE_REX_NODE_CHAR:	
				if (cand->mptr < e->sub.end &&
				    node->u.c == *cand->mptr) 
				{
					/* advance the match pointer to the
					 * next chracter.*/
					nmptr = cand->mptr + 1;
				}
/*qse_printf (QSE_T("matched %c\n"), node->u.c);*/
				break;

			case QSE_REX_NODE_CSET:
			{
				if (cand->mptr < e->sub.end &&
				    charset_matched(e, node, *cand->mptr))
				{
					/* advance the match pointer 
					 * to the next chracter.*/
					nmptr = cand->mptr + 1;
				}

				break;
			}

			default:
			{
				QSE_ASSERTX (0, 
					"SHOUL NEVER HAPPEN - node ID must be"
					"one of QSE_REX_NODE_BOL, "
					"QSE_REX_NODE_EOL, "
					"QSE_REX_NODE_ANY, "
					"QSE_REX_NODE_CHAR, "
					"QSE_REX_NODE_CSET, "
					"QSE_REX_NODE_NOP");

				break;
			}
		}

		if (nmptr != QSE_NULL)
		{
			int n;

			if (cand->occ >= node->occ.min)
			{
				group_t* gx;

				if (cand->occ < node->occ.max && cand->group != QSE_NULL)
				{
					gx = dupgroupstack (e, cand->group);
					if (gx == QSE_NULL) return -1;
				}
				else gx = cand->group;
	
				/* move on to the next candidate */
				refupgroupstack (gx);
				n = addcands (e, gx, node, node->next, nmptr);
				refdowngroupstack (gx, e->rex->mmgr);

				if (n <= -1) return -1;
			}

			if (cand->occ < node->occ.max)
			{
				/* repeat itself more */
				refupgroupstack (cand->group);
				n = addsimplecand (e, cand->group, node, cand->occ+1, nmptr);
				refdowngroupstack (cand->group, e->rex->mmgr);

				if (n <= -1) return -1;
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
		/* swap the pending and active set indices.
		 * the pending set becomes active after which the match()
		 * function tries each candidate in it. New candidates
		 * are added into the pending set which will become active
		 * later when the loop reaches here again */
		int tmp = e->cand.pending;
		e->cand.pending = e->cand.active;
		e->cand.active = tmp;

		if (QSE_LDA_SIZE(&e->cand.set[e->cand.active]) <= 0)
		{
			/* we can't go on with no candidates in the 
			 * active set. */
			break;
		}

		/* clear the pending set */
		qse_lda_clear (&e->cand.set[e->cand.pending]); 

#if 0
{
int i;
qse_printf (QSE_T("SET="));
for (i = 0; i < QSE_LDA_SIZE(&e->cand.set[e->cand.active]); i++)
{
	cand_t* cand = QSE_LDA_DPTR(&e->cand.set[e->cand.active],i);
	qse_rex_node_t* node = cand->node;

	if (node->id == QSE_REX_NODE_CHAR)
		qse_printf (QSE_T("%c "), node->u.c);
	else if (node->id == QSE_REX_NODE_ANY)
		qse_printf (QSE_T(". "), node->u.c);
	else if (node->id == QSE_REX_NODE_BOL)
		qse_printf (QSE_T("^ "));
	else if (node->id == QSE_REX_NODE_EOL)
		qse_printf (QSE_T("$ "));
}
qse_printf (QSE_T("\n"));
}
#endif

		if (match (e) <= -1) return -1;
	}
	while (1);

#if 0
if (e->nmatches > 0)
{
qse_printf (QSE_T("MATCH: %d [%.*s]\n"), 
	(int)(e->matchend - e->sub.ptr), 
	(int)(e->matchend - e->sub.ptr), e->sub.ptr);
}

qse_printf (QSE_T("TOTAL MATCHES FOUND... %d\n"), e->nmatches);
#endif

	return (e->nmatches > 0)? 1: 0;
}

static void refdowngroupstack_incand (qse_lda_t* lda, void* dptr, qse_size_t dlen)
{
	QSE_ASSERT (dlen == 1);
	refdowngroupstack (((cand_t*)dptr)->group, lda->mmgr);
}

static int comp_cand (qse_lda_t* lda,
	const void* dptr1, qse_size_t dlen1,
	const void* dptr2, qse_size_t dlen2)
{
	cand_t* c1 = (cand_t*)dptr1;
	cand_t* c2 = (cand_t*)dptr2;
	return (c1->node == c2->node && 
	        c1->mptr == c2->mptr &&
	        c1->occ == c2->occ)? 0: 1;
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

	qse_lda_setfreeer (&e->cand.set[0], refdowngroupstack_incand);
	qse_lda_setfreeer (&e->cand.set[1], refdowngroupstack_incand);

	qse_lda_setcomper (&e->cand.set[0], comp_cand);
	qse_lda_setcomper (&e->cand.set[1], comp_cand);

	return 0;
}

static void fini_exec_dds (exec_t* e)
{
	qse_lda_fini (&e->cand.set[1]);
	qse_lda_fini (&e->cand.set[0]);
}

int qse_rex_exec (
	qse_rex_t* rex, const qse_cstr_t* str, 
	const qse_cstr_t* substr, qse_cstr_t* matstr)
{
	exec_t e;
	int n = 0;

	if (rex->code == QSE_NULL)
	{
		rex->errnum = QSE_REX_ENOCOMP;
		return -1;
	}

	QSE_MEMSET (&e, 0, QSE_SIZEOF(e));

	e.rex = rex;
	e.str.ptr = str->ptr;
	e.str.end = str->ptr + str->len;
	e.sub.ptr = substr->ptr;
	e.sub.end = substr->ptr + substr->len;

	if (init_exec_dds (&e, rex->mmgr) <= -1) return -1;

	while (e.sub.ptr <= e.sub.end)
	{
		n = exec (&e);
		if (n <= -1) 
		{
			n = -1;
			break;
		}

		if (n >= 1)
		{
			QSE_ASSERT (e.nmatches > 0);
			QSE_ASSERT (e.matchend != QSE_NULL);
			matstr->ptr = e.sub.ptr;
			matstr->len = e.matchend - e.sub.ptr;
			break;
		}

		e.sub.ptr++;
	}

	fini_exec_dds (&e);

	return n;
}
