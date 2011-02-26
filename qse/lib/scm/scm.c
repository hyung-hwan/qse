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

#define IS_SMALLINT(x)   ((qse_uintptr_t)(x) & 1)
#define TO_SMALLINT(x)   ((qse_scm_ent_t*)(qse_uintptr_t)(((x) << 1) | 1))
/* TODO: need more typecasting to something like int? how to i determine 
 *       the best type for the range in CAN_BE_SMALLINT()? 
#define FROM_SMALLINT(x) ((int)((qse_uintptr_t)(x) >> 1))
 */
#define FROM_SMALLINT(x) ((qse_uintptr_t)(x) >> 1)
/* TODO: change the smallint range... */
#define CAN_BE_SMALLINT(x) (((x) >= -16384) && ((x) <= 16383))

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
		QSE_MMGR_FREE (scm->mmgr, ((void**)enb)[-1]);
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
	 * Create a new value block containing as 'len' slots.
	 */

	void* raw;
	qse_scm_enb_t* blk;
	qse_scm_ent_t* v;
	qse_size_t i;

	/* Let me assume that an aligned memory pointer is an even address.
	 * malloc() returns an aligned memory pointer on most systems.
	 * However, I can't simply ignore oddball systems that returns
	 * an unaligned memory pointer. (Is there any?) A user may provide 
	 * a custom memory allocator that does not return unaligned memory
	 * pointer. I make the pointer to an entity block 2-byte aligned 
	 * hoping that the entity pointer alloc_entity() returns is also an
	 * even number. This, of couurse, requires that the size of 
	 * qse_scm_enb_t and qse_scm_ent_t is the multiple of 2.
	 * I do this for SMALLINT, not for memory alignemnt.The test for 
	 * SMALLINT can simply check the lowest bit. Am i doing too much?
	 */ 
	QSE_ASSERTX (
		QSE_SIZEOF(qse_scm_enb_t) % 2 == 0, 
		"This function is written assuming the size of qse_scm_enb_t is even"
	);
	QSE_ASSERTX (
		QSE_SIZEOF(qse_scm_ent_t) % 2 == 0, 
		"This function is written assuming the size of qse_scm_ent_t is even"
	);

	/* The actual memory block size is calculated as shown here:
	 *   QSE_SIZEOF(void*) to store the actual memory block pointer
	 *   1 to secure extra 1 byte required for 2-byte alignement.
	 *   QSE_SIZEOF(qse_scm_enb_t) to store the block header.
	 *   QSE_SIZEOF(qse_Scm_ent_t) * len to store the actual entities.
	 */
	raw = (qse_scm_enb_t*) QSE_MMGR_ALLOC (
		scm->mmgr, 
		QSE_SIZEOF(void*) + 1 + 
		QSE_SIZEOF(qse_scm_enb_t) + 
		QSE_SIZEOF(qse_scm_ent_t) * len
	);
	if (raw == QSE_NULL)
	{
		qse_scm_seterror (scm, QSE_SCM_ENOMEM, QSE_NULL, QSE_NULL);
		return QSE_NULL;
	}

	/* The entity block begins after the memory block pointer. */
	blk = (qse_scm_enb_t*)((qse_byte_t*)raw + QSE_SIZEOF(void*) + 1);

	/* Adjust the block pointer to an even number. 
	 * the resulting address is:
	 *     either the old address
	 *     or the old address - 1
	 */
	blk = (qse_scm_enb_t*)((qse_uintptr_t)blk & ~(qse_uintptr_t)1);

	/* Remember the raw block pointer.
	 * ((void**)blk)[-1] gets naturally aligned as blk is aligned. 
	 * It can be raw + 1 or the same as raw. */
	((void**)blk)[-1] = raw;

	/* Initialize the block fields */
	blk->ptr = (qse_scm_ent_t*)(blk + 1);
	blk->len = len;

	/* Chain the value block to the block list */
	blk->next = scm->mem.ebl;
	scm->mem.ebl = blk;

	/* Chain each slot to the free slot list using 
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
	 * Mark values non-recursively with Deutsch-Schorr-Waite(DSW) algorithm.
	 * This algorithm builds backtraces directly into the value chain
	 * with the help of additional variables.
	 */

	qse_scm_ent_t* parent, * me;

	if (IS_SMALLINT(v)) return;

	/* Initialization */
	parent = QSE_NULL;
	me = v;

	MARK(me) = 1;
	/*if (!ATOM(me))*/ DSWCOUNT(me) = 0;

	while (1)
	{
		if (ATOM(me) || DSWCOUNT(me) >= QSE_COUNTOF(me->u.ref.ent))
		{
			/* 
			 * Backtrack to the parent node 
			 */
			qse_scm_ent_t* child;

			/* Nothing more to backtrack? end of marking */
			if (parent == QSE_NULL) return;

			/* Remember me temporarily for restoration below */
			child = me;

			/* The current parent becomes me */
			me = parent;

			/* Change the parent to the parent of parent */
			parent = me->u.ref.ent[DSWCOUNT(me)];
			
			/* Restore the cell contents */
			me->u.ref.ent[DSWCOUNT(me)] = child;

			/* Increment the counter to indicate that the 
			 * 'count'th field has been processed. */
			DSWCOUNT(me)++;
		}
		else 
		{
			/* 
			 * Move on to an unprocessed child 
			 */
			qse_scm_ent_t* child;

			child = me->u.ref.ent[DSWCOUNT(me)];

			/* Process the field */
			QSE_ASSERT (child != QSE_NULL);

			if (IS_SMALLINT(child) || MARK(child))
			{
				/* Already marked. Increment the count */
				DSWCOUNT(me)++;
			}
			else
			{
				/* Change the contents of the child chosen
				 * to point to the current parent */
				me->u.ref.ent[DSWCOUNT(me)] = parent;

				/* Link me to the head of parent list */
				parent = me;

				/* Let me point to the child chosen */
				me = child;

				MARK(me) = 1;
				/*if (!ATOM(me))*/ DSWCOUNT(me) = 0;
			}
		}
	}
}

