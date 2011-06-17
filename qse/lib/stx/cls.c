/*
 * $Id: class.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include "stx.h"

qse_word_t qse_stx_newclass (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t meta, class;
	qse_word_t class_name;

	QSE_ASSERT (REFISIDX(stx,stx->ref.class_metaclass));
	
	meta = qse_stx_allocwordobj (
		stx, QSE_NULL, QSE_STX_METACLASS_SIZE, QSE_NULL, 0);
	if (meta == stx->ref.nil) return stx->ref.nil;
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
	if (class_name == stx->ref.nil) return stx->ref.nil;

	WORDAT(stx,class,QSE_STX_CLASS_NAME) = class_name;

	return (qse_stx_putdic (stx, stx->ref.sysdic, class_name, class) == stx->ref.nil)? stx->ref.nil: class;
}

qse_word_t qse_stx_findclass (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t assoc, meta, value;

	/* look up the system dictionary for the name given */
	assoc = qse_stx_lookupdic (stx, stx->ref.sysdic, name);
	if (assoc == stx->ref.nil) 
	{
		/*qse_stx_seterrnum (stx, QSE_STX_ENOCLASS, QSE_NULL);*/
		return stx->ref.nil;
	}

	/* get the value part in the association for the name */
	value = WORDAT(stx,assoc,QSE_STX_ASSOC_VALUE);

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

#if 0
int qse_stx_get_instance_variable_index (
	qse_stx_t* stx, qse_word_t class_index, 
	const qse_char_t* name, qse_word_t* index)
{
	qse_word_t index_super = 0;
	qse_stx_class_t* class_obj;
	qse_stx_char_object_t* string;

	class_obj = (qse_stx_class_t*)QSE_STX_OBJPTR(stx, class_index);
	qse_assert (class_obj != QSE_NULL);

	if (class_obj->superclass != stx->nil) {
		if (qse_stx_get_instance_variable_index (
			stx, class_obj->superclass, name, &index_super) == 0) {
			*index = index_super;
			return 0;
		}
	}

	if (class_obj->header.class == stx->class_metaclass) {
		/* metaclass */
		/* TODO: can a metaclas have instance variables? */	
		*index = index_super;
	}
	else {
		if (class_obj->variables == stx->nil) *index = 0;
		else {
			string = QSE_STX_CHAR_OBJECT(stx, class_obj->variables);
			if (qse_stx_strword(string->data, name, index) != QSE_NULL) {
				*index += index_super;
				return 0;
			}
		}

		*index += index_super;
	}

	return -1;
}

qse_word_t qse_stx_lookup_class_variable (
	qse_stx_t* stx, qse_word_t class_index, const qse_char_t* name)
{
	qse_stx_class_t* class_obj;

	class_obj = (qse_stx_class_t*)QSE_STX_OBJPTR(stx, class_index);
	qse_assert (class_obj != QSE_NULL);

	if (class_obj->superclass != stx->nil) {
		qse_word_t tmp;
		tmp = qse_stx_lookup_class_variable (
			stx, class_obj->superclass, name);
		if (tmp != stx->nil) return tmp;
	}

	/* TODO: can a metaclas have class variables? */	
	if (class_obj->header.class != stx->class_metaclass &&
	    class_obj->class_variables != stx->nil) {
		if (qse_stx_dict_lookup(stx,
			class_obj->class_variables,name) != stx->nil) return class_index;
	}

	return stx->nil;
}

qse_word_t qse_stx_lookup_method (qse_stx_t* stx, 
	qse_word_t class_index, const qse_char_t* name, qse_bool_t from_super)
{
	qse_stx_class_t* class_obj;

	class_obj = (qse_stx_class_t*)QSE_STX_OBJPTR(stx, class_index);
	qse_assert (class_obj != QSE_NULL);

#if 0
	if (class_obj->header.class != stx->class_metaclass &&
	    class_obj->methods != stx->nil) {
		qse_word_t assoc;
		assoc = qse_stx_dict_lookup(stx, class_obj->methods, name);
		if (assoc != stx->nil) {
			qse_assert (QSE_STX_CLASS(stx,assoc) == stx->class_association);
			return QSE_STX_WORD_AT(stx, assoc, QSE_STX_ASSOCIATION_VALUE);
		}
	}

	if (class_obj->superclass != stx->nil) {
		qse_word_t tmp;
		tmp = qse_stx_lookup_method (
			stx, class_obj->superclass, name);
		if (tmp != stx->nil) return tmp;
	}
#endif

	while (class_index != stx->nil) {
		class_obj = (qse_stx_class_t*)QSE_STX_OBJPTR(stx, class_index);

		qse_assert (class_obj != QSE_NULL);
		qse_assert (
			class_obj->header.class == stx->class_metaclass ||
			QSE_STX_CLASS(stx,class_obj->header.class) == stx->class_metaclass);

		if (from_super) {	
			from_super = qse_false;
		}
		else if (class_obj->methods != stx->nil) {
			qse_word_t assoc;
			assoc = qse_stx_dict_lookup(stx, class_obj->methods, name);
			if (assoc != stx->nil) {
				qse_assert (QSE_STX_CLASS(stx,assoc) == stx->class_association);
				return QSE_STX_WORD_AT(stx, assoc, QSE_STX_ASSOCIATION_VALUE);
			}
		}

		class_index = class_obj->superclass;
	}

	return stx->nil;
}
#endif

qse_word_t qse_stx_instantiate (
	qse_stx_t* stx, qse_word_t classref, const void* data, 
	const void* variable_data, qse_word_t variable_nflds)
{
	qse_stx_class_t* classptr;
	qse_word_t spec, nflds, inst;
	int variable;


	QSE_ASSERT (REFISIDX(stx,classref));

	/* don't instantiate a metaclass whose instance must be 
	   created in a different way */
	QSE_ASSERT (OBJCLASS(stx,classref) != stx->ref.class_metaclass);

	classptr = (qse_stx_class_t*)PTRBYREF(stx,classref);
	QSE_ASSERT (REFISINT(stx,classptr->spec));

	spec = REFTOINT(stx,classptr->spec);
	nflds = (spec >> SPEC_VARIABLE_BITS);
	variable = spec & SPEC_VARIABLE_MASK;

	switch (variable)
	{
		case SPEC_VARIABLE_BYTE:
			/* variable-size byte class */
			QSE_ASSERT (nflds == 0 && data == QSE_NULL);
			inst = qse_stx_allocbyteobj (
				stx, variable_data, variable_nflds);
			break;

		case SPEC_VARIABLE_CHAR:
			/* variable-size char class */
			QSE_ASSERT (nflds == 0 && data == QSE_NULL);
			inst = qse_stx_alloccharobj (
				stx, variable_data, variable_nflds);
			break;

		case SPEC_VARIABLE_WORD:
			/* variable-size class */
			inst = qse_stx_allocwordobj (
				stx, data, nflds, variable_data, variable_nflds);
			break;

		case SPEC_FIXED_WORD:
			/* fixed size */
			QSE_ASSERT (variable_nflds == 0 && variable_data == QSE_NULL);
			inst = qse_stx_allocwordobj (
				stx, data, nflds, QSE_NULL, 0);
			break;

		default:
			/* this should never happen */	
			QSE_ASSERTX (0, "this should never happen");
			qse_stx_seterrnum (stx, QSE_STX_EINTERN, QSE_NULL);
			return stx->ref.nil;
	}

	QSE_ASSERT (inst != stx->ref.nil);

	OBJCLASS(stx,inst) = classref;
	return inst;
}
