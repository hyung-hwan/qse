#include <xp/stx/stx.h>

#ifdef _DOS
	#include <stdio.h>
	#define xp_printf printf
#else
	#include <xp/bas/stdio.h>
	#include <xp/bas/locale.h>
#endif

#include <xp/stx/bootstrp.h>
#include <xp/stx/object.h>
#include <xp/stx/symbol.h>
#include <xp/stx/context.h>
#include <xp/stx/class.h>
#include <xp/stx/hash.h>

void print_symbol_names (xp_stx_t* stx, xp_word_t sym, void* unused)
{
	xp_printf (XP_TEXT("%lu [%s]\n"), (unsigned long)sym, &XP_STX_CHARAT(stx,sym,0));
}

void print_symbol_names_2 (xp_stx_t* stx, xp_word_t idx, void* unused)
{
	xp_word_t key = XP_STX_WORDAT(stx,idx,XP_STX_PAIRLINK_KEY);
	xp_word_t value = XP_STX_WORDAT(stx,idx,XP_STX_PAIRLINK_VALUE);
	xp_printf (XP_TEXT("%lu [%s] %lu\n"), 
		(unsigned long)key, &XP_STX_CHARAT(stx,key,0), (unsigned long)value);
}

void print_superclasses (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t n;
	xp_stx_class_t* obj;

	n = xp_stx_lookup_class (stx, name);
	xp_printf (XP_TEXT("Class hierarchy for the class '%s'\n"), name);

	while (n != stx->nil) {
		obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx,n);
		xp_printf (XP_TEXT("%lu, %s\n"), 
			(unsigned long)obj->name,
			XP_STX_DATA(stx, obj->name));
		n = obj->superclass;
	}
}

void print_metaclass_superclasses (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t n, x;
	xp_stx_metaclass_t* obj;
	xp_stx_class_t* xobj;

	n = xp_stx_lookup_class (stx, name);
	n = XP_STX_CLASS(stx,n);
	xp_printf (XP_TEXT("Class hierarchy for the metaclass '%s class'\n"), name);

	while (n != stx->nil) {
		/*if (n == stx->class_class) break; */
		if (XP_STX_CLASS(stx,n) != stx->class_metaclass) break;

		obj = (xp_stx_metaclass_t*)XP_STX_WORD_OBJECT(stx,n);
		x = obj->instance_class;
		xobj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx,x);
		xp_printf (XP_TEXT("%lu, %s class\n"), 
			(unsigned long)xobj->name,
			XP_STX_DATA(stx, xobj->name));
		n = obj->superclass;
	}
	while (n != stx->nil) {
		xobj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx,n);
		xp_printf (XP_TEXT("%lu, %s\n"), 
			(unsigned long)xobj->name,
			XP_STX_DATA(stx, xobj->name));
		n = xobj->superclass;
	}
}

void print_class_name (xp_stx_t* stx, xp_word_t class, int tabs)
{
	xp_stx_class_t* xobj;
	xobj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx,class);

	while (tabs-- > 0) xp_printf (XP_TEXT("  "));

	xp_printf (XP_TEXT("%s [%lu]\n"), 
		XP_STX_DATA(stx, xobj->name),
		(unsigned long)class);
}

void print_metaclass_name (xp_stx_t* stx, xp_word_t class, int tabs)
{
	xp_stx_metaclass_t* obj;
	xp_stx_class_t* xobj;

	obj = (xp_stx_metaclass_t*)XP_STX_WORD_OBJECT(stx,class);
	xobj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx,obj->instance_class);

	while (tabs-- > 0) xp_printf (XP_TEXT("  "));

	xp_printf (XP_TEXT("%s class [%lu]\n"), 
		XP_STX_DATA(stx, xobj->name),
		(unsigned long)class);
}

