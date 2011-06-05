/*
 * $Id: interp.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/interp.h>
#include <qse/stx/method.h>
#include <qse/stx/object.h>
#include <qse/stx/array.h>
#include <qse/stx/class.h>
#include <qse/bas/assert.h>
#include <qse/bas/memory.h>

/*
activation record

....
....
....
-------------------
previous stack_base
-------------------
method
-------------------
pc
-------------------
temporaries
-------------------
arguments
-------------------
receiver
-------------------   <----- current stack_base
....
....
....

 */

struct process_t
{
	qse_word_t* stack;
	qse_word_t  stack_size;
	qse_word_t  stack_base;
	qse_word_t  stack_top;

	qse_word_t  receiver;
	qse_word_t  method;
	qse_word_t  pc;

	/* cached information about the method above */
	qse_word_t* literals;
	qse_byte_t* bytecodes;
	qse_word_t  bytecode_size;
	qse_size_t  argcount;
	qse_size_t  tmpcount;
};

typedef struct process_t process_t;

static int __run_process (qse_stx_t* stx, process_t* proc);
static int __push_to_stack (qse_stx_t* stx, 
	process_t* proc, qse_word_t what, qse_word_t index);
static int __store_from_stack (qse_stx_t* stx, 
	process_t* proc, qse_word_t what, qse_word_t index);
static int __send_message (qse_stx_t* stx, process_t* proc, 
	qse_word_t nargs, qse_word_t selector, qse_bool_t to_super);
static int __return_from_message (qse_stx_t* stx, process_t* proc);
static int __dispatch_primitive (qse_stx_t* stx, process_t* proc, qse_word_t no);

int qse_stx_interp (qse_stx_t* stx, qse_word_t receiver, qse_word_t method)
{
	process_t proc;
	qse_stx_method_t* mthobj;
	qse_word_t i;
	int n;

	// TODO: size of process stack.
	proc.stack = (qse_word_t*)qse_malloc (10000 * qse_sizeof(qse_word_t));
	if (proc.stack == QSE_NULL) {
qse_printf (QSE_T("out of memory in qse_stx_interp\n"));
		return -1;
	}
	
	proc.stack_size = 10000;
	proc.stack_base = 0;
	proc.stack_top = 0;
	
	mthobj = (qse_stx_method_t*)QSE_STX_OBJPTR(stx,method);
	qse_assert (mthobj != QSE_NULL);

	proc.literals = mthobj->literals;
	proc.bytecodes = QSE_STX_DATA(stx, mthobj->bytecodes);
	proc.bytecode_size = QSE_STX_SIZE(stx, mthobj->bytecodes);
	/* TODO: disable the method with arguments for start-up */
	proc.argcount = QSE_STX_FROMSMALLINT(mthobj->argcount); 
	proc.tmpcount = QSE_STX_FROMSMALLINT(mthobj->tmpcount);

	proc.receiver = receiver;
	proc.method = method;
	proc.pc = 0;

	proc.stack_base = proc.stack_top;

	/* push the receiver */
	proc.stack[proc.stack_top++] = receiver;

	/* push arguments */
	for (i = 0; i < proc.argcount; i++) {
		proc.stack[proc.stack_top++] = stx->nil;
	}

	/* secure space for temporaries */
	for (i = 0; i < proc.tmpcount; i++) 
		proc.stack[proc.stack_top++] = stx->nil;

	/* push dummy pc */
	proc.stack[proc.stack_top++] = 0;
	/* push dummy method */
	proc.stack[proc.stack_top++] = stx->nil;
	/* push dummy previous stack base */
	proc.stack[proc.stack_top++] = 0;

	n = __run_process (stx, &proc);

	qse_free (proc.stack);
	return n;
}

