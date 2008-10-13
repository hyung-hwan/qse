/*
 * $Id$
 */

#include "awk.h"
#include <ase/utl/stdio.h>
#include <math.h>

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

static void custom_awk_dprintf (void* custom, const ase_char_t* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	ase_vfprintf (stderr, fmt, ap);
	va_end (ap);
}

ase_awk_t* ase_awk_openstd (void)
{
	ase_awk_t* awk;
	ext_t* ext;

	awk = ase_awk_open (ASE_MMGR_GETDFL(), ASE_SIZEOF(ext_t));
	ase_awk_setccls (awk, ASE_CCLS_GETDFL());

	ext = (ext_t*)ase_awk_getextension(awk);
	ext->prmfns.pow     = custom_awk_pow;
	ext->prmfns.sprintf = custom_awk_sprintf;
	ext->prmfns.dprintf = custom_awk_dprintf;
	ext->prmfns.data    = ASE_NULL;
	ase_awk_setprmfns (awk, &ext->prmfns);

	ase_awk_setoption (awk, 
		ASE_AWK_IMPLICIT | ASE_AWK_EXTIO | ASE_AWK_NEWLINE | 
		ASE_AWK_BASEONE | ASE_AWK_PABLOCK);

	/*ase_awk_addfunction ();*/

	return awk;
}

typedef struct sf_t sf_t;

struct sf_t
{
	struct 
	{
		const ase_char_t** files; 
		ase_size_t         count;   /* the number of files */
		ase_size_t         index;   /* current file index */
		ASE_FILE*          handle;  /* the handle to an open file */ 
	} in;

	struct
	{
		const ase_char_t* file;
		ASE_FILE* handle;
	} out;
};

static ase_ssize_t sf_in (int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	sf_t* sf = (sf_t*)arg;
	ase_cint_t c;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		if (sf->in.index >= sf->in.count) return 0;
		sf->in.handle = ase_fopen (sf->in.files[sf->in.index], ASE_T("r"));
		if (sf->in.handle == ASE_NULL) return -1;

		/*
		ase_awk_setsinname ();
		*/

		return 1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		if (sf->in.index >= sf->in.count) return 0;

		fclose (sf->in.handle);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		ase_ssize_t n = 0;
		ASE_FILE* fp;

	retry:
		fp = sf->in.handle;
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
				ase_fclose (fp);
				sf->in.handle = ase_fopen (sf->in.files[sf->in.index], ASE_T("r"));
				if (sf->in.handle == ASE_NULL) return -1;
				/* TODO: reset internal line counters...
					ase_awk_setsinname ();
				*/
				goto retry;
			}
		}
		return n;
	}

	return -1;
}

#if 0
static ase_ssize_t sf_out (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	/*struct srcio_data_t* srcio = (struct srcio_data_t*)arg;*/

	if (cmd == ASE_AWK_IO_OPEN) return 1;
	else if (cmd == ASE_AWK_IO_CLOSE) 
	{
		ase_fflush (stdout);
		return 0;
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		ase_size_t left = size;

		while (left > 0)
		{
			if (*data == ASE_T('\0')) 
			{
				if (ase_fputc (*data, stdout) == ASE_CHAR_EOF) return -1;
				left -= 1; data += 1;
			}
			else
			{
				int chunk = (left > ASE_TYPE_MAX(int))? ASE_TYPE_MAX(int): (int)left;
				int n = ase_fprintf (stdout, ASE_T("%.*s"), chunk, data);
				if (n < 0) return -1;
				left -= n; data += n;
			}
		}

		return size;
	}

	return -1;
}
#endif

int ase_awk_parsefiles (ase_awk_t* awk, const ase_char_t** isf, ase_size_t isfl, const ase_char_t* osf)
{
	sf_t sf;
	ase_awk_srcios_t sio;

	sf.in.files = isf;
	sf.in.count = isfl;
	sf.in.index = 0;
	sf.in.handle = ASE_NULL;

	sf.out.file = osf;
	
        sio.in = sf_in;
        //sio.out = deparse? sf_out: NULL;
        sio.out = ASE_NULL;
        sio.data = &sf;

	return ase_awk_parse (awk, &sio);
}

#if 0
int ase_awk_parsestring (ase_awk_t* awk, const ase_char_t* str, ase_size_t len)
{
	return -1;
}
#endif
