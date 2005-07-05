/*
 * $Id: context.c,v 1.10 2005-07-05 09:02:13 bacon Exp $
 */

#include <xp/stx/context.h>
#include <xp/stx/object.h>
#include <xp/stx/class.h>
#include <xp/stx/misc.h>

xp_word_t xp_stx_new_context (xp_stx_t* stx, 
	xp_word_t method, xp_word_t args, xp_word_t temp)
{
	xp_word_t context;
	xp_stx_context_t* obj;

	context = xp_stx_alloc_word_object(
		stx, XP_NULL, XP_STX_CONTEXT_SIZE, XP_NULL, 0);
	obj = (xp_stx_context_t*)XP_STX_OBJECT(stx,context);
	obj->header.class = xp_stx_lookup_class(stx,XP_TEXT("Context"));
	obj->ip = XP_STX_TO_SMALLINT(0);
	obj->method = method;
	obj->arguments = args;
	obj->temporaries = temp;

	return context;
}

static xp_byte_t __fetch_byte (
	xp_stx_t* stx, xp_stx_context_t* context_obj)
{
	xp_word_t ip, method;

	xp_assert (XP_STX_IS_SMALLINT(context_obj->ip));
	ip = XP_STX_FROM_SMALLINT(context_obj->ip);
	method = context_obj->method;

	/* increment instruction pointer */
	context_obj->ip = XP_STX_TO_SMALLINT(ip + 1);

	xp_assert (XP_STX_TYPE(stx,method) == XP_STX_BYTE_INDEXED);
	return XP_STX_BYTEAT(stx,method,ip);
}

int xp_stx_run_context (xp_stx_t* stx, xp_word_t context)
{
	xp_byte_t byte, operand;
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
