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
	cut->sel.fcount = 0;

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
	cut->sel.fcount = 0;
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

int qse_cut_comp (
	qse_cut_t* cut, qse_cut_sel_id_t sel, 
	const qse_char_t* str, qse_size_t len)
{
	const qse_char_t* p = str;
	const qse_char_t* xnd = str + len;
	qse_cint_t c;

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
			sel = QSE_CUT_SEL_CHAR;
			c = NC (p, xnd);
			while (QSE_ISSPACE(c)) c = NC (p, xnd);
		}
		else if (c == QSE_T('f'))
		{
			sel = QSE_CUT_SEL_FIELD;
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

		cut->sel.lb->range[cut->sel.lb->len].id = sel;
		cut->sel.lb->range[cut->sel.lb->len].start = start;
		cut->sel.lb->range[cut->sel.lb->len].end = end;
		cut->sel.lb->len++;
		cut->sel.count++;
		if (sel == QSE_CUT_SEL_FIELD) cut->sel.fcount++;

		if (EOF(c)) break;
		if (c == QSE_T(',')) c = NC (p, xnd);
	}

	return 0;
}

static int read_char (qse_cut_t* cut, qse_char_t* c)
{
	qse_ssize_t n;

	if (cut->e.in.pos >= cut->e.in.len)
	{
		cut->errnum = QSE_CUT_ENOERR;
		n = cut->e.in.fun (
			cut, QSE_CUT_IO_READ, &cut->e.in.arg, 
			cut->e.in.buf, QSE_COUNTOF(cut->e.in.buf)
		);
		if (n <= -1) 
		{
			if (cut->errnum == QSE_CUT_ENOERR)
				SETERR0 (cut, QSE_CUT_EIOUSR);
			return -1;
		}

		if (n == 0) return 0; /* end of file */

		cut->e.in.len = n;
		cut->e.in.pos = 0;
	}

	*c = cut->e.in.buf[cut->e.in.pos++];
	return 1;
}

static int read_line (qse_cut_t* cut)
{
	qse_size_t len = 0;
	qse_char_t c;
	int n;

	qse_str_clear (&cut->e.in.line);
	if (cut->e.in.eof) return 0;

	while (1)
	{
		n = read_char (cut, &c);
		if (n <= -1) return -1;
		if (n == 0)
		{
			cut->e.in.eof = 1;
			if (len == 0) return 0;
			break;
		}

		if (c == QSE_T('\n')) 
		{
			/* don't include the line terminater to a line */
			/* TODO: support different line end convension */
			break;
		}

		if (qse_str_ccat (&cut->e.in.line, c) == (qse_size_t)-1)
		{
			SETERR0 (cut, QSE_CUT_ENOMEM);
			return -1;
		}
		len++;	
	}

	cut->e.in.num++;
	return 1;	
}

static int flush (qse_cut_t* cut)
{
	qse_size_t pos = 0;
	qse_ssize_t n;

	while (cut->e.out.len > 0)
	{
		cut->errnum = QSE_CUT_ENOERR;
		n = cut->e.out.fun (
			cut, QSE_CUT_IO_WRITE, &cut->e.out.arg,
			&cut->e.out.buf[pos], cut->e.out.len);

		if (n <= -1)
		{
			if (cut->errnum == QSE_CUT_ENOERR)
				SETERR0 (cut, QSE_CUT_EIOUSR);
			return -1;
		}

		if (n == 0)
		{
			/* reached the end of file - this is also an error */
			if (cut->errnum == QSE_CUT_ENOERR)
				SETERR0 (cut, QSE_CUT_EIOUSR);
			return -1;
		}

		pos += n;
		cut->e.out.len -= n;
	}

	return 0;
}

static int write_char (qse_cut_t* cut, qse_char_t c)
{
	cut->e.out.buf[cut->e.out.len++] = c;
	if (c == QSE_T('\n') ||
		cut->e.out.len >= QSE_COUNTOF(cut->e.out.buf))
	{
		return flush (cut);
	}

	return 0;
}

static int write_str (qse_cut_t* cut, const qse_char_t* str, qse_size_t len)
{
	qse_size_t i;
	for (i = 0; i < len; i++)
	{
		if (write_char (cut, str[i]) <= -1) return -1;
	}
		return 0;
}

