/*
 * $Id: parser.c,v 1.6 2005-05-30 07:38:25 bacon Exp $
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

