/*
 * $Id$
 */

#include "stx.h"
#include <qse/cmn/str.h>

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
		QSE_T("classvar1 classvar2")/*QSE_NULL TODO: delete this.....*/,
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
		QSE_T("spec methods superclass subclasses"),
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
	QSE_ASSERT (REFISIDX(stx,stx->ref.class_string));
	QSE_ASSERT (!ISNIL(stx,stx->ref.class_string));

	return qse_stx_instantiate (
		stx, stx->ref.class_string, QSE_NULL, str, qse_strlen(str));
}

qse_word_t QSE_INLINE new_array (qse_stx_t* stx, qse_word_t capa)
{
	QSE_ASSERT (REFISIDX(stx,stx->ref.class_array));
	QSE_ASSERT (!ISNIL(stx,stx->ref.class_array));

	return qse_stx_instantiate (
		stx, stx->ref.class_array, QSE_NULL, QSE_NULL, capa);
}

qse_word_t QSE_INLINE new_systemdictionary (qse_stx_t* stx, qse_word_t capa)
{
	QSE_ASSERT (REFISIDX(stx,stx->ref.class_systemdictionary));
	QSE_ASSERT (!ISNIL(stx,stx->ref.class_systemdictionary));

	/* the system dictionary uses 1 slot dedicated for nil.
	 * so we request to allocate 1 more slot than the given */
	return qse_stx_instantiate (
		stx, stx->ref.class_systemdictionary,
		QSE_NULL, QSE_NULL, capa + 1);
}

qse_word_t new_class (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t meta, class, assoc;
	qse_word_t class_name;

	QSE_ASSERT (REFISIDX(stx,stx->ref.class_metaclass));
	
	meta = qse_stx_allocwordobj (
		stx, QSE_NULL, QSE_STX_METACLASS_SIZE, QSE_NULL, 0);
	if (ISNIL(stx,meta)) return stx->ref.nil;
	OBJCLASS(stx,meta) = stx->ref.class_metaclass;

	/* the spec of the metaclass must be the spec of its
	 * instance. so the QSE_STX_CLASS_SIZE is set */
	WORDAT(stx,meta,QSE_STX_METACLASS_SPEC) = 
		INTTOREF(stx,SPEC_MAKE(QSE_STX_CLASS_SIZE,SPEC_FIXED_WORD));
	
	/* the spec of the class is set later in __create_builtin_classes */
	class = qse_stx_allocwordobj (
		stx, QSE_NULL, QSE_STX_CLASS_SIZE, QSE_NULL, 0);
	OBJCLASS(stx,class) = meta;

	class_name = qse_stx_newsymbol (stx, name);
	if (ISNIL(stx,class_name)) return stx->ref.nil;

	WORDAT(stx,class,QSE_STX_CLASS_NAME) = class_name;
	WORDAT(stx,class,QSE_STX_CLASS_SPEC) = stx->ref.nil;

	assoc = qse_stx_putdic (stx, stx->ref.sysdic, class_name, class);
	return (ISNIL(stx,assoc))? stx->ref.nil: class;
}

qse_word_t find_class (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t assoc, meta, value;

	/* look up the system dictionary for the name given */
	assoc = qse_stx_lookupdic (stx, stx->ref.sysdic, name);
	if (ISNIL(stx,assoc))
	{
		/*qse_stx_seterrnum (stx, QSE_STX_ENOCLASS, QSE_NULL);*/
		return stx->ref.nil;
	}

	/* get the value part in the association for the name */
	value = WORDAT(stx,assoc,QSE_STX_ASSOCIATION_VALUE);

	/* check if its class is Metaclass because the class of
	 * a class object must be Metaclass. */
	meta = OBJCLASS(stx,value);
	if (OBJCLASS(stx,meta) != stx->ref.class_metaclass) 
	{
		/*qse_stx_seterrnum (stx, QSE_STX_ENOTCLASS, QSE_NULL);*/
		return stx->ref.nil;
	}

	return value;
}

static qse_word_t count_names (const qse_char_t* str)
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

static qse_word_t count_subclasses (const qse_char_t* str)
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

static void set_subclasses (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str)
{
	class_info_t* p;
	qse_word_t n = 0, class;

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		if (p->superclass == QSE_NULL) continue;
		if (qse_strcmp (str, p->superclass) != 0) continue;
		class = find_class (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,class));
		array[n++] = class;
	}
}

