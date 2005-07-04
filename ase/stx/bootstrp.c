/*
 * $Id: bootstrp.c,v 1.17 2005-07-04 16:23:13 bacon Exp $
 */

#include <xp/stx/bootstrp.h>
#include <xp/stx/symbol.h>
#include <xp/stx/class.h>
#include <xp/stx/object.h>
#include <xp/stx/hash.h>
#include <xp/stx/misc.h>

static void __create_bootstrapping_objects (xp_stx_t* stx);
static void __create_builtin_classes (xp_stx_t* stx);
static xp_word_t __make_classvar_dict (
	xp_stx_t* stx, xp_word_t class, const xp_char_t* names);
static void __filein_kernel (xp_stx_t* stx);

static xp_word_t __count_names (const xp_char_t* str);
static void __set_names (
	xp_stx_t* stx, xp_word_t* array, const xp_char_t* str);

static xp_word_t __count_subclasses (const xp_char_t* str);
static void __set_subclasses (
	xp_stx_t* stx, xp_word_t* array, const xp_char_t* str);
static void __set_metaclass_subclasses (
	xp_stx_t* stx, xp_word_t* array, const xp_char_t* str);

struct class_info_t 
{
	const xp_char_t* name;
	const xp_char_t* superclass;
	const xp_char_t* instance_variables;
	const xp_char_t* class_variables;
	const xp_char_t* pool_dictionaries;
	const int indexable;
};

typedef struct class_info_t class_info_t;

