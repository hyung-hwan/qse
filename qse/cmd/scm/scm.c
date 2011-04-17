/*
 * $Id: scm.c,v 1.5 2007/05/16 09:15:14 bacon Exp $
 */

#include <qse/scm/scm.h>

#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/opt.h>

#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>

#include <string.h>
#include <stdlib.h>

static qse_ssize_t get_input (
	qse_scm_t* scm, qse_scm_io_cmd_t cmd, 
	qse_scm_io_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	switch (cmd) 
	{
		case QSE_SCM_IO_OPEN:
			arg->handle = stdin;
			return 1;

		case QSE_SCM_IO_CLOSE:
			return 0;

		case QSE_SCM_IO_READ:
		{
			qse_cint_t c;

			if (size <= 0) return -1;
			c = qse_fgetc ((FILE*)arg->handle);

			if (c == QSE_CHAR_EOF) 
			{
				if (ferror((FILE*)arg->handle)) return -1;
				return 0;
			}

			data[0] = c;
			return 1;
		}

		default:
			return -1;
	}
}

static qse_ssize_t put_output (
	qse_scm_t* scm, qse_scm_io_cmd_t cmd, 
	qse_scm_io_arg_t* arg, qse_char_t* data, qse_size_t size)
{
	switch (cmd) 
	{
		case QSE_SCM_IO_OPEN:
			arg->handle = stdout;
			return 1;

		case QSE_SCM_IO_CLOSE:
			return 0;

		case QSE_SCM_IO_WRITE:
		{
			int n = qse_fprintf (
				(FILE*)arg->handle, QSE_T("%.*s"), size, data);
			if (n < 0) return -1;

			return size;
		}

		default:
			return -1;
	}
}

static int opt_memsize = 1000;
static int opt_meminc = 1000;

static void print_usage (const qse_char_t* argv0)
{
	qse_fprintf (QSE_STDERR, 
		QSE_T("Usage: %s [options]\n"), argv0);
	qse_fprintf (QSE_STDERR, 
		QSE_T("  -h          print this message\n"));
	qse_fprintf (QSE_STDERR, 
		QSE_T("  -m integer  number of memory cells\n"));
	qse_fprintf (QSE_STDERR, 
		QSE_T("  -i integer  number of memory cell increments\n"));
}

