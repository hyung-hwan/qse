#include <xp/stx/stx.h>

#ifdef _DOS
	#include <stdio.h>
	#define xp_printf printf
#else
	#include <xp/bas/stdio.h>
	#include <xp/bas/locale.h>
#endif

#include <xp/stx/bootstrap.h>
#include <xp/stx/object.h>
#include <xp/stx/symbol.h>
#include <xp/stx/context.h>
#include <xp/stx/class.h>
#include <xp/stx/hash.h>

void print_symbol_names (xp_stx_t* stx, xp_stx_word_t sym)
{
	xp_printf (XP_TEXT("%lu [%s]\n"), (unsigned long)sym, &XP_STX_CHARAT(stx,sym,0));
}

void print_symbol_names_2 (xp_stx_t* stx, xp_stx_word_t idx)
{
	xp_stx_word_t key = XP_STX_AT(stx,idx,1);
	xp_printf (XP_TEXT("%lu [%s]\n"), (unsigned long)key, &XP_STX_CHARAT(stx,key,0));
}

int xp_main (int argc, xp_char_t* argv[])
{
	xp_stx_t stx;
	xp_stx_word_t i;

#ifndef _DOS
	if (xp_setlocale () == -1) {
		printf ("cannot set locale\n");
		return -1;
	}
#endif

	if (argc != 2) { /* TODO: argument processing */
		xp_printf (XP_TEXT("Usage: %s [-f imageFile] MainClass\n"), argv[0]);
		return -1;
	}

	if (xp_stx_open (&stx, 10000) == XP_NULL) {
		xp_printf (XP_TEXT("cannot open stx\n"));
		return -1;
	}

	if (xp_stx_bootstrap(&stx) == -1) {
		xp_stx_close (&stx);
		xp_printf (XP_TEXT("cannot bootstrap\n"));
		return -1;
	}	

	xp_printf (XP_TEXT("stx.nil %lu\n"), (unsigned long)stx.nil);
	xp_printf (XP_TEXT("stx.true %lu\n"), (unsigned long)stx.true);
	xp_printf (XP_TEXT("stx.false %lu\n"), (unsigned long)stx.false);
	xp_printf (XP_TEXT("-------------\n"));
	
	xp_stx_traverse_symbol_table (&stx, print_symbol_names);
	xp_printf (XP_TEXT("-------------\n"));

	xp_stx_hash_traverse (&stx, stx.smalltalk, print_symbol_names_2);
	xp_printf (XP_TEXT("-------------\n"));

	{
		xp_stx_word_t n;
		xp_stx_class_t* obj;
		n = xp_stx_lookup_class (&stx, XP_STX_TEXT("UndefinedObject"));
		obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(&stx,n);
		xp_printf (XP_TEXT("name of class UndefinedObject: %lu, %s\n"), 
			(unsigned long)obj->name,
			XP_STX_DATA(&stx, obj->name));
	}
	xp_printf (XP_TEXT("-------------\n"));

	{
		xp_stx_word_t n;
		xp_stx_class_t* obj;

		n = xp_stx_lookup_class (&stx, XP_STX_TEXT("SymbolTable"));
		xp_printf (XP_TEXT("Class hierarchy for the class SymbolTable\n"));

		while (n != stx.nil) {
			obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(&stx,n);
			xp_printf (XP_TEXT("%lu, %s\n"), 
				(unsigned long)obj->name,
				XP_STX_DATA(&stx, obj->name));
			n = obj->superclass;
		}
	}
	xp_printf (XP_TEXT("-------------\n"));

	{
		xp_stx_word_t class_name, method_name;
		xp_stx_word_t main_class;
		xp_stx_word_t method, context;

		class_name = xp_stx_new_symbol (&stx,argv[1]);
		method_name = xp_stx_new_symbol (&stx,XP_STX_TEXT("main"));

		if (xp_stx_lookup_global (&stx,class_name,&main_class) == -1) {
			xp_printf (XP_TEXT("non-existent class: %s\n"), argv[1]);
			return -1;
		}

		method = xp_stx_alloc_byte_object (&stx,100);
		XP_STX_CLASS(&stx,method) = stx.class_method;

		XP_STX_BYTEAT(&stx,method,0) = PUSH_OBJECT;
		XP_STX_BYTEAT(&stx,method,1) = main_class;
		XP_STX_BYTEAT(&stx,method,2) = SEND_UNARY_MESSAGE;
		XP_STX_BYTEAT(&stx,method,3) = method_name;
		XP_STX_BYTEAT(&stx,method,4) = HALT;

		context = xp_stx_new_context (&stx, method, stx.nil, stx.nil);
		xp_stx_run_context (&stx, context);
	}

	xp_stx_close (&stx);
	xp_printf (XP_TEXT("== End of program ==\n"));
	return 0;
}

