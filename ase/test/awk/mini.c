/*
 * $Id: mini.c,v 1.1.1.1 2007/03/28 14:05:29 bacon Exp $
 */

#include <ase/awk/awk.h>

#include <ase/cmn/str.h>
#include <ase/cmn/mem.h>

#include <ase/utl/ctype.h>
#include <ase/utl/stdio.h>
#include <ase/utl/main.h>

#include <stdarg.h>
#include <math.h>
#include <stdlib.h>

struct awk_src_io
{
	const ase_char_t* file;
	FILE* handle;
};

static const ase_char_t* data_file = ASE_NULL;

#if defined(vms) || defined(__vms)
/* it seems that the main function should be placed in the main object file
 * in OpenVMS. otherwise, the first function in the main object file seems
 * to become the main function resulting in program start-up failure. */
#include <ase/utl/main.c>
#endif

#ifndef NDEBUG
void ase_assert_abort (void)
{
	abort ();
}

void ase_assert_printf (const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vprintf (fmt, ap);
	va_end (ap);
}
#endif

/* custom memory management function */
void* awk_malloc (void* custom, ase_size_t n) { return malloc (n); }
void* awk_realloc (void* custom, void* ptr, ase_size_t n) { return realloc (ptr, n); }
void awk_free (void* custom, void* ptr) { free (ptr); }

/* custom character class functions */
ase_bool_t awk_isupper (void* custom, ase_cint_t c) { return ase_isupper (c); }
ase_bool_t awk_islower (void* custom, ase_cint_t c) { return ase_islower (c); }
ase_bool_t awk_isalpha (void* custom, ase_cint_t c) { return ase_isalpha (c); }
ase_bool_t awk_isdigit (void* custom, ase_cint_t c) { return ase_isdigit (c); }
ase_bool_t awk_isxdigit (void* custom, ase_cint_t c) { return ase_isxdigit (c); }
ase_bool_t awk_isalnum (void* custom, ase_cint_t c) { return ase_isalnum (c); }
ase_bool_t awk_isspace (void* custom, ase_cint_t c) { return ase_isspace (c); }
ase_bool_t awk_isprint (void* custom, ase_cint_t c) { return ase_isprint (c); }
ase_bool_t awk_isgraph (void* custom, ase_cint_t c) { return ase_isgraph (c); }
ase_bool_t awk_iscntrl (void* custom, ase_cint_t c) { return ase_iscntrl (c); }
ase_bool_t awk_ispunct (void* custom, ase_cint_t c) { return ase_ispunct (c); }
ase_cint_t awk_toupper (void* custom, ase_cint_t c) { return ase_toupper (c); }
ase_cint_t awk_tolower (void* custom, ase_cint_t c) { return ase_tolower (c); }

/* custom miscellaneous functions */
ase_real_t awk_pow (void* custom, ase_real_t x, ase_real_t y) 
{
	return pow (x, y); 
}

int awk_sprintf (void* custom, ase_char_t* buf, ase_size_t size, const ase_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

void awk_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

/* source input handler */
ase_ssize_t awk_srcio_in (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	struct awk_src_io* src_io = (struct awk_src_io*)arg;
	ase_cint_t c;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (src_io->file == ASE_NULL) return 0;
		src_io->handle = ase_fopen (src_io->file, ASE_T("r"));
		if (src_io->handle == NULL) return -1;
		return 1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		if (src_io->file == ASE_NULL) return 0;
		fclose ((FILE*)src_io->handle);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		if (size <= 0) return -1;
		c = ase_fgetc ((FILE*)src_io->handle);
		if (c == ASE_CHAR_EOF) return 0;
		*data = (ase_char_t)c;
		return 1;
	}

	return -1;
}

