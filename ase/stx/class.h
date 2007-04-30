/*
 * $Id: class.h,v 1.1.1.1 2007/03/28 14:05:28 bacon Exp $
 */

#ifndef _ASE_STX_CLASS_H_
#define _ASE_STX_CLASS_H_

#include <ase/stx/stx.h>

/* definitions for common objects */
#define ASE_STX_CLASS_SIZE              8
#define ASE_STX_CLASS_SPEC              0
#define ASE_STX_CLASS_METHODS           1
#define ASE_STX_CLASS_SUPERCLASS        2
#define ASE_STX_CLASS_SUBCLASSES        3
#define ASE_STX_CLASS_NAME              4
#define ASE_STX_CLASS_VARIABLES         5
#define ASE_STX_CLASS_CLASS_VARIABLES   6
#define ASE_STX_CLASS_POOL_DICTIONARIES 7

#define ASE_STX_METACLASS_SIZE           5
#define ASE_STX_METACLASS_SPEC           0
#define ASE_STX_METACLASS_METHODS        1
#define ASE_STX_METACLASS_SUPERCLASS     2
#define ASE_STX_METACLASS_SUBCLASSES     3
#define ASE_STX_METACLASS_INSTANCE_CLASS 4

#define ASE_STX_SPEC_INDEXABLE_BITS 2
#define ASE_STX_SPEC_INDEXABLE_MASK 3
#define ASE_STX_SPEC_NOT_INDEXABLE  0
#define ASE_STX_SPEC_WORD_INDEXABLE 1
#define ASE_STX_SPEC_BYTE_INDEXABLE 2
#define ASE_STX_SPEC_CHAR_INDEXABLE 3

struct ase_stx_class_t
{
	ase_stx_objhdr_t header;
	ase_word_t spec; /* indexable: 2, nfields: the rest */
	ase_word_t methods;
	ase_word_t superclass;
	ase_word_t subclasses;
	ase_word_t name;
	ase_word_t variables;
	ase_word_t class_variables;
	ase_word_t pool_dictonaries;
};

struct ase_stx_metaclass_t
{
	ase_stx_objhdr_t header;
	ase_word_t spec;
	ase_word_t methods;
	ase_word_t superclass;
	ase_word_t subclasses;
	ase_word_t instance_class;
};

typedef struct ase_stx_class_t ase_stx_class_t;
typedef struct ase_stx_metaclass_t ase_stx_metaclass_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_word_t ase_stx_new_class (ase_stx_t* stx, const ase_char_t* name);
ase_word_t ase_stx_lookup_class (ase_stx_t* stx, const ase_char_t* name);

int ase_stx_get_instance_variable_index (
	ase_stx_t* stx, ase_word_t class_index, 
	const ase_char_t* name, ase_word_t* index);

ase_word_t ase_stx_lookup_class_variable (
	ase_stx_t* stx, ase_word_t class_index, const ase_char_t* name);
ase_word_t ase_stx_lookup_method (ase_stx_t* stx, 
	ase_word_t class_index, const ase_char_t* name, ase_bool_t from_super);

#ifdef __cplusplus
}
#endif

#endif
