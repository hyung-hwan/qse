/*
 * $Id: misc.h,v 1.1 2005-05-15 18:37:00 bacon Exp $
 */

#ifndef _XP_STX_MISC_H_
#define _XP_STX_MISC_H_

#include <xp/stx/stx.h>

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_word_t xp_stx_strlen (const xp_stx_char_t* str);

int xp_stx_strcmp (
	const xp_stx_char_t* s1, const xp_stx_char_t* s2);
int xp_stx_strxcmp (
	const xp_stx_char_t* s1, xp_stx_word_t len, const xp_stx_char_t* s2);

xp_stx_word_t xp_stx_strhash (const xp_stx_char_t* str);
xp_stx_word_t xp_stx_strxhash (const xp_stx_char_t* str, xp_stx_word_t len);

#ifdef __cplusplus
}
#endif

#endif
