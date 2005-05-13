/*
 * $Id: interp.c,v 1.1 2005-05-13 16:45:55 bacon Exp $
 */

#include <xp/stx/interp.h>

#define XP_STX_PROCESS_DIMENSION 3
#define XP_STX_PROCESS_STACK     0
#define XP_STX_PROCESS_STACK_TOP 1
#define XP_STX_PROCESS_LINK      2

#define XP_STX_CONTEXT_DIMENSION   6
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

int xp_stx_new_context (xp_stx_t* stx, 
	xp_stx_word_t method, xp_stx_word_t args, xp_stx_word_t temp)
{
	xp_stx_word_t context;

	context = xp_stx_alloc_object(XP_STX_CONTEXT_DIMENSION);
	XP_STX_CLASS(stx,context) = stx->context_class;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_METHOD) = method;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_ARGUMENTS) = args;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_TEMPORARIES) = temp;

	return context;
}

int xp_stx_execute (xp_stx_t* stx, xp_stx_word_t process)
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
