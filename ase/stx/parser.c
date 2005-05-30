/*
 * $Id: parser.c,v 1.8 2005-05-30 15:55:06 bacon Exp $
 */

#include <xp/stx/parser.h>
#include <xp/stx/token.h>
#include <xp/stx/misc.h>

xp_stx_parser_t* xp_stx_parser_open (xp_stx_parser_t* parser)
{
	if (parser == XP_NULL) {
		parser = (xp_stx_parser_t*)
			xp_stx_malloc (xp_sizeof(xp_stx_parser_t));		
		if (parser == XP_NULL) return XP_NULL;
		parser->__malloced = xp_true;
	}
	else parser->__malloced = xp_false;

	if (xp_stx_lexer_open (&parser->lexer) == XP_NULL) {
		if (parser->__malloced) xp_stx_free (parser);
		return XP_NULL;
	}

	parser->token = XP_NULL;
	parser->error_code = 0;
	return parser;
}

void xp_stx_parser_close (xp_stx_parser_t* parser)
{
	xp_stx_lexer_close (&parser->lexer);
	if (parser->__malloced) xp_stx_free (parser);
}

/* set input stream */
/*
int xp_stx_parser_set_ios (xp_stx_parser_t* parser, input_func)
{
}

int xp_stx_parser_parse_filein (xp_stx_parser_t* parser)
{
	xp_stx_token_t* token;

	token = xp_stx_lexer_consume (&parser->lexer);
	if (token == XP_NULL) return -1;

	if (token->type == XP_STX_TOKEN_EXCLM) 
}
*/

int xp_stx_parser_parse_method (
	xp_stx_parser_t* parser, xp_stx_word_t class, xp_stx_char_t* text)
{
	xp_stx_lexer_reset (&parser->lexer, text);

	parser->token = xp_stx_lexer_consume (&parser->lexer);
	if (parser->token == XP_NULL) {
		/*parser->error_code = xxx;*/
		return -1;
	}

	return 0;
}
