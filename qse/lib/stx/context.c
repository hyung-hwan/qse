/*
 * $Id: context.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/context.h>
#include <qse/stx/object.h>
#include <qse/stx/class.h>
#include <qse/stx/misc.h>

qse_word_t qse_stx_new_context (qse_stx_t* stx, 
	qse_word_t method, qse_word_t args, qse_word_t temp)
{
	qse_word_t context;
	qse_stx_context_t* obj;

	context = qse_stx_alloc_word_object(
		stx, QSE_NULL, QSE_STX_CONTEXT_SIZE, QSE_NULL, 0);
	obj = (qse_stx_context_t*)QSE_STX_OBJPTR(stx,context);
	obj->header.class = qse_stx_lookup_class(stx,QSE_T("Context"));
	obj->ip = QSE_STX_TO_SMALLINT(0);
	obj->method = method;
	obj->arguments = args;
	obj->temporaries = temp;

	return context;
}

static qse_byte_t __fetch_byte (
	qse_stx_t* stx, qse_stx_context_t* context_obj)
{
	qse_word_t ip, method;

	QSE_ASSERT (QSE_STX_ISSMALLINT(context_obj->ip));
	ip = QSE_STX_FROMSMALLINT(context_obj->ip);
	method = context_obj->method;

	/* increment instruction pointer */
	context_obj->ip = QSE_STX_TO_SMALLINT(ip + 1);

	qse_assert (QSE_STX_TYPE(stx,method) == QSE_STX_BYTE_INDEXED);
	return QSE_STX_BYTE_AT(stx,method,ip);
}

int qse_stx_run_context (qse_stx_t* stx, qse_word_t context)
{
	qse_byte_t byte, operand;
	qse_stx_context_t* context_obj;

	context_obj = (qse_stx_context_t*)QSE_STX_OBJPTR(stx,context);

	while (!stx->__wantabort) 
	{
		/* check_process_switch (); // hopefully */
		byte = __fetch_byte (stx, context_obj);

#ifdef _DOS
printf (QSE_T("code: %x\n"), byte);
#else
qse_printf (QSE_T("code: %x\n"), byte);
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