int cut_chars (qse_cut_t* cut, qse_size_t start, qse_size_t end)
{
	const qse_char_t* ptr = QSE_STR_PTR(&cut->e.in.line);
	qse_size_t len = QSE_STR_LEN(&cut->e.in.line);

	if (len <= 0) 
	{
		/* TODO: delimited only */
		if (write_char (cut, QSE_T('\n')) <= -1) return -1;
	}
	else if (start <= end)
	{
		if (start <= len && end > 0)
		{
			if (start >= 1) start--;
			if (end >= 1) end--;

			if (end >= len) end = len - 1;

			if (write_str (cut, &ptr[start], end-start+1) <= -1)
				return -1;
		}

		/* TODO: DELIMTIED ONLY */
		if (write_char (cut, QSE_T('\n')) <= -1) return -1;
	}
	else
	{
		if (start > 0 && end <= len)
		{
			qse_size_t i;

			if (start >= 1) start--;
			if (end >= 1) end--;

			if (start >= len) start = len - 1;

			for (i = start; i >= end; i--)
			{
				if (write_char (cut, ptr[i]) <= -1)
					return -1;
			}
		}

		/* TODO: DELIMTIED ONLY */
		if (write_char (cut, QSE_T('\n')) <= -1) return -1;
	}

	return 0;
}

int cut_fields (qse_cut_t* cut, qse_size_t start, qse_size_t end)
{
/* TODO: field splitting... delimited only */
	return -1;
}

int qse_cut_exec (qse_cut_t* cut, qse_cut_io_fun_t inf, qse_cut_io_fun_t outf)
{
	int ret = 0;
	qse_ssize_t n;

	cut->e.out.fun = outf;
	cut->e.out.eof = 0;
	cut->e.out.len = 0;
	
	cut->e.in.fun = inf;
	cut->e.in.eof = 0;
	cut->e.in.len = 0;
	cut->e.in.pos = 0;
	cut->e.in.num = 0;
	if (qse_str_init (&cut->e.in.line, QSE_MMGR(cut), 256) == QSE_NULL)
	{
		SETERR0 (cut, QSE_CUT_ENOMEM);
		return -1;
	}

	cut->errnum = QSE_CUT_ENOERR;
	n = cut->e.in.fun (cut, QSE_CUT_IO_OPEN, &cut->e.in.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		ret = -1;
		if (cut->errnum == QSE_CUT_ENOERR)
			SETERR0 (cut, QSE_CUT_EIOUSR);
		goto done3;
	}
	if (n == 0)
	{
		/* EOF reached upon opening an input stream.
		* no data to process. this is success */
		goto done2;
	}

	cut->errnum = QSE_CUT_ENOERR;
	n = cut->e.out.fun (cut, QSE_CUT_IO_OPEN, &cut->e.out.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		ret = -1;
		if (cut->errnum == QSE_CUT_ENOERR)
			SETERR0 (cut, QSE_CUT_EIOUSR);
		goto done2;
	}
	if (n == 0)
	{
		/* still don't know if we will write something.
		 * just mark EOF on the output stream and continue */
		cut->e.out.eof = 1;
	}

		
	while (1)
	{
		qse_cut_sel_blk_t* b;
		qse_size_t i;

		n = read_line (cut);
		if (n <= -1) { ret = -1; goto done; }
		if (n == 0) goto done;

		if (cut->sel.fcount > 0)
		{
/* split the line into fields */
		}

		for (b = &cut->sel.fb; b != QSE_NULL; b = b->next)
		{
			for (i = 0; i < b->len; i++)
			{
				ret = (b->range[i].id == QSE_CUT_SEL_CHAR)?
					cut_chars (cut, b->range[i].start, b->range[i].end):
					cut_fields (cut, b->range[i].start, b->range[i].end);
				if (ret <= -1) goto done;
			}
		}
	}

done:
	cut->e.out.fun (cut, QSE_CUT_IO_CLOSE, &cut->e.out.arg, QSE_NULL, 0);
done2:
	cut->e.in.fun (cut, QSE_CUT_IO_CLOSE, &cut->e.in.arg, QSE_NULL, 0);
done3:
	qse_str_fini (&cut->e.in.line);
	return ret;
}