static void set_metaclass_subclasses (
	qse_stx_t* stx, qse_word_t* array, const qse_char_t* str)
{
	class_info_t* p;
	qse_word_t n = 0, class;

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		if (p->superclass == QSE_NULL) continue;
		if (qse_strcmp (str, p->superclass) != 0) continue;
		class = find_class (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,class));
		array[n++] = OBJCLASS(stx,class);
	}
}

static qse_word_t make_classvar_dic (
	qse_stx_t* stx, qse_word_t class, const qse_char_t* names)
{
	qse_word_t dic, symbol, assoc;
	const qse_char_t* p = names;
	const qse_char_t* name;

/* TODO: how to implement temporary GC prevention....???? */
	dic = new_systemdictionary (stx, count_names(names));
	if (ISNIL(stx,dic)) return stx->ref.nil;

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
		if (ISNIL(stx,symbol)) return stx->ref.nil;

		assoc = qse_stx_putdic (stx, dic, symbol, stx->ref.nil);
		if (ISNIL(stx,assoc)) return stx->ref.nil;
	} 
	while (1);

	return dic;
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
	if (ISNIL(stx,(var))) return -1; \
)

#define ADD_TO_SYSDIC(stx,key,value) QSE_BLOCK (\
	if (qse_stx_putdic ((stx), (stx)->ref.sysdic, (key), (value)) == (stx)->ref.nil) return -1; \
)

#define NEW_SYMBOL_TO(stx,var,name) QSE_BLOCK (\
	var = qse_stx_newsymbol ((stx), name); \
	if (ISNIL(stx,(var))) return -1; \
)

#define NEW_CLASS_TO(stx,var,name) QSE_BLOCK (\
	var = new_class ((stx), name); \
	if (ISNIL(stx,(var))) return -1; \
)

static int sketch_key_objects (qse_stx_t* stx)
{
	qse_word_t class_SymbolMeta; 
	qse_word_t class_MetaclassMeta;
	qse_word_t class_AssociationMeta;
	qse_word_t symbol_symbol; 
	qse_word_t symbol_metaclass;
	qse_word_t symbol_association;

	QSE_ASSERT (REFISIDX(stx,stx->ref.nil));

	/* Create a symbol table partially initialized.
	 * Especially, the class of the symbol table is not set yet.  
	 * It must be corrected later */
/* TODO: initial symbol table size */
	ALLOC_WORDOBJ_TO (stx, stx->ref.symtab, 1, SYMTAB_INIT_CAPA);
	/* Set tally to 0. */
	WORDAT(stx,stx->ref.symtab,QSE_STX_SYSTEMSYMBOLTABLE_TALLY) = INTTOREF(stx,0);

	/* Create a global system dictionary partially initialized.
	 * Especially, the class of the system dictionary is not set yet.  
	 * It must be corrected later */
/* TODO: initial dictionary size */
	ALLOC_WORDOBJ_TO (stx, stx->ref.sysdic, 1, SYSDIC_INIT_CAPA);
	/* Set tally to 0 */
	WORDAT(stx,stx->ref.sysdic,QSE_STX_SYSTEMDICTIONARY_TALLY) = INTTOREF(stx,0);

	/* Create a few critical class objects needed for maintaining
	 * the symbol table and the system dictionary. At this point,
	 * new_class() cannot be used yet. So the process is
	 * pretty mundane as shown below. */

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
	OBJCLASS(stx,class_SymbolMeta) = stx->ref.class_metaclass;
	/* (Metaclass class) setClass: Metaclass */
	OBJCLASS(stx,class_MetaclassMeta) = stx->ref.class_metaclass;
	/* (Association class) setClass: Metaclass */
	OBJCLASS(stx,class_AssociationMeta) = stx->ref.class_metaclass;

	/* Symbol setClass: (Symbol class) */
	OBJCLASS(stx,stx->ref.class_symbol) = class_SymbolMeta;
	/* Metaclass setClass: (Metaclass class) */
	OBJCLASS(stx,stx->ref.class_metaclass) = class_MetaclassMeta;
	/* Association setClass: (Association class) */
	OBJCLASS(stx,stx->ref.class_association) = class_AssociationMeta;

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
	 * class_symbol are set later in make_intrinsic_classes */

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

	/* propagte the spec field in advance */
	WORDAT(stx,stx->ref.class_symbol,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(0,SPEC_VARIABLE_CHAR));
	WORDAT(stx,stx->ref.class_metaclass,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(QSE_STX_METACLASS_SIZE,SPEC_FIXED_WORD));
	WORDAT(stx,stx->ref.class_association,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(QSE_STX_ASSOCIATION_SIZE,SPEC_FIXED_WORD));

	/* register class names into the system dictionary */
	ADD_TO_SYSDIC (stx, symbol_symbol, stx->ref.class_symbol);
	ADD_TO_SYSDIC (stx, symbol_metaclass, stx->ref.class_metaclass);
	ADD_TO_SYSDIC (stx, symbol_association, stx->ref.class_association);

	return 0;
}

