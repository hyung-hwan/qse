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

#define IS_NIL(x) ((x) == scm->nil)

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

static QSE_INLINE void delete_all_entity_blocks (qse_scm_t* scm)
{
	while (scm->mem.ebl)
	{
		qse_scm_enb_t* enb = scm->mem.ebl;
		scm->mem.ebl = scm->mem.ebl->next;
		QSE_MMGR_FREE (scm->mmgr, enb);
	}
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
				/* change the contents of the child chosen
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
/* TODO: how can i GC away those symbols not actually meaningful?
 *       marking objects referenced in symbol table prevent me from
 *       finding unused symbols... you keep on evaluating expressions
 *       with different symbols. you'll get out of memory. */
	mark (scm, scm->symtab);
	mark (scm, scm->gloenv);

	mark (scm, scm->reg.arg);
	mark (scm, scm->reg.env);
	mark (scm, scm->reg.cod);
	mark (scm, scm->reg.dmp);

	/* mark the temporaries */
	if (x) mark (scm, x);
	if (y) mark (scm, y);


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

	v = alloc_entity (scm, QSE_NULL, QSE_NULL);
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

	v = alloc_entity (scm, QSE_NULL, QSE_NULL);
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
	sym = alloc_entity (scm, nam, QSE_NULL);
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

	/* We piggy-back the syntax code to a symbol name.
	 * The syntax entity is basically a symbol except that the
	 * code field of its label entity is set to non-zero. 
	 * Read the comment in make_procedure_entity() for difference between
	 * the syntax entity and the procedure entity.
	 */
	TYPE(v) |= QSE_SCM_ENT_SYNT; 
	SYNT_CODE(v) = code; 

	return v;
}

static qse_scm_ent_t* make_procedure_entity (
	qse_scm_t* scm, const qse_char_t* name, int code)
{
	qse_scm_ent_t* sym, * proc, * pair;

	/* A procedure entity is a built-in function that can be
	 * overridden by a user while a syntax entity represents a 
	 * lower-level syntatic function that can't be overridden.
	 * 
	 * (define lambda 10) is legal but does not change the
	 *    meaning of lambda when used as a function name. 	
	 *
	 * (define tail 10) changes the meaning of eval totally.
	 * (tail '(1 2 3)) is not legal from now on.
	 *
	 * (define x lambda) is illegal as the lambda symbol
	 *
	 * (define lambda 10) followed by (define x lambda) lets the x symbol
	 * to be associated with 10 but you still can use lambda to create
	 * a closure as in ((lambda (x) (+ x 10)) 50)
	 *
	 * (define x tail) lets the 'x' symbol point to the eval procedure.
	 * (x '(1 2 3)) returns (2 3).
	 *	
	 * We implement the syntax entity as a symbol itself by ORing
	 * the TYPE field with QSE_SCM_ENT_SYNT and setting the syntax
	 * code in the symbol label entity.
	 *
	 * A procedure entity is an independent entity unlike the syntax
	 * entity. We explicitly create a symbol entity for the procedure name
	 * and associate it with the procedure entity in the global environment.
	 * If you redefine the symbol name to be something else, you won't be
	 * able to reference the procedure entity with the name. Worst case,
	 * it may be GCed out.
	 */ 

	/* create a symbol containing the name */
	sym = make_symbol_entity (scm, name);
	if (sym == QSE_NULL) return QSE_NULL;

	/* create an actual procedure value which is a number containing
	 * the opcode for the procedure */
	proc = alloc_entity (scm, sym, QSE_NULL);
	if (proc == QSE_NULL) return QSE_NULL;
	TYPE(proc) = QSE_SCM_ENT_PROC;
	ATOM(proc) = 1;
	PROC_CODE(proc) = code; 
	
	/* create a pair containing the name symbol and the procedure value */
	pair = make_pair_entity (scm, sym, proc);
	if (pair == QSE_NULL) return QSE_NULL;

	/* link it to the global environment */
	pair = make_pair_entity (scm, pair, PAIR_CAR(scm->gloenv));
	if (pair == QSE_NULL) return QSE_NULL;
	PAIR_CAR(scm->gloenv) = pair;

	return proc;
}

#define MAKE_SYNTAX_ENTITY(scm,name,code) QSE_BLOCK( \
	if (make_syntax_entity (scm, name, code) == QSE_NULL) return -1; \
)

