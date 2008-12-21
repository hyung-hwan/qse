/*
 * $Id: bootstrp.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/bootstrp.h>
#include <qse/stx/symbol.h>
#include <qse/stx/class.h>
#include <qse/stx/object.h>
#include <qse/stx/dict.h>
#include <qse/stx/misc.h>

static void __create_bootstrapping_objects (qse_stx_t* stx);
static void __create_builtin_classes (qse_stx_t* stx);
static qse_word_t __make_classvar_dict (
	qse_stx_t* stx, qse_word_t class, const qse_char_t* names);
static void __filein_kernel (qse_stx_t* stx);

static qse_word_t __count_names (const qse_char_t* str);
static void __set_names (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str);

static qse_word_t __count_subclasses (const qse_char_t* str);
static void __set_subclasses (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str);
static void __set_metaclass_subclasses (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str);

struct class_info_t 
{
	const qse_char_t* name;
	const qse_char_t* superclass;
	const qse_char_t* instance_variables;
	const qse_char_t* class_variables;
	const qse_char_t* pool_dictionaries;
	const int indexable;
};

typedef struct class_info_t class_info_t;

static class_info_t class_info[] =
{
	{
		QSE_T("Object"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("UndefinedObject"),
		QSE_T("Object"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		QSE_T("Behavior"),
		QSE_T("Object"),
		QSE_T("spec methods superclass"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		QSE_T("Class"),
		QSE_T("Behavior"),
		QSE_T("name variables classVariables poolDictionaries"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{ 
		QSE_T("Metaclass"),
		QSE_T("Behavior"),
		QSE_T("instanceClass"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Block"),
		QSE_T("Object"),
		QSE_T("context argCount argLoc bytePointer"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Boolean"),
		QSE_T("Object"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("True"),
		QSE_T("Boolean"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("False"),
		QSE_T("Boolean"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Context"),
		QSE_T("Object"),
		QSE_T("stack stackTop receiver pc method"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Method"),
		QSE_T("Object"),
		QSE_T("text selector bytecodes tmpCount argCount"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_WORD_INDEXABLE
	},
	{
		QSE_T("Magnitude"),
		QSE_T("Object"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Association"),
		QSE_T("Magnitude"),
		QSE_T("key value"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Character"),
		QSE_T("Magnitude"),
		QSE_T("value"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Number"),
		QSE_T("Magnitude"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Integer"),
		QSE_T("Number"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("SmallInteger"),
		QSE_T("Integer"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("LargeInteger"),
		QSE_T("Integer"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_BYTE_INDEXABLE
	},
	{
		QSE_T("Collection"),
		QSE_T("Magnitude"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("IndexedCollection"),
		QSE_T("Collection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_T("Array"),
		QSE_T("IndexedCollection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_WORD_INDEXABLE
	},
	{
		QSE_T("ByteArray"),
		QSE_T("IndexedCollection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_BYTE_INDEXABLE
	},
	{
		QSE_T("Dictionary"),
		QSE_T("IndexedCollection"),
		QSE_T("tally"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_WORD_INDEXABLE	
	},
	{
		QSE_T("SystemDictionary"),
		QSE_T("Dictionary"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_WORD_INDEXABLE	
	},
	{
		QSE_T("PoolDictionary"),
		QSE_T("Dictionary"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_WORD_INDEXABLE	
	},
	{
		QSE_T("String"),
		QSE_T("IndexedCollection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_CHAR_INDEXABLE
	},
	{
		QSE_T("Symbol"),
		QSE_T("String"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_CHAR_INDEXABLE
	},
	{
		QSE_T("Link"),
		QSE_T("Object"),
		QSE_T("link"),
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	},
	{
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_STX_SPEC_NOT_INDEXABLE
	}
};

qse_word_t INLINE __new_string (qse_stx_t* stx, const qse_char_t* str)
{
	qse_word_t x;

	qse_assert (stx->class_string != stx->nil);
	x = qse_stx_alloc_char_object (stx, str);
	QSE_STX_CLASS(stx,x) = stx->class_string;

	return x;	
}

int qse_stx_bootstrap (qse_stx_t* stx)
{
	qse_word_t symbol_Smalltalk;
	qse_word_t object_meta;

	__create_bootstrapping_objects (stx);

	/* object, class, and array are precreated for easier instantiation
	 * of builtin classes */
	stx->class_object = qse_stx_new_class (stx, QSE_T("Object"));
	stx->class_class = qse_stx_new_class (stx, QSE_T("Class"));
	stx->class_array = qse_stx_new_class (stx, QSE_T("Array"));
	stx->class_bytearray = qse_stx_new_class (stx, QSE_T("ByteArray"));
	stx->class_string = qse_stx_new_class (stx, QSE_T("String"));
	stx->class_character = qse_stx_new_class (stx, QSE_T("Character"));
	stx->class_context = qse_stx_new_class (stx, QSE_T("Context"));
	stx->class_system_dictionary = 
		qse_stx_new_class (stx, QSE_T("SystemDictionary"));
	stx->class_method = 
		qse_stx_new_class (stx, QSE_T("Method"));
	stx->class_smallinteger = 
		qse_stx_new_class (stx, QSE_T("SmallInteger"));

	__create_builtin_classes (stx);

	/* (Object class) setSuperclass: Class */
	object_meta = QSE_STX_CLASS(stx,stx->class_object);
	QSE_STX_WORD_AT(stx,object_meta,QSE_STX_METACLASS_SUPERCLASS) = stx->class_class;
	/* instance class for Object is set here as it is not 
	 * set in __create_builtin_classes */
	QSE_STX_WORD_AT(stx,object_meta,QSE_STX_METACLASS_INSTANCE_CLASS) = stx->class_object;

	/* for some fun here */
	{
		qse_word_t array;
		array = qse_stx_new_array (stx, 1);
		QSE_STX_WORD_AT(stx,array,0) = object_meta;
		QSE_STX_WORD_AT(stx,stx->class_class,QSE_STX_CLASS_SUBCLASSES) = array;
	}
			
	/* more initialization */
	QSE_STX_CLASS(stx,stx->smalltalk) = stx->class_system_dictionary;

	symbol_Smalltalk = qse_stx_new_symbol (stx, QSE_T("Smalltalk"));
	qse_stx_dict_put (stx, stx->smalltalk, symbol_Smalltalk, stx->smalltalk);

	/* create #nil, #true, #false */
	qse_stx_new_symbol (stx, QSE_T("nil"));
	qse_stx_new_symbol (stx, QSE_T("true"));
	qse_stx_new_symbol (stx, QSE_T("false"));

	/* nil setClass: UndefinedObject */
	QSE_STX_CLASS(stx,stx->nil) =
		qse_stx_lookup_class(stx, QSE_T("UndefinedObject"));
	/* true setClass: True */
	QSE_STX_CLASS(stx,stx->true) =
		qse_stx_lookup_class (stx, QSE_T("True"));
	/* fales setClass: False */
	QSE_STX_CLASS(stx,stx->false) = 
		qse_stx_lookup_class (stx, QSE_T("False"));

	__filein_kernel (stx);
	return 0;
}

