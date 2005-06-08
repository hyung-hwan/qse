/*
 * $Id: extra.h,v 1.2 2005-06-08 16:00:51 bacon Exp $
 */

#ifndef _XP_STX_EXTRA_H_
#define _XP_STX_EXTRA_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_word_t xp_stx_strlen (const xp_char_t* str)

int xp_stx_strcmp (
	const xp_char_t* s1, const xp_char_t* s2);
int xp_stx_strxcmp (
	const xp_char_t* s1, xp_word_t len, const xp_char_t* s2);

xp_word_t xp_stx_strhash (const xp_char_t* str);
xp_word_t xp_stx_strxhash (const xp_char_t* str, xp_word_t len);

#ifdef __cplusplus
}
#endif

#endif