static int __run_process (qse_stx_t* stx, process_t* proc)
{
	int code, next, next2;

	while (proc->pc < proc->bytecode_size) {
		code = proc->bytecodes[proc->pc++];

#ifdef DEBUG
		qse_printf (QSE_T("code = 0x%x\n"), code);
#endif

		if (code >= 0x00 && code <= 0x3F) {
			/* stack - push */
			__push_to_stack (stx, proc, code >> 4, code & 0x0F);
		}
		else if (code >= 0x40 && code <= 0x5F) {
			/* stack - store */
			int what = code >> 4;
			int index = code & 0x0F;
			__store_from_stack (stx, proc, code >> 4, code & 0x0F);
		}

		/* TODO: more here .... */

		else if (code == 0x67) {
			/*  pop stack top */
			proc->stack_top--;
		}

		/* TODO: more here .... */

		else if (code == 0x6A) {
			proc->stack[proc->stack_top++] =  stx->nil;
		}
		else if (code == 0x6B) {
			proc->stack[proc->stack_top++] = stx->true;
		}
		else if (code == 0x6C) {
			proc->stack[proc->stack_top++] = stx->false;
		}
		else if (code == 0x6D) {
			/* push receiver */
			proc->stack[proc->stack_top++] = proc->receiver;
		}

		/* TODO: more here .... */

		else if (code == 0x70) {
			/* send message to self */
			next = proc->bytecodes[proc->pc++];
			if (__send_message (stx, proc, next >> 5, 
				proc->literals[next & 0x1F], qse_false) == -1) break;
		}	
		else if (code == 0x71) {
			/* send message to super */
			next = proc->bytecodes[proc->pc++];
			if (__send_message (stx, proc, next >> 5, 
				proc->literals[next & 0x1F], qse_true) == -1) break;
		}
		else if (code == 0x72) {
			/* send message to self extended */
			next = proc->bytecodes[proc->pc++];
			next2 = proc->bytecodes[proc->pc++];
			if (__send_message (stx, proc, next >> 5, 
				proc->literals[next2], qse_false) == -1) break;
		}
		else if (code == 0x73) {
			/* send message to super extended */
			next = proc->bytecodes[proc->pc++];
			next2 = proc->bytecodes[proc->pc++];
			if (__send_message (stx, proc, next >> 5, 
				proc->literals[next2], qse_true) == -1) break;
		}

		/* more code .... */
		else if (code == 0x78) {
			/* return receiver */
			proc->stack[proc->stack_top++] = proc->receiver;
			if (__return_from_message (stx, proc) == -1) break;
		}

		else if (code == 0x7C) {
			/* return from message */
			if (__return_from_message (stx, proc) == -1) break;
		}

		else if (code >= 0xF0 && code <= 0xFF)  {
			/* primitive */
			next = proc->bytecodes[proc->pc++];
			__dispatch_primitive (stx, proc, ((code & 0x0F) << 8) | next);
		}

		else {
qse_printf (QSE_T("INVALID OPCODE...........\n"));
break;
		}
	}

	return 0;
}

static int __push_to_stack (qse_stx_t* stx, 
	process_t* proc, qse_word_t what, qse_word_t index)
{
	switch (what) {
	case 0: /* receiver variable */
		proc->stack[proc->stack_top++] = 
			QSE_STX_WORD_AT(stx, proc->stack[proc->stack_base], index);
		break;
	case 1: /* temporary variable */
		proc->stack[proc->stack_top++] = 
			proc->stack[proc->stack_base + 1 + index];
		break;
	case 2: /* literal constant */
		proc->stack[proc->stack_top++] = proc->literals[index];
		break;
	case 3: /* literal variable */
		break;
	}

	return 0;
}

static int __store_from_stack (qse_stx_t* stx, 
	process_t* proc, qse_word_t what, qse_word_t index)
{
	switch (what) {
	case 4: /* receiver variable */
		QSE_STX_WORD_AT(stx,proc->stack[proc->stack_base],index) = proc->stack[--proc->stack_top];
		break; 
	case 5: /* temporary location */
		proc->stack[proc->stack_base + 1 + index] = proc->stack[--proc->stack_top];
		break;
	}

	return 0;
}