static void __create_bootstrapping_objects (qse_stx_t* stx)
{
	qse_word_t class_SymbolMeta; 
	qse_word_t class_MetaclassMeta;
	qse_word_t class_AssociationMeta;
	qse_word_t symbol_Symbol; 
	qse_word_t symbol_Metaclass;
	qse_word_t symbol_Association;

	/* allocate three keyword objects */
	stx->nil = qse_stx_alloc_word_object (stx, QSE_NULL, 0, QSE_NULL, 0);
	stx->true = qse_stx_alloc_word_object (stx, QSE_NULL, 0, QSE_NULL, 0);
	stx->false = qse_stx_alloc_word_object (stx, QSE_NULL, 0, QSE_NULL, 0);

	qse_assert (stx->nil == QSE_STX_NIL);
	qse_assert (stx->true == QSE_STX_TRUE);
	qse_assert (stx->false == QSE_STX_FALSE);

	/* system dictionary */
	/* TODO: dictionary size */
	stx->smalltalk = qse_stx_alloc_word_object (
		stx, QSE_NULL, 1, QSE_NULL, 256);
	/* set tally */
	QSE_STX_WORD_AT(stx,stx->smalltalk,0) = QSE_STX_TO_SMALLINT(0);

	/* Symbol */
	stx->class_symbol = qse_stx_alloc_word_object(
		stx, QSE_NULL, QSE_STX_CLASS_SIZE, QSE_NULL, 0);
	/* Metaclass */
	stx->class_metaclass = qse_stx_alloc_word_object(
		stx, QSE_NULL, QSE_STX_CLASS_SIZE, QSE_NULL, 0);
	/* Association */
	stx->class_association = qse_stx_alloc_word_object(
		stx, QSE_NULL, QSE_STX_CLASS_SIZE, QSE_NULL, 0);

	/* Metaclass is a class so it has the same structure 
	 * as a normal class. "Metaclass class" is an instance of
	 * Metaclass. */

	/* Symbol class */
	class_SymbolMeta = qse_stx_alloc_word_object(
		stx, QSE_NULL, QSE_STX_METACLASS_SIZE, QSE_NULL, 0);
	/* Metaclass class */
	class_MetaclassMeta = qse_stx_alloc_word_object(
		stx, QSE_NULL, QSE_STX_METACLASS_SIZE, QSE_NULL, 0);
	/* Association class */
	class_AssociationMeta = qse_stx_alloc_word_object(
		stx, QSE_NULL, QSE_STX_METACLASS_SIZE, QSE_NULL, 0);

	/* (Symbol class) setClass: Metaclass */
	QSE_STX_CLASS(stx,class_SymbolMeta) = stx->class_metaclass;
	/* (Metaclass class) setClass: Metaclass */
	QSE_STX_CLASS(stx,class_MetaclassMeta) = stx->class_metaclass;
	/* (Association class) setClass: Metaclass */
	QSE_STX_CLASS(stx,class_AssociationMeta) = stx->class_metaclass;

	/* Symbol setClass: (Symbol class) */
	QSE_STX_CLASS(stx,stx->class_symbol) = class_SymbolMeta;
	/* Metaclass setClass: (Metaclass class) */
	QSE_STX_CLASS(stx,stx->class_metaclass) = class_MetaclassMeta;
	/* Association setClass: (Association class) */
	QSE_STX_CLASS(stx,stx->class_association) = class_AssociationMeta;

	/* (Symbol class) setSpec: CLASS_SIZE */
	QSE_STX_WORD_AT(stx,class_SymbolMeta,QSE_STX_CLASS_SPEC) = 
		QSE_STX_TO_SMALLINT((QSE_STX_CLASS_SIZE << QSE_STX_SPEC_INDEXABLE_BITS) | QSE_STX_SPEC_NOT_INDEXABLE);
	/* (Metaclass class) setSpec: CLASS_SIZE */
	QSE_STX_WORD_AT(stx,class_MetaclassMeta,QSE_STX_CLASS_SPEC) = 
		QSE_STX_TO_SMALLINT((QSE_STX_CLASS_SIZE << QSE_STX_SPEC_INDEXABLE_BITS) | QSE_STX_SPEC_NOT_INDEXABLE);
	/* (Association class) setSpec: CLASS_SIZE */
	QSE_STX_WORD_AT(stx,class_AssociationMeta,QSE_STX_CLASS_SPEC) = 
		QSE_STX_TO_SMALLINT((QSE_STX_CLASS_SIZE << QSE_STX_SPEC_INDEXABLE_BITS) | QSE_STX_SPEC_NOT_INDEXABLE);

	/* specs for class_metaclass, class_association, 
	 * class_symbol are set later in __create_builtin_classes */

	/* #Symbol */
	symbol_Symbol = qse_stx_new_symbol (stx, QSE_T("Symbol"));
	/* #Metaclass */
	symbol_Metaclass = qse_stx_new_symbol (stx, QSE_T("Metaclass"));
	/* #Association */
	symbol_Association = qse_stx_new_symbol (stx, QSE_T("Association"));

	/* Symbol setName: #Symbol */
	QSE_STX_WORD_AT(stx,stx->class_symbol,QSE_STX_CLASS_NAME) = symbol_Symbol;
	/* Metaclass setName: #Metaclass */
	QSE_STX_WORD_AT(stx,stx->class_metaclass,QSE_STX_CLASS_NAME) = symbol_Metaclass;
	/* Association setName: #Association */
	QSE_STX_WORD_AT(stx,stx->class_association,QSE_STX_CLASS_NAME) = symbol_Association;

	/* register class names into the system dictionary */
	qse_stx_dict_put (stx,
		stx->smalltalk, symbol_Symbol, stx->class_symbol);
	qse_stx_dict_put (stx,
		stx->smalltalk, symbol_Metaclass, stx->class_metaclass);
	qse_stx_dict_put (stx,
		stx->smalltalk, symbol_Association, stx->class_association);
}

