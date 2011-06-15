/*
 * $Id$
 */

#ifndef _QSE_LIB_STX_CLS_H_
#define _QSE_LIB_STX_CLS_H_

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

#define SPEC_VARIABLE_BITS  2
#define SPEC_VARIABLE_MASK  0x03

#define SPEC_FIXED_WORD     0x00
#define SPEC_VARIABLE_WORD  0x01
#define SPEC_VARIABLE_BYTE  0x02
#define SPEC_VARIABLE_CHAR  0x03

#define MAKE_SPEC(nflds,variable) \
	(((nflds) << SPEC_VARIABLE_BITS) | (variable))

#ifdef __cplusplus
extern "C" {
#endif

qse_word_t qse_stx_newclass (
	qse_stx_t*        stx,
	const qse_char_t* name
);

qse_word_t qse_stx_findclass (
	qse_stx_t*        stx,
	const qse_char_t* name
);

#if 0
int qse_stx_get_instance_variable_index (
	qse_stx_t* stx, qse_word_t class_index, 
	const qse_char_t* name, qse_word_t* index);

qse_word_t qse_stx_lookup_class_variable (
	qse_stx_t* stx, qse_word_t class_index, const qse_char_t* name);
qse_word_t qse_stx_lookup_method (qse_stx_t* stx, 
	qse_word_t class_index, const qse_char_t* name, qse_bool_t from_super);
#endif



qse_word_t qse_stx_instantiate (
	qse_stx_t*  stx,
	qse_word_t  classref,
	const void* data,
     const void* variable_data,
	qse_word_t  variable_nfields
);

#ifdef __cplusplus
}
#endif

#endif