static int make_key_classes (qse_stx_t* stx)
{
	/* object, class, and array are precreated for easier instantiation
	 * of intrinsic classes. */
	NEW_CLASS_TO (stx, stx->ref.class_object, QSE_T("Object"));
	NEW_CLASS_TO (stx, stx->ref.class_undefinedobject, QSE_T("UndefinedObject"));
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

	/* set the spec field in advance so that new_string() and new_array() 
	 * can call qse_stx_instantiate() from this point onwards */
	WORDAT(stx,stx->ref.class_string,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(0,SPEC_VARIABLE_CHAR));
	WORDAT(stx,stx->ref.class_array,QSE_STX_CLASS_SPEC) = 
		INTTOREF (stx, SPEC_MAKE(0,SPEC_VARIABLE_WORD));

	return 0;
}

static void set_class_of_more_key_objects (qse_stx_t* stx)
{
	/* nil setClass: UndefinedObject */
	OBJCLASS(stx,stx->ref.nil) = stx->ref.class_undefinedobject;

	/* sysdic(Smalltalk) setClass: SystemDictionary */
	OBJCLASS(stx,stx->ref.sysdic) = stx->ref.class_systemdictionary;

	/* symtab setClass: SystemSymbolTable */
	OBJCLASS(stx,stx->ref.symtab) = stx->ref.class_systemsymboltable;
}

static int make_intrinsic_classes (qse_stx_t* stx)
{
	class_info_t* p;

	QSE_ASSERT (!ISNIL(stx,stx->ref.class_array));

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		qse_word_t classref;
		qse_stx_class_t* classptr;
		qse_word_t nfixed;
		qse_word_t spec;

		classref = find_class(stx, p->name);
		if (ISNIL(stx,classref)) 
		{
			classref = new_class (stx, p->name);
			if (ISNIL(stx,classref)) return stx->ref.nil;
		}

		classptr = (qse_stx_class_t*)PTRBYREF(stx,classref);
		if (p->superclass)
		{
			classptr->superclass = find_class(stx,p->superclass);
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

			superref = find_class (stx, p->superclass);
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
			nfixed += count_names (p->instance_variables);
			classptr->variables = 
				new_string (stx, p->instance_variables);
			if (ISNIL(stx,classptr->variables)) return  -1;
		}

		QSE_ASSERT (
			nfixed <= 0 || 
			(nfixed > 0 && (p->spec == SPEC_FIXED_WORD || 
		                     p->spec == SPEC_VARIABLE_WORD)));
	
		spec = INTTOREF (stx, SPEC_MAKE (nfixed, p->spec));

		QSE_ASSERTX (ISNIL(stx,classptr->spec) || classptr->spec == spec,
			"If the specfication field is already set before this function, "
			"the specification in the class information table must match it. "
			"Otherwise, something is very wrong");

		classptr->spec = spec;
	}

	/* make class variable dictionaries and pool dictionaries */
	for (p = class_info; p->name; p++) 
	{
		qse_word_t classref;
		qse_stx_class_t* classptr;

		classref = find_class (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,classref));

		classptr = (qse_stx_class_t*)PTRBYREF(stx,classref);

		if (p->class_variables)
		{
			classptr->class_variables = 
				make_classvar_dic(stx, classref, p->class_variables);
			if (ISNIL(stx,classptr->class_variables)) return stx->ref.nil;
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
		qse_word_t n;

		n = count_subclasses (p->name);
		array = new_array (stx, n);
		if (ISNIL(stx,array)) return -1;

		set_subclasses (stx, &WORDAT(stx,array,0), p->name);

		classref = find_class (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,classref));

		classptr = (qse_stx_class_t*)PTRBYREF(stx,classref);
		classptr->subclasses = array;
	}

	/* fill subclasses for metaclasses */
	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		qse_word_t classref;

		qse_word_t metaref;
		qse_stx_metaclass_t* metaptr;

		qse_word_t array;
		qse_word_t n;

		n = count_subclasses (p->name);
		array = new_array (stx, n);
		if (ISNIL(stx,array)) return -1;

		set_metaclass_subclasses (stx, &WORDAT(stx,array,0), p->name);

		classref = find_class (stx, p->name);
		QSE_ASSERT (!ISNIL(stx,classref));

		metaref = OBJCLASS(stx,classref);
		metaptr = (qse_stx_metaclass_t*)PTRBYREF(stx,metaref);
		metaptr->subclasses = array;
	}

	return 0;
}