static void __create_builtin_classes (qse_stx_t* stx)
{
	class_info_t* p;
	qse_word_t class, superclass, array;
	qse_stx_class_t* class_obj, * superclass_obj;
	qse_word_t metaclass;
	qse_stx_metaclass_t* metaclass_obj;
	qse_word_t n, nfields;

	qse_assert (stx->class_array != stx->nil);

	for (p = class_info; p->name != QSE_NULL; p++) {
		class = qse_stx_lookup_class(stx, p->name);
		if (class == stx->nil) {
			class = qse_stx_new_class (stx, p->name);
		}

		qse_assert (class != stx->nil);
		class_obj = (qse_stx_class_t*)QSE_STX_OBJECT(stx, class);
		class_obj->superclass = (p->superclass == QSE_NULL)?
			stx->nil: qse_stx_lookup_class(stx,p->superclass);

		nfields = 0;
		if (p->superclass != QSE_NULL) {
			qse_word_t meta;
			qse_stx_metaclass_t* meta_obj;

			superclass = qse_stx_lookup_class(stx,p->superclass);
			qse_assert (superclass != stx->nil);

			meta = class_obj->header.class;
			meta_obj = (qse_stx_metaclass_t*)QSE_STX_OBJECT(stx,meta);
			meta_obj->superclass = QSE_STX_CLASS(stx,superclass);
			meta_obj->instance_class = class;

			while (superclass != stx->nil) {
				superclass_obj = (qse_stx_class_t*)
					QSE_STX_OBJECT(stx,superclass);
				nfields += 
					QSE_STX_FROM_SMALLINT(superclass_obj->spec) >>
					QSE_STX_SPEC_INDEXABLE_BITS;
				superclass = superclass_obj->superclass;
			}

		}

		if (p->instance_variables != QSE_NULL) {
			nfields += __count_names (p->instance_variables);
			class_obj->variables = 
				__new_string (stx, p->instance_variables);
		}

		qse_assert (nfields <= 0 || (nfields > 0 && 
			(p->indexable == QSE_STX_SPEC_NOT_INDEXABLE || 
			 p->indexable == QSE_STX_SPEC_WORD_INDEXABLE)));
	
		class_obj->spec = QSE_STX_TO_SMALLINT(
			(nfields << QSE_STX_SPEC_INDEXABLE_BITS) | p->indexable);
	}

	for (p = class_info; p->name != QSE_NULL; p++) {
		class = qse_stx_lookup_class(stx, p->name);
		qse_assert (class != stx->nil);

		class_obj = (qse_stx_class_t*)QSE_STX_OBJECT(stx, class);

		if (p->class_variables != QSE_NULL) {
			class_obj->class_variables = 
				__make_classvar_dict(stx, class, p->class_variables);
		}

		/*
		TODO:
		if (p->pool_dictionaries != QSE_NULL) {
			class_obj->pool_dictionaries =
				__make_pool_dictionary(stx, class, p->pool_dictionaries);
		}
		*/
	}

	/* fill subclasses */
	for (p = class_info; p->name != QSE_NULL; p++) {
		n = __count_subclasses (p->name);
		array = qse_stx_new_array (stx, n);
		__set_subclasses (stx, QSE_STX_DATA(stx,array), p->name);

		class = qse_stx_lookup_class(stx, p->name);
		qse_assert (class != stx->nil);
		class_obj = (qse_stx_class_t*)QSE_STX_OBJECT(stx, class);
		class_obj->subclasses = array;
	}

	/* fill subclasses for metaclasses */
	for (p = class_info; p->name != QSE_NULL; p++) {
		n = __count_subclasses (p->name);
		array = qse_stx_new_array (stx, n);
		__set_metaclass_subclasses (stx, QSE_STX_DATA(stx,array), p->name);

		class = qse_stx_lookup_class(stx, p->name);
		qse_assert (class != stx->nil);
		metaclass = QSE_STX_CLASS(stx,class);
		metaclass_obj = (qse_stx_metaclass_t*)QSE_STX_OBJECT(stx, metaclass);
		metaclass_obj->subclasses = array;
	}
}

