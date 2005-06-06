#include <xp/stx/stx.h>

#ifdef _DOS
	#include <stdio.h>
	#define xp_printf printf
#else
	#include <xp/bas/stdio.h>
	#include <xp/bas/locale.h>
#endif

#include <xp/stx/parser.h>

#ifdef __linux
#include <mcheck.h>
#endif

struct ss_t
{
	const xp_stx_char_t* text;
	xp_size_t index;
};

typedef struct ss_t ss_t;

int ss_func (int cmd, void* owner, void* arg)
{

	if (cmd == XP_STX_PARSER_INPUT_OPEN) {
		ss_t* ss = *(ss_t**)owner;
		ss->text = (const xp_stx_char_t*)arg;
		ss->index = 0;
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_CLOSE) {
		/*ss_t* ss = (ss_t*)owner; */
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_CONSUME) {
		ss_t* ss = (ss_t*)owner;
		xp_cint_t* c = (xp_cint_t*)arg;
		if (ss->text[ss->index] == XP_STX_CHAR('\0')) {
			*c = XP_STX_CHAR_EOF;
		}
		else *c = ss->text[ss->index++];
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_REWIND) {
		return 0;
	}
	return -1;
}

struct stdio_t
{
	XP_FILE* stdio;
};

typedef struct stdio_t stdio_t;


int stdio_func (int cmd, void* owner, void* arg)
{

	if (cmd == XP_STX_PARSER_INPUT_OPEN) {
		stdio_t* p = *(stdio_t**)owner;
		p->stdio = xp_fopen ((const xp_char_t*)arg, XP_TEXT("r"));
		if (p->stdio == XP_NULL) return -1;
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_CLOSE) {
		stdio_t* p = (stdio_t*)owner;
		xp_fclose (p->stdio);
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_CONSUME) {
		stdio_t* p = (stdio_t*)owner;
		xp_cint_t* c = (xp_cint_t*)arg;
		if (xp_feof(p->stdio)) {
			*c = XP_STX_CHAR_EOF;
		}
		else {
			xp_cint_t t = xp_fgetc (p->stdio);	
			if (t == XP_CHAR_EOF) {
				if (xp_ferror (p->stdio)) return -1;
				*c = XP_STX_CHAR_EOF;
			}
			else *c = t;
		}	
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_REWIND) {
		return 0;
	}
	return -1;
}

int xp_main (int argc, xp_char_t* argv[])
{
	xp_stx_parser_t parser;
	xp_stx_word_t i;

#ifdef __linux
	mtrace ();
#endif

/*
#ifndef _DOS
	if (xp_setlocale () == -1) {
		printf ("cannot set locale\n");
		return -1;
	}
#endif
*/


	if (xp_stx_parser_open(&parser) == XP_NULL) {
		xp_printf (XP_TEXT("cannot open parser\n"));
		return -1;
	}

	{
	/*
		ss_t ss;
		parser.input_owner = (void*)&ss;
		parser.input_func = ss_func;
		xp_stx_parser_parse_method (&parser, 0, 
			XP_TEXT("isNil\n^true"));
	*/
		stdio_t stdio;
		parser.input_owner = (void*)&stdio;
		parser.input_func = stdio_func;
		if (xp_stx_parser_parse_method (&parser, 0, 
			(void*)XP_TEXT("test.st")) == -1) {
			xp_printf (XP_TEXT("parser error\n"));
		}
	}

	xp_stx_parser_close (&parser);
	xp_printf (XP_TEXT("== End of program ==\n"));

#ifdef __linux
	muntrace ();
#endif
	
#ifdef __linux
	{
		char buf[1000];
		snprintf (buf, sizeof(buf), "ls -l /proc/%u/fd", getpid());
		system (buf);
	}
#endif
	return 0;
}