static class_info_t class_info[] =
{
	{
		XP_TEXT("Object"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("UndefinedObject"),
		XP_TEXT("Object"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		XP_TEXT("Behavior"),
		XP_TEXT("Object"),
		XP_TEXT("spec methods superclass"),
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		XP_TEXT("Class"),
		XP_TEXT("Behavior"),
		XP_TEXT("name variables classVariables poolDictionaries"),
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		XP_TEXT("Metaclass"),
		XP_TEXT("Behavior"),
		XP_TEXT("instanceClass"),
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Block"),
		XP_TEXT("Object"),
		XP_TEXT("context argCount argLoc bytePointer"),
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Boolean"),
		XP_TEXT("Object"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("True"),
		XP_TEXT("Boolean"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("False"),
		XP_TEXT("Boolean"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Context"),
		XP_TEXT("Object"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Method"),
		XP_TEXT("Object"),
		XP_TEXT("text message bytecodes literals stackSize temporarySize"),
		//XP_NULL,
		XP_TEXT("Win32Errors"), // TODO: REMOVE THIS
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Magnitude"),
		XP_TEXT("Object"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Collection"),
		XP_TEXT("Magnitude"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("IndexedCollection"),
		XP_TEXT("Collection"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Array"),
		XP_TEXT("IndexedCollection"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_WORD_INDEXABLE
	},
	{
		XP_TEXT("ByteArray"),
		XP_TEXT("IndexedCollection"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_BYTE_INDEXABLE
	},
	{
		XP_TEXT("SymbolTable"),
		XP_TEXT("IndexedCollection"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_WORD_INDEXABLE	
	},
	{
		XP_TEXT("Dictionary"),
		XP_TEXT("IndexedCollection"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_WORD_INDEXABLE	
	},
	{
		XP_TEXT("SystemDictionary"),
		XP_TEXT("Dictionary"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_WORD_INDEXABLE	
	},
	{
		XP_TEXT("PoolDictionary"),
		XP_TEXT("Dictionary"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_WORD_INDEXABLE	
	},
	{
		XP_TEXT("String"),
		XP_TEXT("IndexedCollection"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_CHAR_INDEXABLE
	},
	{
		XP_TEXT("Symbol"),
		XP_TEXT("String"),
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_CHAR_INDEXABLE
	},
	{
		XP_TEXT("Link"),
		XP_TEXT("Object"),
		XP_TEXT("link"),
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},
	{
		XP_TEXT("Symlink"),
		XP_TEXT("Link"),
		XP_TEXT("symbol"),
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	},

	{
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_NULL,
		XP_STX_SPEC_NOT_INDEXABLE
	}
};

xp_word_t xp_stx_new_array (xp_stx_t* stx, xp_word_t size)
{
	xp_word_t x;

	xp_assert (stx->class_array != stx->nil);
	x = xp_stx_alloc_word_object (stx, size);
	XP_STX_CLASS(stx,x) = stx->class_array;

	return x;	
}

xp_word_t xp_stx_new_string (xp_stx_t* stx, const xp_char_t* str)
{
	xp_word_t x;

	xp_assert (stx->class_string != stx->nil);
	x = xp_stx_alloc_char_object (stx, str);
	XP_STX_CLASS(stx,x) = stx->class_string;

	return x;	
}

int xp_stx_bootstrap (xp_stx_t* stx)
{
	xp_word_t symbol_Smalltalk;
	xp_word_t object_meta;

	__create_bootstrapping_objects (stx);

	/* object, class, and array are precreated for easier instantiation
	 * of builtin classes */
	stx->class_object = xp_stx_new_class (stx, XP_TEXT("Object"));
	stx->class_class = xp_stx_new_class (stx, XP_TEXT("Class"));
	stx->class_array = xp_stx_new_class (stx, XP_TEXT("Array"));
	stx->class_string = xp_stx_new_class (stx, XP_TEXT("String"));

	__create_builtin_classes (stx);

	/* (Object class) setSuperclass: Class */
	object_meta = XP_STX_CLASS(stx,stx->class_object);
	XP_STX_WORDAT(stx,object_meta,XP_STX_METACLASS_SUPERCLASS) = stx->class_class;
	/* instance class for Object is set here as it is not 
	 * set in __create_builtin_classes */
	XP_STX_WORDAT(stx,object_meta,XP_STX_METACLASS_INSTANCE_CLASS) = stx->class_object;

	/* for some fun here */
	{
		xp_word_t array;
		array = xp_stx_new_array (stx, 1);
		XP_STX_WORDAT(stx,array,0) = object_meta;
		XP_STX_WORDAT(stx,stx->class_class,XP_STX_CLASS_SUBCLASSES) = array;
	}
			
	/* more initialization */
	XP_STX_CLASS(stx,stx->symbol_table) = 
		xp_stx_lookup_class (stx, XP_TEXT("SymbolTable"));
	XP_STX_CLASS(stx,stx->smalltalk) = 
		xp_stx_lookup_class (stx, XP_TEXT("SystemDictionary"));

	symbol_Smalltalk = 
		xp_stx_new_symbol (stx, XP_TEXT("Smalltalk"));
	xp_stx_hash_insert (stx, stx->smalltalk,
		xp_stx_hash_char_object(stx, symbol_Smalltalk),
		symbol_Smalltalk, stx->smalltalk);	

	/* create #nil, #true, #false */
	xp_stx_new_symbol (stx, XP_TEXT("nil"));
	xp_stx_new_symbol (stx, XP_TEXT("true"));
	xp_stx_new_symbol (stx, XP_TEXT("false"));

	/* nil setClass: UndefinedObject */
	XP_STX_CLASS(stx,stx->nil) =
		xp_stx_lookup_class (stx, XP_TEXT("UndefinedObject"));
	/* true setClass: True */
	XP_STX_CLASS(stx,stx->true) =
		xp_stx_lookup_class (stx, XP_TEXT("True"));
	/* fales setClass: False */
	XP_STX_CLASS(stx,stx->false) = 
		xp_stx_lookup_class (stx, XP_TEXT("False"));

	__filein_kernel (stx);
	return 0;
}

static void __create_bootstrapping_objects (xp_stx_t* stx)
{
	xp_word_t class_SymlinkMeta;
	xp_word_t class_SymbolMeta; 
	xp_word_t class_MetaclassMeta;
	xp_word_t class_PairlinkMeta;
	xp_word_t symbol_Symlink;
	xp_word_t symbol_Symbol; 
	xp_word_t symbol_Metaclass;
	xp_word_t symbol_Pairlink;

	/* allocate three keyword objects */
	stx->nil = xp_stx_alloc_word_object (stx, 0);
	stx->true = xp_stx_alloc_word_object (stx, 0);
	stx->false = xp_stx_alloc_word_object (stx, 0);

	xp_assert (stx->nil == XP_STX_NIL);
	xp_assert (stx->true == XP_STX_TRUE);
	xp_assert (stx->false == XP_STX_FALSE);

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

	/* Metaclass is a class so it has the same structure 
	 * as a normal class. "Metaclass class" is an instance of
	 * Metaclass. */

	class_SymlinkMeta = /* Symlink class */
		xp_stx_alloc_word_object(stx,XP_STX_METACLASS_SIZE);
	class_SymbolMeta = /* Symbol class */
		xp_stx_alloc_word_object(stx,XP_STX_METACLASS_SIZE);
	class_MetaclassMeta = /* Metaclass class */
		xp_stx_alloc_word_object(stx,XP_STX_METACLASS_SIZE);
	class_PairlinkMeta = /* Pairlink class */
		xp_stx_alloc_word_object(stx,XP_STX_METACLASS_SIZE);

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

	/* (Symlink class) setSpec: XP_STX_CLASS_SIZE */
	XP_STX_WORDAT(stx,class_SymlinkMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT((XP_STX_CLASS_SIZE << 2) | XP_STX_SPEC_NOT_INDEXABLE);
	/* (Symbol class) setSpec: CLASS_SIZE */
	XP_STX_WORDAT(stx,class_SymbolMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT((XP_STX_CLASS_SIZE << 2) | XP_STX_SPEC_NOT_INDEXABLE);
	/* (Metaclass class) setSpec: CLASS_SIZE */
	XP_STX_WORDAT(stx,class_MetaclassMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT((XP_STX_CLASS_SIZE << 2) | XP_STX_SPEC_NOT_INDEXABLE);
	/* (Pairlink class) setSpec: CLASS_SIZE */
	XP_STX_WORDAT(stx,class_PairlinkMeta,XP_STX_CLASS_SPEC) = 
		XP_STX_TO_SMALLINT((XP_STX_CLASS_SIZE << 2) | XP_STX_SPEC_NOT_INDEXABLE);

	/* specs for class_metaclass, class_pairlink, 
	 * class_symbol, class_symlink are set later in 
	 * __create_builtin_classes */

	/* #Symlink */
	symbol_Symlink = xp_stx_new_symbol (stx, XP_TEXT("Symlink"));
	/* #Symbol */
	symbol_Symbol = xp_stx_new_symbol (stx, XP_TEXT("Symbol"));
	/* #Metaclass */
	symbol_Metaclass = xp_stx_new_symbol (stx, XP_TEXT("Metaclass"));
	/* #Pairlink */
	symbol_Pairlink = xp_stx_new_symbol (stx, XP_TEXT("Pairlink"));

	/* Symlink setName: #Symlink */
	XP_STX_WORDAT(stx,stx->class_symlink,XP_STX_CLASS_NAME) = symbol_Symlink;
	/* Symbol setName: #Symbol */
	XP_STX_WORDAT(stx,stx->class_symbol,XP_STX_CLASS_NAME) = symbol_Symbol;
	/* Metaclass setName: #Metaclass */
	XP_STX_WORDAT(stx,stx->class_metaclass,XP_STX_CLASS_NAME) = symbol_Metaclass;
	/* Pairlink setName: #Pairlink */
	XP_STX_WORDAT(stx,stx->class_pairlink,XP_STX_CLASS_NAME) = symbol_Pairlink;

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

static void __create_builtin_classes (xp_stx_t* stx)
{
	class_info_t* p;
	xp_word_t class, superclass, array;
	xp_stx_class_t* class_obj, * superclass_obj;
	xp_word_t metaclass;
	xp_stx_metaclass_t* metaclass_obj;
	xp_word_t n, nfields;

	xp_assert (stx->class_array != stx->nil);

	for (p = class_info; p->name != XP_NULL; p++) {
		class = xp_stx_lookup_class(stx, p->name);
		if (class == stx->nil) {
			class = xp_stx_new_class (stx, p->name);
		}

		xp_assert (class != stx->nil);
		class_obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx, class);
		class_obj->superclass = (p->superclass == XP_NULL)?
			stx->nil: xp_stx_lookup_class(stx,p->superclass);

		nfields = 0;
		if (p->superclass != XP_NULL) {
			xp_word_t meta;
			xp_stx_metaclass_t* meta_obj;

			superclass = xp_stx_lookup_class(stx,p->superclass);
			xp_assert (superclass != stx->nil);

			meta = class_obj->header.class;
			meta_obj = (xp_stx_metaclass_t*)XP_STX_WORD_OBJECT(stx,meta);
			meta_obj->superclass = XP_STX_CLASS(stx,superclass);
			meta_obj->instance_class = class;

			while (superclass != stx->nil) {
				superclass_obj = (xp_stx_class_t*)
					XP_STX_WORD_OBJECT(stx,superclass);
				nfields += XP_STX_FROM_SMALLINT(superclass_obj->spec) >> 2;
				superclass = superclass_obj->superclass;
			}

		}

		if (p->instance_variables != XP_NULL) {
			nfields += __count_names (p->instance_variables);
			class_obj->variables = 
				xp_stx_new_string (stx, p->instance_variables);
		}

		xp_assert (nfields <= 0 || (nfields > 0 && 
			(p->indexable == XP_STX_SPEC_NOT_INDEXABLE || 
			 p->indexable == XP_STX_SPEC_WORD_INDEXABLE)));
	
		class_obj->spec = 
			XP_STX_TO_SMALLINT((nfields << 2) | p->indexable);
	}

	for (p = class_info; p->name != XP_NULL; p++) {
		class = xp_stx_lookup_class(stx, p->name);
		xp_assert (class != stx->nil);

		class_obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx, class);

		/*
		if (p->class_variables != XP_NULL) {
			n = __count_names (p->class_variables);
			array = xp_stx_new_array (stx, n);
			__set_names (stx, XP_STX_DATA(stx,array), p->class_variables);
			class_obj->class_variables = array;
		}
		*/

		if (p->class_variables != XP_NULL) {
			class_obj->class_variables = 
				__make_classvar_dict(stx, class, p->class_variables);
		}

		/*
		TODO:
		if (p->pool_dictionaries != XP_NULL) {
			class_obj->pool_dictionaries =
				__make_pool_dictionary(stx, class, p->pool_dictionaries);
		}
		*/
	}

	/* fill subclasses */
	for (p = class_info; p->name != XP_NULL; p++) {
		n = __count_subclasses (p->name);
		array = xp_stx_new_array (stx, n);
		__set_subclasses (stx, XP_STX_DATA(stx,array), p->name);

		class = xp_stx_lookup_class(stx, p->name);
		xp_assert (class != stx->nil);
		class_obj = (xp_stx_class_t*)XP_STX_WORD_OBJECT(stx, class);
		class_obj->subclasses = array;
	}

	/* fill subclasses for metaclasses */
	for (p = class_info; p->name != XP_NULL; p++) {
		n = __count_subclasses (p->name);
		array = xp_stx_new_array (stx, n);
		__set_metaclass_subclasses (stx, XP_STX_DATA(stx,array), p->name);

		class = xp_stx_lookup_class(stx, p->name);
		xp_assert (class != stx->nil);
		metaclass = XP_STX_CLASS(stx,class);
		metaclass_obj = (xp_stx_metaclass_t*)XP_STX_WORD_OBJECT(stx, metaclass);
		metaclass_obj->subclasses = array;
	}
}

static xp_word_t __count_names (const xp_char_t* str)
{
	xp_word_t n = 0;
	const xp_char_t* p = str;

	do {
		while (*p == XP_CHAR(' ') ||
		       *p == XP_CHAR('\t')) p++;
		if (*p == XP_CHAR('\0')) break;

		n++;
		while (*p != XP_CHAR(' ') && 
		       *p != XP_CHAR('\t') && 
		       *p != XP_CHAR('\0')) p++;
	} while (1);

	return n;
}

static void __set_names (
	xp_stx_t* stx, xp_word_t* array, const xp_char_t* str)
{
	xp_word_t n = 0;
	const xp_char_t* p = str;
	const xp_char_t* name;

	do {
		while (*p == XP_CHAR(' ') ||
		       *p == XP_CHAR('\t')) p++;
		if (*p == XP_CHAR('\0')) break;

		name = p;
		while (*p != XP_CHAR(' ') && 
		       *p != XP_CHAR('\t') && 
		       *p != XP_CHAR('\0')) p++;

		array[n++] = xp_stx_new_symbolx (stx, name, p - name);
	} while (1);
}

static xp_word_t __count_subclasses (const xp_char_t* str)
{
	class_info_t* p;
	xp_word_t n = 0;

	for (p = class_info; p->name != XP_NULL; p++) {
		if (p->superclass == XP_NULL) continue;
		if (xp_strcmp (str, p->superclass) == 0) n++;
	}

	return n;
}

static void __set_subclasses (
	xp_stx_t* stx, xp_word_t* array, const xp_char_t* str)
{
	class_info_t* p;
	xp_word_t n = 0, class;

	for (p = class_info; p->name != XP_NULL; p++) {
		if (p->superclass == XP_NULL) continue;
		if (xp_strcmp (str, p->superclass) != 0) continue;
		class = xp_stx_lookup_class (stx, p->name);
		xp_assert (class != stx->nil);
		array[n++] = class;
	}
}

static void __set_metaclass_subclasses (
	xp_stx_t* stx, xp_word_t* array, const xp_char_t* str)
{
	class_info_t* p;
	xp_word_t n = 0, class;

	for (p = class_info; p->name != XP_NULL; p++) {
		if (p->superclass == XP_NULL) continue;
		if (xp_strcmp (str, p->superclass) != 0) continue;
		class = xp_stx_lookup_class (stx, p->name);
		xp_assert (class != stx->nil);
		array[n++] = XP_STX_CLASS(stx,class);
	}
}

static xp_word_t __make_classvar_dict (
	xp_stx_t* stx, xp_word_t class, const xp_char_t* names)
{
	xp_size_t n;
	xp_word_t dict, symbol;
	const xp_char_t* p = names;
	const xp_char_t* name;

	n = __count_names (names);
	dict = xp_stx_alloc_word_object (stx, n);
	XP_STX_CLASS(stx,dict) =  /* TODO */
		xp_stx_lookup_class (stx, XP_TEXT("Dictionary"));

	do {
		while (*p == XP_CHAR(' ') ||
		       *p == XP_CHAR('\t')) p++;
		if (*p == XP_CHAR('\0')) break;

		name = p;
		while (*p != XP_CHAR(' ') && 
		       *p != XP_CHAR('\t') && 
		       *p != XP_CHAR('\0')) p++;

		symbol = xp_stx_new_symbolx (stx, name, p - name);
		
		xp_stx_hash_insert (stx, dict,
			xp_stx_hash_char_object(stx, symbol), symbol, stx->nil);
	} while (1);

	return dict;
}

static void __filein_kernel (xp_stx_t* stx)
{
	class_info_t* p;

	for (p = class_info; p->name != XP_NULL; p++) {
		/* TODO: */
	}
}

