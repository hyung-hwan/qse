/*
 * $Id: bytecode.c,v 1.9 2005-08-11 16:16:04 bacon Exp $
 */
#include <xp/stx/bytecode.h>
#include <xp/stx/class.h>
#include <xp/stx/method.h>
#include <xp/stx/dict.h>

static void __decode1 (xp_stx_t* stx, xp_word_t idx, void* data);
static int __decode2 (xp_stx_t* stx, 
	xp_stx_class_t* class_obj, xp_stx_method_t* method_obj);

int xp_stx_decode (xp_stx_t* stx, xp_word_t class)
{
	xp_stx_class_t* class_obj;

	class_obj = (xp_stx_class_t*)XP_STX_OBJECT(stx, class);
	if (class_obj->methods == stx->nil) return 0;


/* TODO */
	xp_stx_dict_traverse (stx, class_obj->methods, __decode1, class_obj);
	return 0;
}

static void __dump_object (xp_stx_t* stx, xp_word_t obj)
{
	if (XP_STX_IS_SMALLINT(obj)) {
		xp_printf (XP_TEXT("%d"), XP_STX_FROM_SMALLINT(obj));
	}	
	else if (XP_STX_CLASS(stx,obj) == stx->class_character) {
		xp_printf (XP_TEXT("$%c"), XP_STX_WORD_AT(stx,obj,0));
	}
	else if (XP_STX_CLASS(stx,obj) == stx->class_string) {
		xp_printf (XP_TEXT("'%s'"), XP_STX_DATA(stx,obj));
	}
	else if (XP_STX_CLASS(stx,obj) == stx->class_symbol) {
		xp_printf (XP_TEXT("#%s"), XP_STX_DATA(stx,obj));
	}
	else if (XP_STX_IS_CHAR_OBJECT(stx, obj)) {
		xp_printf (XP_TEXT("unknow char object [%s]"), XP_STX_DATA(stx,obj));
	}
	else if (XP_STX_IS_BYTE_OBJECT(stx, obj)) {
		xp_printf (XP_TEXT("unknown byte object"), XP_STX_DATA(stx,obj));
	}
	else if (XP_STX_IS_WORD_OBJECT(stx, obj)) {
		xp_printf (XP_TEXT("unknown word object"), XP_STX_DATA(stx,obj));
	}
	else {
		xp_printf (XP_TEXT("invalid object type"));
	}
}

static void __decode1 (xp_stx_t* stx, xp_word_t idx, void* data)
{
	xp_stx_method_t* method_obj;
	xp_stx_class_t* class_obj;
	xp_word_t key = XP_STX_WORD_AT(stx,idx,XP_STX_ASSOCIATION_KEY);
	xp_word_t value = XP_STX_WORD_AT(stx,idx,XP_STX_ASSOCIATION_VALUE);
	xp_word_t* literals;
	xp_word_t literal_count, i;

	class_obj = (xp_stx_class_t*)data;

	xp_printf (XP_TEXT("Method: %s\n"), XP_STX_DATA(stx, key));
	method_obj = (xp_stx_method_t*)XP_STX_OBJECT(stx, value);

	literals = method_obj->literals;
	literal_count = XP_STX_SIZE(stx, value) - 
		(XP_STX_FROM_SMALLINT(class_obj->spec) >> XP_STX_SPEC_INDEXABLE_BITS);

	xp_printf (XP_TEXT("literal count %d\n"), literal_count);
	for (i = 0; i < literal_count; i++) {
		xp_printf (XP_TEXT("%d. ["), i);
		__dump_object (stx, literals[i]);
		xp_printf (XP_TEXT("]\n"));
	}
	__decode2 (stx, data, method_obj);
}

static int __decode2 (xp_stx_t* stx, 
	xp_stx_class_t* class_obj, xp_stx_method_t* method_obj)
{
	xp_stx_byte_object_t* bytecodes;
	xp_word_t bytecode_size, pc = 0;
	int code, next, next2;

	static const xp_char_t* stack_opcode_names[] = 
	{
		XP_TEXT("push_receiver_variable"),	
		XP_TEXT("push_temporary_location"),	
		XP_TEXT("push_literal_constant"),	
		XP_TEXT("push_literal_variable"),	
		XP_TEXT("store_receiver_variable"),	
		XP_TEXT("store_temporary_location")
	};

	static const xp_char_t* send_opcode_names[] = 
	{
		XP_TEXT("send_to_self"),
		XP_TEXT("send_to_super")
	};

	static const xp_char_t* stack_special_opcode_names[] =
	{
		XP_TEXT("store_pop_stack_top"),
		XP_TEXT("duplicate_pop_stack_top"),
		XP_TEXT("push_active_context"),
		XP_TEXT("push_nil"),
		XP_TEXT("push_true"),
		XP_TEXT("push_false")
	};

	static const xp_char_t* return_opcode_names[] = 
	{
		XP_TEXT("return_receiver"),
		XP_TEXT("return_true"),
		XP_TEXT("return_false"),
		XP_TEXT("return_nil"),
		XP_TEXT("return_from_message"),
		XP_TEXT("return_from_block")
	};

	bytecodes = XP_STX_BYTE_OBJECT(stx, method_obj->bytecodes);
	bytecode_size = XP_STX_SIZE(stx, method_obj->bytecodes);

	while (pc < bytecode_size) {
		code = bytecodes->data[pc++];

		if (code >= 0x00 && code <= 0x5F) {
			/* stack */
			xp_printf (XP_TEXT("%s %d\n"), 
				stack_opcode_names[code >> 4], code & 0x0F);
		}
		else if (code >= 0x60 && code <= 0x65) {
			/* stack extended */
			next = bytecodes->data[pc++];
			xp_printf (XP_TEXT("%s %d\n"), 
				stack_opcode_names[code & 0x0F], next);
		}
		else if (code >= 0x67 && code <= 0x6C) {
			/* stack special */
			xp_printf (XP_TEXT("%s\n"),
				stack_special_opcode_names[code - 0x67]);
		}

		else if (code >= 0x70 && code <=  0x71 ) {
			/* send */
			next = bytecodes->data[pc++];
			xp_printf (XP_TEXT("%s %d %d\n"),
				send_opcode_names[code - 0x70], next >> 5, next >> 0x1F);
		}
		else if (code >= 0x72 && code <=  0x73 ) {
			/* send extended */
			next = bytecodes->data[pc++];
			next2 = bytecodes->data[pc++];
			xp_printf (XP_TEXT("%s %d %d\n"),
				send_opcode_names[code - 0x72],  next, next2);
				
		}
		else if (code >= 0x78 && code <= 0x7D) {
			xp_printf (XP_TEXT("%s\n"),
				return_opcode_names[code - 0x78]);
		}
		else if (code >= 0x80 && code <= 0x8F) {
			// jump
		}
		else if (code >= 0xF0 && code <= 0xFF) {
			// primitive
			next = bytecodes->data[pc++];
			xp_printf (XP_TEXT("do_primitive %d\n"), ((code & 0x0F) << 8) | next);
	
		}
		else {
			xp_printf (XP_TEXT("unknown byte code 0x%x\n"), code);
		}
	}

        return 0;
}