static void gc (qse_scm_t* scm, qse_scm_ent_t* x, qse_scm_ent_t* y)
{
/* TODO: How can i GC away those symbols not actually meaningful?
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

the following identifiers are syntactic keywors and should not be	
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

	if (IS_NIL(scm->mem.free))
	{
		/* if no free slot is available */
		gc (scm, x, y); /* perform garbage collection */
		if (IS_NIL(scm->mem.free))
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

static qse_scm_ent_t* make_number_entity (qse_scm_t* scm, qse_long_t val)
{
	qse_scm_ent_t* v;

	if (CAN_BE_SMALLINT(val)) return TO_SMALLINT(val);

	v = alloc_entity (scm, QSE_NULL, QSE_NULL);
	if (v == QSE_NULL) return QSE_NULL;

	TYPE(v) = QSE_SCM_ENT_NUM;
	ATOM(v) = 1;
	NUM_VALUE(v) = val;

	return v;
}

static qse_scm_ent_t* make_real_entity (qse_scm_t* scm, qse_long_t val)
{
	qse_scm_ent_t* v;

	v = alloc_entity (scm, QSE_NULL, QSE_NULL);
	if (v == QSE_NULL) return QSE_NULL;

	TYPE(v) = QSE_SCM_ENT_REAL;
	ATOM(v) = 1;
	REAL_VALUE(v) = val;

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
	 * lower-level syntactic function that can't be overridden.
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
		{ 0, 1, 1, QSE_SCM_ENT_T | QSE_SCM_ENT_BOOL }, 
		/* t */
		{ 0, 1, 1, QSE_SCM_ENT_F | QSE_SCM_ENT_BOOL }
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


	/* initialize common values */
	scm->nil    = &static_values[0];
	scm->f      = &static_values[1];
	scm->t      = &static_values[2];
	scm->lambda = scm->nil;
	scm->quote  = scm->nil;

	/* initialize entity block list */
	scm->mem.ebl = QSE_NULL;
	scm->mem.free = scm->nil;

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
	scm->r.s    = scm->nil;
	scm->r.e    = scm->nil;

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
	TOK_T       = 1,
	TOK_F       = 2,
	TOK_INT     = 3,
	TOK_REAL    = 4,
	TOK_SYMBOL  = 5,
	TOK_STRING  = 6,
	TOK_LPAREN  = 7,
	TOK_RPAREN  = 8,
	TOK_DOT     = 9,
	TOK_QUOTE   = 10,
	TOK_QQUOTE  = 11, /* quasiquote */
	TOK_COMMA   = 12,
	TOK_COMMAAT = 13,
#if 0
	TOK_INVALID = 50
#endif
};

#define TOK_CLR(scm)      qse_str_clear(&(scm)->r.t.name)
#define TOK_TYPE(scm)     (scm)->r.t.type
#define TOK_IVAL(scm)     (scm)->r.t.ival
#define TOK_RVAL(scm)     (scm)->r.t.rval
#define TOK_NAME(scm)     (&(scm)->r.t.name)
#define TOK_NAME_PTR(scm) TOK_NAME(scm)->ptr
#define TOK_NAME_LEN(scm) TOK_NAME(scm)->len
#define TOK_LOC(scm)      (scm)->r.t.loc

#define TOK_ADD_CHAR(scm,ch) QSE_BLOCK (\
	if (qse_str_ccat(TOK_NAME(scm), ch) == -1) \
	{ \
		qse_scm_seterror (scm, QSE_SCM_ENOMEM, QSE_NULL, &scm->r.curloc); \
		return -1; \
	} \
)

