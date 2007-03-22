/*
 * $Id: interp.c,v 1.20 2007-03-22 11:19:28 bacon Exp $
 */

#include <ase/stx/interp.h>
#include <ase/stx/method.h>
#include <ase/stx/object.h>
#include <ase/stx/array.h>
#include <ase/stx/class.h>
#include <ase/bas/assert.h>
#include <ase/bas/memory.h>

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
	ase_word_t* stack;
	ase_word_t  stack_size;
	ase_word_t  stack_base;
	ase_word_t  stack_top;

	ase_word_t  receiver;
	ase_word_t  method;
	ase_word_t  pc;

	/* cached information about the method above */
	ase_word_t* literals;
	ase_byte_t* bytecodes;
	ase_word_t  bytecode_size;
	ase_size_t  argcount;
	ase_size_t  tmpcount;
};

typedef struct process_t process_t;

static int __run_process (ase_stx_t* stx, process_t* proc);
static int __push_to_stack (ase_stx_t* stx, 
	process_t* proc, ase_word_t what, ase_word_t index);
static int __store_from_stack (ase_stx_t* stx, 
	process_t* proc, ase_word_t what, ase_word_t index);
static int __send_message (ase_stx_t* stx, process_t* proc, 
	ase_word_t nargs, ase_word_t selector, ase_bool_t to_super);
static int __return_from_message (ase_stx_t* stx, process_t* proc);
static int __dispatch_primitive (ase_stx_t* stx, process_t* proc, ase_word_t no);

int ase_stx_interp (ase_stx_t* stx, ase_word_t receiver, ase_word_t method)
{
	process_t proc;
	ase_stx_method_t* mthobj;
	ase_word_t i;
	int n;

	// TODO: size of process stack.
	proc.stack = (ase_word_t*)ase_malloc (10000 * ase_sizeof(ase_word_t));
	if (proc.stack == ASE_NULL) {
ase_printf (ASE_T("out of memory in ase_stx_interp\n"));
		return -1;
	}
	
	proc.stack_size = 10000;
	proc.stack_base = 0;
	proc.stack_top = 0;
	
	mthobj = (ase_stx_method_t*)ASE_STX_OBJECT(stx,method);
	ase_assert (mthobj != ASE_NULL);

	proc.literals = mthobj->literals;
	proc.bytecodes = ASE_STX_DATA(stx, mthobj->bytecodes);
	proc.bytecode_size = ASE_STX_SIZE(stx, mthobj->bytecodes);
	/* TODO: disable the method with arguments for start-up */
	proc.argcount = ASE_STX_FROM_SMALLINT(mthobj->argcount); 
	proc.tmpcount = ASE_STX_FROM_SMALLINT(mthobj->tmpcount);

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

	ase_free (proc.stack);
	return n;
}

static int __run_process (ase_stx_t* stx, process_t* proc)
{
	int code, next, next2;

	while (proc->pc < proc->bytecode_size) {
		code = proc->bytecodes[proc->pc++];

#ifdef DEBUG
		ase_printf (ASE_T("code = 0x%x\n"), code);
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
				proc->literals[next & 0x1F], ase_false) == -1) break;
		}	
		else if (code == 0x71) {
			/* send message to super */
			next = proc->bytecodes[proc->pc++];
			if (__send_message (stx, proc, next >> 5, 
				proc->literals[next & 0x1F], ase_true) == -1) break;
		}
		else if (code == 0x72) {
			/* send message to self extended */
			next = proc->bytecodes[proc->pc++];
			next2 = proc->bytecodes[proc->pc++];
			if (__send_message (stx, proc, next >> 5, 
				proc->literals[next2], ase_false) == -1) break;
		}
		else if (code == 0x73) {
			/* send message to super extended */
			next = proc->bytecodes[proc->pc++];
			next2 = proc->bytecodes[proc->pc++];
			if (__send_message (stx, proc, next >> 5, 
				proc->literals[next2], ase_true) == -1) break;
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
ase_printf (ASE_T("INVALID OPCODE...........\n"));
break;
		}
	}

	return 0;
}