static qse_word_t __count_names (const qse_char_t* str)
{
	qse_word_t n = 0;
	const qse_char_t* p = str;

	do {
		while (*p == QSE_T(' ') ||
		       *p == QSE_T('\t')) p++;
		if (*p == QSE_T('\0')) break;

		n++;
		while (*p != QSE_T(' ') && 
		       *p != QSE_T('\t') && 
		       *p != QSE_T('\0')) p++;
	} while (1);

	return n;
}

static void __set_names (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str)
{
	qse_word_t n = 0;
	const qse_char_t* p = str;
	const qse_char_t* name;

	do {
		while (*p == QSE_T(' ') ||
		       *p == QSE_T('\t')) p++;
		if (*p == QSE_T('\0')) break;

		name = p;
		while (*p != QSE_T(' ') && 
		       *p != QSE_T('\t') && 
		       *p != QSE_T('\0')) p++;

		array[n++] = qse_stx_new_symbolx (stx, name, p - name);
	} while (1);
}

static qse_word_t __count_subclasses (const qse_char_t* str)
{
	class_info_t* p;
	qse_word_t n = 0;

	for (p = class_info; p->name != QSE_NULL; p++) {
		if (p->superclass == QSE_NULL) continue;
		if (qse_strcmp (str, p->superclass) == 0) n++;
	}

	return n;
}

