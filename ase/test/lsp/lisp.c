#include <xp/lisp/lisp.h>
#include <xp/c/stdio.h>
#include <xp/c/ctype.h>

#ifdef LINUX
#include <mcheck.h>
#endif

static int get_char (xp_lisp_cint* ch, void* arg)
{
	xp_lisp_cint c;
   
	c = xp_fgetc(stdin);
	if (c == XP_EOF) {
		if (xp_ferror(stdin)) return -1;
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

int xp_main (int argc, xp_char_t* argv[])
{
	xp_lisp_t* lisp;
	xp_lisp_obj_t* obj;

#ifdef LINUX
	mtrace ();
#endif
	if (argc != 3) {
		xp_fprintf (xp_stderr, XP_TEXT("usage: %ls mem_ubound mem_ubound_inc\n"), argv[0]);
		return -1;
	}

	lisp = xp_lisp_new (to_int(argv[1]), to_int(argv[2]));
	if (lisp == NULL) {
		xp_fprintf (xp_stderr, XP_TEXT("can't create a lisp instance\n"));
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

#ifdef LINUX
	muntrace ();
#endif
	return 0;
}


