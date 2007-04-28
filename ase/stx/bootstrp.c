/*
 * $Id: bootstrp.c,v 1.1 2007/03/28 14:05:25 bacon Exp $
 */

#include <ase/stx/bootstrp.h>
#include <ase/stx/symbol.h>
#include <ase/stx/class.h>
#include <ase/stx/object.h>
#include <ase/stx/dict.h>
#include <ase/stx/misc.h>

static void __create_bootstrapping_objects (ase_stx_t* stx);
static void __create_builtin_classes (ase_stx_t* stx);
static ase_word_t __make_classvar_dict (
	ase_stx_t* stx, ase_word_t class, const ase_char_t* names);
static void __filein_kernel (ase_stx_t* stx);

static ase_word_t __count_names (const ase_char_t* str);
static void __set_names (
	ase_stx_t* stx, ase_word_t* array, const ase_char_t* str);

static ase_word_t __count_subclasses (const ase_char_t* str);
static void __set_subclasses (
	ase_stx_t* stx, ase_word_t* array, const ase_char_t* str);
static void __set_metaclass_subclasses (
	ase_stx_t* stx, ase_word_t* array, const ase_char_t* str);

struct class_info_t 
{
	const ase_char_t* name;
	const ase_char_t* superclass;
	const ase_char_t* instance_variables;
	const ase_char_t* class_variables;
	const ase_char_t* pool_dictionaries;
	const int indexable;
};

typedef struct class_info_t class_info_t;

