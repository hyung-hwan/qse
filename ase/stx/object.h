/*
 * $Id: object.h,v 1.15 2005-05-22 04:34:22 bacon Exp $
 */

#ifndef _XP_STX_OBJECT_H_
#define _XP_STX_OBJECT_H_

#include <xp/stx/stx.h>

#define XP_STX_IS_SMALLINT(x)   (((x) & 0x01) == 0x01)
#define XP_STX_TO_SMALLINT(x)   (((x) << 1) | 0x01)
#define XP_STX_FROM_SMALLINT(x) ((x) >> 1)

/* definitions for common objects */
#define XP_STX_CLASS_SIZE        8
#define XP_STX_CLASS_NAME        0
#define XP_STX_CLASS_SPEC        1
#define XP_STX_CLASS_METHODS     2
#define XP_STX_CLASS_SUPERCLASS  3
#define XP_STX_CLASS_VARIABLES   4
#define XP_STX_CLASS_CLASSVARS   5
#define XP_STX_CLASS_POOLDICT    6
#define XP_STX_CLASS_CATEGORY    7

struct xp_stx_class_t
{
	xp_stx_objhdr_t header;
	xp_stx_word_t name;
	xp_stx_word_t spec;
	xp_stx_word_t methods;
	xp_stx_word_t superclass;
	xp_stx_word_t variables;
	xp_stx_word_t classvars;
	xp_stx_word_t pooldict;
	xp_stx_word_t category;
};

typedef struct xp_stx_class_t xp_stx_class_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_alloc_word_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_byte_object (xp_stx_t* stx, xp_stx_word_t n);
xp_stx_word_t xp_stx_alloc_char_object (
	xp_stx_t* stx, const xp_stx_char_t* str);
xp_stx_word_t xp_stx_allocn_char_object (xp_stx_t* stx, ...);

xp_stx_word_t xp_stx_hash_char_object (xp_stx_t* stx, xp_stx_word_t idx);
xp_stx_word_t xp_stx_new_class (
	xp_stx_t* stx, const xp_stx_char_t* name);

int xp_stx_lookup_global (
	xp_stx_t* stx, xp_stx_word_t key, xp_stx_word_t* value);

#ifdef __cplusplus
}
#endif

#endif
