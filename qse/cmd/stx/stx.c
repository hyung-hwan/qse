/*#include <qse/stx/stx.h>*/
#include "../../lib/stx/stx.h"
#include <qse/cmn/mem.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

void print_class_name (qse_stx_t* stx, qse_word_t class, int tabs);
void print_metaclass_name (qse_stx_t* stx, qse_word_t class, int tabs);


void print_system_symbol_table (qse_stx_t* stx)
{
	qse_word_t ref = stx->ref.symtab;
	qse_word_t capa, i;

	capa = QSE_STX_OBJSIZE(stx,ref) - 1;
	qse_printf (QSE_T("TALLY = %d\n"), QSE_STX_REFTOINT(stx,QSE_STX_WORDAT(stx,ref,QSE_STX_SYSTEMSYMBOLTABLE_TALLY)));
	for (i = 1; i <= capa; i++)
	{
		qse_word_t symref = QSE_STX_WORDAT(stx,ref,i);
		if (symref != stx->ref.nil)
		{
			qse_printf (QSE_T("%lu [%s]\n"), (unsigned long)symref, QSE_STX_CHARPTR(stx,symref));
		}
	}
}

void print_system_dictionary (qse_stx_t* stx)
{
	qse_word_t ref = stx->ref.sysdic;
	qse_word_t capa, i;

	capa = QSE_STX_OBJSIZE(stx,ref) - 1;
	qse_printf (QSE_T("TALLY = %d\n"), QSE_STX_REFTOINT(stx,QSE_STX_WORDAT(stx,ref,QSE_STX_SYSTEMSYMBOLTABLE_TALLY)));
	for (i = 1; i <= capa; i++)
	{
		qse_word_t assref = QSE_STX_WORDAT(stx,ref,i);
		if (assref != stx->ref.nil)
		{
			qse_word_t keyref = QSE_STX_WORDAT(stx,assref,QSE_STX_ASSOCIATION_KEY);
			qse_word_t valref = QSE_STX_WORDAT(stx,assref,QSE_STX_ASSOCIATION_VALUE);
			qse_char_t value[128] = QSE_T("");

			qse_word_t vcref = QSE_STX_OBJCLASS(stx,valref);

			if (QSE_STX_OBJCLASS(stx,vcref) == stx->ref.class_metaclass)
			{
				qse_word_t nameref = QSE_STX_WORDAT(stx,valref,QSE_STX_CLASS_NAME);
				qse_sprintf (value, QSE_COUNTOF(value),
					QSE_T("<<%s>>"), QSE_STX_CHARPTR(stx,nameref));
			}
			else
			{
				qse_word_t nameref = QSE_STX_WORDAT(stx,vcref,QSE_STX_CLASS_NAME);
				qse_sprintf (value, QSE_COUNTOF(value),
					QSE_T("instance of <<%s>>"), QSE_STX_CHARPTR(stx,nameref));
			}

			/* i dind't use other object types for a key... so keys must be symbols */
			QSE_ASSERT (QSE_STX_OBJCLASS(stx,keyref) == stx->ref.class_symbol);

			qse_printf (QSE_T("%lu [%s] => %s\n"), 
				(unsigned long)keyref, QSE_STX_CHARPTR(stx,keyref), value
			);
		}
	}
}

void print_superclasses (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t n;
	qse_stx_class_t* obj;

	n = qse_stx_findclass (stx, name);
	qse_printf (QSE_T("Class hierarchy for the class '%s'\n"), name);

	while (n != stx->ref.nil)
	{
		obj = (qse_stx_class_t*)PTRBYREF(stx,n);
		qse_printf (QSE_T("%lu, %s\n"), (unsigned long)obj->name, CHARPTR(stx, obj->name));
		n = obj->superclass;
	}
}

void print_metaclass_superclasses (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t n, x;
	qse_stx_metaclass_t* obj;
	qse_stx_class_t* xobj;

	n = qse_stx_findclass (stx, name);

	n = QSE_STX_OBJCLASS(stx,n);
	qse_printf (QSE_T("Class hierarchy for the metaclass '%s class'\n"), name);

	while (n != stx->ref.nil) 
	{
		/*if (n == stx->class_class) break; */
		if (QSE_STX_OBJCLASS(stx,n) != stx->ref.class_metaclass) 
		{
			/* probably reached the superclass of 
			 * 'Object class' which is 'Class' * */
			break;
		}

		obj = (qse_stx_metaclass_t*)QSE_STX_PTRBYREF(stx,n);
		x = obj->instance_class;

		xobj = (qse_stx_class_t*)QSE_STX_PTRBYREF(stx,x);
		qse_printf (QSE_T("%lu, %s class\n"), 
			(unsigned long)xobj->name,
			CHARPTR(stx, xobj->name));


		n = obj->superclass;
	}

	while (n != stx->ref.nil) 
	{
		xobj = (qse_stx_class_t*)QSE_STX_PTRBYREF(stx,n);
		qse_printf (QSE_T("%lu, %s\n"), 
			(unsigned long)xobj->name,
			CHARPTR(stx, xobj->name));
		n = xobj->superclass;
	}
}

void print_class_name (qse_stx_t* stx, qse_word_t class, int tabs)
{
	qse_stx_class_t* xobj;
	xobj = (qse_stx_class_t*)QSE_STX_PTRBYREF(stx,class);

	while (tabs-- > 0) qse_printf (QSE_T("  "));

	qse_printf (QSE_T("%s [%lu]\n"), 
		CHARPTR(stx, xobj->name),
		(unsigned long)class);
}

