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

#define IS_NIL(x) ((x) != scm->nil)

static qse_scm_t* qse_scm_init (
	qse_scm_t*  scm,
	qse_mmgr_t* mmgr,
	qse_size_t  mem_ubound,
	qse_size_t  mem_ubound_inc
);

static void qse_scm_fini (
	qse_scm_t* scm
);

static qse_scm_ent_t* make_pair_entity (
	qse_scm_t*     scm,
	qse_scm_ent_t* car, 
	qse_scm_ent_t* cdr
);

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
	while (scm->mem.ebl)
	{
		qse_scm_enb_t* enb = scm->mem.ebl;
		scm->mem.ebl = scm->mem.ebl->next;
		QSE_MMGR_FREE (scm->mmgr, enb);
	}
}

static qse_scm_t* qse_scm_init (
	qse_scm_t* scm, qse_mmgr_t* mmgr, 
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	static qse_scm_ent_t static_values[3] =
	{
		/* dswcount, mark, atom, type */

		/* nil */
		{ 0, 1, 1, QSE_SCM_ENT_NIL },
		/* f */
		{ 0, 1, 1, QSE_SCM_ENT_T }, 
		/* t */
		{ 0, 1, 1, QSE_SCM_ENT_F }
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
	scm->f   = &static_values[1];
	scm->t   = &static_values[2];

	scm->mem.ebl = QSE_NULL;
	scm->mem.free = scm->nil;

	scm->genv = make_pair_entity (scm, scm->nil, scm->nil);
	if (scm->genv == QSE_NULL)
	{
		delete_all_value_blocks (scm);
		qse_str_fini (&scm->r.t.name);
		QSE_MMGR_FREE (scm->mmgr, scm);
		return QSE_NULL;
	}

	scm->symtab = scm->nil;

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
			qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		io->in (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.in, QSE_NULL, 0);
		return -1;
	}

	scm->io.fns = *io;
	scm->r.curc = QSE_CHAR_EOF;
	scm->r.curloc.line = 1;
	scm->r.curloc.colm = 0;

	return 0;
}