#define IS_DIGIT(ch) ((ch) >= QSE_T('0') && (ch) <= QSE_T('9'))
#define IS_SPACE(ch) ((ch) == QSE_T(' ') || (ch) == QSE_T('\t'))
#define IS_NEWLINE(ch) ((ch) == QSE_T('\n') || (ch) == QSE_T('\r'))
#define IS_WHSPACE(ch) IS_SPACE(ch) || IS_NEWLINE(ch)
#define IS_DELIM(ch) \
	(IS_WHSPACE(ch) || (ch) == QSE_T('(') || (ch) == QSE_T(')') || \
	 (ch) == QSE_T('\"') || (ch) == QSE_T(';') || (ch) == QSE_CHAR_EOF)

#define READ_CHAR(scm) QSE_BLOCK(if (read_char(scm) <= -1) return -1;)
#define READ_TOKEN(scm) QSE_BLOCK(if (read_token(scm) <= -1) return -1;)

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

static int read_string_token (qse_scm_t* scm)
{
	qse_cint_t c;
	int escaped = 0;
	int digit_count = 0;
	qse_cint_t c_acc = 0;

	while (1)
	{
		READ_CHAR (scm);
		c = scm->r.curc;

		if (c == QSE_CHAR_EOF)
		{
			qse_scm_seterror (
				scm, QSE_SCM_EENDSTR,
				QSE_NULL, &scm->r.curloc);
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= QSE_T('0') && c <= QSE_T('7'))
			{
				c_acc = c_acc * 8 + c - QSE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOK_ADD_CHAR (scm, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				TOK_ADD_CHAR (scm, c_acc);
				escaped = 0;
			}
		}
		else if (escaped == 2 || escaped == 4 || escaped == 8)
		{
			if (c >= QSE_T('0') && c <= QSE_T('9'))
			{
				c_acc = c_acc * 16 + c - QSE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOK_ADD_CHAR (scm, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= QSE_T('A') && c <= QSE_T('F'))
			{
				c_acc = c_acc * 16 + c - QSE_T('A') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOK_ADD_CHAR (scm, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= QSE_T('a') && c <= QSE_T('f'))
			{
				c_acc = c_acc * 16 + c - QSE_T('a') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOK_ADD_CHAR (scm, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				qse_char_t rc;

				rc = (escaped == 2)? QSE_T('x'):
				     (escaped == 4)? QSE_T('u'): QSE_T('U');

				if (digit_count == 0) TOK_ADD_CHAR (scm, rc);
				else TOK_ADD_CHAR (scm, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == QSE_T('\"'))
		{
			/* terminating quote */
			/*NEXT_CHAR_TO (scm, c);*/
			READ_CHAR (scm);
			break;
		}

		if (escaped == 0 && c == QSE_T('\\'))
		{
			escaped = 1;
			continue;
		}

		if (escaped == 1)
		{
			if (c == QSE_T('n')) c = QSE_T('\n');
			else if (c == QSE_T('r')) c = QSE_T('\r');
			else if (c == QSE_T('t')) c = QSE_T('\t');
			else if (c == QSE_T('f')) c = QSE_T('\f');
			else if (c == QSE_T('b')) c = QSE_T('\b');
			else if (c == QSE_T('v')) c = QSE_T('\v');
			else if (c == QSE_T('a')) c = QSE_T('\a');
			else if (c >= QSE_T('0') && c <= QSE_T('7')) 
			{
				escaped = 3;
				digit_count = 1;
				c_acc = c - QSE_T('0');
				continue;
			}
			else if (c == QSE_T('x')) 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#ifdef QSE_CHAR_IS_WCHAR
			else if (c == QSE_T('u') && QSE_SIZEOF(qse_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == QSE_T('U') && QSE_SIZEOF(qse_char_t) >= 4) 
			{
				escaped = 8;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#endif

			escaped = 0;
		}

		TOK_ADD_CHAR (scm, c);
	}

	TOK_TYPE(scm) = TOK_STRING;
	return 0;
}


enum read_number_token_flag_t
{
	RNT_NEGATIVE         = (1 << 0),
	RNT_SKIP_TO_FRACTION = (1 << 1)
};

static int read_number_token (qse_scm_t* scm, int flags)
{
	qse_long_t ival = 0;
	qse_real_t rval = .0;
	qse_real_t fraction;

	if (flags & RNT_SKIP_TO_FRACTION) goto fraction_part;

	do
	{
		ival = ival * 10 + (scm->r.curc - QSE_T('0'));
		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);
	}
	while (IS_DIGIT(scm->r.curc));

/* TODO: extend parsing floating point number  */
	if (scm->r.curc == QSE_T('.'))
	{
	fraction_part:
		fraction = 0.1;

		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);
		rval = (qse_real_t)ival;

		while (IS_DIGIT(scm->r.curc))
		{
			rval += (qse_real_t)(scm->r.curc - QSE_T('0')) * fraction;
			fraction *= 0.1;
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);
		}

		TOK_RVAL(scm) = rval;
		TOK_TYPE(scm) = TOK_REAL;
		if (flags & RNT_NEGATIVE) rval *= -1;
	}
	else
	{
		TOK_IVAL(scm) = ival;
		TOK_TYPE(scm) = TOK_INT;
		if (flags & RNT_NEGATIVE) ival *= -1;
	}

	return 0;
}

static int read_sharp_token (qse_scm_t* scm)
{
/* TODO: read a token beginning with #.*/

	TOK_ADD_CHAR (scm, scm->r.curc); /* add # to the token name */

	READ_CHAR (scm);
	switch (scm->r.curc)
	{
		case QSE_T('t'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);
			if (!IS_DELIM(scm->r.curc)) goto charname;
			TOK_TYPE(scm) = TOK_T;
			break;

		case QSE_T('f'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);
			if (!IS_DELIM(scm->r.curc)) goto charname;
			TOK_TYPE(scm) = TOK_F;
			break;

		case QSE_T('\\'):
			break;

		case QSE_T('b'):
			break;

		case QSE_T('o'):
			break;

		case QSE_T('d'):
			break;

		case QSE_T('x'):
			break;
	}

	return 0;


charname:
	do
	{
		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);	
	}
	while (!IS_DELIM(scm->r.curc));

/* TODO: character name comparison... */
	qse_scm_seterror (scm, QSE_SCM_ESHARP, QSE_NULL, &scm->r.curloc);
	return -1;
}

static int read_token (qse_scm_t* scm)
{
	int flags = 0;

	TOK_CLR (scm);

	/* skip a series of white spaces and comment lines */
	do
	{
		/* skip white spaces */
		while (IS_WHSPACE(scm->r.curc)) READ_CHAR (scm);

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
			READ_CHAR (scm);	
			if (!IS_DELIM(scm->r.curc)) 
			{
				flags |= RNT_SKIP_TO_FRACTION;
				goto try_number;
			}
			TOK_TYPE(scm) = TOK_DOT;
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
			else TOK_TYPE(scm) = TOK_COMMA;
			return 0;

		case QSE_T('#'):
			return read_sharp_token (scm);

		case QSE_T('\"'):
			return read_string_token (scm);
	}

	if (scm->r.curc == QSE_T('+') || scm->r.curc == QSE_T('-')) 
	{
		/* a number can begin with + or -. we don't know
		 * if it is the part of a number or not yet. 
		 * let's set the NEGATIVE bit in 'flags' if the sign is 
		 * negative for later use in case it is followed by a digit.
		 * we also add the sign character to the token name 
		 * so that we can form a complete symbol if the word turns
		 * out to be a symbol eventually.
		 */
		if (scm->r.curc == QSE_T('-')) flags |= RNT_NEGATIVE;
		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);
	}

	if (IS_DIGIT(scm->r.curc))
	{
	try_number:
		/* we got a digit, maybe or maybe not following a sign.
		 * call read_number_token() to read the current token 
		 * as a number. */
		if (read_number_token (scm, flags) <= -1) return -1;

		/* the read_number() function exits once it sees a character
		 * that can not compose a number. if it is a delimiter,
		 * the token is numeric. */
		if (IS_DELIM(scm->r.curc)) return 0;

		/* otherwise, we carry on reading trailing characters to
		 * compose a symbol token */
	}

	/* we got here as the current token does not begin with special
	 * token characters. treat it as a symbol token. */
	do 
	{
		TOK_ADD_CHAR (scm, scm->r.curc);	
		READ_CHAR (scm);
	} 
	while (!IS_DELIM(scm->r.curc));
	TOK_TYPE(scm) = TOK_SYMBOL; 

	return 0;
	

#if 0
	TOK_TYPE(scm) = TOK_INVALID;
	READ_CHAR (scm); /* consume */
	return 0;
#endif
}

static QSE_INLINE qse_scm_ent_t* push (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	qse_scm_ent_t* pair;

	pair = make_pair_entity (scm, obj, scm->r.s);
	if (pair == QSE_NULL) return QSE_NULL;

	scm->r.s = pair;

	/* return the top of the stack which is the containing pair */
	return pair;
}

static QSE_INLINE_ALWAYS void pop (qse_scm_t* scm)
{
	QSE_ASSERTX (
		!IS_NIL(scm->r.s),
		"You've called pop() more times than push()"
	);
	scm->r.s = PAIR_CDR(scm->r.s);
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
	return (push (scm, TO_SMALLINT(flagv)) == QSE_NULL ||
	        push (scm, scm->nil) == QSE_NULL ||
	        push (scm, scm->nil) == QSE_NULL)? QSE_NULL: scm->r.s;
}

static QSE_INLINE_ALWAYS qse_scm_ent_t* leave_list (qse_scm_t* scm, int* flagv)
{
	qse_scm_ent_t* head;

	/* the stack must not be empty */
	QSE_ASSERTX (
		!IS_NIL(scm->r.s), 
		"You cannot leave a list without entering it"
	);

	/* remember the current list head */
	head = PAIR_CAR(PAIR_CDR(scm->r.s));

	/* upon leaving a list, it pops the three cells off the stack */
	pop (scm);
	pop (scm);
	pop (scm);

	if (IS_NIL(scm->r.s))
	{
		/* the stack is empty after popping. 
		 * it is back to the top level. 
		 * the top level can never be quoted. */
		*flagv = 0;
	}
	else
	{
		/* restore the flag for the outer returning level */
		qse_scm_ent_t* flag = PAIR_CDR(PAIR_CDR(scm->r.s));
		QSE_ASSERT (TYPE(PAIR_CAR(flag)) == QSE_SCM_ENT_NUM);
		*flagv = NUM_VALUE(PAIR_CAR(flag));
	}

	/* return the head of the list being left */
	return head;
}

static QSE_INLINE_ALWAYS void dot_list (qse_scm_t* scm)
{
	qse_scm_ent_t* pair;
	int flagv;

	QSE_ASSERT (!IS_NIL(scm->r.s));

	/* mark the state that a dot has appeared in the list */
	pair = PAIR_CDR(PAIR_CDR(scm->r.s));
	flagv = FROM_SMALLINT(PAIR_CAR(pair));
	PAIR_CAR(pair) = TO_SMALLINT(flagv | DOTTED);
}

static qse_scm_ent_t* chain_to_list (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	qse_scm_ent_t* cell, * head, * tail, *flag;
	int flagv;

	/* the stack top is the pair pointing to the list tail */
	tail = scm->r.s;
	QSE_ASSERT (!IS_NIL(tail));

	/* the pair pointing to the list head is below the tail cell
	 * connected via cdr. */
	head = PAIR_CDR(tail);
	QSE_ASSERT (!IS_NIL(head));

	/* the pair pointing to the flag is below the head cell
	 * connected via cdr */
	flag = PAIR_CDR(head);

	/* retrieve the numeric flag value */
	QSE_ASSERT(IS_SMALLINT(PAIR_CAR(flag)));
	flagv = (int)FROM_SMALLINT(PAIR_CAR(flag));

	if (flagv & CLOSED)
	{
		/* the list has already been closed. cannot add more items.  */
		qse_scm_seterror (scm, QSE_SCM_ERPAREN, QSE_NULL, &TOK_LOC(scm));
		return QSE_NULL;
	}
	else if (flagv & DOTTED)
	{
		/* the list must not be empty to have reached the dotted state */
		QSE_ASSERT (!IS_NIL(PAIR_CAR(tail)));

		/* chain the object via 'cdr' of the tail cell */
		PAIR_CDR(PAIR_CAR(tail)) = obj;

		/* update the flag to CLOSED so that you can have more than
		 * one item after the dot. */
		PAIR_CAR(flag) = TO_SMALLINT(flagv | CLOSED);
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
		!IS_NIL(scm->r.s), 
		"You can not call this function while the stack is empty"		
	);

	/* if the tail pointer is pointing to nil, the list is empty */
	return IS_NIL(PAIR_CAR(scm->r.s));
}

static int read_entity (qse_scm_t* scm)
{
	/* this function read an s-expression non-recursively
	 * by manipulating its own stack. */

	int level = 0, flagv = 0; 
	qse_scm_ent_t* obj;

	while (1)
	{
	redo:
		switch (TOK_TYPE(scm)) 
		{
			default:
				QSE_ASSERT (!"should never happen - invalid token type");
				qse_scm_seterror (scm, QSE_SCM_EINTERN, QSE_NULL, QSE_NULL);
				return -1;

#if 0
			case TOK_INVALID:
				qse_scm_seterror (
					scm, QSE_SCM_ESYNTAX,
					QSE_NULL, &TOK_LOC(scm));
				return -1;
#endif
			
			case TOK_END:
				qse_scm_seterror (
					scm, QSE_SCM_EEND,
					QSE_NULL, &TOK_LOC(scm));
				return -1;

			case TOK_QUOTE:
				if (level >= QSE_TYPE_MAX(int))
				{
					/* the nesting level has become too deep */
					qse_scm_seterror (
						scm, QSE_SCM_ELSTDEEP,
						QSE_NULL, &TOK_LOC(scm));
					return -1;
				}

				/* enter a quoted string */
				flagv |= QUOTED;
				if (enter_list (scm, flagv) == QSE_NULL) return -1;
				level++;

				/* force-chain the quote symbol to the new list entered */
				if (chain_to_list (scm, scm->quote) == QSE_NULL) return -1;

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
					return -1;
				}

				/* enter a normal string */
				flagv = 0;
				if (enter_list (scm, flagv) == QSE_NULL) return -1;
				level++;

				/* read the next token */
				READ_TOKEN (scm);
				goto redo;

			case TOK_DOT:
				if (level <= 0 || is_list_empty (scm))
				{
					qse_scm_seterror (
						scm, QSE_SCM_EDOT, 
						QSE_NULL, &TOK_LOC(scm));
					return -1;
				}

				dot_list (scm);
				READ_TOKEN (scm);
				goto redo;
		
			case TOK_RPAREN:
				if ((flagv & QUOTED) || level <= 0)
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
						scm, QSE_SCM_ELPAREN, 
						QSE_NULL, &TOK_LOC(scm));
					return -1;
				}

				obj = leave_list (scm, &flagv);

				level--;
				break;

			case TOK_T:
				obj = scm->t;
				break;

			case TOK_F:
				obj = scm->f;
				break;

			case TOK_INT:
				obj = make_number_entity (scm, TOK_IVAL(scm));
				break;

			case TOK_REAL:
				obj = make_real_entity (scm, TOK_RVAL(scm));
				break;
	
			case TOK_STRING:
				obj = make_string_entity (
					scm, TOK_NAME_PTR(scm), TOK_NAME_LEN(scm));
				break;

			case TOK_SYMBOL:
				obj = make_symbol_entity (scm, TOK_NAME_PTR(scm));
				break;
		}

		/* check if the element is read for a quoted list */
		while (flagv & QUOTED)
		{
			QSE_ASSERT (level > 0);

			/* if so, append the element read into the quote list */
			if (chain_to_list (scm, obj) == QSE_NULL) return -1;

			/* exit out of the quoted list. the quoted list can have 
			 * one element only. */
			obj = leave_list (scm, &flagv);

			/* one level up toward the top */
			level--;
		}

		/* check if we are at the top level */
		if (level <= 0) break; /* yes */

		/* if not, append the element read into the current list.
		 * if we are not at the top level, we must be in a list */
		if (chain_to_list (scm, obj) == QSE_NULL) return -1;

		/* read the next token */
		READ_TOKEN (scm);
	}

	/* upon exit, we must be at the top level */
	QSE_ASSERT (level == 0);

	scm->r.e = obj; 
	return 0;
}	

qse_scm_ent_t* qse_scm_read (qse_scm_t* scm)
{
	QSE_ASSERTX (
		scm->io.fns.in != QSE_NULL, 
		"Specify input function before calling qse_scm_read()"
	);

	if (read_char(scm) <= -1) return QSE_NULL;
	if (read_token(scm) <= -1) return QSE_NULL;

#if 0
	scm.r.state = READ_NORMAL;
	do 
	{
		if (func[scm.r.state] (scm) <= -1) return QSE_NULL;
	}
	while (scm.r.state != READ_DONE)
#endif

#if 0
	do
	{
		qse_printf (QSE_T("TOKEN: [%s]\n"), TOK_NAME_PTR(scm));
		if (read_token(scm) <= -1) return QSE_NULL;
	}
	while (TOK_TYPE(scm) != TOK_END);
#endif

	if (read_entity (scm) <= -1) return QSE_NULL;

#if 0
{
	int i;
	for (i = 0; i < 100; i++)
	{
		qse_printf (QSE_T("%p\n"), alloc_entity(scm, QSE_NULL, QSE_NULL));
	}
}
#endif
	return scm->r.e;
}

