/*
 * $Id: bytecode.c,v 1.4 2005-07-08 11:32:50 bacon Exp $
 */
#include <xp/stx/bytecode.h>
#include <xp/stx/class.h>
#include <xp/stx/method.h>
#include <xp/stx/hash.h>

static void __decode1 (xp_stx_t* stx, xp_word_t idx, void* data);
static int __decode2 (xp_stx_t* stx, 
	xp_stx_class_t* class_obj, xp_stx_method_t* method_obj);

int xp_stx_decode (xp_stx_t* stx, xp_word_t class)
{
	xp_stx_class_t* class_obj;

	class_obj = (xp_stx_class_t*)XP_STX_OBJECT(stx, class);
	if (class_obj->methods == stx->nil) return 0;

/* TODO */
	xp_stx_hash_traverse (stx, class_obj->methods, __decode1, class_obj);
	return 0;
}

static void __decode1 (xp_stx_t* stx, xp_word_t idx, void* data)
{
	xp_stx_method_t* method_obj;
	xp_word_t key = XP_STX_WORDAT(stx,idx,XP_STX_PAIRLINK_KEY);
	xp_word_t value = XP_STX_WORDAT(stx,idx,XP_STX_PAIRLINK_VALUE);

	xp_printf (XP_TEXT("Method: %s\n"), XP_STX_DATA(stx, key));
	method_obj = (xp_stx_method_t*)XP_STX_OBJECT(stx, value);
	__decode2 (stx, data, method_obj);
}

static const xp_char_t* opcode_names[] =
{
	XP_TEXT("PUSH_VARIABLE"),
	XP_TEXT("PUSH_TEMPORARY"),
	XP_TEXT("PUSH_LITERAL_CONSTANT"),
	XP_TEXT("PUSH_LITERAL_VARIABLE"),
	XP_TEXT("STORE_VARIABLE"),
	XP_TEXT("STORE_TEMPORARY"),
	XP_TEXT("SEND"),
	XP_TEXT("JUMP"),
	XP_TEXT("DO_SPECIAL"),
	XP_TEXT("DO_PRIMITIVE"),
	XP_TEXT("PUSH_VARIABLE_EXTENDED"),
	XP_TEXT("PUSH_TEMPORARY_EXTENDED"),
	XP_TEXT("STORE_VARIABLE_EXTENDED"),
	XP_TEXT("STORE_TEMPORARY_EXTENDED"),
	XP_TEXT("DO_SPECIAL_EXTENDED"),
	XP_TEXT("DO_PRIMITIVE_EXTENDED")
};

static int __decode2 (xp_stx_t* stx, 
	xp_stx_class_t* class_obj, xp_stx_method_t* method_obj)
{
	xp_stx_byte_object_t* bytecodes;
	xp_word_t bytecode_size, pc = 0;
	xp_byte_t code;
	int opcode, operand;

	bytecodes = XP_STX_BYTE_OBJECT(stx, method_obj->bytecodes);
	bytecode_size = XP_STX_SIZE(stx, method_obj->bytecodes);

	while (pc < bytecode_size) {
		code = bytecodes->data[pc++];

		opcode = (code & 0xF0) >> 4;
		operand = code & 0x0F;
		if (opcode > 0x9) {
			if (pc >= bytecode_size) {
				/* TODO: */	
				xp_printf (XP_TEXT("error in bytecodes\n"));
				return -1;
			}
			code = bytecodes->data[pc++];
			operand |= (code << 4);
		}

		xp_printf (XP_TEXT("%s(0x%x), operand = %d\n"), 
			opcode_names[opcode], opcode, operand);
	}


        return 0;
}

