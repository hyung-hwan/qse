#include <xp/stx/stx.h>

#ifdef _DOS
	#include <stdio.h>
	#define xp_printf printf
#else
	#include <xp/bas/stdio.h>
	#include <xp/bas/locale.h>
#endif

#include <xp/stx/parser.h>
#include <xp/stx/bootstrp.h>
#include <xp/stx/class.h>
#include <xp/stx/bytecode.h>
#include <xp/stx/interp.h>

#ifdef __linux
#include <mcheck.h>
#endif

struct ss_t
{
	const xp_char_t* text;
	xp_size_t index;
};

typedef struct ss_t ss_t;

int ss_func (int cmd, void* owner, void* arg)
{

	if (cmd == XP_STX_PARSER_INPUT_OPEN) {
		ss_t* ss = *(ss_t**)owner;
		ss->text = (const xp_char_t*)arg;
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
		if (ss->text[ss->index] == XP_CHAR('\0')) {
			*c = XP_CHAR_EOF;
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
		xp_cint_t t = xp_fgetc (p->stdio);	
		if (t == XP_CHAR_EOF) {
			if (xp_ferror (p->stdio)) return -1;
			*c = XP_CHAR_EOF;
		}
		else *c = t;
		return 0;
	}
	else if (cmd == XP_STX_PARSER_INPUT_REWIND) {
		return 0;
	}
	return -1;
}

int xp_main (int argc, xp_char_t* argv[])
{
	xp_stx_t stx;
	xp_stx_parser_t parser;

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

	if (argc != 2) {
		xp_printf (XP_TEXT("usage: %s class_name\n"), argv[0]);
		return -1;
	}

	if (xp_stx_open (&stx, 10000) == XP_NULL) {
		xp_printf (XP_TEXT("cannot open stx\n"));
		return -1;
	}

	if (xp_stx_bootstrap(&stx) == -1) {
		xp_stx_close (&stx);
		xp_printf (XP_TEXT("cannot bootstrap\n"));
		return -1;
	}


	if (xp_stx_parser_open(&parser, &stx) == XP_NULL) {
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
		xp_word_t n = xp_stx_lookup_class (&stx, argv[1]);
		xp_word_t m;

		parser.input_owner = (void*)&stdio;
		parser.input_func = stdio_func;

		if (n == stx.nil) {
			xp_printf (XP_TEXT("Cannot find class - %s\n"), argv[1]);
			goto exit_program;
		}

		/* compile the method to n's class */
		if (xp_stx_parser_parse_method (&parser, XP_STX_CLASS(&stx,n), 
			(void*)XP_TEXT("test.st")) == -1) {
			xp_printf (XP_TEXT("parser error <%s>\n"), 
				xp_stx_parser_error_string (&parser));
		}

		if (xp_stx_parser_parse_method (&parser, stx.class_symbol,
			(void*)XP_TEXT("test1.st")) == -1) {
			xp_printf (XP_TEXT("parser error <%s>\n"), 
				xp_stx_parser_error_string (&parser));
		}

		if (xp_stx_parser_parse_method (&parser, stx.class_symbol,
			(void*)XP_TEXT("test2.st")) == -1) {
			xp_printf (XP_TEXT("parser error <%s>\n"), 
				xp_stx_parser_error_string (&parser));
		}

		if (xp_stx_parser_parse_method (&parser, stx.class_string,
			(void*)XP_TEXT("test3.st")) == -1) {
			xp_printf (XP_TEXT("parser error <%s>\n"), 
				xp_stx_parser_error_string (&parser));
		}

		xp_printf (XP_TEXT("\n== Decoded Methods ==\n"));
		if (xp_stx_decode(&stx, XP_STX_CLASS(&stx,n)) == -1) {
			xp_printf (XP_TEXT("parser error <%s>\n"), 
				xp_stx_parser_error_string (&parser));
		}

		xp_printf (XP_TEXT("\n== Decoded Methods for Symbol ==\n"));
		if (xp_stx_decode(&stx, stx.class_symbol) == -1) {
			xp_printf (XP_TEXT("parser error <%s>\n"), 
				xp_stx_parser_error_string (&parser));
		}

		xp_printf (XP_TEXT("\n== Decoded Methods for String ==\n"));
		if (xp_stx_decode(&stx, stx.class_string) == -1) {
			xp_printf (XP_TEXT("parser error <%s>\n"), 
				xp_stx_parser_error_string (&parser));
		}

		xp_printf (XP_TEXT("== Running the main method ==\n"));
		m = xp_stx_lookup_method (
			&stx, XP_STX_CLASS(&stx,n), XP_TEXT("main"), xp_false);
		if (m == stx.nil) {	
			xp_printf (XP_TEXT("cannot lookup method main\n"));
		}
		else {
			xp_stx_interp (&stx, n, m);
		}
	}

exit_program:
	xp_stx_parser_close (&parser);
	xp_stx_close (&stx);
	xp_printf (XP_TEXT("== End of program ==\n"));

#ifdef __linux
	muntrace ();
#endif
	
/*
#ifdef __linux
	{
		char buf[1000];
		snprintf (buf, sizeof(buf), "ls -l /proc/%u/fd", getpid());
		system (buf);
	}
#endif
*/
	return 0;
}

