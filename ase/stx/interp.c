/*
 * $Id: interp.c,v 1.13 2005-09-13 11:15:41 bacon Exp $
 */

#include <xp/stx/interp.h>
#include <xp/stx/method.h>
#include <xp/stx/object.h>
#include <xp/stx/array.h>
#include <xp/stx/class.h>
#include <xp/bas/assert.h>
#include <xp/bas/memory.h>

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
	xp_word_t* stack;
	xp_word_t  stack_size;
	xp_word_t  stack_base;
	xp_word_t  stack_top;

	xp_word_t  method;
	xp_word_t  pc;

	/* cached information about the method above */
	xp_word_t* literals;
	xp_byte_t* bytecodes;
	xp_word_t  bytecode_size;
	xp_size_t  argcount;
	xp_size_t  tmpcount;
};

typedef struct process_t process_t;

static int __dispatch_primitive (xp_stx_t* stx, process_t* proc, xp_word_t no);

static int __init_process (process_t* proc, xp_word_t stack_size)
{
	/* don't use the object model for process */
	proc->stack = (xp_word_t*)xp_malloc (stack_size * xp_sizeof(xp_word_t));
	if (proc->stack == XP_NULL) return -1;
	
	proc->stack_size = stack_size;
	proc->stack_base = 0;
	proc->stack_top = 0;

	return 0;
}

static void __deinit_process (process_t* proc)
{
	/* TODO: */
}


static int __send_to_self (xp_stx_t* stx, 
	process_t* proc, xp_word_t nargs, xp_word_t selector)
{
	xp_word_t receiver, method; 
	xp_word_t i, tmpcount, argcount;
	xp_stx_method_t* mthobj;

	xp_assert (XP_STX_CLASS(stx,selector) == stx->class_symbol);

	receiver = proc->stack[proc->stack_top - nargs - 1];
	method = xp_stx_lookup_method (stx, 
		XP_STX_CLASS(stx,receiver), XP_STX_DATA(stx,selector));
	if (method == stx->nil) {
xp_printf (XP_TEXT("cannot find the method....\n"));
		return -1;	
	}

	mthobj = (xp_stx_method_t*)XP_STX_OBJECT(stx,method);

	argcount = XP_STX_FROM_SMALLINT(mthobj->argcount);
	tmpcount = XP_STX_FROM_SMALLINT(mthobj->tmpcount);
	xp_assert (argcount == nargs);

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
	xp_assert (proc->stack_base > 0);

	proc->method = method;
	proc->pc = 0;

	proc->literals = mthobj->literals;
	proc->bytecodes = XP_STX_DATA(stx, mthobj->bytecodes);
	proc->bytecode_size = XP_STX_SIZE(stx, mthobj->bytecodes);
	proc->argcount = argcount;
	proc->tmpcount = tmpcount;

	return 0;
}

static int __return_from_message (xp_stx_t* stx, process_t* proc, int code)
{
	xp_word_t method, pc, stack_base;
	xp_stx_method_t* mthobj;

	if (proc->stack_base == 0) {
		/* return from the startup method */
		return -1;
	}

	stack_base = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount + 2];
	method = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount + 1];
	pc = proc->stack[proc->stack_base + 1 + proc->tmpcount + proc->argcount];


	mthobj = (xp_stx_method_t*)XP_STX_OBJECT(stx,method);
	xp_assert (mthobj != XP_NULL);
	
	proc->stack_top = proc->stack_base;
	proc->stack_base = stack_base;

	proc->method = method;
	proc->pc = pc;

	proc->literals = mthobj->literals;
	proc->bytecodes = XP_STX_DATA(stx, mthobj->bytecodes);
	proc->bytecode_size = XP_STX_SIZE(stx, mthobj->bytecodes);
	proc->argcount = XP_STX_FROM_SMALLINT(mthobj->argcount); 
	proc->tmpcount = XP_STX_FROM_SMALLINT(mthobj->tmpcount);

	return 0;
}