void print_metaclass_name (qse_stx_t* stx, qse_word_t class, int tabs)
{
	qse_stx_metaclass_t* obj;
	qse_stx_class_t* xobj;

	obj = (qse_stx_metaclass_t*)QSE_STX_PTRBYREF(stx,class);
	xobj = (qse_stx_class_t*)QSE_STX_PTRBYREF(stx,obj->instance_class);

	while (tabs-- > 0) qse_printf (QSE_T("  "));

	qse_printf (QSE_T("%s class [%lu]\n"), 
		CHARPTR(stx, xobj->name),
		(unsigned long)class);
}

void print_subclass_names (qse_stx_t* stx, qse_word_t class, int tabs)
{
	qse_stx_class_t* obj;

	if (QSE_STX_OBJCLASS(stx,class) == stx->ref.class_metaclass) 
	{
		print_metaclass_name (stx, class, tabs);
	}
	else 
	{
		print_class_name (stx, class, tabs);
	}

	obj = (qse_stx_class_t*)QSE_STX_PTRBYREF(stx,class);
	if (obj->subclasses != stx->ref.nil) 
	{
		qse_word_t count = QSE_STX_OBJSIZE(stx, obj->subclasses);
		while (count-- > 0) 
		{
			print_subclass_names (stx, 
				QSE_STX_WORDAT(stx,obj->subclasses,count), tabs + 1);
		}
	}
}

void print_subclasses (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t class;
	class = qse_stx_findclass (stx, name);	
	qse_printf (QSE_T("== NORMAL == \n"));
	print_subclass_names (stx, class, 0);
	qse_printf (QSE_T("== META == \n"));
	print_subclass_names (stx, QSE_STX_OBJCLASS(stx,class), 0);
}

static int stx_main (int argc, qse_char_t* argv[])
{
	qse_stx_t* stx;
	//qse_word_t i;

#if 0
	if (argc != 2) { /* TODO: argument processing */
		qse_printf (QSE_T("Usage: %s [-f imageFile] MainClass\n"), argv[0]);
		return -1;
	}
#endif

	stx = qse_stx_open (QSE_MMGR_GETDFL(), 0, 10000);
	if (stx == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open stx\n"));
		return -1;
	}

	if (qse_stx_boot(stx) <= -1) 
	{
		qse_stx_close (stx);
		qse_printf (QSE_T("cannot bootstrap\n"));
		return -1;
	}	

	qse_printf (QSE_T("stx.nil %lu %lu\n"), (unsigned long)stx->ref.nil, (unsigned long)QSE_STX_REFTOIDX(stx,stx->ref.nil));
	qse_printf (QSE_T("stx.true %lu %lu\n"), (unsigned long)stx->ref.true, (unsigned long)QSE_STX_REFTOIDX(stx,stx->ref.true));
	qse_printf (QSE_T("stx.false %lu %lu\n"), (unsigned long)stx->ref.false, (unsigned long)QSE_STX_REFTOIDX(stx,stx->ref.false));
	qse_printf (QSE_T("-------------\n"));
	
	
	qse_printf (QSE_T(">> SYSTEM_SYMBOL_TABLE\n"));
	print_system_symbol_table (stx);
	qse_printf (QSE_T("-------------\n"));

	qse_printf (QSE_T(">> SYSTEM_DICTIONARY\n"));
	print_system_dictionary (stx);
	qse_printf (QSE_T("-------------\n"));

	print_superclasses (stx, QSE_T("Array"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (stx, QSE_T("Array"));
	qse_printf (QSE_T("-------------\n"));
	print_superclasses (stx, QSE_T("False"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (stx, QSE_T("False"));
	qse_printf (QSE_T("-------------\n"));
	print_superclasses (stx, QSE_T("Metaclass"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (stx, QSE_T("Metaclass"));
	qse_printf (QSE_T("-------------\n"));
	print_superclasses (stx, QSE_T("Class"));
	qse_printf (QSE_T("-------------\n"));
	print_metaclass_superclasses (stx, QSE_T("Class"));
	qse_printf (QSE_T("-------------\n"));

	print_subclasses (stx, QSE_T("Object"));
	qse_printf (QSE_T("-------------\n"));

#if 0
	{
		qse_word_t method_name;
		qse_word_t main_class;
		qse_word_t method, context;

		method_name = qse_stx_new_symbol (stx,QSE_T("main"));

		main_class = qse_stx_findclass (stx,argv[1]);
		if (main_class == stx.nil) {
			qse_printf (QSE_T("non-existent class: %s\n"), argv[1]);
			return -1;
		}

		/*
		method = qse_stx_alloc_byte_object (stx,100);
		QSE_STX_CLASS(stx,method) = stx.class_method;
		*/
		method = qse_stx_instantiate (stx, QSE_T("Method"));

		QSE_STX_BYTEAT(stx,method,0) = PUSH_OBJECT;
		QSE_STX_BYTEAT(stx,method,1) = main_class;
		QSE_STX_BYTEAT(stx,method,2) = SEND_UNARY_MESSAGE;
		QSE_STX_BYTEAT(stx,method,3) = method_name;
		QSE_STX_BYTEAT(stx,method,4) = HALT;

		/*
		context = qse_stx_new_context (stx, method, stx.nil, stx.nil);
		*/
		context = qse_stx_instantiate (stx, QSE_T("Context"));
		qse_stx_runcontext (stx, context);
	}
#endif

	qse_stx_close (stx);
	qse_printf (QSE_T("== End of program ==\n"));
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, stx_main);
}

