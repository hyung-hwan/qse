/*
 * $Id: class.h 118 2008-03-03 11:21:33Z baconevi $
 */

#ifndef _QSE_STX_CLASS_H_
#define _QSE_STX_CLASS_H_

#include <qse/stx/stx.h>

/* definitions for common objects */
#define QSE_STX_CLASS_SIZE              8
#define QSE_STX_CLASS_SPEC              0
#define QSE_STX_CLASS_METHODS           1
#define QSE_STX_CLASS_SUPERCLASS        2
#define QSE_STX_CLASS_SUBCLASSES        3
#define QSE_STX_CLASS_NAME              4
#define QSE_STX_CLASS_VARIABLES         5
#define QSE_STX_CLASS_CLASS_VARIABLES   6
#define QSE_STX_CLASS_POOL_DICTIONARIES 7

#define QSE_STX_METACLASS_SIZE           5
#define QSE_STX_METACLASS_SPEC           0
#define QSE_STX_METACLASS_METHODS        1
#define QSE_STX_METACLASS_SUPERCLASS     2
#define QSE_STX_METACLASS_SUBCLASSES     3
#define QSE_STX_METACLASS_INSTANCE_CLASS 4

#define QSE_STX_SPEC_INDEXABLE_BITS 2
#define QSE_STX_SPEC_INDEXABLE_MASK 3
#define QSE_STX_SPEC_NOT_INDEXABLE  0
#define QSE_STX_SPEC_WORD_INDEXABLE 1
#define QSE_STX_SPEC_BYTE_INDEXABLE 2
#define QSE_STX_SPEC_CHAR_INDEXABLE 3

struct qse_stx_class_t
{
	qse_stx_objhdr_t header;
	qse_word_t spec; /* indexable: 2, nfields: the rest */
	qse_word_t methods;
	qse_word_t superclass;
	qse_word_t subclasses;
	qse_word_t name;
	qse_word_t variables;
	qse_word_t class_variables;
	qse_word_t pool_dictonaries;
};

struct qse_stx_metaclass_t
{
	qse_stx_objhdr_t header;
	qse_word_t spec;
	qse_word_t methods;
	qse_word_t superclass;
	qse_word_t subclasses;
	qse_word_t instance_class;
};

typedef struct qse_stx_class_t qse_stx_class_t;
typedef struct qse_stx_metaclass_t qse_stx_metaclass_t;

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_new_class (qse_stx_t* stx, const qse_char_t* name);
qse_word_t qse_stx_lookup_class (qse_stx_t* stx, const qse_char_t* name);

int qse_stx_get_instance_variable_index (
	qse_stx_t* stx, qse_word_t class_index, 
	const qse_char_t* name, qse_word_t* index);

qse_word_t qse_stx_lookup_class_variable (
	qse_stx_t* stx, qse_word_t class_index, const qse_char_t* name);
qse_word_t qse_stx_lookup_method (qse_stx_t* stx, 
	qse_word_t class_index, const qse_char_t* name, qse_bool_t from_super);

#ifdef __cplusplus
}
#endif

#endif