static class_info_t class_info[] =
{
	{
		ASE_T("Object"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("UndefinedObject"),
		ASE_T("Object"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		ASE_T("Behavior"),
		ASE_T("Object"),
		ASE_T("spec methods superclass"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		ASE_T("Class"),
		ASE_T("Behavior"),
		ASE_T("name variables classVariables poolDictionaries"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		ASE_T("Metaclass"),
		ASE_T("Behavior"),
		ASE_T("instanceClass"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Block"),
		ASE_T("Object"),
		ASE_T("context argCount argLoc bytePointer"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Boolean"),
		ASE_T("Object"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("True"),
		ASE_T("Boolean"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("False"),
		ASE_T("Boolean"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Context"),
		ASE_T("Object"),
		ASE_T("stack stackTop receiver pc method"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Method"),
		ASE_T("Object"),
		ASE_T("text selector bytecodes tmpCount argCount"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_WORD_INDEXABLE
	},
	{
		ASE_T("Magnitude"),
		ASE_T("Object"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Association"),
		ASE_T("Magnitude"),
		ASE_T("key value"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Character"),
		ASE_T("Magnitude"),
		ASE_T("value"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Number"),
		ASE_T("Magnitude"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Integer"),
		ASE_T("Number"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("SmallInteger"),
		ASE_T("Integer"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("LargeInteger"),
		ASE_T("Integer"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_BYTE_INDEXABLE
	},
	{
		ASE_T("Collection"),
		ASE_T("Magnitude"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("IndexedCollection"),
		ASE_T("Collection"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_T("Array"),
		ASE_T("IndexedCollection"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_WORD_INDEXABLE
	},
	{
		ASE_T("ByteArray"),
		ASE_T("IndexedCollection"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_BYTE_INDEXABLE
	},
	{
		ASE_T("Dictionary"),
		ASE_T("IndexedCollection"),
		ASE_T("tally"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_WORD_INDEXABLE	
	},
	{
		ASE_T("SystemDictionary"),
		ASE_T("Dictionary"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_WORD_INDEXABLE	
	},
	{
		ASE_T("PoolDictionary"),
		ASE_T("Dictionary"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_WORD_INDEXABLE	
	},
	{
		ASE_T("String"),
		ASE_T("IndexedCollection"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_CHAR_INDEXABLE
	},
	{
		ASE_T("Symbol"),
		ASE_T("String"),
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_CHAR_INDEXABLE
	},
	{
		ASE_T("Link"),
		ASE_T("Object"),
		ASE_T("link"),
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	},
	{
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_NULL,
		ASE_STX_SPEC_NOT_INDEXABLE
	}
};

ase_word_t INLINE __new_string (ase_stx_t* stx, const ase_char_t* str)
{
	ase_word_t x;

	ase_assert (stx->class_string != stx->nil);
	x = ase_stx_alloc_char_object (stx, str);
	ASE_STX_CLASS(stx,x) = stx->class_string;

	return x;	
}

int ase_stx_bootstrap (ase_stx_t* stx)
{
	ase_word_t symbol_Smalltalk;
	ase_word_t object_meta;

	__create_bootstrapping_objects (stx);

	/* object, class, and array are precreated for easier instantiation
	 * of builtin classes */
	stx->class_object = ase_stx_new_class (stx, ASE_T("Object"));
	stx->class_class = ase_stx_new_class (stx, ASE_T("Class"));
	stx->class_array = ase_stx_new_class (stx, ASE_T("Array"));
	stx->class_bytearray = ase_stx_new_class (stx, ASE_T("ByteArray"));
	stx->class_string = ase_stx_new_class (stx, ASE_T("String"));
	stx->class_character = ase_stx_new_class (stx, ASE_T("Character"));
	stx->class_context = ase_stx_new_class (stx, ASE_T("Context"));
	stx->class_system_dictionary = 
		ase_stx_new_class (stx, ASE_T("SystemDictionary"));
	stx->class_method = 
		ase_stx_new_class (stx, ASE_T("Method"));
	stx->class_smallinteger = 
		ase_stx_new_class (stx, ASE_T("SmallInteger"));

	__create_builtin_classes (stx);

	/* (Object class) setSuperclass: Class */
	object_meta = ASE_STX_CLASS(stx,stx->class_object);
	ASE_STX_WORD_AT(stx,object_meta,ASE_STX_METACLASS_SUPERCLASS) = stx->class_class;
	/* instance class for Object is set here as it is not 
	 * set in __create_builtin_classes */
	ASE_STX_WORD_AT(stx,object_meta,ASE_STX_METACLASS_INSTANCE_CLASS) = stx->class_object;

	/* for some fun here */
	{
		ase_word_t array;
		array = ase_stx_new_array (stx, 1);
		ASE_STX_WORD_AT(stx,array,0) = object_meta;
		ASE_STX_WORD_AT(stx,stx->class_class,ASE_STX_CLASS_SUBCLASSES) = array;
	}
			
	/* more initialization */
	ASE_STX_CLASS(stx,stx->smalltalk) = stx->class_system_dictionary;

	symbol_Smalltalk = ase_stx_new_symbol (stx, ASE_T("Smalltalk"));
	ase_stx_dict_put (stx, stx->smalltalk, symbol_Smalltalk, stx->smalltalk);

	/* create #nil, #true, #false */
	ase_stx_new_symbol (stx, ASE_T("nil"));
	ase_stx_new_symbol (stx, ASE_T("true"));
	ase_stx_new_symbol (stx, ASE_T("false"));

	/* nil setClass: UndefinedObject */
	ASE_STX_CLASS(stx,stx->nil) =
		ase_stx_lookup_class(stx, ASE_T("UndefinedObject"));
	/* true setClass: True */
	ASE_STX_CLASS(stx,stx->true) =
		ase_stx_lookup_class (stx, ASE_T("True"));
	/* fales setClass: False */
	ASE_STX_CLASS(stx,stx->false) = 
		ase_stx_lookup_class (stx, ASE_T("False"));

	__filein_kernel (stx);
	return 0;
}

static void __create_bootstrapping_objects (ase_stx_t* stx)
{
	ase_word_t class_SymbolMeta; 
	ase_word_t class_MetaclassMeta;
	ase_word_t class_AssociationMeta;
	ase_word_t symbol_Symbol; 
	ase_word_t symbol_Metaclass;
	ase_word_t symbol_Association;

	/* allocate three keyword objects */
	stx->nil = ase_stx_alloc_word_object (stx, ASE_NULL, 0, ASE_NULL, 0);
	stx->true = ase_stx_alloc_word_object (stx, ASE_NULL, 0, ASE_NULL, 0);
	stx->false = ase_stx_alloc_word_object (stx, ASE_NULL, 0, ASE_NULL, 0);

	ase_assert (stx->nil == ASE_STX_NIL);
	ase_assert (stx->true == ASE_STX_TRUE);
	ase_assert (stx->false == ASE_STX_FALSE);

	/* system dictionary */
	/* TODO: dictionary size */
	stx->smalltalk = ase_stx_alloc_word_object (
		stx, ASE_NULL, 1, ASE_NULL, 256);
	/* set tally */
	ASE_STX_WORD_AT(stx,stx->smalltalk,0) = ASE_STX_TO_SMALLINT(0);

	/* Symbol */
	stx->class_symbol = ase_stx_alloc_word_object(
		stx, ASE_NULL, ASE_STX_CLASS_SIZE, ASE_NULL, 0);
	/* Metaclass */
	stx->class_metaclass = ase_stx_alloc_word_object(
		stx, ASE_NULL, ASE_STX_CLASS_SIZE, ASE_NULL, 0);
	/* Association */
	stx->class_association = ase_stx_alloc_word_object(
		stx, ASE_NULL, ASE_STX_CLASS_SIZE, ASE_NULL, 0);

	/* Metaclass is a class so it has the same structure 
	 * as a normal class. "Metaclass class" is an instance of
	 * Metaclass. */

	/* Symbol class */
	class_SymbolMeta = ase_stx_alloc_word_object(
		stx, ASE_NULL, ASE_STX_METACLASS_SIZE, ASE_NULL, 0);
	/* Metaclass class */
	class_MetaclassMeta = ase_stx_alloc_word_object(
		stx, ASE_NULL, ASE_STX_METACLASS_SIZE, ASE_NULL, 0);
	/* Association class */
	class_AssociationMeta = ase_stx_alloc_word_object(
		stx, ASE_NULL, ASE_STX_METACLASS_SIZE, ASE_NULL, 0);

	/* (Symbol class) setClass: Metaclass */
	ASE_STX_CLASS(stx,class_SymbolMeta) = stx->class_metaclass;
	/* (Metaclass class) setClass: Metaclass */
	ASE_STX_CLASS(stx,class_MetaclassMeta) = stx->class_metaclass;
	/* (Association class) setClass: Metaclass */
	ASE_STX_CLASS(stx,class_AssociationMeta) = stx->class_metaclass;

	/* Symbol setClass: (Symbol class) */
	ASE_STX_CLASS(stx,stx->class_symbol) = class_SymbolMeta;
	/* Metaclass setClass: (Metaclass class) */
	ASE_STX_CLASS(stx,stx->class_metaclass) = class_MetaclassMeta;
	/* Association setClass: (Association class) */
	ASE_STX_CLASS(stx,stx->class_association) = class_AssociationMeta;

	/* (Symbol class) setSpec: CLASS_SIZE */
	ASE_STX_WORD_AT(stx,class_SymbolMeta,ASE_STX_CLASS_SPEC) = 
		ASE_STX_TO_SMALLINT((ASE_STX_CLASS_SIZE << ASE_STX_SPEC_INDEXABLE_BITS) | ASE_STX_SPEC_NOT_INDEXABLE);
	/* (Metaclass class) setSpec: CLASS_SIZE */
	ASE_STX_WORD_AT(stx,class_MetaclassMeta,ASE_STX_CLASS_SPEC) = 
		ASE_STX_TO_SMALLINT((ASE_STX_CLASS_SIZE << ASE_STX_SPEC_INDEXABLE_BITS) | ASE_STX_SPEC_NOT_INDEXABLE);
	/* (Association class) setSpec: CLASS_SIZE */
	ASE_STX_WORD_AT(stx,class_AssociationMeta,ASE_STX_CLASS_SPEC) = 
		ASE_STX_TO_SMALLINT((ASE_STX_CLASS_SIZE << ASE_STX_SPEC_INDEXABLE_BITS) | ASE_STX_SPEC_NOT_INDEXABLE);

	/* specs for class_metaclass, class_association, 
	 * class_symbol are set later in __create_builtin_classes */

	/* #Symbol */
	symbol_Symbol = ase_stx_new_symbol (stx, ASE_T("Symbol"));
	/* #Metaclass */
	symbol_Metaclass = ase_stx_new_symbol (stx, ASE_T("Metaclass"));
	/* #Association */
	symbol_Association = ase_stx_new_symbol (stx, ASE_T("Association"));

	/* Symbol setName: #Symbol */
	ASE_STX_WORD_AT(stx,stx->class_symbol,ASE_STX_CLASS_NAME) = symbol_Symbol;
	/* Metaclass setName: #Metaclass */
	ASE_STX_WORD_AT(stx,stx->class_metaclass,ASE_STX_CLASS_NAME) = symbol_Metaclass;
	/* Association setName: #Association */
	ASE_STX_WORD_AT(stx,stx->class_association,ASE_STX_CLASS_NAME) = symbol_Association;

	/* register class names into the system dictionary */
	ase_stx_dict_put (stx,
		stx->smalltalk, symbol_Symbol, stx->class_symbol);
	ase_stx_dict_put (stx,
		stx->smalltalk, symbol_Metaclass, stx->class_metaclass);
	ase_stx_dict_put (stx,
		stx->smalltalk, symbol_Association, stx->class_association);
}

static void __create_builtin_classes (ase_stx_t* stx)
{
	class_info_t* p;
	ase_word_t class, superclass, array;
	ase_stx_class_t* class_obj, * superclass_obj;
	ase_word_t metaclass;
	ase_stx_metaclass_t* metaclass_obj;
	ase_word_t n, nfields;

	ase_assert (stx->class_array != stx->nil);

	for (p = class_info; p->name != ASE_NULL; p++) {
		class = ase_stx_lookup_class(stx, p->name);
		if (class == stx->nil) {
			class = ase_stx_new_class (stx, p->name);
		}

		ase_assert (class != stx->nil);
		class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class);
		class_obj->superclass = (p->superclass == ASE_NULL)?
			stx->nil: ase_stx_lookup_class(stx,p->superclass);

		nfields = 0;
		if (p->superclass != ASE_NULL) {
			ase_word_t meta;
			ase_stx_metaclass_t* meta_obj;

			superclass = ase_stx_lookup_class(stx,p->superclass);
			ase_assert (superclass != stx->nil);

			meta = class_obj->header.class;
			meta_obj = (ase_stx_metaclass_t*)ASE_STX_OBJECT(stx,meta);
			meta_obj->superclass = ASE_STX_CLASS(stx,superclass);
			meta_obj->instance_class = class;

			while (superclass != stx->nil) {
				superclass_obj = (ase_stx_class_t*)
					ASE_STX_OBJECT(stx,superclass);
				nfields += 
					ASE_STX_FROM_SMALLINT(superclass_obj->spec) >>
					ASE_STX_SPEC_INDEXABLE_BITS;
				superclass = superclass_obj->superclass;
			}

		}

		if (p->instance_variables != ASE_NULL) {
			nfields += __count_names (p->instance_variables);
			class_obj->variables = 
				__new_string (stx, p->instance_variables);
		}

		ase_assert (nfields <= 0 || (nfields > 0 && 
			(p->indexable == ASE_STX_SPEC_NOT_INDEXABLE || 
			 p->indexable == ASE_STX_SPEC_WORD_INDEXABLE)));
	
		class_obj->spec = ASE_STX_TO_SMALLINT(
			(nfields << ASE_STX_SPEC_INDEXABLE_BITS) | p->indexable);
	}

	for (p = class_info; p->name != ASE_NULL; p++) {
		class = ase_stx_lookup_class(stx, p->name);
		ase_assert (class != stx->nil);

		class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class);

		if (p->class_variables != ASE_NULL) {
			class_obj->class_variables = 
				__make_classvar_dict(stx, class, p->class_variables);
		}

		/*
		TODO:
		if (p->pool_dictionaries != ASE_NULL) {
			class_obj->pool_dictionaries =
				__make_pool_dictionary(stx, class, p->pool_dictionaries);
		}
		*/
	}

	/* fill subclasses */
	for (p = class_info; p->name != ASE_NULL; p++) {
		n = __count_subclasses (p->name);
		array = ase_stx_new_array (stx, n);
		__set_subclasses (stx, ASE_STX_DATA(stx,array), p->name);

		class = ase_stx_lookup_class(stx, p->name);
		ase_assert (class != stx->nil);
		class_obj = (ase_stx_class_t*)ASE_STX_OBJECT(stx, class);
		class_obj->subclasses = array;
	}

	/* fill subclasses for metaclasses */
	for (p = class_info; p->name != ASE_NULL; p++) {
		n = __count_subclasses (p->name);
		array = ase_stx_new_array (stx, n);
		__set_metaclass_subclasses (stx, ASE_STX_DATA(stx,array), p->name);

		class = ase_stx_lookup_class(stx, p->name);
		ase_assert (class != stx->nil);
		metaclass = ASE_STX_CLASS(stx,class);
		metaclass_obj = (ase_stx_metaclass_t*)ASE_STX_OBJECT(stx, metaclass);
		metaclass_obj->subclasses = array;
	}
}

static ase_word_t __count_names (const ase_char_t* str)
{
	ase_word_t n = 0;
	const ase_char_t* p = str;

	do {
		while (*p == ASE_T(' ') ||
		       *p == ASE_T('\t')) p++;
		if (*p == ASE_T('\0')) break;

		n++;
		while (*p != ASE_T(' ') && 
		       *p != ASE_T('\t') && 
		       *p != ASE_T('\0')) p++;
	} while (1);

	return n;
}

static void __set_names (
	ase_stx_t* stx, ase_word_t* array, const ase_char_t* str)
{
	ase_word_t n = 0;
	const ase_char_t* p = str;
	const ase_char_t* name;

	do {
		while (*p == ASE_T(' ') ||
		       *p == ASE_T('\t')) p++;
		if (*p == ASE_T('\0')) break;

		name = p;
		while (*p != ASE_T(' ') && 
		       *p != ASE_T('\t') && 
		       *p != ASE_T('\0')) p++;

		array[n++] = ase_stx_new_symbolx (stx, name, p - name);
	} while (1);
}

static ase_word_t __count_subclasses (const ase_char_t* str)
{
	class_info_t* p;
	ase_word_t n = 0;

	for (p = class_info; p->name != ASE_NULL; p++) {
		if (p->superclass == ASE_NULL) continue;
		if (ase_strcmp (str, p->superclass) == 0) n++;
	}

	return n;
}

static void __set_subclasses (
	ase_stx_t* stx, ase_word_t* array, const ase_char_t* str)
{
	class_info_t* p;
	ase_word_t n = 0, class;

	for (p = class_info; p->name != ASE_NULL; p++) {
		if (p->superclass == ASE_NULL) continue;
		if (ase_strcmp (str, p->superclass) != 0) continue;
		class = ase_stx_lookup_class (stx, p->name);
		ase_assert (class != stx->nil);
		array[n++] = class;
	}
}

static void __set_metaclass_subclasses (
	ase_stx_t* stx, ase_word_t* array, const ase_char_t* str)
{
	class_info_t* p;
	ase_word_t n = 0, class;

	for (p = class_info; p->name != ASE_NULL; p++) {
		if (p->superclass == ASE_NULL) continue;
		if (ase_strcmp (str, p->superclass) != 0) continue;
		class = ase_stx_lookup_class (stx, p->name);
		ase_assert (class != stx->nil);
		array[n++] = ASE_STX_CLASS(stx,class);
	}
}

static ase_word_t __make_classvar_dict (
	ase_stx_t* stx, ase_word_t class, const ase_char_t* names)
{
	ase_word_t dict, symbol;
	const ase_char_t* p = names;
	const ase_char_t* name;

	dict = ase_stx_instantiate (
		stx, stx->class_system_dictionary,
		ASE_NULL, ASE_NULL, __count_names(names));

	do {
		while (*p == ASE_T(' ') ||
		       *p == ASE_T('\t')) p++;
		if (*p == ASE_T('\0')) break;

		name = p;
		while (*p != ASE_T(' ') && 
		       *p != ASE_T('\t') && 
		       *p != ASE_T('\0')) p++;

		symbol = ase_stx_new_symbolx (stx, name, p - name);
		ase_stx_dict_put (stx, dict, symbol, stx->nil);
	} while (1);

	return dict;
}

static void __filein_kernel (ase_stx_t* stx)
{
	class_info_t* p;

	for (p = class_info; p->name != ASE_NULL; p++) {
		/* TODO: */
	}
}