void print_subclass_names (xp_stx_t* stx, xp_word_t class, int tabs)
{
	xp_stx_class_t* obj;

	obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx,class);
	if (obj->header.class == stx->class_metaclass) {
		print_metaclass_name (stx, class, tabs);
	}
	else {
		print_class_name (stx, class, tabs);
	}

	if (obj->subclasses != stx->nil) {
		xp_word_t count = XP_STX_SIZE(stx, obj->subclasses);
		while (count-- > 0) {
			print_subclass_names (stx, 
				XP_STX_WORDAT(stx,obj->subclasses,count), tabs + 1);
		}
	}
}

void print_subclasses (xp_stx_t* stx, const xp_char_t* name)
{
	xp_word_t class;
	class = xp_stx_lookup_class (stx, name);	
	xp_printf (XP_TEXT("== NORMAL == \n"));
	print_subclass_names (stx, class, 0);
	xp_printf (XP_TEXT("== META == \n"));
	print_subclass_names (stx, XP_STX_CLASS(stx,class), 0);
}

int xp_main (int argc, xp_char_t* argv[])
{
	xp_stx_t stx;
	//xp_word_t i;

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
	
	xp_stx_traverse_symbol_table (&stx, print_symbol_names, XP_NULL);
	xp_printf (XP_TEXT("-------------\n"));

	xp_stx_hash_traverse (&stx, stx.smalltalk, print_symbol_names_2, XP_NULL);
	xp_printf (XP_TEXT("-------------\n"));

	print_superclasses (&stx, XP_TEXT("Array"));
	xp_printf (XP_TEXT("-------------\n"));
	print_metaclass_superclasses (&stx, XP_TEXT("Array"));
	xp_printf (XP_TEXT("-------------\n"));
	print_superclasses (&stx, XP_TEXT("False"));
	xp_printf (XP_TEXT("-------------\n"));
	print_metaclass_superclasses (&stx, XP_TEXT("False"));
	xp_printf (XP_TEXT("-------------\n"));
	print_superclasses (&stx, XP_TEXT("Metaclass"));
	xp_printf (XP_TEXT("-------------\n"));
	print_metaclass_superclasses (&stx, XP_TEXT("Metaclass"));
	xp_printf (XP_TEXT("-------------\n"));
	print_superclasses (&stx, XP_TEXT("Class"));
	xp_printf (XP_TEXT("-------------\n"));
	print_metaclass_superclasses (&stx, XP_TEXT("Class"));
	xp_printf (XP_TEXT("-------------\n"));

	print_subclasses (&stx, XP_TEXT("Object"));
	xp_printf (XP_TEXT("-------------\n"));

#if 0
	{
		xp_word_t method_name;
		xp_word_t main_class;
		xp_word_t method, context;

		method_name = xp_stx_new_symbol (&stx,XP_TEXT("main"));

		main_class = xp_stx_lookup_class (&stx,argv[1]);
		if (main_class == stx.nil) {
			xp_printf (XP_TEXT("non-existent class: %s\n"), argv[1]);
			return -1;
		}

		/*
		method = xp_stx_alloc_byte_object (&stx,100);
		XP_STX_CLASS(&stx,method) = stx.class_method;
		*/
		method = xp_stx_instantiate (&stx, XP_TEXT("Method"));

		XP_STX_BYTEAT(&stx,method,0) = PUSH_OBJECT;
		XP_STX_BYTEAT(&stx,method,1) = main_class;
		XP_STX_BYTEAT(&stx,method,2) = SEND_UNARY_MESSAGE;
		XP_STX_BYTEAT(&stx,method,3) = method_name;
		XP_STX_BYTEAT(&stx,method,4) = HALT;

		/*
		context = xp_stx_new_context (&stx, method, stx.nil, stx.nil);
		*/
		context = xp_stx_instantiate (&stx, XP_TEXT("Context"));
		xp_stx_run_context (&stx, context);
	}
#endif

	xp_stx_close (&stx);
	xp_printf (XP_TEXT("== End of program ==\n"));
	return 0;
}