static void __set_subclasses (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str)
{
	class_info_t* p;
	qse_word_t n = 0, class;

	for (p = class_info; p->name != QSE_NULL; p++) {
		if (p->superclass == QSE_NULL) continue;
		if (qse_strcmp (str, p->superclass) != 0) continue;
		class = qse_stx_lookup_class (stx, p->name);
		qse_assert (class != stx->nil);
		array[n++] = class;
	}
}

static void __set_metaclass_subclasses (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str)
{
	class_info_t* p;
	qse_word_t n = 0, class;

	for (p = class_info; p->name != QSE_NULL; p++) {
		if (p->superclass == QSE_NULL) continue;
		if (qse_strcmp (str, p->superclass) != 0) continue;
		class = qse_stx_lookup_class (stx, p->name);
		qse_assert (class != stx->nil);
		array[n++] = QSE_STX_CLASS(stx,class);
	}
}

static qse_word_t __make_classvar_dict (
	qse_stx_t* stx, qse_word_t class, const qse_char_t* names)
{
	qse_word_t dict, symbol;
	const qse_char_t* p = names;
	const qse_char_t* name;

	dict = qse_stx_instantiate (
		stx, stx->class_system_dictionary,
		QSE_NULL, QSE_NULL, __count_names(names));

	do {
		while (*p == QSE_T(' ') ||
		       *p == QSE_T('\t')) p++;
		if (*p == QSE_T('\0')) break;

		name = p;
		while (*p != QSE_T(' ') && 
		       *p != QSE_T('\t') && 
		       *p != QSE_T('\0')) p++;

		symbol = qse_stx_new_symbolx (stx, name, p - name);
		qse_stx_dict_put (stx, dict, symbol, stx->nil);
	} while (1);

	return dict;
}

static void __filein_kernel (qse_stx_t* stx)
{
	class_info_t* p;

	for (p = class_info; p->name != QSE_NULL; p++) {
		/* TODO: */
	}
}