static int __send_message (qse_stx_t* stx, process_t* proc, 
	qse_word_t nargs, qse_word_t selector, qse_bool_t to_super)
{
	qse_word_t receiver, method; 
	qse_word_t i, tmpcount, argcount;
	qse_stx_method_t* mthobj;

	qse_assert (QSE_STX_CLASS(stx,selector) == stx->class_symbol);

	receiver = proc->stack[proc->stack_top - nargs - 1];
	method = qse_stx_lookup_method (
		stx, QSE_STX_CLASS(stx,receiver), 
		QSE_STX_DATA(stx,selector), to_super);
	if (method == stx->nil) {
qse_printf (QSE_T("cannot find the method....\n"));
		return -1;	
	}

	mthobj = (qse_stx_method_t*)QSE_STX_OBJPTR(stx,method);

	argcount = QSE_STX_FROMSMALLINT(mthobj->argcount);
	tmpcount = QSE_STX_FROMSMALLINT(mthobj->tmpcount);
	qse_assert (argcount == nargs);

	/* secure space for temporaries */
	for (i = 0; i < tmpcount; i++) {
		proc->stack[proc->stack_top++] = stx->nil;
	}

	/* push pc */
	proc->stack[proc->stack_top++] = proc->pc;
	/* push method */
	proc->stack[proc->stack_top++] = proc->method;
	/* push previous stack base */
	proc->stack[proc->stack_top++] = proc->stack_base;

	proc->stack_base = proc->stack_top - 3 - tmpcount - argcount - 1;
	qse_assert (proc->stack_base > 0);

	proc->receiver = receiver;
	proc->method = method;
	proc->pc = 0;

	proc->literals = mthobj->literals;
	proc->bytecodes = QSE_STX_DATA(stx, mthobj->bytecodes);
	proc->bytecode_size = QSE_STX_SIZE(stx, mthobj->bytecodes);
	proc->argcount = argcount;
	proc->tmpcount = tmpcount;

	return 0;
}

static int __return_from_message (qse_stx_t* stx, process_t* proc)
{
	qse_word_t method, pc, stack_base;
	qse_stx_method_t* mthobj;

	if (proc->stack_base == 0) {
		/* return from the startup method */
		return -1;
	}

	stack_base = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount + 2];
	method = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount + 1];
	pc = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount];

	mthobj = (qse_stx_method_t*)QSE_STX_OBJPTR(stx,method);
	qse_assert (mthobj != QSE_NULL);
	
	/* return value is located on top of the previous stack */
	proc->stack[proc->stack_base - 1] = proc->stack[proc->stack_top - 1];

	/* restore the stack pointers */
	proc->stack_top = proc->stack_base;
	proc->stack_base = stack_base;

	proc->receiver = proc->stack[stack_base];
	proc->method = method;
	proc->pc = pc;

	proc->literals = mthobj->literals;
	proc->bytecodes = QSE_STX_DATA(stx, mthobj->bytecodes);
	proc->bytecode_size = QSE_STX_SIZE(stx, mthobj->bytecodes);
	proc->argcount = QSE_STX_FROMSMALLINT(mthobj->argcount); 
	proc->tmpcount = QSE_STX_FROMSMALLINT(mthobj->tmpcount);

	return 0;
}


static int __dispatch_primitive (qse_stx_t* stx, process_t* proc, qse_word_t no)
{
	switch (no) {
	case 0:
		qse_printf (QSE_T("[[  hello stx smalltalk  ]]\n"));
		break;
	case 1:
		qse_printf (QSE_T("<<  AMAZING STX SMALLTALK WORLD  >>\n"));
		break;
	case 2:
		qse_printf (QSE_T("<<  FUNKY STX SMALLTALK  >> %d\n"), 
			QSE_STX_FROMSMALLINT(proc->stack[proc->stack_base + 1]));
		break;
	case 3:
		qse_printf (QSE_T("<<  HIGH STX SMALLTALK  >> %d, %d\n"), 
			QSE_STX_FROMSMALLINT(proc->stack[proc->stack_base + 1]),
			QSE_STX_FROMSMALLINT(proc->stack[proc->stack_base + 2]));
		break;
	case 20:
		qse_printf (QSE_T("<< PRIMITIVE 20 >>\n"));
		break;
	}
	
	return 0;
}
