/*
 * $Id: interp.c,v 1.6 2005-08-16 15:49:04 bacon Exp $
 */

#include <xp/stx/interp.h>
#include <xp/stx/method.h>
#include <xp/stx/object.h>
#include <xp/stx/array.h>
#include <xp/bas/assert.h>

#define XP_STX_CONTEXT_SIZE      5
#define XP_STX_CONTEXT_STACK     0
#define XP_STX_CONTEXT_STACK_TOP 1
#define XP_STX_CONTEXT_METHOD    2
#define XP_STX_CONTEXT_RECEIVER  3
#define XP_STX_CONTEXT_IP        4

struct xp_stx_context_t
{
	xp_stx_objhdr_t header;
	xp_word_t stack;
	xp_word_t stack_top;
	xp_word_t method;
	xp_word_t receiver;
	xp_word_t ip;
};

typedef struct xp_stx_context_t xp_stx_context_t;

xp_word_t xp_stx_new_context (xp_stx_t* stx, xp_word_t method, xp_word_t receiver)
{
	xp_word_t context;
	xp_stx_context_t* ctxobj;

	context = xp_stx_alloc_word_object(
		stx, XP_NULL, XP_STX_CONTEXT_SIZE, XP_NULL, 0);
	XP_STX_CLASS(stx,context) = stx->class_context;

	ctxobj = (xp_stx_context_t*)XP_STX_OBJECT(stx,context);
	ctxobj->stack = xp_stx_new_array (stx, 256); /* TODO: initial stack size */
	ctxobj->stack_top = XP_STX_TO_SMALLINT(0);
	ctxobj->method = method;
	ctxobj->receiver = receiver;
	ctxobj->ip = XP_STX_TO_SMALLINT(0);

	return context;
}

static int __push_receiver_variable (
	xp_stx_t* stx, int index, xp_stx_context_t* ctxobj);
static int __push_temporary_variable (
	xp_stx_t* stx, int index, xp_stx_context_t* ctxobj);

int xp_stx_interp (xp_stx_t* stx, xp_word_t context)
{
	xp_stx_context_t* ctxobj;
	xp_stx_method_t* mthobj;
	xp_stx_byte_object_t* bytecodes;
	xp_word_t bytecode_size;
	xp_word_t* literals;
	xp_word_t pc = 0;
	int code, next, next2;

	ctxobj = (xp_stx_context_t*)XP_STX_OBJECT(stx,context);
	mthobj = (xp_stx_method_t*)XP_STX_OBJECT(stx, ctxobj->method);

	literals = mthobj->literals;
	bytecodes = XP_STX_BYTE_OBJECT(stx, mthobj->bytecodes);
	bytecode_size = XP_STX_SIZE(stx, mthobj->bytecodes);

	while (pc < bytecode_size) {
		code = bytecodes->data[pc++];

		if (code >= 0x00 && code <= 0x3F) {
			/* stack - push */
			int what = code >> 4;
			int index = code & 0x0F;

			switch (what) {
			case 0: /* receiver variable */
				__push_receiver_variable (stx, index, ctxobj);
				break;
			case 1: /* temporary variable */
				__push_temporary_variable (stx, index, ctxobj);
				break;
			case 2: /* literal constant */
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
				break; 
			case 5: /* temporary location */
				break;
			}
		}
	}

	return 0;	
}

static int __push_receiver_variable (
	xp_stx_t* stx, int index, xp_stx_context_t* ctxobj)
{
	xp_word_t* stack;
	xp_word_t stack_top;

	xp_assert (XP_STX_IS_WORD_OBJECT(stx, ctxobj->receiver));

	stack_top = XP_STX_FROM_SMALLINT(ctxobj->stack_top);
	stack = XP_STX_DATA(stx, ctxobj->stack);
	stack[stack_top++] = XP_STX_WORD_AT(stx, ctxobj->receiver, index);
	ctxobj->stack_top = XP_STX_TO_SMALLINT(stack_top);

	return 0;
}

static int __push_temporary_variable (
	xp_stx_t* stx, int index, xp_stx_context_t* ctxobj)
{
	xp_word_t* stack;
	xp_word_t stack_top;

	xp_assert (XP_STX_IS_WORD_OBJECT(stx, ctxobj->receiver));

	stack_top = XP_STX_FROM_SMALLINT(ctxobj->stack_top);
	stack = XP_STX_DATA(stx, ctxobj->stack);
	stack[stack_top++] = XP_STX_WORD_AT(stx, ctxobj->receiver, index);
	ctxobj->stack_top = XP_STX_TO_SMALLINT(stack_top);

	return 0;
}
