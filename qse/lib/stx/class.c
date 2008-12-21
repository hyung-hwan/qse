/*
 * $Id: class.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include <qse/stx/class.h>
#include <qse/stx/symbol.h>
#include <qse/stx/object.h>
#include <qse/stx/dict.h>
#include <qse/stx/misc.h>

qse_word_t qse_stx_new_class (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t meta, class;
	qse_word_t class_name;

	meta = qse_stx_alloc_word_object (
		stx, QSE_NULL, QSE_STX_METACLASS_SIZE, QSE_NULL, 0);
	QSE_STX_CLASS(stx,meta) = stx->class_metaclass;
	/* the spec of the metaclass must be the spec of its
	 * instance. so the QSE_STX_CLASS_SIZE is set */
	QSE_STX_WORD_AT(stx,meta,QSE_STX_METACLASS_SPEC) = 
		QSE_STX_TO_SMALLINT((QSE_STX_CLASS_SIZE << QSE_STX_SPEC_INDEXABLE_BITS) | QSE_STX_SPEC_NOT_INDEXABLE);
	
	/* the spec of the class is set later in __create_builtin_classes */
	class = qse_stx_alloc_word_object (
		stx, QSE_NULL, QSE_STX_CLASS_SIZE, QSE_NULL, 0);
	QSE_STX_CLASS(stx,class) = meta;
	class_name = qse_stx_new_symbol (stx, name);
	QSE_STX_WORD_AT(stx,class,QSE_STX_CLASS_NAME) = class_name;

	qse_stx_dict_put (stx, stx->smalltalk, class_name, class);
	return class;
}

qse_word_t qse_stx_lookup_class (qse_stx_t* stx, const qse_char_t* name)
{
	qse_word_t assoc, meta, value;

	assoc = qse_stx_dict_lookup (stx, stx->smalltalk, name);
	if (assoc == stx->nil) {
		return stx->nil;
	}

	value = QSE_STX_WORD_AT(stx,assoc,QSE_STX_ASSOCIATION_VALUE);
	meta = QSE_STX_CLASS(stx,value);
	if (QSE_STX_CLASS(stx,meta) != stx->class_metaclass) return stx->nil;

	return value;
}

int qse_stx_get_instance_variable_index (
	qse_stx_t* stx, qse_word_t class_index, 
	const qse_char_t* name, qse_word_t* index)
{
	qse_word_t index_super = 0;
	qse_stx_class_t* class_obj;
	qse_stx_char_object_t* string;

	class_obj = (qse_stx_class_t*)QSE_STX_OBJECT(stx, class_index);
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

	class_obj = (qse_stx_class_t*)QSE_STX_OBJECT(stx, class_index);
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

	class_obj = (qse_stx_class_t*)QSE_STX_OBJECT(stx, class_index);
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
		class_obj = (qse_stx_class_t*)QSE_STX_OBJECT(stx, class_index);

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