static int __run_process (xp_stx_t* stx, process_t* proc)
{
	int code, next, next2;

	while (proc->pc < proc->bytecode_size) {
		code = proc->bytecodes[proc->pc++];

#ifdef DEBUG
		xp_printf (XP_TEXT("code = 0x%x\n"), code);
#endif

		if (code >= 0x00 && code <= 0x3F) {
			/* stack - push */
			int what = code >> 4;
			int index = code & 0x0F;

			switch (what) {
#if 0
			case 0: /* receiver variable */
				proc->stack[proc->stack_top++] = XP_STX_WORD_AT(stx, proc->receiver, index);
				break;
			case 1: /* temporary variable */
				proc->stack[proc->stack_top++] = proc->stack[index];
				break;
#endif
			case 2: /* literal constant */
				proc->stack[proc->stack_top++] = proc->literals[index];
				break;
			case 3: /* literal variable */
				break;
			}
		}
		else if (code >= 0x40 && code <= 0x5F) {
			/* stack - store */
			int what = code >> 4;
			int index = code & 0x0F;

#if 0
			switch (what) {
			case 4: /* receiver variable */
				XP_STX_WORD_AT(stx,proc->receiver,index) = proc->stack[--proc->stack_top];
				break; 
			case 5: /* temporary location */
				proc->stack[index] = proc->stack[--proc->stack_top];
				break;
			}
#endif
		}

		/* more here .... */

		else if (code == 0x70) {
			next = proc->bytecodes[proc->pc++];
//xp_printf (XP_TEXT("%d, %d\n"), next >> 5, next & 0x1F);
			__send_to_self (stx, 
				proc, next >> 5, proc->literals[next & 0x1F]);
//xp_printf (XP_TEXT("done %d, %d\n"), next >> 5, next & 0x1F);
		}	
		else if (code == 0x71) {
			/* send to super */
			next = proc->bytecodes[proc->pc++];
			//__send_to_super (stx, 
			//	proc, next >> 5, proc->literals[next & 0x1F]);
		}
		else if (code == 0x72) {
			/* send to self extended */
			next = proc->bytecodes[proc->pc++];
			next2 = proc->bytecodes[proc->pc++];
			__send_to_self (stx, 
				proc, next >> 5, proc->literals[next2]);
		}
		else if (code == 0x73) {
			/* send to super extended */
			next = proc->bytecodes[proc->pc++];
			next2 = proc->bytecodes[proc->pc++];
			//__send_to_super (stx, 
			//	proc, next >> 5, proc->literals[next2]);
		}

		/* more code .... */
		else if (code == 0x7C) {
			/* return from message */
			if (__return_from_message (stx, proc, code) == -1) break;
		}

		else if (code >= 0xF0 && code <= 0xFF)  {
			/* primitive */
			next = proc->bytecodes[proc->pc++];
			__dispatch_primitive (stx, proc, ((code & 0x0F) << 8) | next);
		}
	}

	return 0;
}

int xp_stx_interp (xp_stx_t* stx, xp_word_t receiver, xp_word_t method)
{
	process_t proc;
	xp_stx_method_t* mthobj;
	xp_word_t i;

	// TODO: size of process stack.
	if (__init_process(&proc, 10000) == -1) return -1;
	
	mthobj = (xp_stx_method_t*)XP_STX_OBJECT(stx,method);
	xp_assert (mthobj != XP_NULL);

	proc.literals = mthobj->literals;
	proc.bytecodes = XP_STX_DATA(stx, mthobj->bytecodes);
	proc.bytecode_size = XP_STX_SIZE(stx, mthobj->bytecodes);
	/* TODO: disable the method with arguments for start-up */
	proc.argcount = XP_STX_FROM_SMALLINT(mthobj->argcount); 
	proc.tmpcount = XP_STX_FROM_SMALLINT(mthobj->tmpcount);

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

	return __run_process (stx, &proc);
}


static int __dispatch_primitive (xp_stx_t* stx, process_t* proc, xp_word_t no)
{
	switch (no) {
	case 0:
		xp_printf (XP_TEXT("[[  hello stx smalltalk  ]]\n"));
		break;
	case 1:
		xp_printf (XP_TEXT("<<  AMAZING STX SMALLTALK WORLD  >>\n"));
		break;
	case 2:
		xp_printf (XP_TEXT("<<  FUNKY STX SMALLTALK  >> %d\n"), 
			XP_STX_FROM_SMALLINT(proc->stack[proc->stack_base + 1]));
		break;
	}
	
}
