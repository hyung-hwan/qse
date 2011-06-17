/*
 * $Id$
 */

#include "stx.h"
#include <qse/cmn/str.h>

static int make_intrinsic_classes (qse_stx_t* stx);

static qse_word_t __make_classvar_dic (
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
	const int spec;
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
		SPEC_FIXED_WORD
	},
	{
		QSE_T("UndefinedObject"),
		QSE_T("Object"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{ 
		QSE_T("Behavior"),
		QSE_T("Object"),
		QSE_T("spec methods superclass"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{ 
		QSE_T("Class"),
		QSE_T("Behavior"),
		QSE_T("name variables classVariables poolDictionaries"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{ 
		QSE_T("Metaclass"),
		QSE_T("Behavior"),
		QSE_T("instanceClass"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Block"),
		QSE_T("Object"),
		QSE_T("context argCount argLoc bytePointer"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Boolean"),
		QSE_T("Object"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("True"),
		QSE_T("Boolean"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("False"),
		QSE_T("Boolean"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Context"),
		QSE_T("Object"),
		QSE_T("stack stackTop receiver pc method"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Method"),
		QSE_T("Object"),
		QSE_T("text selector bytecodes tmpCount argCount"),
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_WORD
	},
	{
		QSE_T("Magnitude"),
		QSE_T("Object"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Association"),
		QSE_T("Magnitude"),
		QSE_T("key value"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Character"),
		QSE_T("Magnitude"),
		QSE_T("value"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Number"),
		QSE_T("Magnitude"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Integer"),
		QSE_T("Number"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("SmallInteger"),
		QSE_T("Integer"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("LargeInteger"),
		QSE_T("Integer"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_BYTE
	},
	{
		QSE_T("Collection"),
		QSE_T("Magnitude"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("IndexedCollection"),
		QSE_T("Collection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_T("Array"),
		QSE_T("IndexedCollection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_WORD
	},
	{
		QSE_T("ByteArray"),
		QSE_T("IndexedCollection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_BYTE
	},
	{   
		QSE_T("SystemSymbolTable"),
		QSE_T("IndexedCollection"),
		QSE_T("tally"),
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_WORD	
	},
	{
		QSE_T("Dictionary"),
		QSE_T("IndexedCollection"),
		QSE_T("tally"),
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_WORD	
	},
		
	{
		QSE_T("SystemDictionary"),
		QSE_T("Dictionary"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_WORD	
	},
	{
		QSE_T("PoolDictionary"),
		QSE_T("Dictionary"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_WORD	
	},
	{
		QSE_T("String"),
		QSE_T("IndexedCollection"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_CHAR
	},
	{
		QSE_T("Symbol"),
		QSE_T("String"),
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_VARIABLE_CHAR
	},
	{
		QSE_T("Link"),
		QSE_T("Object"),
		QSE_T("link"),
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	},
	{
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		QSE_NULL,
		SPEC_FIXED_WORD
	}
};

qse_word_t QSE_INLINE new_string (qse_stx_t* stx, const qse_char_t* str)
{
	qse_word_t x;

	QSE_ASSERT (REFISIDX(stx,stx->ref.class_string));
	QSE_ASSERT (!ISNIL(stx,stx->ref.class_string));

	x = qse_stx_alloccharobj (stx, str, qse_strlen(str));
	if (x != stx->ref.nil) OBJCLASS(stx,x) = stx->ref.class_string;

	return x;	
}

static int make_intrinsic_classes (qse_stx_t* stx)
{
	class_info_t* p;
	qse_word_t n;

	QSE_ASSERT (!ISNIL(stx,stx->ref.class_array));

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		qse_word_t classref;
		qse_stx_class_t* classptr;
		qse_word_t nfixed;

		classref = qse_stx_findclass(stx, p->name);
		if (ISNIL(stx,classref)) 
		{
			classref = qse_stx_newclass (stx, p->name);
			if (ISNIL(stx,classref)) return NIL(stx);
		}

		classptr = (qse_stx_class_t*)PTRBYREF(stx,classref);
		if (p->superclass)
		{
			classptr->superclass = qse_stx_findclass(stx,p->superclass);
			QSE_ASSERT (!ISNIL(stx,classptr->superclass));
		}
		
		nfixed = 0;

		/* resolve the number of fixed fields in the superclass chain */
		if (p->superclass)
		{
			qse_word_t superref; 
			qse_stx_class_t* superptr;

			qse_word_t metaref;
			qse_stx_metaclass_t* metaptr;

			superref = qse_stx_findclass (stx, p->superclass);
			QSE_ASSERT (!ISNIL(stx,superref));

			metaref = OBJCLASS(stx,classref);
			metaptr = (qse_stx_metaclass_t*)PTRBYREF(stx,metaref);
			metaptr->superclass = OBJCLASS(stx,superref);
			metaptr->instance_class = classref;

			do
			{
				superptr = (qse_stx_class_t*)PTRBYREF(stx,superref);
				nfixed += SPEC_GETFIXED(REFTOINT(stx,superptr->spec));
				superref = superptr->superclass;
			}
			while (!ISNIL(stx,superref));
		}

		/* add the number of instance variables to the number of 
		 * fixed fields */
		if (p->instance_variables)
		{
			nfixed += __count_names (p->instance_variables);
			classptr->variables = 
				new_string (stx, p->instance_variables);
			if (ISNIL(stx,classptr->variables)) return  -1;
		}

		QSE_ASSERT (
			nfixed <= 0 || 
			(nfixed > 0 && (p->spec == SPEC_FIXED_WORD || 
		                      p->spec == SPEC_VARIABLE_WORD)));
	
		classptr->spec = SPEC_MAKE (nfixed, p->spec);
	}

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		qse_word_t classref;
		qse_stx_class_t* classptr;

		classref = qse_stx_findclass (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,classref));

		classptr = (qse_stx_class_t*)PTRBYREF(stx,classref);

		if (p->class_variables != QSE_NULL) 
		{
			classptr->class_variables = 
				__make_classvar_dic(stx, classref, p->class_variables);

			if (ISNIL(stx,classptr->class_variables)) return NIL(stx);
		}

		/*
		TODO:
		if (p->pool_dictionaries != QSE_NULL) {
			classptr->pool_dictionaries =
				__make_pool_dictionary(stx, class, p->pool_dictionaries);
		}
		*/
	}

	/* fill subclasses */
	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		qse_word_t classref;
		qse_stx_class_t* classptr;
		qse_word_t array;

		n = __count_subclasses (p->name);
		array = qse_stx_newarray (stx, n);
		if (ISNIL(stx,array)) return NIL(stx);

		__set_subclasses (stx, &WORDAT(stx,array,0), p->name);

		classref = qse_stx_findclass (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,classref));

		classptr = (qse_stx_class_t*)PTRBYREF(stx,classref);
		classptr->subclasses = array;
	}

	/* fill subclasses for metaclasses */
	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		qse_word_t classref;
		qse_stx_class_t* classptr;

		qse_word_t metaref;
		qse_stx_metaclass_t* metaptr;

		qse_word_t array;

		n = __count_subclasses (p->name);
		array = qse_stx_new_array (stx, n);
		__set_metaclass_subclasses (stx, &WORDAT(stx,array,0), p->name);

		classref = qse_stx_findclass (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,classref));

		metaref = OBJCLASS(stx,classref);
		metaptr = (qse_stx_metaclass_t*)PTRBYREF(stx,metaref);
		metaptr->subclasses = array;
	}

	return 0;
}

static qse_word_t __count_names (const qse_char_t* str)
{
	qse_word_t n = 0;
	const qse_char_t* p = str;

	do 
	{
		while (*p == QSE_T(' ') ||
		       *p == QSE_T('\t')) p++;
		if (*p == QSE_T('\0')) break;

		n++;
		while (*p != QSE_T(' ') && 
		       *p != QSE_T('\t') && 
		       *p != QSE_T('\0')) p++;
	} 
	while (1);

	return n;
}

static void __set_names (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str)
{
	qse_word_t n = 0;
	const qse_char_t* p = str;
	const qse_char_t* name;

	do 
	{
		while (*p == QSE_T(' ') ||
		       *p == QSE_T('\t')) p++;
		if (*p == QSE_T('\0')) break;

		name = p;
		while (*p != QSE_T(' ') && 
		       *p != QSE_T('\t') && 
		       *p != QSE_T('\0')) p++;

		array[n++] = qse_stx_newsymbolx (stx, name, p - name);
	}
	while (1);
}

static qse_word_t __count_subclasses (const qse_char_t* str)
{
	class_info_t* p;
	qse_word_t n = 0;

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
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

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		if (p->superclass == QSE_NULL) continue;
		if (qse_strcmp (str, p->superclass) != 0) continue;
		class = qse_stx_findclass (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,class));
		array[n++] = class;
	}
}

static void __set_metaclass_subclasses (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str)
{
	class_info_t* p;
	qse_word_t n = 0, class;

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		if (p->superclass == QSE_NULL) continue;
		if (qse_strcmp (str, p->superclass) != 0) continue;
		class = qse_stx_findclass (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,class));
		array[n++] = OBJCLASS(stx,class);
	}
}

static qse_word_t __make_classvar_dic (
	qse_stx_t* stx, qse_word_t class, const qse_char_t* names)
{
	qse_word_t dic, symbol, assoc;
	const qse_char_t* p = names;
	const qse_char_t* name;

/* TODO: how to implement temporary GC prevention....???? */
	dic = qse_stx_instantiate (
		stx, stx->ref.class_systemdictionary,
		QSE_NULL, QSE_NULL, __count_names(names));
	if (ISNIL(stx,dic)) return NIL(stx);

	do 
	{
		while (*p == QSE_T(' ') ||
		       *p == QSE_T('\t')) p++;
		if (*p == QSE_T('\0')) break;

		name = p;
		while (*p != QSE_T(' ') && 
		       *p != QSE_T('\t') && 
		       *p != QSE_T('\0')) p++;

		symbol = qse_stx_newsymbolx (stx, name, p - name);
		if (ISNIL(stx,symbol)) return NIL(stx);

		assoc =qse_stx_putdic (stx, dic, symbol, stx->ref.nil);
		if (ISNIL(stx,assoc)) return NIL(stx);
	} 
	while (1);

	return dic;
}

static void __filein_kernel (qse_stx_t* stx)
{
	class_info_t* p;

	for (p = class_info; p->name != QSE_NULL; p++) {
		/* TODO: */
	}
}

static int sketch_nil (qse_stx_t* stx)
{
	qse_stx_objidx_t idx;
	qse_word_t ref;
	qse_stx_wordobj_t* ptr;

	/* nil contains no member fields. allocate space for 
	 * an object header */
	idx = qse_stx_allocmem (stx, QSE_SIZEOF(qse_stx_objhdr_t));
	if (idx == QSE_STX_OBJIDX_INVALID) return -1;

	ref = IDXTOREF(stx,idx);
	ptr = (qse_stx_wordobj_t*)PTRBYIDX(stx,idx);

	/* store the nil reference first */
	stx->ref.nil = ref;

	/* nil is a word object containing no member fields.
	 * initialize it accordingly */
	ptr->h._type    = QSE_STX_WORDOBJ;
	ptr->h._mark    = 0;
	ptr->h._refcnt  = 0;
	ptr->h._size    = 0;
	ptr->h._class   = stx->ref.nil; /* the class is yet to be set */
	ptr->h._backref = ref;

	return 0;
}

#define ALLOC_WORDOBJ_TO(stx,var,nflds,nvflds) QSE_BLOCK (\
	var = qse_stx_allocwordobj ((stx), QSE_NULL, (nflds), QSE_NULL, nvflds); \
	if ((var) == (stx)->ref.nil) return -1; \
)

#define ADD_TO_SYSDIC(stx,key,value) QSE_BLOCK (\
	if (qse_stx_putdic ((stx), (stx)->ref.sysdic, (key), (value)) == (stx)->ref.nil) return -1; \
)

#define NEW_SYMBOL_TO(stx,var,name) QSE_BLOCK (\
	var = qse_stx_newsymbol ((stx), name); \
	if (var == (stx)->ref.nil) return -1; \
)

#define NEW_CLASS_TO(stx,var,name) QSE_BLOCK (\
	var = qse_stx_newclass ((stx), name); \
	if (var == (stx)->ref.nil) return -1; \
)

static int sketch_key_objects (qse_stx_t* stx)
{
	qse_word_t class_SymbolMeta; 
	qse_word_t class_MetaclassMeta;
	qse_word_t class_AssociationMeta;
	qse_word_t symbol_symbol; 
	qse_word_t symbol_metaclass;
	qse_word_t symbol_association;

	/* allocate true and false. the class pointer is not correct yet */
	ALLOC_WORDOBJ_TO (stx, stx->ref.true, 0, 0);
	ALLOC_WORDOBJ_TO (stx, stx->ref.false, 0, 0);

	/* create a symbol table partially initialized  */
/* TODO: initial symbol table size */
	ALLOC_WORDOBJ_TO (stx, stx->ref.symtab, 1, SYMTAB_INIT_CAPA);
	/* set tally to 0. */
	WORDAT(stx,stx->ref.symtab,QSE_STX_SYMTAB_TALLY) = INTTOREF(stx,0);

	/* global system dictionary */
/* TODO: initial dictionary size */
	ALLOC_WORDOBJ_TO (stx, stx->ref.sysdic, 1, SYSDIC_INIT_CAPA);
	/* set tally to 0 */
	WORDAT(stx,stx->ref.sysdic,QSE_STX_DIC_TALLY) = INTTOREF(stx,0);

	/* Symbol */
	ALLOC_WORDOBJ_TO (stx, stx->ref.class_symbol, QSE_STX_CLASS_SIZE, 0);
	/* Metaclass */
	ALLOC_WORDOBJ_TO (stx, stx->ref.class_metaclass, QSE_STX_CLASS_SIZE, 0);
	/* Association */
	ALLOC_WORDOBJ_TO (stx, stx->ref.class_association, QSE_STX_CLASS_SIZE, 0);

	/* Metaclass is a class so it has the same structure 
	 * as a normal class. "Metaclass class" is an instance of
	 * Metaclass. */

	/* Symbol class */
	ALLOC_WORDOBJ_TO (stx, class_SymbolMeta, QSE_STX_METACLASS_SIZE, 0);
	/* Metaclass class */
	ALLOC_WORDOBJ_TO (stx, class_MetaclassMeta, QSE_STX_METACLASS_SIZE, 0);
	/* Association class */
	ALLOC_WORDOBJ_TO (stx, class_AssociationMeta, QSE_STX_METACLASS_SIZE, 0);

	/* (Symbol class) setClass: Metaclass */
	QSE_STX_OBJCLASS(stx,class_SymbolMeta) = stx->ref.class_metaclass;
	/* (Metaclass class) setClass: Metaclass */
	QSE_STX_OBJCLASS(stx,class_MetaclassMeta) = stx->ref.class_metaclass;
	/* (Association class) setClass: Metaclass */
	QSE_STX_OBJCLASS(stx,class_AssociationMeta) = stx->ref.class_metaclass;

	/* Symbol setClass: (Symbol class) */
	QSE_STX_OBJCLASS(stx,stx->ref.class_symbol) = class_SymbolMeta;
	/* Metaclass setClass: (Metaclass class) */
	QSE_STX_OBJCLASS(stx,stx->ref.class_metaclass) = class_MetaclassMeta;
	/* Association setClass: (Association class) */
	QSE_STX_OBJCLASS(stx,stx->ref.class_association) = class_AssociationMeta;

	/* (Symbol class) setSpec: CLASS_SIZE */
	WORDAT(stx,class_SymbolMeta,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(QSE_STX_CLASS_SIZE,SPEC_FIXED_WORD));
	/* (Metaclass class) setSpec: CLASS_SIZE */
	WORDAT(stx,class_MetaclassMeta,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(QSE_STX_CLASS_SIZE,SPEC_FIXED_WORD));
	/* (Association class) setSpec: CLASS_SIZE */
	WORDAT(stx,class_AssociationMeta,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(QSE_STX_CLASS_SIZE,SPEC_FIXED_WORD));

	/* specs for class_metaclass, class_association, 
	 * class_symbol are set later in make_builtin_classes */

	/* #Symbol */
	NEW_SYMBOL_TO (stx, symbol_symbol, QSE_T("Symbol"));
	/* #Metaclass */
	NEW_SYMBOL_TO (stx, symbol_metaclass, QSE_T("Metaclass"));
	/* #Association */
	NEW_SYMBOL_TO (stx, symbol_association, QSE_T("Association"));

	/* Symbol setName: #Symbol */
	WORDAT(stx,stx->ref.class_symbol,QSE_STX_CLASS_NAME) = symbol_symbol;
	/* Metaclass setName: #Metaclass */
	WORDAT(stx,stx->ref.class_metaclass,QSE_STX_CLASS_NAME) = symbol_metaclass;
	/* Association setName: #Association */
	WORDAT(stx,stx->ref.class_association,QSE_STX_CLASS_NAME) = symbol_association;

	/* register class names into the system dictionary */
	ADD_TO_SYSDIC (stx, symbol_symbol, stx->ref.class_symbol);
	ADD_TO_SYSDIC (stx, symbol_metaclass, stx->ref.class_metaclass);
	ADD_TO_SYSDIC (stx, symbol_association, stx->ref.class_association);

	return 0;
}

int make_key_classes (qse_stx_t* stx)
{
	/* object, class, and array are precreated for easier instantiation
	 * of intrinsic classes */
	NEW_CLASS_TO (stx, stx->ref.class_object, QSE_T("Object"));
	NEW_CLASS_TO (stx, stx->ref.class_class, QSE_T("Class"));
	NEW_CLASS_TO (stx, stx->ref.class_array, QSE_T("Array"));
	NEW_CLASS_TO (stx, stx->ref.class_bytearray, QSE_T("ByteArray"));
	NEW_CLASS_TO (stx, stx->ref.class_string, QSE_T("String"));
	NEW_CLASS_TO (stx, stx->ref.class_character, QSE_T("Character"));
	NEW_CLASS_TO (stx, stx->ref.class_context, QSE_T("Context"));
	NEW_CLASS_TO (stx, stx->ref.class_systemsymboltable, QSE_T("SystemSymbolTable"));
	NEW_CLASS_TO (stx, stx->ref.class_systemdictionary, QSE_T("SystemDictionary"));
	NEW_CLASS_TO (stx, stx->ref.class_method, QSE_T("Method"));
	NEW_CLASS_TO (stx, stx->ref.class_smallinteger, QSE_T("SmallInteger"));
	return 0;
}


int qse_stx_boot (qse_stx_t* stx)
{
	qse_word_t symbol_smalltalk;
	qse_word_t object_meta;

	/* create a partially initialized nil object for bootstrapping */
	if (sketch_nil (stx) <= -1) goto oops;

	/* continue intializing other key objects */
	if (sketch_key_objects (stx) <= -1) goto oops;

	if (make_key_classes (stx) <= -1) goto oops;

	if (make_intrisic_classes (stx) <= -1) goto oops;

	return 0;

#if 0
	/* (Object class) setSuperclass: Class */
	object_meta = QSE_STX_CLASS(stx,stx->class_object);
	QSE_STX_WORD_AT(stx,object_meta,QSE_STX_METACLASS_SUPERCLASS) = stx->class_class;
	/* instance class for Object is set here as it is not 
	 * set in make_intrisic_classes */
	QSE_STX_WORD_AT(stx,object_meta,QSE_STX_METACLASS_INSTANCE_CLASS) = stx->class_object;

	/* for some fun here */
	{
		qse_word_t array;
		array = qse_stx_new_array (stx, 1);
		QSE_STX_WORD_AT(stx,array,0) = object_meta;
		QSE_STX_WORD_AT(stx,stx->class_class,QSE_STX_CLASS_SUBCLASSES) = array;
	}
			
	/* more initialization */
	OBJCLASS(stx,stx->ref.sysdic) = stx->class_systemdictionary;
	NEW_SYMBOL_TO (stx, symbol_smalltalk, QSE_T("Smalltalk"));
	ADD_TO_SYSDIC (stx, symbol_smalltalk, stx->ref.sysdic);

	/* create #nil, #true, #false */
	NEW_SYMBOL_TO (stx, symbol_nil, QSE_T("nil"));
	NEW_SYMBOL_TO (stx, symbol_true, QSE_T("true"));
	NEW_SYMBOL_TO (stx, symbol_false, QSE_T("false"));

	/* nil setClass: UndefinedObject */
	OBJCLASS(stx,stx->ref.nil) = qse_stx_findclass (stx, QSE_T("UndefinedObject"));
	/* true setClass: True */
	OBJCLASS(stx,stx->ref.true) = qse_stx_findclass (stx, QSE_T("True"));
	/* fales setClass: False */
	OBJCLASS(stx,stx->ref.false) = qse_stx_findclass (stx, QSE_T("False"));

	__filein_kernel (stx);
	return 0;
#endif

oops:
	return -1;
}

