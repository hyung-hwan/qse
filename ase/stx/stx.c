/*
 * $Id: stx.c,v 1.26 2005-05-22 15:03:20 bacon Exp $
 */

#include <xp/stx/stx.h>
#include <xp/stx/memory.h>
#include <xp/stx/symbol.h>
#include <xp/stx/class.h>
#include <xp/stx/object.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

static void __create_bootstrapping_objects (xp_stx_t* stx);

xp_stx_t* xp_stx_open (xp_stx_t* stx, xp_stx_word_t capacity)
{
	if (stx == XP_NULL) {
		stx = (xp_stx_t*)xp_stx_malloc (xp_sizeof(stx));
		if (stx == XP_NULL) return XP_NULL;
		stx->__malloced = xp_true;
	}
	else stx->__malloced = xp_false;

	if (xp_stx_memory_open (&stx->memory, capacity) == XP_NULL) {
		if (stx->__malloced) xp_stx_free (stx);
		return XP_NULL;
	}

	stx->nil = XP_STX_NIL;
	stx->true = XP_STX_TRUE;
	stx->false = XP_STX_FALSE;

	stx->symbol_table = XP_STX_NIL;
	stx->smalltalk = XP_STX_NIL;

	stx->class_symlink = XP_STX_NIL;
	stx->class_symbol = XP_STX_NIL;
	stx->class_metaclass = XP_STX_NIL;
	stx->class_pairlink = XP_STX_NIL;

	stx->class_method = XP_STX_NIL;
	stx->class_context = XP_STX_NIL;

	stx->__wantabort = xp_false;
	return stx;
}

void xp_stx_close (xp_stx_t* stx)
{
	xp_stx_memory_close (&stx->memory);
	if (stx->__malloced) xp_stx_free (stx);
}

int xp_stx_bootstrap (xp_stx_t* stx)
{
	xp_stx_word_t symbol_Smalltalk;
	xp_stx_word_t class_Object, class_Class;
	xp_stx_word_t tmp;

	__create_bootstrapping_objects (stx);

	/* more initialization */
	XP_STX_CLASS(stx,stx->symbol_table) = 
		xp_stx_new_class (stx, XP_STX_TEXT("SymbolTable"));
	XP_STX_CLASS(stx,stx->smalltalk) = 
		xp_stx_new_class (stx, XP_STX_TEXT("SystemDictionary"));

	symbol_Smalltalk = 
		xp_stx_new_symbol (stx, XP_STX_TEXT("Smalltalk"));
	xp_stx_hash_insert (stx, stx->smalltalk,
		xp_stx_hash_char_object(stx,symbol_Smalltalk),
		symbol_Smalltalk, stx->smalltalk);	

	/* create #nil, #true, #false */
	xp_stx_new_symbol (stx, XP_STX_TEXT("nil"));
	xp_stx_new_symbol (stx, XP_STX_TEXT("true"));
	xp_stx_new_symbol (stx, XP_STX_TEXT("false"));

	/* nil setClass: UndefinedObject */
	XP_STX_CLASS(stx,stx->nil) =
		xp_stx_new_class (stx, XP_STX_TEXT("UndefinedObject"));
	/* true setClass: True */
	XP_STX_CLASS(stx,stx->true) =
		xp_stx_new_class (stx, XP_STX_TEXT("True"));
	/* fales setClass: False */
	XP_STX_CLASS(stx,stx->false) = 
		xp_stx_new_class (stx, XP_STX_TEXT("False"));

	/* weave the class-metaclass chain */
	class_Object = xp_stx_new_class (stx, XP_STX_TEXT("Object"));
	class_Class = xp_stx_new_class (stx, XP_STX_TEXT("Class"));
	tmp = XP_STX_CLASS(stx,class_Object);
	XP_STX_AT(stx,tmp,XP_STX_CLASS_SUPERCLASS) = class_Class;

	/* useful classes */
	stx->class_method = xp_stx_new_class (stx, XP_STX_TEXT("Method"));
	stx->class_context = xp_stx_new_class (stx, XP_STX_TEXT("Context"));

	return 0;
}

