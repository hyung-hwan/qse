/*
 * $Id: stxi.h,v 1.1 2005-08-18 15:16:39 bacon Exp $
 */

#ifndef _XP_STX_STXI_H_
#define _XP_STX_STXI_H_

#include <xp/stx/stx.h>

typedef struct xp_stx_memory_t xp_stx_memory_t;
typedef struct xp_stx_symtab_t xp_stx_symtab_t;

struct xp_stx_memory_t
{
	xp_word_t capacity;
	xp_stx_object_t** slots;
	xp_stx_object_t** free;
	xp_bool_t __malloced;
};

struct xp_stx_symtab_t
{
	xp_word_t* datum;
	xp_word_t  size;
	xp_word_t  capacity;
};


struct xp_stx_t
{
	xp_stx_memory_t memory;
	xp_stx_symtab_t symtab;

	xp_word_t nil;
	xp_word_t true;
	xp_word_t false;

	xp_word_t smalltalk;

	xp_word_t class_symbol;
	xp_word_t class_metaclass;
	xp_word_t class_association;

	xp_word_t class_object;
	xp_word_t class_class;
	xp_word_t class_array;
	xp_word_t class_bytearray;
	xp_word_t class_string;
	xp_word_t class_character;
	xp_word_t class_context;
	xp_word_t class_system_dictionary;
	xp_word_t class_method;
	xp_word_t class_smallinteger;

	xp_bool_t __malloced;
	xp_bool_t __wantabort; /* TODO: make it a function pointer */
};

#endif
