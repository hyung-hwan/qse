/*
 * $Id: bytecode.c 118 2008-03-03 11:21:33Z baconevi $
 */
#include <qse/stx/bytecode.h>
#include <qse/stx/class.h>
#include <qse/stx/method.h>
#include <qse/stx/dict.h>

static void __decode1 (qse_stx_t* stx, qse_word_t idx, void* data);
static int __decode2 (qse_stx_t* stx, 
	qse_stx_class_t* class_obj, qse_stx_method_t* method_obj);

int qse_stx_decode (qse_stx_t* stx, qse_word_t class)
{
	qse_stx_class_t* class_obj;

	class_obj = (qse_stx_class_t*)QSE_STX_OBJPTR(stx, class);
	if (class_obj->methods == stx->nil) return 0;

/* TODO */
	qse_stx_dict_traverse (stx, class_obj->methods, __decode1, class_obj);
	return 0;
}

#include <qse/bas/stdio.h>
static void __dump_object (qse_stx_t* stx, qse_word_t obj)
{
	if (QSE_STX_ISSMALLINT(obj)) {
		qse_printf (QSE_T("%d"), QSE_STX_FROMSMALLINT(obj));
	}	
	else if (QSE_STX_CLASS(stx,obj) == stx->class_character) {
		qse_printf (QSE_T("$%c"), QSE_STX_WORD_AT(stx,obj,0));
	}
	else if (QSE_STX_CLASS(stx,obj) == stx->class_string) {
		qse_printf (QSE_T("'%s'"), QSE_STX_DATA(stx,obj));
	}
	else if (QSE_STX_CLASS(stx,obj) == stx->class_symbol) {
		qse_printf (QSE_T("#%s"), QSE_STX_DATA(stx,obj));
	}
	else if (QSE_STX_ISCHAROBJECT(stx, obj)) {
		qse_printf (QSE_T("unknow char object [%s]"), QSE_STX_DATA(stx,obj));
	}
	else if (QSE_STX_ISBYTEOBJECT(stx, obj)) {
		qse_printf (QSE_T("unknown byte object"), QSE_STX_DATA(stx,obj));
	}
	else if (QSE_STX_ISWORDOBJECT(stx, obj)) {
		qse_printf (QSE_T("unknown word object"), QSE_STX_DATA(stx,obj));
	}
	else {
		qse_printf (QSE_T("invalid object type"));
	}
}

static void __decode1 (qse_stx_t* stx, qse_word_t idx, void* data)
{
	qse_stx_method_t* method_obj;
	qse_stx_class_t* class_obj;
	qse_word_t key = QSE_STX_WORD_AT(stx,idx,QSE_STX_ASSOCIATION_KEY);
	qse_word_t value = QSE_STX_WORD_AT(stx,idx,QSE_STX_ASSOCIATION_VALUE);
	qse_word_t* literals;
	qse_word_t literal_count, i;

	qse_word_t method_class;
	qse_stx_class_t* method_class_obj;

	class_obj = (qse_stx_class_t*)data;

	qse_printf (QSE_T("* Method: %s\n"), QSE_STX_DATA(stx, key));
	method_obj = (qse_stx_method_t*)QSE_STX_OBJPTR(stx, value);

	literals = method_obj->literals;
	/*
	literal_count = QSE_STX_SIZE(stx, value) - 
		(QSE_STX_FROMSMALLINT(class_obj->spec) >> QSE_STX_SPEC_INDEXABLE_BITS);
	*/
	method_class = QSE_STX_CLASS(stx,value);
	method_class_obj = QSE_STX_OBJPTR(stx, method_class);
	literal_count = QSE_STX_SIZE(stx,value) - 
		(QSE_STX_FROMSMALLINT(method_class_obj->spec) >> QSE_STX_SPEC_INDEXABLE_BITS);

	qse_printf (QSE_T("* Literal Count: %d, Temporary Count: %d, Argument Count: %d\n"),
		literal_count, 
		QSE_STX_FROMSMALLINT(method_obj->tmpcount), 
		QSE_STX_FROMSMALLINT(method_obj->argcount));
	for (i = 0; i < literal_count; i++) {
		qse_printf (QSE_T("%d. ["), i);
		__dump_object (stx, literals[i]);
		qse_printf (QSE_T("]\n"));
	}
	__decode2 (stx, data, method_obj);
}

