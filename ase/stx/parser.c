/*
 * $Id: parser.c,v 1.1 2005-05-12 15:49:07 bacon Exp $
 */

#include <xp/stx/parser.h>

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

int xp_stx_parse (
	xp_stx_t* stx, xp_stx_word_t method, const xp_char_t* text)
{
	return 0;
}

