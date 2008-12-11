/*
 * $Id$
 */

#include "awk.h"
#include <ase/cmn/sio.h>

#include <math.h>
#include <ase/utl/stdio.h>

typedef struct ext_t
{
	ase_awk_prmfns_t prmfns;
} 
ext_t;

static ase_real_t custom_awk_pow (void* custom, ase_real_t x, ase_real_t y)
{
	return pow (x, y);
}

static int custom_awk_sprintf (
	void* custom, ase_char_t* buf, ase_size_t size, 
	const ase_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

ase_awk_t* ase_awk_opensimple (void)
{
	ase_awk_t* awk;
	ext_t* ext;

	awk = ase_awk_open (ASE_MMGR_GETDFL(), ASE_SIZEOF(ext_t));
	ase_awk_setccls (awk, ASE_CCLS_GETDFL());

	ext = (ext_t*)ase_awk_getextension(awk);
	ext->prmfns.pow     = custom_awk_pow;
	ext->prmfns.sprintf = custom_awk_sprintf;
	ext->prmfns.data    = ASE_NULL;
	ase_awk_setprmfns (awk, &ext->prmfns);

	ase_awk_setoption (awk, 
		ASE_AWK_IMPLICIT | ASE_AWK_EXTIO | ASE_AWK_NEWLINE | 
		ASE_AWK_BASEONE | ASE_AWK_PABLOCK);

	/*ase_awk_addfunction ();*/

	return awk;
}

/*** PARSESIMPLE ***/

typedef struct sf_t sf_t;

struct sf_t
{
	struct 
	{
		const ase_char_t*const* files; 
		ase_size_t              count;  /* the number of files */
		ase_size_t              index;  /* current file index */
		ase_sio_t*              handle; /* the handle to an open file */
	} in;

	struct
	{
		const ase_char_t*       file;
		ase_sio_t*              handle;
	} out;

	ase_awk_t* awk;
};

static ase_ssize_t sf_in (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	sf_t* sf = (sf_t*)arg;
	ase_cint_t c;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (sf->in.index >= sf->in.count) return 0;

		if (sf->in.files[sf->in.index][0] == ASE_T('\0'))
		{
			sf->in.handle = ase_sio_in;
		}
		else
		{
			sf->in.handle = ase_sio_open (
				ase_awk_getmmgr(sf->awk),
				0,
				sf->in.files[sf->in.index],
				ASE_SIO_READ
			);
			if (sf->in.handle == ASE_NULL) return -1;
		}

		/*
		ase_awk_setsinname ();
		*/

		return 1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		if (sf->in.index >= sf->in.count) return 0;
		if (sf->in.handle != ase_sio_in) ase_sio_close (sf->in.handle);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		ase_ssize_t n = 0;
		ase_sio_t* fp;

	retry:
		fp = sf->in.handle;

		n = ase_sio_getsx (fp, data, size);
		if (n == 0 && ++sf->in.index < sf->in.count)
		{
			if (fp != ase_sio_in) ase_sio_close (fp);
			if (sf->in.files[sf->in.index][0] == ASE_T('\0'))
			{
				sf->in.handle = ase_sio_in;
			}
			else
			{
				sf->in.handle = ase_sio_open (
					ase_awk_getmmgr(sf->awk),
					0,
					sf->in.files[sf->in.index],
					ASE_SIO_READ
				);
				if (sf->in.handle == ASE_NULL) return -1;
			}
			/* TODO: reset internal line counters...
				ase_awk_setsinname ();
			*/
			goto retry;
		}

		#if 0
		while (!ase_feof(fp) && n < size)
		{
			ase_cint_t c = ase_fgetc (fp);
			if (c == ASE_CHAR_EOF) 
			{
				if (ase_ferror(fp)) n = -1;
				break;
			}
			data[n++] = c;
		}

		if (n == 0)
		{
			sf->in.index++;
			if (sf->in.index < sf->in.count)
			{
				if (fp != ase_sio_in) ase_sio_close (fp);
				if (sf->in.files[sf->in.index][0] == ASE_T('\0'))
				{
					sf->in.handle = ase_sio_in;
				}
				else
				{
					sf->in.handle = ase_sio_open (
						ase_awk_getmmgr(sf->awk), 0,
						sf->in.files[sf->in.index], ASE_SIO_READ);
					if (sf->in.handle == ASE_NULL) return -1;
				}
				/* TODO: reset internal line counters...
					ase_awk_setsinname ();
				*/
				goto retry;
			}
		}
		#endif

		return n;
	}

	return -1;
}

static ase_ssize_t sf_out (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	sf_t* sf = (sf_t*)arg;

	if (cmd == ASE_AWK_IO_OPEN) 
	{
		if (sf->out.file[0] == ASE_T('\0'))
		{
			sf->out.handle = ase_sio_out;
		}
		else 
		{
			sf->out.handle = ase_sio_open (
				ase_awk_getmmgr(sf->awk), 
				0,
				sf->out.file, 
				ASE_SIO_WRITE|ASE_SIO_CREATE|ASE_SIO_TRUNCATE
			);
			if (sf->out.handle == ASE_NULL) return -1;
		}

		return 1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE) 
	{
		ase_sio_flush (sf->out.handle);
		if (sf->out.handle != ase_sio_out &&
		    sf->out.handle != ase_sio_err) ase_sio_close (sf->out.handle);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		/*
		ase_size_t left = size;

		while (left > 0)
		{
			if (*data == ASE_T('\0')) 
			{
				if (ase_fputc (*data, sf->out.handle) == ASE_CHAR_EOF) return -1;
				left -= 1; data += 1;
			}
			else
			{
				int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
				int n = ase_fprintf (sf->out.handle, ASE_T("%.*s"), chunk, data);
				if (n < 0) return -1;
				left -= n; data += n;
			}

		}
		*/

		return ase_sio_putsx (sf->out.handle, data, size);
	}

	return -1;
}

int ase_awk_parsesimple (
	ase_awk_t* awk, const void* is, ase_size_t isl, 
	const ase_char_t* osf, int opt)
{
	sf_t sf;
	ase_awk_srcios_t sio;

	if (is == ASE_NULL || isl == 0) 
	{
		ase_awk_seterrnum (awk, ASE_AWK_EINVAL);
		return -1;
	}

	if (opt == ASE_AWK_PARSE_FILES)
	{
		sf.in.files = is;
		sf.in.count = isl;
		sf.in.index = 0;
		sf.in.handle = ASE_NULL;
	}
	else if (opt == ASE_AWK_PARSE_STRING)
	{
		/* TODO */
	}
	else
	{
		ase_awk_seterrnum (awk, ASE_AWK_EINVAL);
		return -1;
	}

	sf.out.file = osf;
	sf.awk = awk;
	
        sio.in = sf_in;
        sio.out = (osf == ASE_NULL)? ASE_NULL: sf_out;
        sio.data = &sf;

	return ase_awk_parse (awk, &sio);
}

/*** RUNSIMPLE ***/

ase_awk_run_t* ase_awk_runsimple (
	ase_awk_t* awk, ase_size_t argc, const char* args[])
{
	return ASE_NULL;
}