static int build_syntax_entities (qse_scm_t* scm)
{
	qse_scm_ent_t* v;

	v = make_syntax_entity (scm, QSE_T("lambda"), 1);
	if (v == QSE_NULL) return -1;
	scm->lambda = v;

	v = make_syntax_entity (scm, QSE_T("quote"), 2);
	if (v == QSE_NULL) return -1;
	scm->quote = v;


	MAKE_SYNTAX_ENTITY (scm, QSE_T("define"), 3);
	MAKE_SYNTAX_ENTITY (scm, QSE_T("if"),     4);
	MAKE_SYNTAX_ENTITY (scm, QSE_T("begin"),  5);

	return 0;
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
	if (qse_str_init(&scm->r.t.name, mmgr, 256) == QSE_NULL) return QSE_NULL;

	scm->mem.ebl = QSE_NULL;
	scm->mem.free = scm->nil;

	/* initialize common values */
	scm->nil = &static_values[0];
	scm->f   = &static_values[1];
	scm->t   = &static_values[2];

	scm->lambda = scm->nil;
	scm->quote = scm->nil;

	/* initialize all the key data to nil before make_pair_entity()
	 * below. make_pair_entity() calls alloc_entity() that invokes
	 * gc() as this is the first time. As gc() marks all the key data,
	 * we need to initialize these to nil. */
	scm->reg.arg = scm->nil;
	scm->reg.dmp = scm->nil;
	scm->reg.cod = scm->nil;
	scm->reg.env = scm->nil;

	scm->symtab = scm->nil;
	scm->gloenv = scm->nil;
	scm->rstack = scm->nil;

	/* build the global environment entity as a pair */
	scm->gloenv = make_pair_entity (scm, scm->nil, scm->nil);
	if (scm->gloenv == QSE_NULL) goto oops;

	/* update the current environment to the global environment */
	scm->reg.env = scm->gloenv;

	if (build_syntax_entities (scm) <= -1) goto oops;
	return scm;

oops:
	delete_all_entity_blocks (scm);
	qse_str_fini (&scm->r.t.name);
	return QSE_NULL;
}

static void qse_scm_fini (qse_scm_t* scm)
{
	delete_all_entity_blocks (scm);
	qse_str_fini (&scm->r.t.name);
}


/*---------------------------------------------------------------------------
 * READER
 *---------------------------------------------------------------------------*/

enum list_flag_t
{
	QUOTED = (1 << 0),
	DOTTED = (1 << 1),
	CLOSED = (1 << 2)
};

enum tok_type_t
{
	TOK_END     = 0,
	TOK_INT     = 1,
	TOK_REAL    = 2,
	TOK_STRING  = 3,
	TOK_LPAREN  = 4,
	TOK_RPAREN  = 5,
	TOK_IDENT   = 6,
	TOK_DOT     = 7,
	TOK_QUOTE   = 8,
	TOK_QQUOTE  = 9, /* quasiquote */
	TOK_COMMA   = 10,
	TOK_COMMAAT = 11,
	TOK_INVALID = 50
};

#define TOK_CLEAR(scm) qse_str_clear(&(scm)->r.t.name)
#define TOK_TYPE(scm)  (scm)->r.t.type
#define TOK_IVAL(scm)  (scm)->r.t.ival
#define TOK_RVAL(scm)  (scm)->r.t.rval
#define TOK_STR(scm)   (scm)->r.t.name
#define TOK_SPTR(scm)  (scm)->r.t.name.ptr
#define TOK_SLEN(scm)  (scm)->r.t.name.len
#define TOK_LOC(scm) (scm)->r.t.loc
#define READ_CHAR(scm) QSE_BLOCK(if (read_char(scm) <= -1) return -1;)

static int read_char (qse_scm_t* scm)
{
	qse_ssize_t n;
	qse_char_t c;

/* TODO: do bufferring */
	scm->err.num = QSE_SCM_ENOERR;
	n = scm->io.fns.in (scm, QSE_SCM_IO_READ, &scm->io.arg.in, &c, 1);
	if (n <= -1)
	{
		if (scm->err.num == QSE_SCM_ENOERR)
			qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		return -1;
	}

/* TODO: handle the case when a new file is included or loaded ... 
 *       stacking of curloc is needed??? see qseawk for reference
 */
	if (n == 0) scm->r.curc = QSE_CHAR_EOF;
	else
	{
		scm->r.curc = c;

		if (c == QSE_T('\n'))
		{
			scm->r.curloc.colm = 0;
			scm->r.curloc.line++;
		}
		else scm->r.curloc.colm++;
	}

	return 0;
}

