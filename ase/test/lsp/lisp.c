#include <xp/lisp/lisp.h>
#include <xp/c/stdio.h>
#include <xp/c/ctype.h>
#include <xp/c/stdcli.h>

#ifdef __linux
#include <mcheck.h>
#endif

static int get_char (xp_cint_t* ch, void* arg)
{
	xp_cint_t c;
   
	c = xp_fgetc(xp_stdin);
	if (c == XP_EOF) {
		if (xp_ferror(xp_stdin)) return -1;
		c = XP_EOF;
	}

	*ch = c;
	return 0;
}

int to_int (const xp_char_t* str)
{
	int r = 0;

	while (*str != XP_CHAR('\0')) {
		if (!xp_isdigit(*str))	break;
		r = r * 10 + (*str - XP_CHAR('0'));
		str++;
	}

	return r;
}

#include <locale.h>

int handle_cli_error (
	const xp_cli_t* cli, int code, 
	const xp_char_t* name, const xp_char_t* value)
{
	xp_printf (XP_TEXT("usage: %s /memory=nnn /increment=nnn\n"), cli->verb);

	if (code == XP_CLI_ERROR_INVALID_OPTNAME) {
		xp_printf (XP_TEXT("unknown option - %s\n"), name);
	}
	else if (code == XP_CLI_ERROR_MISSING_OPTNAME) {
		xp_printf (XP_TEXT("missing option - %s\n"), name);
	}
	else if (code == XP_CLI_ERROR_REDUNDANT_OPTVAL) {
		xp_printf (XP_TEXT("redundant value %s for %s\n"), value, name);
	}
	else if (code == XP_CLI_ERROR_MISSING_OPTVAL) {
		xp_printf (XP_TEXT("missing value for %s\n"), name);
	}
	else if (code == XP_CLI_ERROR_MEMORY) {
		xp_printf (XP_TEXT("memory error in processing %s\n"), name);
	}
	else {
		xp_printf (XP_TEXT("error code: %d\n"), code);
	}

	return -1;
}
xp_cli_t* parse_cli (int argc, xp_char_t* argv[])
{
	static const xp_char_t* optsta[] =
	{
		XP_TEXT("/"), XP_TEXT("--"), XP_NULL
	};

	static xp_cliopt_t opts[] =
	{
		{ XP_TEXT("memory"), XP_CLI_OPTNAME | XP_CLI_OPTVAL },
        { XP_TEXT("increment"), XP_CLI_OPTNAME | XP_CLI_OPTVAL },
        { XP_NULL, 0 }
    };

	static xp_cli_t cli =
	{
		handle_cli_error,
		optsta,
		XP_TEXT("="),
		opts
	};

	if (xp_parsecli (argc, argv, &cli) == -1) return XP_NULL;
	return &cli;
}

int xp_main (int argc, xp_char_t* argv[])
{
	xp_lisp_t* lisp;
	xp_lisp_obj_t* obj;
	xp_cli_t* cli;
	int mem, inc;

#ifdef __linux
	mtrace ();
#endif

setlocale (LC_ALL, "");

	if ((cli = parse_cli (argc, argv)) == XP_NULL) return -1;
	mem = to_int(xp_getclioptval(cli, XP_TEXT("memory")));
	inc = to_int(xp_getclioptval(cli, XP_TEXT("increment")));
	xp_clearcli (cli);

	if (mem <= 0) {
		xp_fprintf (xp_stderr,
			XP_TEXT("error: invalid memory size given\n"));
		return -1;
	}

	lisp = xp_lisp_new (mem, inc);
	if (lisp == NULL) {
		xp_fprintf (xp_stderr, 
			XP_TEXT("error: cannot create a lisp instance\n"));
		return -1;
	}

	xp_printf (XP_TEXT("LISP 0.0001\n"));

	xp_lisp_set_creader (lisp, get_char, NULL);

	for (;;) {
		xp_printf (XP_TEXT("%s> "), argv[0]);

		obj = xp_lisp_read (lisp);
		if (obj == NULL) {
			if (lisp->error != XP_LISP_ERR_END && 
			    lisp->error != XP_LISP_ERR_ABORT) {
				xp_fprintf (xp_stderr, 
					XP_TEXT("error while reading: %d\n"), lisp->error);
			}

			if (lisp->error < XP_LISP_ERR_SYNTAX) break;
			continue;
		}

		if ((obj = xp_lisp_eval (lisp, obj)) != NULL) {
			xp_lisp_print (lisp, obj);
			xp_printf (XP_TEXT("\n"));
		}
		else {
			if (lisp->error == XP_LISP_ERR_ABORT) break;
			xp_fprintf (xp_stderr, 
				XP_TEXT("error while reading: %d\n"), lisp->error);
		}

		/*
		printf ("-----------\n");
		xp_lisp_print (lisp, obj);
		printf ("\n-----------\n");
		*/
	}

	xp_lisp_free (lisp);

#ifdef __linux
	muntrace ();
#endif
	return 0;
}