static int __decode2 (qse_stx_t* stx, 
	qse_stx_class_t* class_obj, qse_stx_method_t* method_obj)
{
	qse_stx_byte_object_t* bytecodes;
	qse_word_t bytecode_size, pc = 0;
	int code, next, next2;

	static const qse_char_t* stack_opcode_names[] = 
	{
		QSE_T("push_receiver_variable"),	
		QSE_T("push_temporary_location"),	
		QSE_T("push_literal_constant"),	
		QSE_T("push_literal_variable"),	
		QSE_T("store_receiver_variable"),	
		QSE_T("store_temporary_location")
	};

	static const qse_char_t* send_opcode_names[] = 
	{
		QSE_T("send_to_self"),
		QSE_T("send_to_super")
	};

	static const qse_char_t* stack_special_opcode_names[] =
	{
		QSE_T("pop_stack_top"),
		QSE_T("duplicate_pop_stack_top"),
		QSE_T("push_active_context"),
		QSE_T("push_nil"),
		QSE_T("push_true"),
		QSE_T("push_false"),
		QSE_T("push_receiver")
	};

	static const qse_char_t* return_opcode_names[] = 
	{
		QSE_T("return_receiver"),
		QSE_T("return_true"),
		QSE_T("return_false"),
		QSE_T("return_nil"),
		QSE_T("return_from_message"),
		QSE_T("return_from_block")
	};

	bytecodes = QSE_STX_BYTE_OBJECT(stx, method_obj->bytecodes);
	bytecode_size = QSE_STX_SIZE(stx, method_obj->bytecodes);

	while (pc < bytecode_size) {
		code = bytecodes->data[pc++];

		if (code >= 0x00 && code <= 0x5F) {
			/* stack */
			qse_printf (QSE_T("%s %d\n"), 
				stack_opcode_names[code >> 4], code & 0x0F);
		}
		else if (code >= 0x60 && code <= 0x65) {
			/* stack extended */
			next = bytecodes->data[pc++];
			qse_printf (QSE_T("%s %d\n"), 
				stack_opcode_names[code & 0x0F], next);
		}
		else if (code >= 0x67 && code <= 0x6D) {
			/* stack special */
			qse_printf (QSE_T("%s\n"),
				stack_special_opcode_names[code - 0x67]);
		}

		else if (code >= 0x70 && code <=  0x71 ) {
			/* send message */
			next = bytecodes->data[pc++];
			qse_printf (QSE_T("%s nargs(%d) selector(%d)\n"),
				send_opcode_names[code - 0x70], next >> 5, next & 0x1F);
		}
		else if (code >= 0x72 && code <=  0x73 ) {
			/* send message extended */
			next = bytecodes->data[pc++];
			next2 = bytecodes->data[pc++];
			qse_printf (QSE_T("%s %d %d\n"),
				send_opcode_names[code - 0x72],  next, next2);
				
		}
		else if (code >= 0x78 && code <= 0x7D) {
			qse_printf (QSE_T("%s\n"),
				return_opcode_names[code - 0x78]);
		}
		else if (code >= 0x80 && code <= 0x8F) {
			// jump
		}
		else if (code >= 0xF0 && code <= 0xFF) {
			// primitive
			next = bytecodes->data[pc++];
			qse_printf (QSE_T("do_primitive %d\n"), ((code & 0x0F) << 8) | next);
	
		}
		else {
			qse_printf (QSE_T("unknown byte code 0x%x\n"), code);
		}
	}

        return 0;
}
