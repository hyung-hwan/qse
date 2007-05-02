/*
 * $Id: bytecode.c,v 1.3 2007/04/30 08:32:40 bacon Exp $
 */
#include <ase/stx/bytecode.h>
#include <ase/stx/class.h>
#include <ase/stx/method.h>
#include <ase/stx/dict.h>

static void __decode1 (ase_stx_t* stx, ase_word_t idx, void* data);
static int __decode2 (ase_stx_t* stx, 
	ase_stx_class_t* class_obj, ase_stx_method_t* method_obj);

int ase_stx_decode (ase_stx_t* stx, ase_word_t class)
{
	ase_stx_class_t* class_obj;

	class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class);
	if (class_obj->methods == stx->nil) return 0;

/* TODO */
	ase_stx_dict_traverse (stx, class_obj->methods, __decode1, class_obj);
	return 0;
}

#include <ase/bas/stdio.h>
static void __dump_object (ase_stx_t* stx, ase_word_t obj)
{
	if (ASE_STX_IS_SMALLINT(obj)) {
		ase_printf (ASE_T("%d"), ASE_STX_FROM_SMALLINT(obj));
	}	
	else if (ASE_STX_CLASS(stx,obj) == stx->class_character) {
		ase_printf (ASE_T("$%c"), ASE_STX_WORD_AT(stx,obj,0));
	}
	else if (ASE_STX_CLASS(stx,obj) == stx->class_string) {
		ase_printf (ASE_T("'%s'"), ASE_STX_DATA(stx,obj));
	}
	else if (ASE_STX_CLASS(stx,obj) == stx->class_symbol) {
		ase_printf (ASE_T("#%s"), ASE_STX_DATA(stx,obj));
	}
	else if (ASE_STX_IS_CHAR_OBJECT(stx, obj)) {
		ase_printf (ASE_T("unknow char object [%s]"), ASE_STX_DATA(stx,obj));
	}
	else if (ASE_STX_IS_BYTE_OBJECT(stx, obj)) {
		ase_printf (ASE_T("unknown byte object"), ASE_STX_DATA(stx,obj));
	}
	else if (ASE_STX_IS_WORD_OBJECT(stx, obj)) {
		ase_printf (ASE_T("unknown word object"), ASE_STX_DATA(stx,obj));
	}
	else {
		ase_printf (ASE_T("invalid object type"));
	}
}

static void __decode1 (ase_stx_t* stx, ase_word_t idx, void* data)
{
	ase_stx_method_t* method_obj;
	ase_stx_class_t* class_obj;
	ase_word_t key = ASE_STX_WORD_AT(stx,idx,ASE_STX_ASSOCIATION_KEY);
	ase_word_t value = ASE_STX_WORD_AT(stx,idx,ASE_STX_ASSOCIATION_VALUE);
	ase_word_t* literals;
	ase_word_t literal_count, i;

	ase_word_t method_class;
	ase_stx_class_t* method_class_obj;

	class_obj = (ase_stx_class_t*)data;

	ase_printf (ASE_T("* Method: %s\n"), ASE_STX_DATA(stx, key));
	method_obj = (ase_stx_method_t*)ASE_STX_OBJECT(stx, value);

	literals = method_obj->literals;
	/*
	literal_count = ASE_STX_SIZE(stx, value) - 
		(ASE_STX_FROM_SMALLINT(class_obj->spec) >> ASE_STX_SPEC_INDEXABLE_BITS);
	*/
	method_class = ASE_STX_CLASS(stx,value);
	method_class_obj = ASE_STX_OBJECT(stx, method_class);
	literal_count = ASE_STX_SIZE(stx,value) - 
		(ASE_STX_FROM_SMALLINT(method_class_obj->spec) >> ASE_STX_SPEC_INDEXABLE_BITS);

	ase_printf (ASE_T("* Literal Count: %d, Temporary Count: %d, Argument Count: %d\n"),
		literal_count, 
		ASE_STX_FROM_SMALLINT(method_obj->tmpcount), 
		ASE_STX_FROM_SMALLINT(method_obj->argcount));
	for (i = 0; i < literal_count; i++) {
		ase_printf (ASE_T("%d. ["), i);
		__dump_object (stx, literals[i]);
		ase_printf (ASE_T("]\n"));
	}
	__decode2 (stx, data, method_obj);
}

