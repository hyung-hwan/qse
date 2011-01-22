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

#include "scm.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (scm)

static qse_scm_val_t static_values[3];

static qse_scm_t* qse_scm_init (
	qse_scm_t*           scm,
	qse_mmgr_t*          mmgr,
	qse_size_t           mem_ubound,
	qse_size_t mem_ubound_inc
);

static void qse_scm_fini (qse_scm_t* scm);
static qse_scm_val_t* mkcons (
	qse_scm_t* scm, qse_scm_val_t* car, qse_scm_val_t* cdr);

qse_scm_t* qse_scm_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize,
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	qse_scm_t* scm;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	scm = (qse_scm_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(qse_scm_t) + xtnsize
	);
	if (scm == QSE_NULL) return QSE_NULL;

	if (qse_scm_init (scm, mmgr, mem_ubound, mem_ubound_inc) == QSE_NULL)
	{
		QSE_MMGR_FREE (scm->mmgr, scm);
		return QSE_NULL;
	}

	return scm;
}

void qse_scm_close (qse_scm_t* scm)
{
	qse_scm_fini (scm);
	QSE_MMGR_FREE (scm->mmgr, scm);
}

static QSE_INLINE void delete_all_value_blocks (qse_scm_t* scm)
{
	while (scm->mem.vbl)
	{
		qse_scm_vbl_t* vbl = scm->mem.vbl;
		scm->mem.vbl = scm->mem.vbl->next;
		QSE_MMGR_FREE (scm->mmgr, vbl);
	}
}

static qse_scm_t* qse_scm_init (
	qse_scm_t* scm, qse_mmgr_t* mmgr, 
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	static qse_scm_val_t static_values[3] =
	{
		/* nil */
		{ (QSE_SCM_VAL_ATOM | QSE_SCM_VAL_MARK) },
		/* f */
		{ (QSE_SCM_VAL_ATOM | QSE_SCM_VAL_MARK) },
		/* t */
		{ (QSE_SCM_VAL_ATOM | QSE_SCM_VAL_MARK) }
	};

	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (scm, 0, QSE_SIZEOF(*scm));

	scm->mmgr = mmgr;

	/* set the default error string function */
	scm->err.str = qse_scm_dflerrstr;

	/* initialize error data */
	scm->err.num = QSE_SCM_ENOERR;
	scm->err.msg[0] = QSE_T('\0');

	/* initialize read data */
	scm->r.curc = QSE_CHAR_EOF;
	scm->r.curloc.line = 1;
	scm->r.curloc.colm = 0;

	if (qse_str_init(&scm->r.t.name, mmgr, 256) == QSE_NULL) 
	{
		QSE_MMGR_FREE (scm->mmgr, scm);
		return QSE_NULL;
	}

	/* initialize common values */
	scm->nil = &static_values[0];
	scm->f = &static_values[1];
	scm->t = &static_values[2];

	scm->mem.vbl = QSE_NULL;
	scm->mem.free = scm->nil;

	scm->genv = mkcons (scm, scm->nil, scm->nil);
	if (scm->genv == QSE_NULL)
	{
		delete_all_value_blocks (scm);
		qse_str_fini (&scm->r.t.name);
		QSE_MMGR_FREE (scm->mmgr, scm);
		return QSE_NULL;
	}

	scm->reg.dmp = scm->nil;
	scm->reg.env = scm->genv;

	return scm;
}

static void qse_scm_fini (qse_scm_t* scm)
{
	delete_all_value_blocks (scm);
	qse_str_fini (&scm->r.t.name);
}

void qse_scm_detachio (qse_scm_t* scm)
{
	if (scm->io.fns.out)
	{
		scm->io.fns.out (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.out, QSE_NULL, 0);
		scm->io.fns.out = QSE_NULL;
	}

	if (scm->io.fns.in)
	{
		scm->io.fns.in (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.in, QSE_NULL, 0);
		scm->io.fns.in = QSE_NULL;

		scm->r.curc = QSE_CHAR_EOF; /* TODO: needed??? */
	}
}

