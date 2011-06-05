#include <qse/stx/stx.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

#if 0
#include <qse/stx/bootstrp.h>
#include <qse/stx/object.h>
#include <qse/stx/symbol.h>
#include <qse/stx/context.h>
#include <qse/stx/class.h>
#include <qse/stx/dict.h>
#endif

#if 0
void print_symbol_names (qse_stx_t* stx, qse_word_t sym, void* unused)
{
	qse_printf (QSE_T("%lu [%s]\n"), (unsigned long)sym, QSE_STX_DATA(stx,sym));
}

void print_symbol_names_2 (qse_stx_t* stx, qse_word_t idx, void* unused)
{
	qse_word_t key = QSE_STX_WORD_AT(stx,idx,QSE_STX_ASSOCIATION_KEY);
	qse_word_t value = QSE_STX_WORD_AT(stx,idx,QSE_STX_ASSOCIATION_VALUE);
	qse_printf (QSE_T("%lu [%s] %lu\n"), 
		(unsigned long)key, QSE_STX_DATA(stx,key), (unsigned long)value);
}

void print_superclasses (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t n;
	qse_stx_class_t* obj;

	n = qse_stx_lookup_class (stx, name);
	qse_printf (QSE_T("Class hierarchy for the class '%s'\n"), name);

	while (n != stx->nil) {
		obj = (qse_stx_class_t*)QSE_STX_WORD_OBJECT(stx,n);
		qse_printf (QSE_T("%lu, %s\n"), 
			(unsigned long)obj->name,
			QSE_STX_DATA(stx, obj->name));
		n = obj->superclass;
	}
}

void print_metaclass_superclasses (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t n, x;
	qse_stx_metaclass_t* obj;
	qse_stx_class_t* xobj;

	n = qse_stx_lookup_class (stx, name);
	n = QSE_STX_CLASS(stx,n);
	qse_printf (QSE_T("Class hierarchy for the metaclass '%s class'\n"), name);

	while (n != stx->nil) {
		/*if (n == stx->class_class) break; */
		if (QSE_STX_CLASS(stx,n) != stx->class_metaclass) break;

		obj = (qse_stx_metaclass_t*)QSE_STX_WORD_OBJECT(stx,n);
		x = obj->instance_class;
		xobj = (qse_stx_class_t*)QSE_STX_WORD_OBJECT(stx,x);
		qse_printf (QSE_T("%lu, %s class\n"), 
			(unsigned long)xobj->name,
			QSE_STX_DATA(stx, xobj->name));
		n = obj->superclass;
	}
	while (n != stx->nil) {
		xobj = (qse_stx_class_t*)QSE_STX_WORD_OBJECT(stx,n);
		qse_printf (QSE_T("%lu, %s\n"), 
			(unsigned long)xobj->name,
			QSE_STX_DATA(stx, xobj->name));
		n = xobj->superclass;
	}
}

void print_class_name (qse_stx_t* stx, qse_word_t class, int tabs)
{
	qse_stx_class_t* xobj;
	xobj = (qse_stx_class_t*)QSE_STX_WORD_OBJECT(stx,class);

	while (tabs-- > 0) qse_printf (QSE_T("  "));

	qse_printf (QSE_T("%s [%lu]\n"), 
		QSE_STX_DATA(stx, xobj->name),
		(unsigned long)class);
}

void print_metaclass_name (qse_stx_t* stx, qse_word_t class, int tabs)
{
	qse_stx_metaclass_t* obj;
	qse_stx_class_t* xobj;

	obj = (qse_stx_metaclass_t*)QSE_STX_WORD_OBJECT(stx,class);
	xobj = (qse_stx_class_t*)QSE_STX_WORD_OBJECT(stx,obj->instance_class);

	while (tabs-- > 0) qse_printf (QSE_T("  "));

	qse_printf (QSE_T("%s class [%lu]\n"), 
		QSE_STX_DATA(stx, xobj->name),
		(unsigned long)class);
}

void print_subclass_names (qse_stx_t* stx, qse_word_t class, int tabs)
{
	qse_stx_class_t* obj;

	obj = (qse_stx_class_t*)QSE_STX_WORD_OBJECT(stx,class);
	if (obj->header.class == stx->class_metaclass) {
		print_metaclass_name (stx, class, tabs);
	}
	else {
		print_class_name (stx, class, tabs);
	}

	if (obj->subclasses != stx->nil) {
		qse_word_t count = QSE_STX_SIZE(stx, obj->subclasses);
		while (count-- > 0) {
			print_subclass_names (stx, 
				QSE_STX_WORD_AT(stx,obj->subclasses,count), tabs + 1);
		}
	}
}

