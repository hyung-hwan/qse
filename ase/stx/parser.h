/*
 * $Id: parser.h,v 1.1 2005-05-12 15:49:07 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

int xp_stx_parse (
	xp_stx_t* stx, xp_stx_word_t method, const xp_char_t* text);

#ifdef __cplusplus
}
#endif

#endif