static void __create_bootstrapping_objects (xp_stx_t* stx)
{
	xp_stx_word_t class_SymlinkMeta;
	xp_stx_word_t class_SymbolMeta; 
	xp_stx_word_t class_MetaclassMeta;
	xp_stx_word_t class_PairlinkMeta;
	xp_stx_word_t symbol_Symlink;
	xp_stx_word_t symbol_Symbol; 
	xp_stx_word_t symbol_Metaclass;
	xp_stx_word_t symbol_Pairlink;

	/* allocate three keyword objects */
	stx->nil = xp_stx_alloc_word_object (stx, 0);
	stx->true = xp_stx_alloc_word_object (stx, 0);
	stx->false = xp_stx_alloc_word_object (stx, 0);

	xp_stx_assert (stx->nil == XP_STX_NIL);
	xp_stx_assert (stx->true == XP_STX_TRUE);
	xp_stx_assert (stx->false == XP_STX_FALSE);

	/* symbol table & system dictionary */
	/* TODO: symbol table and dictionary size */
	stx->symbol_table = xp_stx_alloc_word_object (stx, 1000); 
	stx->smalltalk = xp_stx_alloc_word_object (stx, 2000);

	stx->class_symlink = /* Symlink */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);
	stx->class_symbol =  /* Symbol */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);
	stx->class_metaclass =  /* Metaclass */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);
	stx->class_pairlink =  /* Pairlink */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);

	class_SymlinkMeta = /* Symlink class */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);
	class_SymbolMeta = /* Symbol class */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);
	class_MetaclassMeta = /* Metaclass class */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);
	class_PairlinkMeta = /* Pairlink class */
		xp_stx_alloc_word_object(stx,XP_STX_CLASS_SIZE);

	/* (Symlink class) setClass: Metaclass */
	XP_STX_CLASS(stx,class_SymlinkMeta) = stx->class_metaclass;
	/* (Symbol class) setClass: Metaclass */
	XP_STX_CLASS(stx,class_SymbolMeta) = stx->class_metaclass;
	/* (Metaclass class) setClass: Metaclass */
	XP_STX_CLASS(stx,class_MetaclassMeta) = stx->class_metaclass;
	/* (Pairlink class) setClass: Metaclass */
	XP_STX_CLASS(stx,class_PairlinkMeta) = stx->class_metaclass;

	/* Symlink setClass: (Symlink class) */
	XP_STX_CLASS(stx,stx->class_symlink) = class_SymlinkMeta;
	/* Symbol setClass: (Symbol class) */
	XP_STX_CLASS(stx,stx->class_symbol) = class_SymbolMeta;
	/* Metaclass setClass: (Metaclass class) */
	XP_STX_CLASS(stx,stx->class_metaclass) = class_MetaclassMeta;
	/* Pairlink setClass: (Pairlink class) */
	XP_STX_CLASS(stx,stx->class_pairlink) = class_PairlinkMeta;

	/* (Symlink class) setSpec: CLASS_SIZE */
	XP_STX_AT(stx,class_SymlinkMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT(XP_STX_CLASS_SIZE);
	/* (Symbol class) setSpec: CLASS_SIZE */
	XP_STX_AT(stx,class_SymbolMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT(XP_STX_CLASS_SIZE);
	/* (Metaclass class) setSpec: CLASS_SIZE */
	XP_STX_AT(stx,class_MetaclassMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT(XP_STX_CLASS_SIZE);
	/* (Pairlink class) setSpec: CLASS_SIZE */
	XP_STX_AT(stx,class_PairlinkMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT(XP_STX_CLASS_SIZE);

	/* #Symlink */
	symbol_Symlink = xp_stx_new_symbol (stx, XP_STX_TEXT("Symlink"));
	/* #Symbol */
	symbol_Symbol = xp_stx_new_symbol (stx, XP_STX_TEXT("Symbol"));
	/* #Metaclass */
	symbol_Metaclass = xp_stx_new_symbol (stx, XP_STX_TEXT("Metaclass"));
	/* #Pairlink */
	symbol_Pairlink = xp_stx_new_symbol (stx, XP_STX_TEXT("Pairlink"));

	/* Symlink setName: #Symlink */
	XP_STX_AT(stx,stx->class_symlink,XP_STX_CLASS_NAME) = symbol_Symlink;
	/* Symbol setName: #Symbol */
	XP_STX_AT(stx,stx->class_symbol,XP_STX_CLASS_NAME) = symbol_Symbol;
	/* Metaclass setName: #Metaclass */
	XP_STX_AT(stx,stx->class_metaclass,XP_STX_CLASS_NAME) = symbol_Metaclass;
	/* Pairlink setName: #Pairlink */
	XP_STX_AT(stx,stx->class_pairlink,XP_STX_CLASS_NAME) = symbol_Pairlink;

	/* register class names into the system dictionary */
	xp_stx_hash_insert (stx, stx->smalltalk,
		xp_stx_hash_char_object(stx, symbol_Symlink),
		symbol_Symlink, stx->class_symlink);
	xp_stx_hash_insert (stx, stx->smalltalk,
		xp_stx_hash_char_object(stx, symbol_Symbol),
		symbol_Symbol, stx->class_symbol);
	xp_stx_hash_insert (stx, stx->smalltalk,
		xp_stx_hash_char_object(stx, symbol_Metaclass),
		symbol_Metaclass, stx->class_metaclass);
	xp_stx_hash_insert (stx, stx->smalltalk,
		xp_stx_hash_char_object(stx, symbol_Pairlink),
		symbol_Pairlink, stx->class_pairlink);
}


