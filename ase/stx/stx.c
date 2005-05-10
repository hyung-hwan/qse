/*
 * $Id: stx.c,v 1.7 2005-05-10 08:21:10 bacon Exp $
 */

#include <xp/stx/stx.h>
#include <xp/stx/memory.h>
#include <xp/stx/object.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_stx_word_t capacity)
{
	if (stx == XP_NULL) {
		stx = (xp_stx_t*) xp_malloc (xp_sizeof(stx));
		if (stx == XP_NULL) return XP_NULL;
		stx->__malloced = xp_true;
	}
	else stx->__malloced = xp_false;

	if (xp_stx_memory_open (&stx->memory, capacity) == XP_NULL) {
		if (stx->__malloced) xp_free (stx);
		return XP_NULL;
	}

	stx->nil = XP_STX_NIL;
	stx->true = XP_STX_TRUE;
	stx->false = XP_STX_FALSE;

	stx->link_class = XP_STX_NIL;
	stx->symbol_table = XP_STX_NIL;

	return stx;
}

void xp_stx_close (xp_stx_t* stx)
{
	xp_stx_memory_close (&stx->memory);
	if (stx->__malloced) xp_free (stx);
}

int xp_stx_bootstrap (xp_stx_t* stx)
{
	xp_stx_word_t symtab;
	xp_stx_word_t symbol_Symbol, symbol_SymbolMeta;
	xp_stx_word_t symbol_Metaclass, symbol_MetaclassMeta;
	xp_stx_word_t class_Symbol, class_SymbolMeta;
	xp_stx_word_t class_Metaclass, class_MetaclassMeta;

	/* allocate three keyword objects */
	stx->nil = xp_stx_alloc_object (stx, 0);
	stx->true = xp_stx_alloc_object (stx, 0);
	stx->false = xp_stx_alloc_object (stx, 0);

	xp_assert (stx->nil == XP_STX_NIL);
	xp_assert (stx->true == XP_STX_TRUE);
	xp_assert (stx->false == XP_STX_FALSE);

	/* build a symbol table */   // TODO: symbol table size
	symtab = xp_stx_alloc_object (stx, 1000); 

	/* tweak the initial object structure */
	symbol_Symbol = 
		xp_stx_alloc_string_object(stx, XP_STX_TEXT("Symbol"));
	symbol_SymbolMeta = 
		xp_stx_alloc_string_object(stx,XP_STX_TEXT("SymbolMeta"));
	symbol_Metaclass = 
		xp_stx_alloc_string_object(stx, XP_STX_TEXT("Metaclass"));
	symbol_MetaclassMeta = 
		xp_stx_alloc_string_object(stx, XP_STX_TEXT("MetaclassMeta"));

	// TODO: class size: maybe other than 5?
	class_Metaclass = xp_stx_alloc_object(stx, 5);
	class_MetaclassMeta = xp_stx_alloc_object(stx, 5);
	class_Symbol = xp_stx_alloc_object(stx, 5);
	class_SymbolMeta = xp_stx_alloc_object(stx, 5);

	XP_STX_CLASS(stx,symbol_Symbol) = class_Symbol;
	XP_STX_CLASS(stx,symbol_SymbolMeta) = class_Symbol;
	XP_STX_CLASS(stx,symbol_Metaclass) = class_Symbol;
	XP_STX_CLASS(stx,symbol_MetaclassMeta) = class_Symbol;

	XP_STX_CLASS(stx,class_Symbol) = class_SymbolMeta;
	XP_STX_CLASS(stx,class_SymbolMeta) = class_Metaclass;
	XP_STX_CLASS(stx,class_Metaclass) = class_MetaclassMeta;
	XP_STX_CLASS(stx,class_MetaclassMeta) = class_Metaclass;
	
	/*
	class_Symbol = xp_stx_instantiate_class (XP_STX_TEXT("Symbol"));
	XP_STX_CLASS(stx,symbol_Symbol) = class_Symbol;
	XP_STX_CLASS(stx,symbol_Symbol_class) = class_Symbol;

	class_Metaclass = xp_stx_instantiate_class (XP_STX_TEXT("Metaclass"));

	XP_STX_CLASS(stx,class_Symbol) = class_Metaclass;
	XP_STX_CLASS(stx,class_Metaclass) = class_Metaclass;

	class_UndefinedObject = xp_stx_instantiate_class (XP_STX_TEXT("UndefinedObject"));
	class_True = xp_stx_instantiate_class (XP_STX_TEXT("True"));
	class_False = xp_stx_instantiate_class (XP_STX_TEXT("False"));
	symbol_nil = xp_stx_instantiate_symbol (XP_STX_TEXT("nil"));
	symbol_true = xp_stx_instantiate_symbol (XP_STX_TEXT("true"));
	symbol_false = xp_stx_instantiate_symbol (XP_STX_TEXT("false"));

	XP_STX_CLASS(stx,stx->nil) = class_UndefinedObject;
	XP_STX_CLASS(stx,stx->true) = class_True;
	XP_STX_CLASS(stx,stx->false) = class_False;

	insert_into_symbol_table (stx, symbol_table, symbol_nil, stx->nil);
	insert_into_symbol_table (stx, symbol_table, symbol_true, stx->true);
	insert_into_symbol_table (stx, symbol_table, symbol_false, stx->false);

	class_Link = xp_stx_instantiate_class (XP_STX_TEXT("Link"));
	
//	TODO here

	class_Array =  xp_stx_instantiate_class (XP_STX_TEXT("Array"));
	class_SymbolTable = xp_stx_instantiate_class (XP_STX_TEXT("SymbolTable"));		

	XP_STX_CLASS(stx,hash_table) = class_Array;
	XP_STX_CLASS(stx,symbol_table) = class_SymbolTable;

	insert_into_symbol_table (stx, symbol_table, symbol_table, symbol_table);

	class_Object = xp_stx_instantiate_class (XP_STX_TEXT("Object"));
	class_Class = xp_stx_instantiate_class (XP_STX_TEXT("Class"));
	XP_STX_AT(stx,classOf(class_Object),superClass,class_Class);
*/

	return 0;
}