static int handle_args (int argc, qse_char_t* argv[])
{
	qse_opt_t opt;
	qse_cint_t c;

	qse_memset (&opt, 0, QSE_SIZEOF(opt));
	opt.str = QSE_T("hm:i:");

	while ((c = qse_getopt (argc, argv, &opt)) != QSE_CHAR_EOF)
	{
		switch (c)
		{
			case QSE_T('h'):
				print_usage (argv[0]);
				return -1;

			case QSE_T('m'):
				opt_memsize = qse_strtoi(opt.arg);
				break;

			case QSE_T('i'):
				opt_meminc = qse_strtoi(opt.arg);
				break;

			case QSE_T('?'):
				qse_fprintf (QSE_STDERR, QSE_T("ERROR: illegal option - %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;

			case QSE_T(':'):
				qse_fprintf (QSE_STDERR, QSE_T("ERROR: missing argument for %c\n"), opt.opt);
				print_usage (argv[0]);
				return -1;
		}
	}

	if (opt.ind < argc)
	{
		qse_printf (QSE_T("ERROR: redundant argument - %s\n"), argv[opt.ind]);
		print_usage (argv[0]);
		return -1;
	}

	if (opt_memsize <= 0)
	{
		qse_printf (QSE_T("ERROR: invalid memory size given\n"));
		return -1;
	}
	return 0;
}

#include <qse/cmn/pio.h>
static int pio1 (const qse_char_t* cmd, int oflags, qse_pio_hid_t rhid)
{
	qse_pio_t* pio;
	int x;

	pio = qse_pio_open (
		QSE_NULL,
		0,
		cmd,
		oflags
	);
	if (pio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	while (1)
	{
		qse_byte_t buf[128];
		qse_ssize_t i;

		/*qse_pio_canread (pio, QSE_PIO_ERR, 1000)*/
		qse_ssize_t n = qse_pio_read (pio, buf, sizeof(buf), rhid);
		if (n == 0) break;
		if (n <= -1)
		{
			qse_printf (
				QSE_T("qse_pio_read() returned error - %s\n"),
				qse_pio_geterrmsg(pio)
			);
			break;
		}	

		qse_printf (QSE_T("N===> %d buf => ["), (int)n);
		for (i = 0; i < n; i++)
		{
		#ifdef QSE_CHAR_IS_MCHAR
			qse_printf (QSE_T("%c"), buf[i]);
		#else
			qse_printf (QSE_T("%C"), buf[i]);
		#endif
		}	
		qse_printf (QSE_T("]\n"));
	}

	x = qse_pio_wait (pio);
	qse_printf (QSE_T("qse_pio_wait returns %d\n"), x);
	if (x <= -1)
	{
		qse_printf (QSE_T("error code : %d, error string: %s\n"),
			(int)qse_pio_geterrnum(pio), qse_pio_geterrmsg(pio));
	}

	qse_pio_close (pio);

	return 0;
}

int scm_main (int argc, qse_char_t* argv[])
{
	qse_scm_t* scm;
	qse_scm_ent_t* obj;

	if (handle_args (argc, argv) == -1) return -1;
	
	scm = qse_scm_open (QSE_NULL, 0, opt_memsize, opt_meminc);
	if (scm == QSE_NULL) 
	{
		qse_printf (QSE_T("ERROR: cannot create a scm instance\n"));
		return -1;
	}

	qse_printf (QSE_T("QSESCM 0.0001\n"));

	{
		qse_scm_io_t io = { get_input, put_output };
		qse_scm_attachio (scm, &io);
	}



{
   int i;
   for (i = 0; i<2; i++) 
#if defined(_WIN32)
pio1 (QSE_T("c:\\winnt\\system32\\netstat.exe -an"), QSE_PIO_READOUT|QSE_PIO_WRITEIN|/*QSE_PIO_SHELL|*/QSE_PIO_DROPERR, QSE_PIO_OUT);   
#elif defined(__OS2__)
pio1 (QSE_T("pstat.exe /c"), QSE_PIO_READOUT|QSE_PIO_WRITEIN|/*QSE_PIO_SHELL|*/QSE_PIO_DROPERR, QSE_PIO_OUT);   
#else
pio1 (QSE_T("ls -laF"), QSE_PIO_READOUT|QSE_PIO_WRITEIN|/*QSE_PIO_SHELL|*/QSE_PIO_DROPERR, QSE_PIO_OUT);   
#endif
}


{
	qse_printf (QSE_T("%d\n"), (int)qse_strspn (QSE_T("abcdefg"), QSE_T("cdab")));
	qse_printf (QSE_T("%d\n"), (int)qse_strcspn (QSE_T("abcdefg"), QSE_T("fg")));
	qse_printf (QSE_T("%s\n"), qse_strpbrk (QSE_T("abcdefg"), QSE_T("fb")));

	qse_printf (QSE_T("%s\n"), qse_strrcasestr (QSE_T("fbFBFBFBxyz"), QSE_T("fb")));
	qse_printf (QSE_T("%s\n"), qse_strcasestr (QSE_T("fbFBFBFBxyz"), QSE_T("fb")));

	qse_printf (QSE_T("%s\n"), qse_strword (QSE_T("ilove lov LOVE love"), QSE_T("love")));
	qse_printf (QSE_T("%s\n"), qse_strcaseword (QSE_T("ilove lov LOVE love"), QSE_T("love")));
	qse_printf (QSE_T("%s\n"), qse_strxword (QSE_T("ilove love you"), 14, QSE_T("love")));
}

{
	qse_char_t str[256];
	qse_size_t len;

	qse_strcpy (str, QSE_T("what a Wonderful WORLD"));
	len = qse_strlwr(str);
	qse_printf (QSE_T("%d %s\n"), (int)len, str);
	len = qse_strupr(str);
	qse_printf (QSE_T("%d %s\n"), (int)len, str);
}

{
	qse_printf (QSE_T("sizeof(int) = %d\n"), (int)sizeof(int));
	qse_printf (QSE_T("sizeof(long) = %d\n"), (int)sizeof(long));
	qse_printf (QSE_T("sizeof(long long) = %d\n"), (int)sizeof(long long));
	qse_printf (QSE_T("sizeof(float) = %d\n"), (int)sizeof(float));
	qse_printf (QSE_T("sizeof(double) = %d\n"), (int)sizeof(double));
	qse_printf (QSE_T("sizeof(long double) = %d\n"), (int)sizeof(long double));
	qse_printf (QSE_T("sizeof(void*) = %d\n"), (int)sizeof(void*));
	qse_printf (QSE_T("sizeof(wchar_t) = %d\n"), (int)sizeof(wchar_t));
}

{
	const qse_char_t* x = QSE_T("rate:num,burst:num,mode:abc,name:xxx,size:num,max:num,expire:num,gcinterval:num");

	const qse_char_t* p = x;
	qse_cstr_t tok;

	while (p)
	{
		p = qse_strtok (p, QSE_T(","), &tok);
		qse_printf (QSE_T("[%.*s]\n"), (int)tok.len, tok.ptr);
	}
}

{
	qse_char_t abc[100];

	qse_strcpy (abc, QSE_T("abcdefghilklmn"));
	qse_strrev (abc);
	qse_printf (QSE_T("<%s>\n"), abc);
	qse_strrot (abc, -1, 5);
	qse_printf (QSE_T("<%s>\n"), abc);
}

{
	qse_char_t abc[100];

	qse_strcpy (abc, QSE_T("abcdefghilklmnabcdefghik"));
	qse_printf (QSE_T("ORIGINAL=><%s>\n"), abc);
	qse_strexcl (abc, QSE_T("adfikl"));
	qse_printf (QSE_T("AFTER EXCL<%s>\n"), abc);
	qse_strincl (abc, QSE_T("bcmn"));
	qse_printf (QSE_T("AFTER INCL<%s>\n"), abc);
}

{
qse_scm_ent_t* x1, * x2;

qse_printf (QSE_T("QSESCM> "));
x1 = qse_scm_read (scm);
if (x1 == QSE_NULL)
{
	qse_printf (QSE_T("ERROR: %s\n"), qse_scm_geterrmsg(scm));
}
else
{
	x2 = qse_scm_eval (scm, x1);
	if (x2 == QSE_NULL)
	{
		qse_printf (QSE_T("ERROR: %s ...\n   "), qse_scm_geterrmsg(scm));
		qse_scm_print (scm, x1);
		qse_printf (QSE_T("\n"));
		
	}
	else
	{
		qse_printf (QSE_T("Evaluated...\n   "));
		qse_scm_print (scm, x1);
		qse_printf (QSE_T("\nTo...\n   "));
		qse_scm_print (scm, x2);
		qse_printf (QSE_T("\n"));
	}

}

}

#if 0
	while (1)
	{
		qse_printf (QSE_T("QSESCM $ "));
		qse_fflush (stdout);

qse_scm_gc (scm);

		obj = qse_scm_read (scm);
		if (obj == QSE_NULL) 
		{
			qse_scm_errnum_t errnum;
			qse_scm_loc_t errloc;
			const qse_char_t* errmsg;

			qse_scm_geterror (scm, &errnum, &errmsg, &errloc);

			if (errnum != QSE_SCM_EEND && 
			    errnum != QSE_SCM_EEXIT) 
			{
				qse_printf (
					QSE_T("error in read: [%d] %s at line %d column %d\n"), 
					errnum, errmsg, (int)errloc.line, (int)errloc.colm);
			}

			/* TODO: change the following check */
			if (errnum < QSE_SCM_ESYNTAX) break; 
			continue;
		}

		if ((obj = qse_scm_eval (scm, obj)) != QSE_NULL) 
		{
			qse_scm_print (scm, obj);
			qse_printf (QSE_T("\n"));
		}
		else 
		{
			qse_scm_errnum_t errnum;
			qse_scm_loc_t errloc;
			const qse_char_t* errmsg;

			qse_scm_geterror (scm, &errnum, &errmsg, &errloc);
			if (errnum == QSE_SCM_EEXIT) break;

			qse_printf (
				QSE_T("error in eval: [%d] %s at line %d column %d\n"), 
				errnum, errmsg, (int)errloc.line, (int)errloc.colm);
		}
	}
#endif

	qse_scm_close (scm);

	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, scm_main);
}