static int __push_to_stack (ase_stx_t* stx, 
	process_t* proc, ase_word_t what, ase_word_t index)
{
	switch (what) {
	case 0: /* receiver variable */
		proc->stack[proc->stack_top++] = 
			ASE_STX_WORD_AT(stx, proc->stack[proc->stack_base], index);
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

static int __store_from_stack (ase_stx_t* stx, 
	process_t* proc, ase_word_t what, ase_word_t index)
{
	switch (what) {
	case 4: /* receiver variable */
		ASE_STX_WORD_AT(stx,proc->stack[proc->stack_base],index) = proc->stack[--proc->stack_top];
		break; 
	case 5: /* temporary location */
		proc->stack[proc->stack_base + 1 + index] = proc->stack[--proc->stack_top];
		break;
	}

	return 0;
}

static int __send_message (ase_stx_t* stx, process_t* proc, 
	ase_word_t nargs, ase_word_t selector, ase_bool_t to_super)
{
	ase_word_t receiver, method; 
	ase_word_t i, tmpcount, argcount;
	ase_stx_method_t* mthobj;

	ase_assert (ASE_STX_CLASS(stx,selector) == stx->class_symbol);

	receiver = proc->stack[proc->stack_top - nargs - 1];
	method = ase_stx_lookup_method (
		stx, ASE_STX_CLASS(stx,receiver), 
		ASE_STX_DATA(stx,selector), to_super);
	if (method == stx->nil) {
ase_printf (ASE_T("cannot find the method....\n"));
		return -1;	
	}

	mthobj = (ase_stx_method_t*)ASE_STX_OBJECT(stx,method);

	argcount = ASE_STX_FROM_SMALLINT(mthobj->argcount);
	tmpcount = ASE_STX_FROM_SMALLINT(mthobj->tmpcount);
	ase_assert (argcount == nargs);

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
	ase_assert (proc->stack_base > 0);

	proc->receiver = receiver;
	proc->method = method;
	proc->pc = 0;

	proc->literals = mthobj->literals;
	proc->bytecodes = ASE_STX_DATA(stx, mthobj->bytecodes);
	proc->bytecode_size = ASE_STX_SIZE(stx, mthobj->bytecodes);
	proc->argcount = argcount;
	proc->tmpcount = tmpcount;

	return 0;
}

static int __return_from_message (ase_stx_t* stx, process_t* proc)
{
	ase_word_t method, pc, stack_base;
	ase_stx_method_t* mthobj;

	if (proc->stack_base == 0) {
		/* return from the startup method */
		return -1;
	}

	stack_base = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount + 2];
	method = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount + 1];
	pc = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount];

	mthobj = (ase_stx_method_t*)ASE_STX_OBJECT(stx,method);
	ase_assert (mthobj != ASE_NULL);
	
	/* return value is located on top of the previous stack */
	proc->stack[proc->stack_base - 1] = proc->stack[proc->stack_top - 1];

	/* restore the stack pointers */
	proc->stack_top = proc->stack_base;
	proc->stack_base = stack_base;

	proc->receiver = proc->stack[stack_base];
	proc->method = method;
	proc->pc = pc;

	proc->literals = mthobj->literals;
	proc->bytecodes = ASE_STX_DATA(stx, mthobj->bytecodes);
	proc->bytecode_size = ASE_STX_SIZE(stx, mthobj->bytecodes);
	proc->argcount = ASE_STX_FROM_SMALLINT(mthobj->argcount); 
	proc->tmpcount = ASE_STX_FROM_SMALLINT(mthobj->tmpcount);

	return 0;
}


static int __dispatch_primitive (ase_stx_t* stx, process_t* proc, ase_word_t no)
{
	switch (no) {
	case 0:
		ase_printf (ASE_T("[[  hello stx smalltalk  ]]\n"));
		break;
	case 1:
		ase_printf (ASE_T("<<  AMAZING STX SMALLTALK WORLD  >>\n"));
		break;
	case 2:
		ase_printf (ASE_T("<<  FUNKY STX SMALLTALK  >> %d\n"), 
			ASE_STX_FROM_SMALLINT(proc->stack[proc->stack_base + 1]));
		break;
	case 3:
		ase_printf (ASE_T("<<  HIGH STX SMALLTALK  >> %d, %d\n"), 
			ASE_STX_FROM_SMALLINT(proc->stack[proc->stack_base + 1]),
			ASE_STX_FROM_SMALLINT(proc->stack[proc->stack_base + 2]));
		break;
	case 20:
		ase_printf (ASE_T("<< PRIMITIVE 20 >>\n"));
		break;
	}
	
	return 0;
}
