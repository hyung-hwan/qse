/*
 * $Id: parser.c,v 1.4 2005-05-23 14:43:03 bacon Exp $
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

	return parser;
}

void xp_stx_parser_close (xp_stx_parser_t* parser)
{
	if (parser->__malloced) xp_stx_free (parser);
}
/*

static void __emit_code (
	xp_stx_t* stx, xp_stx_word_t method, int value)
{
}

static void __emit_instruction (
	xp_stx_t* stx, xp_stx_word_t method, int high, int low)
{
	if (low >= 16) {
		__emit_instruction (stx, method, Extended, high);
		__emit_code (low);
	}
	else __emit_code (high * 16 + low);
}
*/

int xp_stx_parser_parse_method (xp_stx_parser_t* parser, const xp_char_t* text)
{
	return 0;
}


int xp_stx_filein_raw (xp_stx_t* stx, xp_stx_getc_t getc)
{
/*
	xp_cint_t c;

	getc()
	gettoken ();
	if (token->type == XP_STX_TOKEN_IDENT) {
		ident: 
	}
*/

	return 0;
}

int xp_stx_get_token ()
{
/*	getc	*/
}
