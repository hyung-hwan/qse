/*
 * $Id: parser.c,v 1.11 2005-06-04 16:27:30 bacon Exp $
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

	if (xp_stx_token_open (&parser->token, 256) == XP_NULL) {
		if (parser->__malloced) xp_stx_free (parser);
		return XP_NULL;
	}

	parser->error_code = 0;
	parser->text = XP_NULL;
	parser->curc = XP_CHAR_EOF;
	parser->curp = XP_NULL;

	return parser;
}

void xp_stx_parser_close (xp_stx_parser_t* parser)
{
	xp_stx_token_close (&parser->token);
	if (parser->__malloced) xp_stx_free (parser);
}

int xp_stx_parser_parse_method (xp_stx_parser_t* parser, 
	xp_stx_word_t method_class, xp_stx_char_t* method_text)
{
	xp_stx_assert (method_text != XP_NULL);

	parser->error_code = 0;
	parser->text = method_text;
	parser->curp = method_text;
	NEXT_CHAR (parser);

	return 0;
}

static int __get_token (xp_stx_parser_t* parser)
{
	xp_cint_t c = parser->curc;

	__skip_spaces (parser);
	__skip_comment (parser);

	switch (c) {
	case 
	}

	return -1;
}

static int __get_char (xp_stx_parser_t* parser)
{
	xp_cint_t c = parser->curp;

	if (c == XP_STX_CHAR('\0')) {
		parser->curc = XP_EOF_CHAR;
	}
	else {
		parser->curc = c;
		parser->curp++;
	}

	return 0;
}

static int __skip_spaces (xp_stx_parser_t* parser)
{
	while (xp_stx_isspace(parser->curc)) __get_char
}
