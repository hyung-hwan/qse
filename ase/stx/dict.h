/*
 * $Id: dict.h,v 1.4 2005-08-18 15:28:18 bacon Exp $
 */

#ifndef _XP_STX_DICT_H_
#define _XP_STX_DICT_H_

#include <xp/stx/stx.h>

#define XP_STX_ASSOCIATION_SIZE  2
#define XP_STX_ASSOCIATION_KEY   0
#define XP_STX_ASSOCIATION_VALUE 1

struct xp_stx_association_t
{
	xp_stx_objhdr_t header;
	xp_word_t key;
	xp_word_t value;
};

typedef struct xp_stx_association_t xp_stx_association_t;

#ifdef __cplusplus
extern "C"
#endif

xp_word_t xp_stx_dict_lookup (
        xp_stx_t* stx, xp_word_t dict, const xp_char_t* key);
xp_word_t xp_stx_dict_get (
        xp_stx_t* stx, xp_word_t dict, xp_word_t key);
xp_word_t xp_stx_dict_put (
        xp_stx_t* stx, xp_word_t dict, xp_word_t key, xp_word_t value);
void xp_stx_dict_traverse (
        xp_stx_t* stx, xp_word_t dict,
        void (*func) (xp_stx_t*,xp_word_t,void*), void* data);


#ifdef __cplusplus
}
#endif

#endif
