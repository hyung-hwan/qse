/*
 * $Id: context.c,v 1.3 2005-05-18 04:01:51 bacon Exp $
 */

#include <xp/stx/context.h>
#include <xp/stx/object.h>

#define XP_STX_CONTEXT_SIZE        4
#define XP_STX_CONTEXT_IP          0
#define XP_STX_CONTEXT_METHOD      1
#define XP_STX_CONTEXT_ARGUMENTS   2
#define XP_STX_CONTEXT_TEMPORARIES 3

xp_stx_word_t xp_stx_new_context (xp_stx_t* stx, 
	xp_stx_word_t method, xp_stx_word_t args, xp_stx_word_t temp)
{
	xp_stx_word_t context;

	context = xp_stx_alloc_object(stx,XP_STX_CONTEXT_SIZE);
	XP_STX_CLASS(stx,context) = stx->class_context;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_IP) = XP_STX_TO_SMALLINT(0);
	XP_STX_AT(stx,context,XP_STX_CONTEXT_METHOD) = method;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_ARGUMENTS) = args;
	XP_STX_AT(stx,context,XP_STX_CONTEXT_TEMPORARIES) = temp;

	return context;
}

static xp_stx_byte_t __fetch_byte (xp_stx_t* stx, xp_stx_word_t context)
{
	xp_stx_word_t method, ip;

	ip = XP_STX_AT(stx,context,XP_STX_CONTEXT_IP);
	method = XP_STX_AT(stx,context,XP_STX_CONTEXT_METHOD);

	/* increment instruction pointer */
	XP_STX_AT(stx,context,XP_STX_CONTEXT_IP) =
		XP_STX_TO_SMALLINT((XP_STX_FROM_SMALLINT(ip) + 1));

	return XP_STX_BYTEAT(stx,method,XP_STX_FROM_SMALLINT(ip));
}

int xp_stx_run_context (xp_stx_t* stx, xp_stx_word_t context)
{
	xp_stx_byte_t byte, operand;

	while (!stx->__wantabort) {
		/* check_process_switch (); // hopefully */
		byte = __fetch_byte (stx, context);

xp_printf (XP_TEXT("code: %x\n"), byte);

		switch (byte) {
		case PUSH_OBJECT:
			operand = __fetch_byte (stx, context);	
			break;
		case SEND_UNARY_MESSAGE:
			operand = __fetch_byte (stx, context);
			break;
		case HALT:
			goto exit_run_context;		
		}
	}	

exit_run_context:
	return 0;
}
