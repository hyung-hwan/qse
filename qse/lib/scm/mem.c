/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

	if (IS_SMALLINT(scm,v)) return;

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

			if (IS_SMALLINT(scm,child) || MARK(child))
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

	mark (scm, scm->r.s);
	mark (scm, scm->r.e);
	mark (scm, scm->p.s);
	mark (scm, scm->p.e);
	mark (scm, scm->e.arg);
	mark (scm, scm->e.env);
	mark (scm, scm->e.cod);
	mark (scm, scm->e.dmp);

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

	if (IS_NIL(scm,scm->mem.free))
	{
		/* if no free slot is available */
		gc (scm, x, y); /* perform garbage collection */
		if (IS_NIL(scm,scm->mem.free))
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

qse_scm_ent_t* qse_scm_makepairent (
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

qse_scm_ent_t* qse_scm_makenument (qse_scm_t* scm, qse_long_t val)
{
	qse_scm_ent_t* v;

	if (CAN_BE_SMALLINT(scm,val)) return TO_SMALLINT(scm,val);

	v = alloc_entity (scm, QSE_NULL, QSE_NULL);
	if (v == QSE_NULL) return QSE_NULL;

	TYPE(v) = QSE_SCM_ENT_NUM;
	ATOM(v) = 1;
	NUM_VALUE(v) = val;

	return v;
}

qse_scm_ent_t* qse_scm_makerealent (qse_scm_t* scm, qse_long_t val)
{
	qse_scm_ent_t* v;

	v = alloc_entity (scm, QSE_NULL, QSE_NULL);
	if (v == QSE_NULL) return QSE_NULL;

	TYPE(v) = QSE_SCM_ENT_REAL;
	ATOM(v) = 1;
	REAL_VALUE(v) = val;

	return v;
}

qse_scm_ent_t* qse_scm_makestrent (
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

qse_scm_ent_t* qse_scm_makenamentity (qse_scm_t* scm, const qse_char_t* str)
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
	LAB_UPTR(v) = QSE_NULL;

	return v;
}

qse_scm_ent_t* qse_scm_makesyment (qse_scm_t* scm, const qse_char_t* name)
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
	for (pair = scm->symtab; !IS_NIL(scm,pair); pair = PAIR_CDR(pair))
	{
		sym = PAIR_CAR(pair);
		if (qse_strcmp(name, LAB_PTR(SYM_NAME(sym))) == 0) return sym;
	}
	
	/* no existing symbol with such a name is found.  
	 * let's create a new symbol. the first step is to create a 
	 * string entity to contain the symbol name */
	nam = qse_scm_makenamentity (scm, name);
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
	pair = qse_scm_makepairent (scm, sym, scm->symtab);
	if (pair == QSE_NULL) return QSE_NULL;
	scm->symtab = pair;

	return sym;
}

qse_scm_ent_t* qse_scm_makesyntent (
	qse_scm_t* scm, const qse_char_t* name, void* uptr)
{
	qse_scm_ent_t* v;

	QSE_ASSERTX (uptr != QSE_NULL, "Syntax uptr must not be null");

	v = qse_scm_makesyment (scm, name);
	if (v == QSE_NULL) return QSE_NULL;

	SYNT(v) = 1;
	SYNT_UPTR(v) = uptr; 

	return v;
}

qse_scm_ent_t* qse_scm_makeprocent (
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
	sym = qse_scm_makesyment (scm, name);
	if (sym == QSE_NULL) return QSE_NULL;

	/* create an actual procedure value which is a number containing
	 * the opcode for the procedure */
	proc = alloc_entity (scm, sym, QSE_NULL);
	if (proc == QSE_NULL) return QSE_NULL;
	TYPE(proc) = QSE_SCM_ENT_PROC;
	ATOM(proc) = 1;
	PROC_CODE(proc) = code; 
	
	/* create a pair containing the name symbol and the procedure value */
	pair = qse_scm_makepairent (scm, sym, proc);
	if (pair == QSE_NULL) return QSE_NULL;

	/* link it to the global environment */
	pair = qse_scm_makepairent (scm, pair, PAIR_CAR(scm->gloenv));
	if (pair == QSE_NULL) return QSE_NULL;
	PAIR_CAR(scm->gloenv) = pair;

	return proc;
}

qse_scm_ent_t* qse_scm_makeclosent (
	qse_scm_t* scm, qse_scm_ent_t* code, qse_scm_ent_t* env)
{
	qse_scm_ent_t* clos;
	
	clos = alloc_entity (scm, code, env);
	if (clos == QSE_NULL) return QSE_NULL;

	TYPE(clos) = QSE_SCM_ENT_CLOS;
	CLOS_CODE(clos) = code;	
	CLOS_ENV(clos) = env;	

	return clos;
}
