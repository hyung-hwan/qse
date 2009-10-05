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

#include "cut.h"
#include "../cmn/mem.h"
#include <qse/cmn/rex.h>
#include <qse/cmn/chr.h>

#define MAX QSE_TYPE_MAX(qse_size_t)

QSE_IMPLEMENT_COMMON_FUNCTIONS (cut)

static qse_cut_t* qse_cut_init (qse_cut_t* cut, qse_mmgr_t* mmgr);
static void qse_cut_fini (qse_cut_t* cut);

#define SETERR0(cut,num) \
do { qse_cut_seterror (cut, num, QSE_NULL); } while (0)

#define SETERR1(cut,num,argp,argl) \
do { \
	qse_cstr_t __ea__; \
	__ea__.ptr = argp; __ea__.len = argl; \
	qse_cut_seterror (cut, num, &__ea__); \
} while (0)

static int add_selector_block (qse_cut_t* cut)
{
	qse_cut_sel_blk_t* b;

	b = (qse_cut_sel_blk_t*) QSE_MMGR_ALLOC (cut->mmgr, QSE_SIZEOF(*b));
	if (b == QSE_NULL)
	{
		SETERR0 (cut, QSE_CUT_ENOMEM);
		return -1;
	}

	QSE_MEMSET (b, 0, QSE_SIZEOF(*b));
	b->next = QSE_NULL;
	b->len = 0;

	cut->sel.lb->next = b;
	cut->sel.lb = b;
	cut->sel.count = 0;

	return 0;
}

static void free_all_selector_blocks (qse_cut_t* cut)
{
	qse_cut_sel_blk_t* b;

	for (b = cut->sel.fb.next; b != QSE_NULL; )
	{
		qse_cut_sel_blk_t* nxt = b->next;
		QSE_MMGR_FREE (cut->mmgr, b);
		b = nxt;
	}

	cut->sel.lb = &cut->sel.fb;
	cut->sel.lb->len = 0;
	cut->sel.lb->next = QSE_NULL;
	cut->sel.count = 0;
}

qse_cut_t* qse_cut_open (qse_mmgr_t* mmgr, qse_size_t xtn)
{
	qse_cut_t* cut;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	cut = (qse_cut_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_cut_t) + xtn);
	if (cut == QSE_NULL) return QSE_NULL;

	if (qse_cut_init (cut, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (cut->mmgr, cut);
		return QSE_NULL;
	}

	return cut;
}

void qse_cut_close (qse_cut_t* cut)
{
	qse_cut_fini (cut);
	QSE_MMGR_FREE (cut->mmgr, cut);
}

static qse_cut_t* qse_cut_init (qse_cut_t* cut, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (cut, 0, QSE_SIZEOF(*cut));

	cut->mmgr = mmgr;
	cut->errstr = qse_cut_dflerrstr;

	/* on init, the last points to the first */
	cut->sel.lb = &cut->sel.fb;
	/* the block has no data yet */
	cut->sel.fb.len = 0;

	return cut;
}


static void qse_cut_fini (qse_cut_t* cut)
{
	free_all_selector_blocks (cut);
}

void qse_cut_setoption (qse_cut_t* cut, int option)
{
	cut->option = option;
}

int qse_cut_getoption (qse_cut_t* cut)
{
	return cut->option;
}

qse_size_t qse_cut_getmaxdepth (qse_cut_t* cut, qse_cut_depth_t id)
{
	return (id & QSE_CUT_DEPTH_REX_BUILD)? cut->depth.rex.build:
	       (id & QSE_CUT_DEPTH_REX_MATCH)? cut->depth.rex.match: 0;
}

void qse_cut_setmaxdepth (qse_cut_t* cut, int ids, qse_size_t depth)
{
	if (ids & QSE_CUT_DEPTH_REX_BUILD) cut->depth.rex.build = depth;
	if (ids & QSE_CUT_DEPTH_REX_MATCH) cut->depth.rex.match = depth;
}

