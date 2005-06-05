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
	const xp_stx_char_t* text;
	xp_size_t size;
	xp_size_t index;
};

typedef struct ss_t ss_t;

int ss_input (void* owner, int cmd, void* arg)
{
	ss_t* ss = (ss_t*)owner;

	if (cmd == XP_STX_PARSER_INPUT_OPEN) {
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_CLOSE) {
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_CONSUME) {
		if (ss->index < ss->size) *c = ss->text[ss->index++];
		else *c = XP_STX_CHAR_EOF;
	}
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

	parser.input_func = ss_func;

	{
		/*
		ss_t ss = {
			XP_STX_TEXT("isNil\n^true"),
			11,	
			0
		};
		*/
		xp_stx_parser_parse_method (&parser, 0, &ss);
	}

	xp_stx_parser_close (&parser);
	xp_printf (XP_TEXT("== End of program ==\n"));
	return 0;
}

