/*
 * $Id: parser.c,v 1.7 2005-05-30 15:24:12 bacon Exp $
 */

#include <xp/stx/parser.h>
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

int xp_stx_parser_parse (xp_stx_parser_t* parser)
{
	parser->token = xp_stx_lexer_consume (&parser->lexer);
	if (parser->token == XP_NULL) {
		/*parser->error_code = xxx;*/
		return -1;
	}

	return 0;
}


