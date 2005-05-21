/*
 * $Id: context.c,v 1.5 2005-05-21 16:11:06 bacon Exp $
 */

#include <xp/stx/context.h>
#include <xp/stx/object.h>
#include <xp/stx/misc.h>

xp_stx_word_t xp_stx_new_context (xp_stx_t* stx, 
	xp_stx_word_t method, xp_stx_word_t args, xp_stx_word_t temp)
{
	xp_stx_word_t context;
	xp_stx_context_t* obj;

	context = xp_stx_alloc_object(stx,XP_STX_CONTEXT_SIZE);
	/*
	XP_STX_CLASS(stx,context) = stx->class_context;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_IP) = XP_STX_TO_SMALLINT(0);
	XP_STX_AT(stx,context,XP_STX_CONTEXT_METHOD) = method;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_ARGUMENTS) = args;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_TEMPORARIES) = temp;
	*/

	obj = (xp_stx_context_t*)XP_STX_OBJECT(stx,context);
	obj->header.class = stx->class_context;
	obj->ip = XP_STX_TO_SMALLINT(0);
	obj->method = method;
	obj->arguments = args;
	obj->temporaries = temp;

	return context;
}

static xp_stx_byte_t __fetch_byte (
	xp_stx_t* stx, xp_stx_context_t* context_obj)
{
	xp_stx_word_t ip, method;

	xp_stx_assert (XP_STX_IS_SMALLINT(context_obj->ip));
	ip = XP_STX_FROM_SMALLINT(context_obj->ip);
	method = context_obj->method;

	/* increment instruction pointer */
	context_obj->ip = XP_STX_TO_SMALLINT(ip + 1);

	xp_stx_assert (XP_STX_TYPE(stx,method) == XP_STX_BYTE_INDEXED);
	return XP_STX_BYTEAT(stx,method,ip);
}

int xp_stx_run_context (xp_stx_t* stx, xp_stx_word_t context)
{
	xp_stx_byte_t byte, operand;
	xp_stx_context_t* context_obj;

	context_obj = (xp_stx_context_t*)XP_STX_OBJECT(stx,context);

	while (!stx->__wantabort) {
		/* check_process_switch (); // hopefully */
		byte = __fetch_byte (stx, context_obj);

#ifdef _DOS
printf (XP_TEXT("code: %x\n"), byte);
#else
xp_printf (XP_TEXT("code: %x\n"), byte);
#endif

		switch (byte) {
		case PUSH_OBJECT:
			operand = __fetch_byte (stx, context_obj);	
			break;
		case SEND_UNARY_MESSAGE:
			operand = __fetch_byte (stx, context_obj);
			break;
		case HALT:
			goto exit_run_context;		
		}
	}	

exit_run_context:
	return 0;
}