static qse_scm_enb_t* new_entity_block (qse_scm_t* scm, qse_size_t len)
{
	/* 
	 * create a new value block containing as many slots as len
	 */

	qse_scm_enb_t* blk;
	qse_scm_ent_t* v;
	qse_size_t i;

	blk = (qse_scm_enb_t*) QSE_MMGR_ALLOC (
		scm->mmgr, 
		QSE_SIZEOF(qse_scm_enb_t) + 
		QSE_SIZEOF(qse_scm_ent_t) * len
	);
	if (blk == QSE_NULL)
	{
		qse_scm_seterror (scm, QSE_SCM_ENOMEM, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	/* initialize the block fields */
	blk->ptr = (qse_scm_ent_t*)(blk + 1);
	blk->len = len;

	/* chain the value block to the block list */
	blk->next = scm->mem.ebl;
	scm->mem.ebl = blk;

	/* chain each slot to the free slot list using 
	 * the CDR field of an entity */
	v = &blk->ptr[0];
	for (i = 0; i < len -1; i++) 
	{
		qse_scm_ent_t* tmp = v++;
		PAIR_CDR(tmp) = v;
	}
	PAIR_CDR(v) = scm->mem.free;
	scm->mem.free = &blk->ptr[0];

	return blk;
};

static void mark (qse_scm_t* scm, qse_scm_ent_t* v)
{
	/* 
	 * mark values non-recursively with Deutsch-Schorr-Waite(DSW) algorithm 
	 * this algorithm builds backtraces directly into the value chain
	 * with the help of additional variables.
	 */

	qse_scm_ent_t* parent, * me;

	/* initialization */
	parent = QSE_NULL;
	me = v;

	MARK(me) = 1;
	/*if (!ATOM(me))*/ DSWCOUNT(me) = 0;

	while (1)
	{
		if (ATOM(me) || DSWCOUNT(me) >= QSE_COUNTOF(me->u.ref.ent))
		{
			/* 
			 * backtrack to the parent node 
			 */
			qse_scm_ent_t* child;

			/* nothing more to backtrack? end of marking */
			if (parent == QSE_NULL) return;

			/* remember me temporarily for restoration below */
			child = me;

			/* the current parent becomes me */
			me = parent;

			/* change the parent to the parent of parent */
			parent = me->u.ref.ent[DSWCOUNT(me)];
			
			/* restore the cell contents */
			me->u.ref.ent[DSWCOUNT(me)] = child;

			/* increment the counter to indicate that the 
			 * 'count'th field has been processed. */
			DSWCOUNT(me)++;
		}
		else 
		{
			/* 
			 * move on to an unprocessed child 
			 */
			qse_scm_ent_t* child;

			child = me->u.ref.ent[DSWCOUNT(me)];

			/* process the field */
			if (child && !MARK(child))
			{
				/* change the contents of the child chonse 
				 * to point to the current parent */
				me->u.ref.ent[DSWCOUNT(me)] = parent;

				/* link me to the head of parent list */
				parent = me;

				/* let me point to the child chosen */
				me = child;

				MARK(me) = 1;
				/*if (!ATOM(me))*/ DSWCOUNT(me) = 0;
			}
			else
			{
				/* increment the count */
				DSWCOUNT(me)++;
			}
		}
	}
}


#if 0
static void mark (qse_scm_t* scm, qse_scm_ent_t* v)
{
	qse_scm_ent_t* t, * p, * q;

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

static void gc (qse_scm_t* scm, qse_scm_ent_t* x, qse_scm_ent_t* y)
{
	//mark (scm, scm->symtab);
	mark (scm, scm->genv);

	mark (scm, scm->reg.arg);
	mark (scm, scm->reg.env);
	mark (scm, scm->reg.cod);
	mark (scm, scm->reg.dmp);

	/* mark the temporaries */
	mark (scm, x);
	mark (scm, y);


	/* scan the allocated values */
}

/*

rsr4 

the following identifiers are syntatic keywors and should not be	
used as variables.

 =>           do            or
 and          else          quasiquote
 begin        if            quote
 case         lambda        set!
 cond         let           unquote
 define       let*          unquote-splicing
 delay        letrec

however, you can allow for these keywords to be used as variables...

biniding, unbound...
environment.. a set of visible bindings at some point in a program.



                  type           atom       cons        
  number          NUMBER         Y 
  string          STRING         Y
  symbol          SYMBOL                    name,NIL
  syntax          SYNTAX|SYMBOL             name,NIL 
  proc            PROC           Y
  pair            PAIR           Y
  closure
  continuation

  an atom does not reference any other values.
  a symbol can be assoicated with property list
	(put 'a 'name "brian")
	(put 'a 'city "daegu")
	-------------------------
	(define a1 'a)
	(put a1 'name "brian")
	(put a1 'city "daegu")
	-------------------------
	(get a1 'name)
	(get a1 'city)

  a procedure is a privimitive routine built-in to scheme.
  a closure is an anonymous routine defined with lambda.
  both can be bound to a variable in the environment.

  a syntax is more primitive than a procedure.
  a syntax is created as if it is a symbol but not registerd 
  into an environment

         car            cdr
| STR  | PTR CHR ARR  |  -1           |
| PROC | PROCNUM      |               |
| SYM  | REF STR      | REF PROP LIST |
| SYN  | REF STR      | REF PROP LIST | 

*/
    
static qse_scm_ent_t* alloc_entity (
	qse_scm_t* scm, qse_scm_ent_t* x, qse_scm_ent_t* y)
{
	/* find a free value slot and return it.
	 * two parameters x and y are saved from garbage collection */

	qse_scm_ent_t* v;

	if (scm->mem.free == scm->nil)
	{
		/* if no free slot is available */
		gc (scm, x, y); /* perform garbage collection */
		if (scm->mem.free == scm->nil)
		{
			/* if no free slot is available after garbage collection,
			 * make new value blocks containing more free slots */

/* TODO: make the value block size configurable */
			if (new_entity_block (scm, 1000) == QSE_NULL) return QSE_NULL;
			QSE_ASSERT (scm->mem.free != scm->nil);
		}
	}

	v = scm->mem.free;
	scm->mem.free = PAIR_CDR(v);
	
	return v;
}

static qse_scm_ent_t* make_pair_entity (
	qse_scm_t* scm, qse_scm_ent_t* car, qse_scm_ent_t* cdr)
{
	qse_scm_ent_t* v;

	v = alloc_entity (scm, car, cdr);
	if (v == QSE_NULL) return QSE_NULL;

	TYPE(v) = QSE_SCM_ENT_PAIR;
	ATOM(v) = 0; /* a pair is not an atom as it references other entities */
	PAIR_CAR(v) = car;
	PAIR_CDR(v) = cdr;

	return v;
}


static qse_scm_ent_t* make_string_entity (
	qse_scm_t* scm, const qse_char_t* str, qse_size_t len)
{
	qse_scm_ent_t* v;

	v = alloc_entity (scm, scm->nil, scm->nil);
	if (v == QSE_NULL) return QSE_NULL;

	TYPE(v) = QSE_SCM_ENT_STR;
	ATOM(v) = 1;
/* TODO: allocate a string from internal managed region .
Calling strdup is not an option as it is not managed...
*/
	STR_PTR(v) = qse_strxdup (str, len, QSE_MMGR(scm));
	if (STR_PTR(v) == QSE_NULL) 
	{
		qse_scm_seterror (scm, QSE_SCM_ENOMEM, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}
	STR_LEN(v) = len;

	return v;
}

static qse_scm_ent_t* make_name_entity (qse_scm_t* scm, const qse_char_t* str)
{
	qse_scm_ent_t* v;

	v = alloc_entity (scm, scm->nil, scm->nil);
	if (v == QSE_NULL) return QSE_NULL;

	TYPE(v) = QSE_SCM_ENT_NAM;
	ATOM(v) = 1;
/* TODO: allocate a string from internal managed region .
Calling strdup is not an option as it is not managed...
*/
	LAB_PTR(v) = qse_strdup (str, QSE_MMGR(scm));
	if (LAB_PTR(v) == QSE_NULL) 
	{
		qse_scm_seterror (scm, QSE_SCM_ENOMEM, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}
	LAB_CODE(v) = 0;

	return v;
}

static qse_scm_ent_t* make_symbol_entity (qse_scm_t* scm, const qse_char_t* name)
{
	qse_scm_ent_t* pair, * sym, * nam;

/* TODO: use a hash table, red-black tree to maintain symbol table 
 * The current linear search algo is not performance friendly...
 */

	/* find if the symbol already exists by traversing the pair list 
	 * and inspecting the symbol name pointed to by CAR of each pair. 
	 *
	 * the symbol table is a list of pairs whose CAR points to a symbol
	 * and CDR is used for chaining.
	 *   
	 *   +-----+-----+
	 *   |     |     |
	 *   +-----+-----+
	 *  car |     | cdr        +-----+-----+
	 *      |     +----------> |     |     |
	 *      V                  +-----+-----+
	 *    +--------+          car | 
      *    | symbol |              V
	 *    +--------+           +--------+
	 *                         | symbol |
	 *                         +--------+
	 */
	for (pair = scm->symtab; !IS_NIL(pair); pair = PAIR_CDR(pair))
	{
		sym = PAIR_CAR(pair);
		if (qse_strcmp(name, LAB_PTR(SYM_NAME(sym))) == 0) return sym;
	}
	
	/* no existing symbol with such a name is found.  
	 * let's create a new symbol. the first step is to create a 
	 * string entity to contain the symbol name */
	nam = make_name_entity (scm, name);
	if (nam == QSE_NULL) return QSE_NULL;

	/* let's allocate the actual symbol entity that references the
	 * the symbol name entity created above */
	sym = alloc_entity (scm, nam, scm->nil);
	if (sym == QSE_NULL) return QSE_NULL;
	TYPE(sym) = QSE_SCM_ENT_SYM;
	ATOM(sym) = 0;
	SYM_NAME(sym) = nam;
	SYM_PROP(sym) = scm->nil; /* no properties yet */

	/* chain the symbol entity to the symbol table for lookups later */
	pair = make_pair_entity (scm, sym, scm->symtab);
	if (pair == QSE_NULL) return QSE_NULL;
	scm->symtab = pair;

	return sym;
}

static qse_scm_ent_t* make_syntax_entity (
	qse_scm_t* scm, const qse_char_t* name, int code)
{
	qse_scm_ent_t* v;

	QSE_ASSERTX (code > 0, "Syntax code must be greater than 0");

	v = make_symbol_entity (scm, name);
	if (v == QSE_NULL) return QSE_NULL;

	/* we piggy-back the syntax code to a symbol name.
	 * the syntax entity is basically a symbol except that the
	 * code field of its label entity is set to non-zero. 
	 */
	TYPE(v) |= QSE_SCM_ENT_SYNT; 
	SYNT_CODE(v) = code; 

	return v;
}

static qse_scm_ent_t* make_proc_entity (
	qse_scm_t* scm, const qse_char_t* name, int code)
{
	qse_scm_ent_t* sym, * proc, * pair;

	/* a procedure entity is a built-in function that
	 * that can be overridden by a user while a syntax entity
	 * represents a lower-level syntatic function that can't 
	 * be overriden. 
	 * (define lambda 10) is legal but does not change the
	 *    meaning of lambda when used as a function name. 	
	 * (define eval 10) changes the meaning of eval totally.
	 */ 

	/* create a symbol containing the name */
	sym = make_symbol_entity (scm, name);
	if (sym == QSE_NULL) return QSE_NULL;

	/* create an actual procecure value which is a number containing
	 * the opcode for the procedure */
	proc = alloc_entity (scm, scm->nil, scm->nil);
	if (proc == QSE_NULL) return QSE_NULL;
	TYPE(proc) = QSE_SCM_ENT_PROC;
	ATOM(proc) = 1;
	PROC_CODE(proc) = code; 
	
	/* create a pair containing the name symbol and the procedure value */
	pair = make_pair_entity (scm, sym, proc);
	if (pair == QSE_NULL) return QSE_NULL;

	/* link it to the global environment */
	pair = make_pair_entity (scm, pair, PAIR_CAR(scm->genv));
	if (pair == QSE_NULL) return QSE_NULL;
	PAIR_CAR(scm->genv) = pair;

	return proc;
}