int qse_scm_attachio (qse_scm_t* scm, qse_scm_io_t* io)
{
	qse_scm_detachio(scm);

	QSE_ASSERT (scm->io.fns.in == QSE_NULL);
	QSE_ASSERT (scm->io.fns.out == QSE_NULL);

	scm->err.num = QSE_SCM_ENOERR;
	if (io->in (scm, QSE_SCM_IO_OPEN, &scm->io.arg.in, QSE_NULL, 0) <= -1)
	{
		if (scm->err.num == QSE_SCM_ENOERR)
			qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		return -1;
	}

	scm->err.num = QSE_SCM_ENOERR;
	if (io->out (scm, QSE_SCM_IO_OPEN, &scm->io.arg.out, QSE_NULL, 0) <= -1)
	{
		if (scm->err.num == QSE_SCM_ENOERR)
			qse_scm_seterror (scm,QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		io->in (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.in, QSE_NULL, 0);
		return -1;
	}

	scm->io.fns = *io;
	scm->r.curc = QSE_CHAR_EOF;
	scm->r.curloc.line = 1;
	scm->r.curloc.colm = 0;

	return 0;
}

static qse_scm_vbl_t* newvbl (qse_scm_t* scm, qse_size_t len)
{
	/* 
	 * create a new value block containing as many slots as len
	 */

	qse_scm_vbl_t* blk;
	qse_scm_val_t* v;
	qse_size_t i;

	blk = (qse_scm_vbl_t*) QSE_MMGR_ALLOC (
		scm->mmgr, 
		QSE_SIZEOF(qse_scm_vbl_t) + 
		QSE_SIZEOF(qse_scm_val_t) * len
	);
	if (blk == QSE_NULL)
	{
		scm->err.num = QSE_SCM_ENOMEM;
		return QSE_NULL;
	}

	/* initialize the block fields */
	blk->ptr = (qse_scm_val_t*)(blk + 1);
	blk->len = len;

	/* chain the value block to the block list */
	blk->next = scm->mem.vbl;
	scm->mem.vbl = blk;

	/* chain each slot to the free slot list */
	v = &blk->ptr[0];
	for (i = 0; i < len -1; i++) 
	{
		qse_scm_val_t* tmp = v++;
		tmp->u.cons.cdr = v;
	}
	v->u.cons.cdr = scm->mem.free;
	scm->mem.free = &blk->ptr[0];

	return blk;
};

/* TODO: redefine this ... */
#define IS_ATOM(v)  ((v)->flags & (QSE_SCM_VAL_STRING | QSE_SCM_VAL_NUMBER | QSE_SCM_VAL_PROC)

#define IS_MARKED(v)  ((v)->mark)
#define SET_MARK(v)   ((v)->mark = 1)
#define CLEAR_MARK(v) ((v)->mark = 0)

#define ZERO_DSW_COUNT(v) ((v)->dsw_count = 0)
#define GET_DSW_COUNT(v)  ((v)->dsw_count)
#define INC_DSW_COUNT(v)  ((v)->dsw_count++)

static void mark (qse_scm_t* scm, qse_scm_val_t* v)
{
	/* 
	 * mark values non-recursively with Deutsch-Schorr-Waite(DSW) algorithm 
	 * this algorithm builds backtraces directly into the value chain
	 * with the help of additional variables.
	 */

	qse_scm_val_t* parent, * me;

	/* initialization */
	parent = QSE_NULL;
	me = v;

	SET_MARK (me);
	/*if (!IS_ATOM(me))*/ ZERO_DSW_COUNT (me);

	while (1)
	{
		if (IS_ATOM(me) || GET_DSW_COUNT(me) >= 2)
		{
			/* 
			 * backtrack to the parent node 
			 */
			qse_scm_val_t* child;

			/* nothing more to backtrack? end of marking */
			if (parent == QSE_NULL) return;

			/* remember me temporarily for restoration below */
			child = me;

			/* the current parent becomes me */
			me = parent;

			/* change the parent to the parent of parent */
			parent = me->u.cona.val[GET_DSW_COUNT(me)];
			
			/* restore the cell contents */
			me->u.cona.val[GET_DSW_COUNT(me)] = child;

			/* increment the counter to indicate that the 
			 * 'count'th field has been processed.
			INC_DSW_COUNT (me);
		}
		else 
		{
			/* 
			 * move on to an unprocessed child 
			 */
			qse_scm_val_t* child;

			child = me->u.cona.val[GET_DSW_COUNT(me)];

			/* process the field */
			if (child && !ismark(child))
			{
				/* change the contents of the child chonse 
				 * to point to the current parent */
				me->u.cona.val[GET_DSW_COUNT(me)] = parent;

				/* link me to the head of parent list */
				parent = me;

				/* let me point to the child chosen */
				me = child;

				SET_MARK (me);
				/*if (!IS_ATOM(me))*/ ZERO_DSW_COUNT (me);
			}
			else
			{
				INC_DSW_COUNT (me)
			}
		}
	}
}


#if 0
static void mark (qse_scm_t* scm, qse_scm_val_t* v)
{
	qse_scm_val_t* t, * p, * q;

	t = QSE_NULL;
	p = v;

E2:
	setmark (p);

E3:
	if (isatom(p)) goto E6;

E4:
	q = p->u.cons.car;
	if (q && !ismark(q))
	{
		setatom (p);
		p->u.cons.car = t;
		t = p;
		p = q;
		goto E2;
	}

E5:
	q = p->u.cons.cdr;
	if (q && !ismark(q))
	{
		p->u.cons.cdr = t;
		t = p;
		p = q;
		goto E2;
	}

E6:
	if (!t) return;
	q = t;
	if (isatom(q))
	{
		clratom (q);
		t = q->u.cons.car;
		q->u.cons.car = p;
		p = q;
		goto E5;
	}
	else
	{
		t = q->u.cons.cdr;
		q->u.cons.cdr = p;
		p = q;
		goto E6;
	}
}
#endif

static void gc (qse_scm_t* scm, qse_scm_val_t* x, qse_scm_val_t* y)
{
	//mark (scm, scm->oblist);
	mark (scm, scm->genv);

	mark (scm, scm->reg.arg);
	mark (scm, scm->reg.env);
	mark (scm, scm->reg.cod);
	mark (scm, scm->reg.dmp);

	mark (scm, x);
	mark (scm, y);
}

static qse_scm_val_t* mkval (qse_scm_t* scm, qse_scm_val_t* x, qse_scm_val_t* y)
{
	qse_scm_val_t* v;

	if (scm->mem.free == scm->nil)
	{
		gc (scm, x, y);
		if (scm->mem.free == scm->nil)
		{
			if (newvbl (scm,  1000) == QSE_NULL) return QSE_NULL;
			QSE_ASSERT (scm->mem.free != scm->nil);
		}
	}

	v = scm->mem.free;
	scm->mem.free = v->u.cons.cdr;
	
	return v;
}


static qse_scm_val_t* mkcons (
	qse_scm_t* scm, qse_scm_val_t* car, qse_scm_val_t* cdr)
{
	qse_scm_val_t* v;

	v = mkval (scm, car, cdr);
	if (v == QSE_NULL) return QSE_NULL;

	v->flag = QSE_SCM_VAL_PAIR;
	v->u.cons.car = car;
	v->u.cons.cdr = car;

	return v;
}