static void make_metaclass_top_hierarchy (qse_stx_t* stx)
{
	qse_word_t metaclass_of_object;

	/* make the superclass of Object class to be Class */

	/* metaclass_of_object := Object class */
	metaclass_of_object = OBJCLASS (stx, stx->ref.class_object);
	
	/* (Object class) setSuperclass: Class */
	WORDAT(stx,metaclass_of_object,QSE_STX_METACLASS_SUPERCLASS) = stx->ref.class_class;

	/* Set the instance class for Object here as it is not 
	 * set in make_intrisic_classes */
	WORDAT(stx,metaclass_of_object,QSE_STX_METACLASS_INSTANCE_CLASS) = stx->ref.class_object;
}


static int make_key_objects_accessible_by_name (qse_stx_t* stx)
{
	qse_word_t tmp;

#if 0
	/* create #nil, #true, #false */
	NEW_SYMBOL_TO (stx, tmp, QSE_T("nil"));
	NEW_SYMBOL_TO (stx, tmp, QSE_T("true"));
	NEW_SYMBOL_TO (stx, tmp, QSE_T("false"));
#endif

	NEW_SYMBOL_TO (stx, tmp, QSE_T("Smalltalk"));
	/* Smalltalk at: #Smalltalk put: stx->ref.sysdic */
	ADD_TO_SYSDIC (stx, tmp, stx->ref.sysdic);

	NEW_SYMBOL_TO (stx, tmp, QSE_T("SymbolTable"));
	/* Smalltalk at: #SymbolTable put: stx->ref.sysdic */
	ADD_TO_SYSDIC (stx, tmp, stx->ref.symtab);

	return 0;
}

static int make_true_and_false (qse_stx_t* stx)
{
	stx->ref.true = qse_stx_instantiate (
		stx, find_class(stx,QSE_T("True")), 
		QSE_NULL, QSE_NULL, 0
	);
	if (ISNIL(stx,stx->ref.true)) return -1;

	stx->ref.false = qse_stx_instantiate (
		stx, find_class(stx,QSE_T("False")),
		QSE_NULL, QSE_NULL, 0
	);
	if (ISNIL(stx,stx->ref.false)) return -1;
	return 0;
}

static void filein_kernel_source (qse_stx_t* stx)
{
	class_info_t* p;

	for (p = class_info; p->name != QSE_NULL; p++) 
	{
		/* TODO: */
	}
}

int qse_stx_boot (qse_stx_t* stx)
{
	/* you must not call this function more than once... */
	QSE_ASSERTX (
		stx->ref.nil == 0 &&
		stx->ref.true == 0 &&
		stx->ref.false == 0,
		"You must not call qse_stx_boot() more than once"
	);

	if (sketch_nil (stx) <= -1) return -1;

	if (sketch_key_objects (stx) <= -1) return -1;

	if (make_key_classes (stx) <= -1) return -1;

	set_class_of_more_key_objects (stx);

	if (make_intrinsic_classes (stx) <= -1) return -1;

	make_metaclass_top_hierarchy (stx);

	if (make_key_objects_accessible_by_name (stx) <= -1) return -1;

	if (make_true_and_false (stx) <= -1) return -1;

	return 0;
}


/* for debugging for the time begin ... */
qse_word_t qse_stx_findclass (qse_stx_t* stx, const qse_char_t* name)
{
	return find_class (stx, name);
}
