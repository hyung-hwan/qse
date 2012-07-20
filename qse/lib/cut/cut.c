/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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
#include <qse/cmn/chr.h>

QSE_IMPLEMENT_COMMON_FUNCTIONS (cut)

static int qse_cut_init (qse_cut_t* cut, qse_mmgr_t* mmgr);
static void qse_cut_fini (qse_cut_t* cut);

#define SETERR0(cut,num) \
	do { qse_cut_seterror (cut, num, QSE_NULL); } while (0)

#define DFL_LINE_CAPA 256

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
	cut->sel.ccount = 0;

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
	cut->sel.ccount = 0;
}

qse_cut_t* qse_cut_open (qse_mmgr_t* mmgr, qse_size_t xtn)
{
	qse_cut_t* cut;

	cut = (qse_cut_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_cut_t) + xtn);
	if (cut == QSE_NULL) return QSE_NULL;

	if (qse_cut_init (cut, mmgr) <= -1)
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

static int qse_cut_init (qse_cut_t* cut, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (cut, 0, QSE_SIZEOF(*cut));

	cut->mmgr = mmgr;
	cut->errstr = qse_cut_dflerrstr;

	/* on init, the last points to the first */
	cut->sel.lb = &cut->sel.fb;
	/* the block has no data yet */
	cut->sel.fb.len = 0;

	cut->e.in.cflds = QSE_COUNTOF(cut->e.in.sflds);
	cut->e.in.flds = cut->e.in.sflds;

	if (qse_str_init (
		&cut->e.in.line, QSE_MMGR(cut), DFL_LINE_CAPA) <= -1)
	{
		SETERR0 (cut, QSE_CUT_ENOMEM);
		return -1;
	}

	return 0;
}

static void qse_cut_fini (qse_cut_t* cut)
{
	free_all_selector_blocks (cut);
	if (cut->e.in.flds != cut->e.in.sflds)
		QSE_MMGR_FREE (cut->mmgr, cut->e.in.flds);
	qse_str_fini (&cut->e.in.line);
}

void qse_cut_setoption (qse_cut_t* cut, int option)
{
	cut->option = option;
}

int qse_cut_getoption (qse_cut_t* cut)
{
	return cut->option;
}

void qse_cut_clear (qse_cut_t* cut)
{
	free_all_selector_blocks (cut);
	if (cut->e.in.flds != cut->e.in.sflds)
		QSE_MMGR_FREE (cut->mmgr, cut->e.in.flds);
	cut->e.in.cflds = QSE_COUNTOF(cut->e.in.sflds);
	cut->e.in.flds = cut->e.in.sflds;

	qse_str_clear (&cut->e.in.line);
	qse_str_setcapa (&cut->e.in.line, DFL_LINE_CAPA);
}