static int read_token (qse_scm_t* scm)
{
	TOK_CLEAR (scm);

	/* skip a series of white spaces and comment lines */
	do
	{
		/* skip white spaces */
		while (QSE_SCM_ISSPACE(scm,scm->r.curc)) READ_CHAR (scm);

		if (scm->r.curc != QSE_T(';')) break;

		/* skip a comment line */
		do { READ_CHAR (scm); }
		while (scm->r.curc != QSE_T('\n') &&
		       scm->r.curc != QSE_CHAR_EOF);
	} 
	while (1);

	TOK_LOC(scm) = scm->r.curloc;	
	if (scm->r.curc == QSE_CHAR_EOF)
	{
		TOK_TYPE(scm) = TOK_END;
		return 0;
	}

	switch (scm->r.curc)
	{
		case QSE_T('('):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_LPAREN;
			READ_CHAR (scm);	
			return 0;

		case QSE_T(')'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_RPAREN;
			READ_CHAR (scm);	
			return 0;

		case QSE_T('.'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_DOT;
			READ_CHAR (scm);	
			return 0;

		case QSE_T('\''):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_QUOTE;
			READ_CHAR (scm);	
			return 0;

		case QSE_T('`'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_QQUOTE;
			READ_CHAR (scm);	
			return 0;

		case QSE_T(','):
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);

			if (scm->r.curc == QSE_T('@'))
			{
				TOK_TYPE(scm) = TOK_COMMAAT;
				READ_CHAR (scm);	
			}
			else
			{

				TOK_TYPE(scm) = TOK_COMMA;
			}

			return 0;

		case QSE_T('#'):
			return 0;

		case QSE_T('\"'):
			return read_string_token (scm);
	}

	TOK_TYPE(scm) = TOK_INVALID;
	READ_CHAR (scm); /* consume */
	return 0;
}

static QSE_INLINE qse_scm_ent_t* push (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	qse_scm_ent_t* pair;

	pair = make_pair_entity (scm, obj, scm->rstack);
	if (pair == QSE_NULL) return QSE_NULL;

	scm->rstack = pair;

	/* return the top of the staich which is the containing pair */
	return pair;
}

static QSE_INLINE_ALWAYS void pop (qse_scm_t* scm)
{
	QSE_ASSERTX (
		scm->rstack != scm->nil,
		"You've called pop() more than push()"
	);
	scm->rstack = PAIR_CDR(scm->rstack);
}

static QSE_INLINE qse_scm_ent_t* enter_list (qse_scm_t* scm, int flagv)
{
	/* upon entering a list, it pushes three cells into a stack.
	 *
      *  rstack -------+
      *                 V
	 *             +---cons--+    
	 *         +------  |  -------+
	 *      car|   +---------+    |cdr
	 *         V                  |
	 *        nil#1               V
	 *                          +---cons--+
	 *                      +------  |  --------+
	 *                   car|   +---------+     |cdr
	 *                      v                   |
	 *                     nil#2                V
	 *                                       +---cons--+
	 *                                   +------  | --------+
	 *                                car|   +---------+    |cdr
	 *                                   V                  |
	 *                                flag number           V
	 *                                                  previous stack top
	 *
	 * nil#1 to store the first element in the list.
	 * nil#2 to store the last element in the list.
	 * both to be updated in chain_to_list() as items are added.
	 */
	return (push (scm, scm->mem.num[flagv]) == QSE_NULL ||
	        push (scm, scm->nil) == QSE_NULL ||
	        push (scm, scm->nil) == QSE_NULL)? QSE_NULL: scm->rstack;
}

static QSE_INLINE_ALWAYS qse_scm_ent_t* leave_list (qse_scm_t* scm, int* flagv)
{
	qse_scm_ent_t* head;

	/* the stack must not be empty */
	QSE_ASSERT (scm->rstack != scm->nil);

	/* remember the current list head */
	head = PAIR_CAR(PAIR_CDR(scm->rstack));

	/* upon leaving a list, it pops the three cells off the stack */
	pop (scm);
	pop (scm);
	pop (scm);

	if (scm->rstack == scm->nil)
	{
		/* the stack is empty after popping. 
		 * it is back to the top level. 
		 * the top level can never be quoted. */
		*flagv = 0;
	}
	else
	{
		/* restore the flag for the outer returning level */
		qse_scm_ent_t* flag = PAIR_CDR(PAIR_CDR(scm->rstack));
		QSE_ASSERT (QSE_SCM_TYPE(PAIR_CAR(flag)) == QSE_SCM_ENT_INT);
		*flagv = QSE_SCM_IVAL(PAIR_CAR(flag));
	}

	/* return the head of the list being left */
	return head;
}

static QSE_INLINE_ALWAYS void dot_list (qse_scm_t* scm)
{
	qse_scm_ent_t* cell;

	/* mark the state that a dot has appeared in the list */
	QSE_ASSERT (scm->rstack != scm->nil);
	cell = PAIR_CDR(PAIR_CDR(scm->rstack));
	PAIR_CAR(cell) = scm->mem.num[QSE_SCM_IVAL(PAIR_CAR(cell)) | DOTTED];
}

static qse_scm_ent_t* chain_to_list (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	qse_scm_ent_t* cell, * head, * tail, *flag;
	int flagv;

	/* the stack top is the pair pointing to the list tail */
	tail = scm->rstack;
	QSE_ASSERT (tail != scm->nil);

	/* the pair pointing to the list head is below the tail cell
	 * connected via cdr. */
	head = PAIR_CDR(tail);
	QSE_ASSERT (head != scm->nil);

	/* the pair pointing to the flag is below the head cell
	 * connected via cdr */
	flag = PAIR_CDR(head);

	/* retrieve the numeric flag value */
	QSE_ASSERT(QSE_SCM_TYPE(PAIR_CAR(flag)) == QSE_SCM_ENT_INT);
	flagv = (int)QSE_SCM_IVAL(PAIR_CAR(flag));

	if (flagv & CLOSED)
	{
		/* the list has already been closed. cannot add more items.  */
		qse_scm_seterror (scm, QSE_SCM_ERPAREN, QSE_NULL, &TOK_LOC(scm));
		return QSE_NULL;
	}
	else if (flagv & DOTTED)
	{
		/* the list must not be empty to have reached the dotted state */
		QSE_ASSERT (PAIR_CAR(tail) != scm->nil);

		/* chain the object via 'cdr' of the tail cell */
		PAIR_CDR(PAIR_CAR(tail)) = obj;

		/* update the flag to CLOSED */
		PAIR_CAR(flag) = scm->mem.num[flagv | CLOSED];
	}
	else
	{
		cell = make_pair_entity (scm, obj, scm->nil);
		if (cell == QSE_NULL) return QSE_NULL;

		if (PAIR_CAR(head) == scm->nil)
		{
			/* the list head is not set yet. it is the first
			 * element added to the list. let both head and tail
			 * point to the new cons cell */
			QSE_ASSERT (PAIR_CAR(tail) == scm->nil);
			PAIR_CAR(head) = cell; 
			PAIR_CAR(tail) = cell;
		}
		else
		{
			/* the new cons cell is not the first element.
			 * append it to the list */
			PAIR_CDR(PAIR_CAR(tail)) = cell;
			PAIR_CAR(tail) = cell;
		}
	}

	return obj;
}

static QSE_INLINE_ALWAYS int is_list_empty (qse_scm_t* scm)
{
	/* the stack must not be empty */
	QSE_ASSERTX (
		!IS_NIL(scm->rstack), 
		"You can not call this function while the stack is empty"		
	);

	/* if the tail pointer is pointing to nil, the list is empty */
	return IS_NIL(PAIR_CAR(scm->rstack));
}

static qse_scm_ent_t* read_entity (qse_scm_t* scm)
{
	/* this function read an s-expression non-recursively
	 * by manipulating its own stack. */

	int level = 0, flag = 0; 
	qse_scm_ent_t* obj;

	while (1)
	{
	redo:
		switch (TOK_TYPE(scm)) 
		{
			default:
				QSE_ASSERT (!"should never happen - invalid token type");
				qse_scm_seterror (scm, QSE_SCM_EINTERN, QSE_NULL, QSE_NULL);
				return QSE_NULL;

			case TOK_INVALID:
				qse_scm_seterror (
					scm, QSE_SCM_ESYNTAX,
					QSE_NULL, &TOK_LOC(scm));
				return QSE_NULL;
			
			case TOK_END:
				qse_scm_seterror (
					scm, QSE_SCM_EEND,
					QSE_NULL, &TOK_LOC(scm));
				return QSE_NULL;

			case TOK_QUOTE:
				if (level >= QSE_TYPE_MAX(int))
				{
					/* the nesting level has become too deep */
					qse_scm_seterror (
						scm, QSE_SCM_ELSTDEEP,
						QSE_NULL, &TOK_LOC(scm));
					return QSE_NULL;
				}

				/* enter a quoted string */
				flag |= QUOTED;
				if (enter_list (scm, flag) == QSE_NULL) return QSE_NULL;
				level++;

				/* force-chain the quote symbol to the new list entered */
				if (chain_to_list (scm, scm->mem.quote) == QSE_NULL) return QSE_NULL;

				/* read the next token */
				READ_TOKEN (scm);
				goto redo;
	
			case TOK_LPAREN:
				if (level >= QSE_TYPE_MAX(int))
				{
					/* the nesting level has become too deep */
					qse_scm_seterror (
						scm, QSE_SCM_ELSTDEEP,
						QSE_NULL, &TOK_LOC(scm));
					return QSE_NULL;
				}

				/* enter a normal string */
				flag = 0;
				if (enter_list (scm, flag) == QSE_NULL) return QSE_NULL;
				level++;

				/* read the next token */
				READ_TOKEN (scm);
				goto redo;

			case TOK_DOT:
				if (level <= 0 || is_list_empty (scm))
				{
					qse_scm_seterror (scm, QSE_SCM_ESYNTAX, QSE_NULL, &TOK_LOC(scm));
					return QSE_NULL;
				}

				dot_list (scm);
				READ_TOKEN (scm);
				goto redo;
		
			case TOK_RPAREN:
				if ((flag & QUOTED) || level <= 0)
				{
					/* the right parenthesis can never appear while 
					 * 'quoted' is true. 'quoted' is set to false when 
					 * entering a normal list. 'quoted' is set to true 
					 * when entering a quoted list. a quoted list does
					 * not have an explicit right parenthesis.
					 * so the right parenthesis can only pair up with 
					 * the left parenthesis for the normal list.
					 *
					 * For example, '(1 2 3 ') 5 6)
					 *
					 * this condition is triggerred when the first ) is 
					 * met after the second quote.
					 *
					 * also it is illegal to have the right parenthesis 
					 * with no opening(left) parenthesis, which is 
					 * indicated by level<=0.
					 */
					qse_scm_seterror (
						scm, QSE_SCM_ESYNTAX, 
						QSE_NULL, &TOK_LOC(scm));
					return QSE_NULL;
				}

				obj = leave_list (scm, &flag);

				level--;
				break;

			case TOK_INT:
				obj = qse_scm_makeint (&scm->mem, TOK_IVAL(scm));
				break;

			case TOK_REAL:
				obj = qse_scm_makereal (&scm->mem, TOK_RVAL(scm));
				break;
	
			case TOK_STRING:
				obj = make_string_entity (
					&scm->mem, TOK_SPTR(scm), TOK_SLEN(scm));
				break;

			case TOK_IDENT:
				obj = make_symbol_entity (scm, TOK_SPTR(scm));
				break;
		}

		/* check if the element is read for a quoted list */
		while (flag & QUOTED)
		{
			QSE_ASSERT (level > 0);

			/* if so, append the element read into the quote list */
			if (chain_to_list (scm, obj) == QSE_NULL) return QSE_NULL;

			/* exit out of the quoted list. the quoted list can have 
			 * one element only. */
			obj = leave_list (scm, &flag);

			/* one level up toward the top */
			level--;
		}

		/* check if we are at the top level */
		if (level <= 0) break; /* yes */

		/* if not, append the element read into the current list.
		 * if we are not at the top level, we must be in a list */
		if (chain_to_list (scm, obj) == QSE_NULL) return QSE_NULL;

		/* read the next token */
		READ_TOKEN (scm);
	}

	/* upon exit, we must be at the top level */
	QSE_ASSERT (level == 0);

	return obj;
}	
qse_scm_ent_t* qse_scm_read (qse_scm_t* scm)
{
	QSE_ASSERTX (
		scm->io.fns.in != QSE_NULL, 
		"Specify input function before calling qse_scm_read()"
	);

	while (1)
	{
	}
	return QSE_NULL;
}

