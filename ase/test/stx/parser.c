#include <xp/stx/stx.h>

#ifdef _DOS
	#include <stdio.h>
	#define xp_printf printf
#else
	#include <xp/bas/stdio.h>
	#include <xp/bas/locale.h>
#endif

#include <xp/stx/parser.h>

struct ss_t
{
	xp_stx_char_t* text;
	xp_size_t size;
	xp_size_t index;
};

typedef struct ss_t ss_t;

int ss_reset (xp_stx_parser_t* parser)
{
	return 0;
}

int ss_consume (xp_stx_parser_t* parser, xp_stx_cint_t* c)
{
	ss_t* ss = (ss_t*)parser->input;
	if (ss->index < ss->size) *c = ss->text[ss->index++];
	else *c = XP_STX_CHAR_EOF;
	return 0;
}

int xp_main (int argc, xp_char_t* argv[])
{
	xp_stx_parser_t parser;
	xp_stx_word_t i;

#ifndef _DOS
	if (xp_setlocale () == -1) {
		printf ("cannot set locale\n");
		return -1;
	}
#endif

	if (xp_stx_parser_open(&parser) == XP_NULL) {
		xp_printf (XP_TEXT("cannot open parser\n"));
		return -1;
	}

	parser.input_reset = ss_reset;
	parser.input_consume = ss_consume;

	xp_stx_parser_close (&parser);
	xp_printf (XP_TEXT("== End of program ==\n"));
	return 0;
}