int qse_cut_comp (qse_cut_t* cut, const qse_char_t* str, qse_size_t len)
{
	const qse_char_t* p = str;
	const qse_char_t* xnd = str + len;
	qse_cint_t c;
	int type = CHAR;

#define CC(x,y) (((x) <= (y))? ((qse_cint_t)*(x)): QSE_CHAR_EOF)
#define NC(x,y) (((x) < (y))? ((qse_cint_t)*(++(x))): QSE_CHAR_EOF)
#define EOF(x) ((x) == QSE_CHAR_EOF)
#define MASK_START (1 << 1)
#define MASK_END (1 << 2)

	free_all_selector_blocks (cut);

	if (len <= 0) return 0;

	xnd--; c = CC (p, xnd);
	while (1)
	{
		qse_size_t start = 0, end = 0;
		int mask = 0;

		while (QSE_ISSPACE(c)) c = NC (p, xnd); 
		if (EOF(c)) 
		{
			if (cut->sel.count > 0)
			{
				SETERR0 (cut, QSE_CUT_ESELNV);
				return -1;
			}

			break;
		}

		if (c == QSE_T('c'))
		{
			type = CHAR;
			c = NC (p, xnd);
			while (QSE_ISSPACE(c)) c = NC (p, xnd);
		}
		else if (c == QSE_T('f'))
		{
			type = FIELD;
			c = NC (p, xnd);
			while (QSE_ISSPACE(c)) c = NC (p, xnd);
		}

		if (QSE_ISDIGIT(c))
		{
			do 
			{ 
				start = start * 10 + (c - QSE_T('0')); 
				c = NC (p, xnd);
			} 
			while (QSE_ISDIGIT(c));

			while (QSE_ISSPACE(c)) c = NC (p, xnd);
			mask |= MASK_START;
		}
		else start++;

		if (c == QSE_T('-'))
		{
			c = NC (p, xnd);
			while (QSE_ISSPACE(c)) c = NC (p, xnd);

			if (QSE_ISDIGIT(c))
			{
				do 
				{ 
					end = end * 10 + (c - QSE_T('0')); 
					c = NC (p, xnd);
				} 
				while (QSE_ISDIGIT(c));
				mask |= MASK_END;
			}
			else end = MAX;

			while (QSE_ISSPACE(c)) c = NC (p, xnd);
		}
		else end = start;

		if (!(mask & (MASK_START | MASK_END)))
		{
			SETERR0 (cut, QSE_CUT_ESELNV);
			return -1;
		}

		if (cut->sel.lb->len >= QSE_COUNTOF(cut->sel.lb->range))
		{
			if (add_selector_block (cut) <= -1) 
			{
				return -1;
			}
		}

		cut->sel.lb->range[cut->sel.lb->len].type = type;
		cut->sel.lb->range[cut->sel.lb->len].start = start;
		cut->sel.lb->range[cut->sel.lb->len].end = end;
		cut->sel.lb->len++;
		cut->sel.count++;

		if (EOF(c)) break;
		if (c == QSE_T(',')) c = NC (p, xnd);
	}

	return 0;
}

int qse_cut_exec (qse_cut_t* cut, qse_cut_io_fun_t inf, qse_cut_io_fun_t outf)

{
	/* selector: c12-30, b30-40, f1-3,6-7,10
	 * default input field delimiter: TAB
	 * default output field delimiter: same as input delimiter 
	 * option: QSE_CUT_ONLYDELIMITED
	 */

{
int i;
for (i = 0; i < cut->sel.count; i++)
{
qse_printf (QSE_T("%d start = %llu, end = %llu\n"), 
	cut->sel.fb.range[i].type, 
	(unsigned long long)cut->sel.fb.range[i].start, 
	(unsigned long long)cut->sel.fb.range[i].end);
}
}
	return -1;
}
