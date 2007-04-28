/*
 * $Id: context.c,v 1.1 2007/03/28 14:05:28 bacon Exp $
 */

#include <ase/stx/context.h>
#include <ase/stx/object.h>
#include <ase/stx/class.h>
#include <ase/stx/misc.h>

ase_word_t ase_stx_new_context (ase_stx_t* stx, 
	ase_word_t method, ase_word_t args, ase_word_t temp)
{
	ase_word_t context;
	ase_stx_context_t* obj;

	context = ase_stx_alloc_word_object(
		stx, ASE_NULL, ASE_STX_CONTEXT_SIZE, ASE_NULL, 0);
	obj = (ase_stx_context_t*)ASE_STX_OBJECT(stx,context);
	obj->header.class = ase_stx_lookup_class(stx,ASE_T("Context"));
	obj->ip = ASE_STX_TO_SMALLINT(0);
	obj->method = method;
	obj->arguments = args;
	obj->temporaries = temp;

	return context;
}

static ase_byte_t __fetch_byte (
	ase_stx_t* stx, ase_stx_context_t* context_obj)
{
	ase_word_t ip, method;

	ase_assert (ASE_STX_IS_SMALLINT(context_obj->ip));
	ip = ASE_STX_FROM_SMALLINT(context_obj->ip);
	method = context_obj->method;

	/* increment instruction pointer */
	context_obj->ip = ASE_STX_TO_SMALLINT(ip + 1);

	ase_assert (ASE_STX_TYPE(stx,method) == ASE_STX_BYTE_INDEXED);
	return ASE_STX_BYTE_AT(stx,method,ip);
}

int ase_stx_run_context (ase_stx_t* stx, ase_word_t context)
{
	ase_byte_t byte, operand;
	ase_stx_context_t* context_obj;

	context_obj = (ase_stx_context_t*)ASE_STX_OBJECT(stx,context);

	while (!stx->__wantabort) {
		/* check_process_switch (); // hopefully */
		byte = __fetch_byte (stx, context_obj);

#ifdef _DOS
printf (ASE_T("code: %x\n"), byte);
#else
ase_printf (ASE_T("code: %x\n"), byte);
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
