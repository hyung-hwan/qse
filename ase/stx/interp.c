/*
 * $Id: interp.c,v 1.12 2005-09-12 15:55:13 bacon Exp $
 */

#include <xp/stx/interp.h>
#include <xp/stx/method.h>
#include <xp/stx/object.h>
#include <xp/stx/array.h>
#include <xp/bas/assert.h>

#define XP_STX_CONTEXT_SIZE      5
#define XP_STX_CONTEXT_STACK     0
#define XP_STX_CONTEXT_STACK_TOP 1
#define XP_STX_CONTEXT_RECEIVER  2
#define XP_STX_CONTEXT_PC        3
#define XP_STX_CONTEXT_METHOD    4

struct xp_stx_context_t
{
	xp_stx_objhdr_t header;
	xp_word_t stack;
	xp_word_t stack_top;
	xp_word_t receiver;
	xp_word_t pc;
	xp_word_t method;
};

typedef struct xp_stx_context_t xp_stx_context_t;

/* data structure for internal vm operation */
struct vmcontext_t
{
	/* from context */
	xp_word_t* stack;
	xp_word_t  stack_size;
	xp_word_t  stack_top;
	xp_word_t  receiver;
	xp_word_t  pc;

	/* from method */
	xp_byte_t* bytecodes;
	xp_word_t  bytecode_size;
	xp_word_t* literals;
};

typedef struct vmcontext_t vmcontext_t;

static int __dispatch_primitive (xp_stx_t* stx, int no, vmcontext_t* vmc);

xp_word_t xp_stx_new_context (xp_stx_t* stx, xp_word_t receiver, xp_word_t method)
{
	xp_word_t context;
	xp_stx_context_t* ctxobj;

	context = xp_stx_alloc_word_object(
		stx, XP_NULL, XP_STX_CONTEXT_SIZE, XP_NULL, 0);
	XP_STX_CLASS(stx,context) = stx->class_context;

	ctxobj = (xp_stx_context_t*)XP_STX_OBJECT(stx,context);
	ctxobj->stack = xp_stx_new_array (stx, 512); /* TODO: initial stack size */
	ctxobj->stack_top = XP_STX_TO_SMALLINT(0);
	ctxobj->receiver = receiver;
	ctxobj->pc = XP_STX_TO_SMALLINT(0);
	ctxobj->method = method;

	return context;
}

static int __activate_method (
	xp_stx_t* stx, vmcontext_t* vmc, xp_word_t argcount)
{
	/* TODO check stack overflow... 
	if (vmc->stack_top >= vmc->stack_size) PANIC
	*/

	//tmpcount = ...
	vmc->stack_top += argcount;
	while (tmpcount-- > 0) {
		vmc->stack[vmc->stack_top++] = stx->nil;
	}
}

int xp_stx_interp (xp_stx_t* stx, xp_word_t context)
{
	xp_stx_context_t* ctxobj;
	xp_stx_method_t* mthobj;
	vmcontext_t vmc;
	int code, next, next2;

	ctxobj = (xp_stx_context_t*)XP_STX_OBJECT(stx,context);
	mthobj = (xp_stx_method_t*)XP_STX_OBJECT(stx,ctxobj->method);

	vmc.stack = XP_STX_DATA(stx,ctxobj->stack);
	vmc.stack_size = XP_STX_SIZE(stx,ctxobj->stack);
	/* the beginning of the stack is reserved for temporaries */
	vmc.stack_top =
		XP_STX_FROM_SMALLINT(ctxobj->stack_top) + 
		XP_STX_FROM_SMALLINT(mthobj->tmpcount);
	vmc.receiver = ctxobj->receiver;
	vmc.pc = XP_STX_FROM_SMALLINT(ctxobj->pc);

	vmc.literals = mthobj->literals;
	vmc.bytecodes = XP_STX_DATA(stx, mthobj->bytecodes);
	vmc.bytecode_size = XP_STX_SIZE(stx, mthobj->bytecodes);

	while (vmc.pc < vmc.bytecode_size) {
		code = vmc.bytecodes[vmc.pc++];

#ifdef DEBUG
		xp_printf (XP_TEXT("code = 0x%x, %x\n"), code);
#endif

		if (code >= 0x00 && code <= 0x3F) {
			/* stack - push */
			int what = code >> 4;
			int index = code & 0x0F;

			switch (what) {
			case 0: /* receiver variable */
				vmc.stack[vmc.stack_top++] = XP_STX_WORD_AT(stx, vmc.receiver, index);
				break;
			case 1: /* temporary variable */
				vmc.stack[vmc.stack_top++] = vmc.stack[index];
				break;
			case 2: /* literal constant */
				vmc.stack[vmc.stack_top++] = vmc.literals[index];
				break;
			case 3: /* literal variable */
				break;
			}
		}
		else if (code >= 0x40 && code <= 0x5F) {
			/* stack - store */
			int what = code >> 4;
			int index = code & 0x0F;

			switch (what) {
			case 4: /* receiver variable */
				XP_STX_WORD_AT(stx,vmc.receiver,index) = vmc.stack[--vmc.stack_top];
				break; 
			case 5: /* temporary location */
				vmc.stack[index] = vmc.stack[--vmc.stack_top];
				break;
			}
		}

		/* more here .... */

		else if (code == 0x70) {
			/* send to self */
			int nargs, selector;
			next = vmc.bytecodes[vmc.pc++];

			nargs = next >> 5;

			/*
			selector = vmc.literals[next & 0x1F];
			receiver = vmc.stack[--vmc.stack_top];

			xp_stx_lookup_method (stx, class of receiver, );
			*/
		}
		else if (code == 0x71) {
			/* send to super */
			int nargs, selector;
			next = vmc.bytecodes[vmc.pc++];

			nargs = next >> 5;
			selector = next & 0x1F;
			
		}
		else if (code == 0x72) {
			/* send to self extended */
			next = vmc.bytecodes[vmc.pc++];
			next2 = vmc.bytecodes[vmc.pc++];
		}
		else if (code == 0x73) {
			/* send to super extended */
			next = vmc.bytecodes[vmc.pc++];
			next2 = vmc.bytecodes[vmc.pc++];
		}

		/* more code .... */

		else if (code >= 0xF0 && code <= 0xFF)  {
			/* primitive */
			next = vmc.bytecodes[vmc.pc++];
			__dispatch_primitive (stx, ((code & 0x0F) << 8) | next, &vmc);
		}
	}

	return 0;	
}

static int __dispatch_primitive (xp_stx_t* stx, int no, vmcontext_t* vmc)
{
	switch (no) {
	case 0:
		xp_printf (XP_TEXT("Hello, STX Smalltalk\n"));
		break;
	}
	
}