void print_subclasses (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t class;
	class = qse_stx_lookup_class (stx, name);	
	qse_printf (QSE_T("== NORMAL == \n"));
	print_subclass_names (stx, class, 0);
	qse_printf (QSE_T("== META == \n"));
	print_subclass_names (stx, QSE_STX_CLASS(stx,class), 0);
}

static int stx_main (int argc, qse_char_t* argv[])
{
	qse_stx_t stx;
	//qse_word_t i;

#ifndef _DOS
	if (qse_setlocale () == -1) {
		printf ("cannot set locale\n");
		return -1;
	}
#endif

	if (argc != 2) { /* TODO: argument processing */
		qse_printf (QSE_T("Usage: %s [-f imageFile] MainClass\n"), argv[0]);
		return -1;
	}

	if (qse_stx_open (&stx, 10000) == QSE_NULL) {
		qse_printf (QSE_T("cannot open stx\n"));
		return -1;
	}

	if (qse_stx_bootstrap(&stx) == -1) {
		qse_stx_close (&stx);
		qse_printf (QSE_T("cannot bootstrap\n"));
		return -1;
	}	

	qse_printf (QSE_T("stx.nil %lu\n"), (unsigned long)stx.nil);
	qse_printf (QSE_T("stx.true %lu\n"), (unsigned long)stx.true);
	qse_printf (QSE_T("stx.false %lu\n"), (unsigned long)stx.false);
	qse_printf (QSE_T("-------------\n"));
	
	
	qse_printf (QSE_T(">> SYMBOL_TABLE (%u/%u symbols/slots) <<\n"), 
		(unsigned int)stx.symtab.size, (unsigned int)stx.symtab.capacity);
	qse_stx_traverse_symbol_table (&stx, print_symbol_names, QSE_NULL);
	qse_printf (QSE_T("-------------\n"));

	qse_stx_dict_traverse (&stx, stx.smalltalk, print_symbol_names_2, QSE_NULL);
	qse_printf (QSE_T("-------------\n"));

	print_superclasses (&stx, QSE_T("Array"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (&stx, QSE_T("Array"));
	qse_printf (QSE_T("-------------\n"));
	print_superclasses (&stx, QSE_T("False"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (&stx, QSE_T("False"));
	qse_printf (QSE_T("-------------\n"));
	print_superclasses (&stx, QSE_T("Metaclass"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (&stx, QSE_T("Metaclass"));
	qse_printf (QSE_T("-------------\n"));
	print_superclasses (&stx, QSE_T("Class"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (&stx, QSE_T("Class"));
	qse_printf (QSE_T("-------------\n"));

	print_subclasses (&stx, QSE_T("Object"));
	qse_printf (QSE_T("-------------\n"));

#if 0
	{
		qse_word_t method_name;
		qse_word_t main_class;
		qse_word_t method, context;

		method_name = qse_stx_new_symbol (&stx,QSE_T("main"));

		main_class = qse_stx_lookup_class (&stx,argv[1]);
		if (main_class == stx.nil) {
			qse_printf (QSE_T("non-existent class: %s\n"), argv[1]);
			return -1;
		}

		/*
		method = qse_stx_alloc_byte_object (&stx,100);
		QSE_STX_CLASS(&stx,method) = stx.class_method;
		*/
		method = qse_stx_instantiate (&stx, QSE_T("Method"));

		QSE_STX_BYTEAT(&stx,method,0) = PUSH_OBJECT;
		QSE_STX_BYTEAT(&stx,method,1) = main_class;
		QSE_STX_BYTEAT(&stx,method,2) = SEND_UNARY_MESSAGE;
		QSE_STX_BYTEAT(&stx,method,3) = method_name;
		QSE_STX_BYTEAT(&stx,method,4) = HALT;

		/*
		context = qse_stx_new_context (&stx, method, stx.nil, stx.nil);
		*/
		context = qse_stx_instantiate (&stx, QSE_T("Context"));
		qse_stx_run_context (&stx, context);
	}
#endif

	qse_stx_close (&stx);
	qse_printf (QSE_T("== End of program ==\n"));
	return 0;
}
#endif

static int stx_main (int argc, qse_char_t* argv[])
{
	qse_stx_t* stx;

	stx = qse_stx_open (QSE_NULL, 0, 1000);
	if (stx == QSE_NULL)
	{
		qse_printf (QSE_T("Cannot open stx\n"));
		return -1;
	}

	qse_stx_close (stx);
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, stx_main);
}

