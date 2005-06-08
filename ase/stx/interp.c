/*
 * $Id: interp.c,v 1.4 2005-06-08 16:00:51 bacon Exp $
 */

#include <xp/stx/interp.h>

#define XP_STX_PROCESS_SIZE 3
#define XP_STX_PROCESS_STACK     0
#define XP_STX_PROCESS_STACK_TOP 1
#define XP_STX_PROCESS_LINK      2

#define XP_STX_CONTEXT_SIZE   6
#define XP_STX_PROCESS_LINK        0
#define XP_STX_PROCESS_METHOD      1
#define XP_STX_PROCESS_ARGUMENTS   2
#define XP_STX_PROCESS_TEMPORARIES 3

typedef int (*byte_code_func_t) (xp_stx_t* 

static byte_code_func_t byte_code_funcs[] =
{
	XP_NULL,
	push_instance,
	push_argyment,
	push_temporary,
	push_literal,
	push_constant,
	store_instance,
	store_temporary,
	send_message,
	send_unary,
	send_binary,
	XP_NULL,
	do_primitive,
	XP_NULL,
	do_special
};

xp_word_t xp_stx_new_method (xp_stx_t* stx)
{
	xp_word_t method;
	method = xp_stx_alloc_object(XP_STX_METHOD_SIZE);

	return method;
}

xp_word_t xp_stx_new_context (xp_stx_t* stx, 
	xp_word_t method, xp_word_t args, xp_word_t temp)
{
	xp_word_t context;

	context = xp_stx_alloc_object(XP_STX_CONTEXT_SIZE);
	XP_STX_CLASS(stx,context) = stx->class_context;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_METHOD) = method;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_ARGUMENTS) = args;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_TEMPORARIES) = temp;

	return context;
}

xp_word_t xp_stx_new_process (xp_stx_t* stx, xp_word_t method)
{
	xp_word_t process, stx;

	process = xp_stx_alloc_object(XP_STX_PROCESS_SIZE);
	stack = xp_new_array(stx,50);
	
	XP_STX_CLASS(stx,process) = stx->class_process;
	XP_STX_AT(stx,process,XP_STX_PROCESS_STACK) = stack;
	XP_STX_AT(stx,process,XP_STX_PROCESS_STACKTOP) = XP_STX_FROM_SMALLINT(6);
	XP_STX_AT(stx,process,XP_STX_PROCESS_LINK) = XP_STX_FROM_SMALLINT(1);

	XP_STX_AT(stx,stack,0) = stx->nil; /* argument */
	XP_STX_AT(stx,stack,1) = XP_STX_FROM_SMALLINT(0); /* previous link */
	XP_STX_AT(stx,stack,2) = stx->nil; /* context */
	XP_STX_AT(stx,stack,3) = XP_STX_FROM_SMALLINT(1); /* return point */
	XP_STX_AT(stx,stack,4) = method;
	XP_STX_AT(stx,stack,5) = XP_STX_FROM_SMALLINT(1); /* byte offset */

	return process;	
}

int xp_stx_execute (xp_stx_t* stx, xp_word_t process)
{
	int low, high;
	byte_code_func_t bcfunc;

	stack = XP_STX_AT(stx,process,XP_PROCESS_STACK);
	stack_top = XP_STX_AT(stx,process,XP_PROCESS_STACK_TOP);
	link = XP_STX_AT(stx,process,XP_PROCESS_LINK);

	for (;;) {
		low = (high = nextByte(&es)) & 0x0F;
		high >>= 4;
		if(high == 0) {
			high = low;
			low = nextByte(&es);
		}

		bcfunc = byte_code_funcs[high];
		if (bcfunc != XP_NULL) {
			bcfunc (stx, low);
		}
	}	

	return 0;	
}
