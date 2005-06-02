/*
 * $Id: parser.h,v 1.7 2005-06-02 16:14:58 bacon Exp $
 */

#ifndef _XP_STX_PARSER_H_
#define _XP_STX_PARSER_H_

#include <xp/stx/stx.h>
#include <xp/stx/lexer.h>

struct xp_stx_parser_t
{
	xp_stx_lexer_t lexer;
	int error_code;
	xp_bool_t __malloced;
};

typedef struct xp_stx_parser_t xp_stx_parser_t;

#ifdef __cplusplus
extern "C" {
#endif

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser);
void xp_stx_parser_close (xp_stx_parser_t* parser);

int xp_stx_parse_method (xp_stx_parser_t* parser, 
	xp_stx_word_t method_class, xp_stx_char_t* method_text);

#ifdef __cplusplus
}
#endif

#endif