static int __decode2 (ase_stx_t* stx, 
	ase_stx_class_t* class_obj, ase_stx_method_t* method_obj)
{
	ase_stx_byte_object_t* bytecodes;
	ase_word_t bytecode_size, pc = 0;
	int code, next, next2;

	static const ase_char_t* stack_opcode_names[] = 
	{
		ASE_T("push_receiver_variable"),	
		ASE_T("push_temporary_location"),	
		ASE_T("push_literal_constant"),	
		ASE_T("push_literal_variable"),	
		ASE_T("store_receiver_variable"),	
		ASE_T("store_temporary_location")
	};

	static const ase_char_t* send_opcode_names[] = 
	{
		ASE_T("send_to_self"),
		ASE_T("send_to_super")
	};

	static const ase_char_t* stack_special_opcode_names[] =
	{
		ASE_T("pop_stack_top"),
		ASE_T("duplicate_pop_stack_top"),
		ASE_T("push_active_context"),
		ASE_T("push_nil"),
		ASE_T("push_true"),
		ASE_T("push_false"),
		ASE_T("push_receiver")
	};

	static const ase_char_t* return_opcode_names[] = 
	{
		ASE_T("return_receiver"),
		ASE_T("return_true"),
		ASE_T("return_false"),
		ASE_T("return_nil"),
		ASE_T("return_from_message"),
		ASE_T("return_from_block")
	};

	bytecodes = ASE_STX_BYTE_OBJECT(stx, method_obj->bytecodes);
	bytecode_size = ASE_STX_SIZE(stx, method_obj->bytecodes);

	while (pc < bytecode_size) {
		code = bytecodes->data[pc++];

		if (code >= 0x00 && code <= 0x5F) {
			/* stack */
			ase_printf (ASE_T("%s %d\n"), 
				stack_opcode_names[code >> 4], code & 0x0F);
		}
		else if (code >= 0x60 && code <= 0x65) {
			/* stack extended */
			next = bytecodes->data[pc++];
			ase_printf (ASE_T("%s %d\n"), 
				stack_opcode_names[code & 0x0F], next);
		}
		else if (code >= 0x67 && code <= 0x6D) {
			/* stack special */
			ase_printf (ASE_T("%s\n"),
				stack_special_opcode_names[code - 0x67]);
		}

		else if (code >= 0x70 && code <=  0x71 ) {
			/* send message */
			next = bytecodes->data[pc++];
			ase_printf (ASE_T("%s nargs(%d) selector(%d)\n"),
				send_opcode_names[code - 0x70], next >> 5, next & 0x1F);
		}
		else if (code >= 0x72 && code <=  0x73 ) {
			/* send message extended */
			next = bytecodes->data[pc++];
			next2 = bytecodes->data[pc++];
			ase_printf (ASE_T("%s %d %d\n"),
				send_opcode_names[code - 0x72],  next, next2);
				
		}
		else if (code >= 0x78 && code <= 0x7D) {
			ase_printf (ASE_T("%s\n"),
				return_opcode_names[code - 0x78]);
		}
		else if (code >= 0x80 && code <= 0x8F) {
			// jump
		}
		else if (code >= 0xF0 && code <= 0xFF) {
			// primitive
			next = bytecodes->data[pc++];
			ase_printf (ASE_T("do_primitive %d\n"), ((code & 0x0F) << 8) | next);
	
		}
		else {
			ase_printf (ASE_T("unknown byte code 0x%x\n"), code);
		}
	}

        return 0;
}