int qse_cut_comp (qse_cut_t* cut, const qse_char_t* str, qse_size_t len)
{
	const qse_char_t* p = str;
	const qse_char_t* lastp = str + len;
	qse_cint_t c;
	int sel = QSE_SED_SEL_CHAR;

#define CC(x,y) (((x) <= (y))? ((qse_cint_t)*(x)): QSE_CHAR_EOF)
#define NC(x,y) (((x) < (y))? ((qse_cint_t)*(++(x))): QSE_CHAR_EOF)
#define EOF(x) ((x) == QSE_CHAR_EOF)
#define MASK_START (1 << 1)
#define MASK_END (1 << 2)
#define MAX QSE_TYPE_MAX(qse_size_t)

	/* free selector blocks compiled previously */
	free_all_selector_blocks (cut);

	/* set the default delimiters */
	cut->sel.din = QSE_T(' ');
	cut->sel.dout = QSE_T(' ');

	/* if the selector string is empty, don't need to proceed */
	if (len <= 0) return 0;

	/* compile the selector string */
	lastp--; c = CC (p, lastp);
	while (1)
	{
		qse_size_t start = 0, end = 0;
		int mask = 0;

		while (QSE_ISSPACE(c)) c = NC (p, lastp); 
		if (EOF(c)) 
		{
			if (cut->sel.count > 0)
			{
				SETERR0 (cut, QSE_CUT_ESELNV);
				return -1;
			}

			break;
		}

		if (c == QSE_T('d'))
		{
			/* the next character is the input delimiter.
			 * the output delimiter defaults to the input 
			 * delimiter. */
			c = NC (p, lastp);
			if (EOF(c))
			{
				SETERR0 (cut, QSE_CUT_ESELNV);
				return -1;
			}
			cut->sel.din = c;
			cut->sel.dout = c;

			c = NC (p, lastp);
		}
		else if (c == QSE_T('D'))
		{
			/* the next two characters are the input and 
			 * the output delimiter each. */
			c = NC (p, lastp);
			if (EOF(c))
			{
				SETERR0 (cut, QSE_CUT_ESELNV);
				return -1;
			}
			cut->sel.din = c;

			c = NC (p, lastp);
			if (EOF(c))
			{
				SETERR0 (cut, QSE_CUT_ESELNV);
				return -1;
			}
			cut->sel.dout = c;

			c = NC (p, lastp);
		}
		else 
		{
			if (c == QSE_T('c') || c == QSE_T('f'))
			{
				sel = c;
				c = NC (p, lastp);
				while (QSE_ISSPACE(c)) c = NC (p, lastp);
			}
	
			if (QSE_ISDIGIT(c))
			{
				do 
				{ 
					start = start * 10 + (c - QSE_T('0')); 
					c = NC (p, lastp);
				} 
				while (QSE_ISDIGIT(c));
	
				while (QSE_ISSPACE(c)) c = NC (p, lastp);
				mask |= MASK_START;
			}
			else start++;

			if (c == QSE_T('-'))
			{
				c = NC (p, lastp);
				while (QSE_ISSPACE(c)) c = NC (p, lastp);

				if (QSE_ISDIGIT(c))
				{
					do 
					{ 
						end = end * 10 + (c - QSE_T('0')); 
						c = NC (p, lastp);
					} 
					while (QSE_ISDIGIT(c));
					mask |= MASK_END;
				}
				else end = MAX;

				while (QSE_ISSPACE(c)) c = NC (p, lastp);
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
			if (sel == QSE_SED_SEL_FIELD) cut->sel.fcount++;
			else cut->sel.ccount++;
		}

		if (EOF(c)) break;
		if (c == QSE_T(',')) c = NC (p, lastp);
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

	if (cut->option & QSE_CUT_TRIMSPACE) qse_str_trm (&cut->e.in.line);
	if (cut->option & QSE_CUT_NORMSPACE) qse_str_pac (&cut->e.in.line);
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

static int write_linebreak (qse_cut_t* cut)
{
	/* TODO: different line termination convention */
	return write_char (cut, QSE_T('\n'));
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

static int cut_chars (
	qse_cut_t* cut, qse_size_t start, qse_size_t end, int delim)
{
	const qse_char_t* ptr = QSE_STR_PTR(&cut->e.in.line);
	qse_size_t len = QSE_STR_LEN(&cut->e.in.line);

	if (len <= 0) return 0;

	if (start <= end)
	{
		if (start <= len && end > 0)
		{
			if (start >= 1) start--;
			if (end >= 1) end--;

			if (end >= len) end = len - 1;
						
			if (delim && write_char (cut, cut->sel.dout) <= -1)
				return -1;

			if (write_str (cut, &ptr[start], end-start+1) <= -1)
				return -1;

			return 1;
		}
	}
	else
	{
		if (start > 0 && end <= len)
		{
			qse_size_t i;

			if (start >= 1) start--;
			if (end >= 1) end--;

			if (start >= len) start = len - 1;

			if (delim && write_char (cut, cut->sel.dout) <= -1)
				return -1;

			for (i = start; i >= end; i--)
			{
				if (write_char (cut, ptr[i]) <= -1)
					return -1;
			}

			return 1;
		}
	}

	return 0;
}

static int isdelim (qse_cut_t* cut, qse_char_t c)
{
	return ((cut->option & QSE_CUT_WHITESPACE) && QSE_ISSPACE(c)) ||
	        (!(cut->option & QSE_CUT_WHITESPACE) && c == cut->sel.din);
}

static int split_line (qse_cut_t* cut)
{
	const qse_char_t* ptr = QSE_STR_PTR(&cut->e.in.line);
	qse_size_t len = QSE_STR_LEN(&cut->e.in.line);
	qse_size_t i, x = 0, xl = 0;

	cut->e.in.delimited = 0;
	cut->e.in.flds[x].ptr = ptr;
	for (i = 0; i < len; )
	{
		qse_char_t c = ptr[i++];
		if (isdelim(cut,c))
		{
			if (cut->option & QSE_CUT_FOLDDELIMS)
			{
				while (i < len && isdelim(cut,ptr[i])) i++;
			}

			cut->e.in.flds[x++].len = xl;

			if (x >= cut->e.in.cflds)
			{
				qse_cstr_t* tmp;
				qse_size_t nsz;

				nsz = cut->e.in.cflds;
				if (nsz > 100000) nsz += 100000;
				else nsz *= 2;
				
				tmp = QSE_MMGR_ALLOC (cut->mmgr, 
					QSE_SIZEOF(*tmp) * nsz);
				if (tmp == QSE_NULL) 
				{
					SETERR0 (cut, QSE_CUT_ENOMEM);
					return -1;
				}

				QSE_MEMCPY (tmp, cut->e.in.flds, 
					QSE_SIZEOF(*tmp) * cut->e.in.cflds);

				if (cut->e.in.flds != cut->e.in.sflds)
					QSE_MMGR_FREE (cut->mmgr, cut->e.in.flds);
				cut->e.in.flds = tmp;
				cut->e.in.cflds = nsz;
			}

			xl = 0;
			cut->e.in.flds[x].ptr = &ptr[i];
			cut->e.in.delimited = 1;
		}
		else xl++;
	}
	cut->e.in.flds[x].len = xl;
	cut->e.in.nflds = ++x;
	return 0;
}

static int cut_fields (
	qse_cut_t* cut, qse_size_t start, qse_size_t end, int delim)
{
	qse_size_t len = cut->e.in.nflds;

	if (!cut->e.in.delimited /*|| len <= 0*/) return 0;

	QSE_ASSERT (len > 0);
	if (start <= end)
	{
		if (start <= len && end > 0)
		{
			qse_size_t i;

			if (start >= 1) start--;
			if (end >= 1) end--;

			if (end >= len) end = len - 1;

			if (delim && write_char (cut, cut->sel.dout) <= -1)
				return -1;

			for (i = start; i <= end; i++)
			{
				if (write_str (cut, cut->e.in.flds[i].ptr, cut->e.in.flds[i].len) <= -1)
					return -1;

				if (i < end && write_char (cut, cut->sel.dout) <= -1)
					return -1;
			}

			return 1;
		}
	}
	else
	{
		if (start > 0 && end <= len)
		{
			qse_size_t i;

			if (start >= 1) start--;
			if (end >= 1) end--;

			if (start >= len) start = len - 1;

			if (delim && write_char (cut, cut->sel.dout) <= -1)
				return -1;

			for (i = start; i >= end; i--)
			{
				if (write_str (cut, cut->e.in.flds[i].ptr, cut->e.in.flds[i].len) <= -1)
					return -1;

				if (i > end && write_char (cut, cut->sel.dout) <= -1)
					return -1;
			}

			return 1;
		}
	}

	return 0;
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
		int id = 0; /* mark 'no output' so far */
		int delimited = 0;
		int linebreak = 0;

		n = read_line (cut);
		if (n <= -1) { ret = -1; goto done; }
		if (n == 0) goto done;

		if (cut->sel.fcount > 0)
		{
			if (split_line (cut) <= -1) { ret = -1; goto done; }
			delimited = cut->e.in.delimited;
		}

		for (b = &cut->sel.fb; b != QSE_NULL; b = b->next)
		{
			qse_size_t i;

			for (i = 0; i < b->len; i++)
			{
				if (b->range[i].id == QSE_SED_SEL_CHAR)
				{
					n = cut_chars (
						cut,
						b->range[i].start,
						b->range[i].end,
						id == 2 
					);
					if (n >= 1) 
					{
						/* mark a char's been output */
						id = 1;
					}
				}
				else
				{
					n = cut_fields (
						cut,
						b->range[i].start,
						b->range[i].end,
						id > 0 
					);
					if (n >= 1) 
					{
						/* mark a field's been output */
						id = 2;
					}
				}

				if (n <= -1) { ret = -1; goto done; }
			}
		}

		if (cut->sel.ccount > 0)
		{
			/* so long as there is a character selector,
			 * a newline must be printed */
			linebreak = 1;
		}
		else if (cut->sel.fcount > 0)
		{
			/* if only field selectors are specified */

			if (delimited) 
			{
				/* and if the input line is delimited,
				 * write a line break */
				linebreak = 1;
			}
			else if (!(cut->option & QSE_CUT_DELIMONLY))
			{
				/* if not delimited, write the
				 * entire undelimited input line depending
				 * on the option set. */
				if (write_str (cut,
					QSE_STR_PTR(&cut->e.in.line),
					QSE_STR_LEN(&cut->e.in.line)) <= -1)
				{
					ret = -1; goto done; 	
				}

				/* a line break is needed in this case */
				linebreak = 1;
			}
		}

		if (linebreak && write_linebreak(cut) <= -1) 
		{ 
			ret = -1; goto done; 
		}
	}

done:
	cut->e.out.fun (cut, QSE_CUT_IO_CLOSE, &cut->e.out.arg, QSE_NULL, 0);
done2:
	cut->e.in.fun (cut, QSE_CUT_IO_CLOSE, &cut->e.in.arg, QSE_NULL, 0);
done3:
	return ret;
}
