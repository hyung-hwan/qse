/*
 * $Id: bytecode.c,v 1.5 2005-07-10 09:21:46 bacon Exp $
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

static int __decode2 (xp_stx_t* stx, 
	xp_stx_class_t* class_obj, xp_stx_method_t* method_obj)
{
	xp_stx_byte_object_t* bytecodes;
	xp_word_t bytecode_size, pc = 0;
	int code, next;

	static const xp_char_t* stack_opcode_names[] = {
		XP_TEXT("push_receiver_variable"),	
		XP_TEXT("push_temporary_location"),	
		XP_TEXT("push_literal_constant"),	
		XP_TEXT("push_literal_variable"),	
		XP_TEXT("store_receiver_variable"),	
		XP_TEXT("store_temporary_location")
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
		else if (code >= 0x70 && code <=  0x73 ) {
			/* send */
		}
		else if (code >= 0x78 && code <= 0x7D) {
			// return
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
