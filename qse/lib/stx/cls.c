/*
 * $Id$
 */

#include "stx.h"

#if 0
qse_char_t* qse_stx_strword (
	const qse_char_t* str, const qse_char_t* word, qse_word_t* word_index)
{
	qse_char_t* p = (qse_char_t*)str;
	qse_char_t* tok;
	qse_size_t len;
	qse_word_t index = 0;

	while (p != QSE_NULL) 
	{
		p = qse_strtok (p, QSE_T(""), &tok, &len);
		if (qse_strxcmp (tok, len, word) == 0) 
		{
			*word_index = index;
			return tok;
		}

		index++;
	}

	*word_index = index;
	return QSE_NULL;
}

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