/* external i/o handler for pipe */
ase_ssize_t awk_extio_pipe (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	switch (cmd)
	{
		case ASE_AWK_IO_OPEN:
		{
			FILE* handle;
			const ase_char_t* mode;

			if (epa->mode == ASE_AWK_EXTIO_PIPE_READ)
				mode = ASE_T("r");
			else if (epa->mode == ASE_AWK_EXTIO_PIPE_WRITE)
				mode = ASE_T("w");
			else return -1;

			handle = ase_popen (epa->name, mode);
			if (handle == NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (ase_fgets (data, size, (FILE*)epa->handle) == ASE_NULL) 
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
			return ase_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
		#if defined(ASE_CHAR_IS_WCHAR) && defined(__linux)
			/* fwprintf seems to return an error with the file
			 * pointer opened by popen, as of this writing. 
			 * anyway, hopefully the following replacement 
			 * will work all the way. */
			int n = fprintf (
				(FILE*)epa->handle, "%.*ls", size, data);
		#else
			int n = ase_fprintf (
				(FILE*)epa->handle, ASE_T("%.*s"), size, data);
		#endif
			if (n < 0) return -1;

			return size;
		}

		case ASE_AWK_IO_FLUSH:
		{
			if (epa->mode == ASE_AWK_EXTIO_PIPE_READ) return -1;
			else return 0;
		}

		case ASE_AWK_IO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

/* external i/o handler for file */
ase_ssize_t awk_extio_file (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	switch (cmd)
	{
		case ASE_AWK_IO_OPEN:
		{
			FILE* handle;
			const ase_char_t* mode;

			if (epa->mode == ASE_AWK_EXTIO_FILE_READ)
				mode = ASE_T("r");
			else if (epa->mode == ASE_AWK_EXTIO_FILE_WRITE)
				mode = ASE_T("w");
			else if (epa->mode == ASE_AWK_EXTIO_FILE_APPEND)
				mode = ASE_T("a");
			else return -1;

			handle = ase_fopen (epa->name, mode);
			if (handle == NULL) return -1;

			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			fclose ((FILE*)epa->handle);
			epa->handle = NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			if (ase_fgets (data, size, (FILE*)epa->handle) == ASE_NULL) 
			{
				if (ferror((FILE*)epa->handle)) return -1;
				return 0;
			}
			return ase_strlen(data);
		}

		case ASE_AWK_IO_WRITE:
		{
			int n = ase_fprintf (
				(FILE*)epa->handle, ASE_T("%.*s"), size, data);
			if (n < 0) return -1;

			return size;
		}

		case ASE_AWK_IO_FLUSH:
		{
			if (fflush ((FILE*)epa->handle) == EOF) return -1;
			return 0;
		}

		case ASE_AWK_IO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

/* external i/o handler for console */
ase_ssize_t awk_extio_console (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (epa->mode == ASE_AWK_EXTIO_CONSOLE_READ)
		{
			FILE* fp = ase_fopen (data_file, ASE_T("r"));
			if (fp == ASE_NULL) return -1;

			if (ase_awk_setfilename (
				epa->run, data_file, ase_strlen(data_file)) == -1)
			{
				fclose (fp);
				return -1;
			}

			epa->handle = fp;

			return 1;
		}
		else if (epa->mode == ASE_AWK_EXTIO_CONSOLE_WRITE)
		{
			epa->handle = stdout;
			return 1;
		}

		return -1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		fclose ((FILE*)epa->handle);
		epa->handle = NULL;
		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		while (ase_fgets (data, size, (FILE*)epa->handle) == ASE_NULL)
		{
			if (ferror((FILE*)epa->handle)) return -1;
			return 0;
		}

		return ase_strlen(data);
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		int n = ase_fprintf ((FILE*)epa->handle, ASE_T("%.*s"), size, data);
		if (n < 0) return -1;

		return size;
	}
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		if (fflush ((FILE*)epa->handle) == EOF) return -1;
		return 0;
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		return -1;
	}

	return -1;
}

int ase_main (int argc, ase_char_t* argv[])
{
	ase_awk_t* awk;

	ase_awk_prmfns_t prmfns;
	ase_awk_srcios_t srcios;
	ase_awk_runios_t runios;

	struct awk_src_io src_io = { NULL, NULL };

	if (argc != 3)
	{
		ase_printf (ASE_T("Usage: %s source-file data-file\n"), argv[0]);
		return -1;
	}

	src_io.file = argv[1];
	data_file = argv[2];

	ase_memset (&prmfns, 0, ASE_SIZEOF(prmfns));

	prmfns.mmgr.malloc      = awk_malloc;
	prmfns.mmgr.realloc     = awk_realloc;
	prmfns.mmgr.free        = awk_free;
	prmfns.mmgr.custom_data = ASE_NULL;

	prmfns.ccls.is_upper    = awk_isupper;
	prmfns.ccls.is_lower    = awk_islower;
	prmfns.ccls.is_alpha    = awk_isalpha;
	prmfns.ccls.is_digit    = awk_isdigit;
	prmfns.ccls.is_xdigit   = awk_isxdigit;
	prmfns.ccls.is_alnum    = awk_isalnum;
	prmfns.ccls.is_space    = awk_isspace;
	prmfns.ccls.is_print    = awk_isprint;
	prmfns.ccls.is_graph    = awk_isgraph;
	prmfns.ccls.is_cntrl    = awk_iscntrl;
	prmfns.ccls.is_punct    = awk_ispunct;
	prmfns.ccls.to_upper    = awk_toupper;
	prmfns.ccls.to_lower    = awk_tolower;
	prmfns.ccls.custom_data = ASE_NULL;

	prmfns.misc.pow         = awk_pow;
	prmfns.misc.sprintf     = awk_sprintf;
	prmfns.misc.dprintf     = awk_dprintf;
	prmfns.misc.custom_data = ASE_NULL;

	if ((awk = ase_awk_open(&prmfns, ASE_NULL)) == ASE_NULL) 
	{
		ase_printf (ASE_T("ERROR: cannot open awk\n"));
		return -1;
	}

	ase_awk_setoption (awk, 
		ASE_AWK_IMPLICIT | ASE_AWK_EXPLICIT | ASE_AWK_UNIQUEFN | 
		ASE_AWK_IDIV | ASE_AWK_SHADING | ASE_AWK_SHIFT | 
		ASE_AWK_EXTIO | ASE_AWK_BLOCKLESS | ASE_AWK_STRBASEONE | 
		ASE_AWK_STRIPSPACES | ASE_AWK_NEXTOFILE);

	srcios.in = awk_srcio_in;
	srcios.out = ASE_NULL;
	srcios.custom_data = &src_io;

	if (ase_awk_parse (awk, &srcios) == -1) 
	{
		ase_printf (
			ASE_T("PARSE ERROR: CODE [%d] LINE [%u] %s\n"), 
			ase_awk_geterrnum(awk),
			(unsigned int)ase_awk_geterrlin(awk), 
			ase_awk_geterrmsg(awk));
		ase_awk_close (awk);
		return -1;
	}

	runios.pipe = awk_extio_pipe;
	runios.file = awk_extio_file;
	runios.console = awk_extio_console;
	runios.custom_data = ASE_NULL;

	if (ase_awk_run (awk, ASE_NULL, &runios, ASE_NULL, ASE_NULL, ASE_NULL) == -1)
	{
		ase_printf (
			ASE_T("RUN ERROR: CODE [%d] LINE [%u] %s\n"), 
			ase_awk_geterrnum(awk),
			(unsigned int)ase_awk_geterrlin(awk), 
			ase_awk_geterrmsg(awk));

		ase_awk_close (awk);
		return -1;
	}

	ase_awk_close (awk);
	return 0;
}
