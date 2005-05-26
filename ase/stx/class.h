/*
 * $Id: class.h,v 1.5 2005-05-26 15:39:32 bacon Exp $
 */

#ifndef _XP_STX_CLASS_H_
#define _XP_STX_CLASS_H_

#include <xp/stx/stx.h>

/* definitions for common objects */
#define XP_STX_CLASS_SIZE              8
#define XP_STX_CLASS_SPEC              0
#define XP_STX_CLASS_METHODS           1
#define XP_STX_CLASS_SUPERCLASS        2
#define XP_STX_CLASS_SUBCLASSES        3
#define XP_STX_CLASS_NAME              4
#define XP_STX_CLASS_VARIABLES         5
#define XP_STX_CLASS_CLASS_VARIABLES   6
#define XP_STX_CLASS_POOL_DICTIONARIES 7

#define XP_STX_METACLASS_SIZE           5
#define XP_STX_METACLASS_SPEC           0
#define XP_STX_METACLASS_METHODS        1
#define XP_STX_METACLASS_SUPERCLASS     2
#define XP_STX_METACLASS_SUBCLASSES     3
#define XP_STX_METACLASS_INSTANCE_CLASS 4

struct xp_stx_class_t
{
	xp_stx_objhdr_t header;
	xp_stx_word_t spec; /* indexable: 1, nfields: the rest */
	xp_stx_word_t methods;
	xp_stx_word_t superclass;
	xp_stx_word_t subclasses;
	xp_stx_word_t name;
	xp_stx_word_t variables;
	xp_stx_word_t class_variables;
	xp_stx_word_t pool_dictonaries;
};

struct xp_stx_metaclass_t
{
	xp_stx_objhdr_t header;
	xp_stx_word_t spec;
	xp_stx_word_t methods;
	xp_stx_word_t superclass;
	xp_stx_word_t subclasses;
	xp_stx_word_t instance_class;
};

typedef struct xp_stx_class_t xp_stx_class_t;
typedef struct xp_stx_metaclass_t xp_stx_metaclass_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_new_class (xp_stx_t* stx, const xp_stx_char_t* name);
xp_stx_word_t xp_stx_lookup_class (xp_stx_t* stx, const xp_stx_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
