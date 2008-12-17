/*
 * $Id$
 */

#include "awk.h"
#include <ase/cmn/sio.h>
#include <ase/cmn/str.h>
#include <ase/cmn/time.h>

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ase/utl/stdio.h>

typedef struct xtn_t
{
	ase_awk_prmfns_t prmfns;
} xtn_t;

typedef struct rxtn_t
{
	unsigned int seed;	
} rxtn_t;

static ase_real_t custom_awk_pow (void* custom, ase_real_t x, ase_real_t y)
{
#if defined(HAVE_POWL)
	return powl (x, y);
#elif defined(HAVE_POW)
	return pow (x, y);
#elif defined(HAVE_POWF)
	return powf (x, y);
#else
	#error ### no pow function available ###
#endif
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

static int add_functions (ase_awk_t* awk);

ase_awk_t* ase_awk_opensimple (ase_size_t xtnsize)
{
	ase_awk_t* awk;
	xtn_t* xtn;

	awk = ase_awk_open (ASE_MMGR_GETDFL(), xtnsize + ASE_SIZEOF(xtn_t));
	ase_awk_setccls (awk, ASE_CCLS_GETDFL());

	xtn = (xtn_t*)((ase_byte_t*)ase_awk_getxtn(awk) + xtnsize);

	xtn->prmfns.pow     = custom_awk_pow;
	xtn->prmfns.sprintf = custom_awk_sprintf;
	xtn->prmfns.data    = ASE_NULL;
	ase_awk_setprmfns (awk, &xtn->prmfns);

	ase_awk_setoption (awk, 
		ASE_AWK_IMPLICIT | ASE_AWK_EXTIO | ASE_AWK_NEWLINE | 
		ASE_AWK_BASEONE | ASE_AWK_PABLOCK);

	if (add_functions (awk) == -1)
	{
		ase_awk_close (awk);
		return ASE_NULL;
	}

	return awk;
}

/*** PARSESIMPLE ***/

typedef struct sf_t sf_t;

struct sf_t
{
	struct 
	{
		int                     type;
		union
		{
			const ase_char_t*const* files; 
			const ase_char_t* str;
		} p;
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
		if (sf->in.type == ASE_AWK_PARSE_FILES)
		{
			if (sf->in.p.files[sf->in.index] == ASE_NULL) return 0;

			if (sf->in.p.files[sf->in.index][0] == ASE_T('\0'))
			{
				sf->in.handle = ase_sio_in;
			}
			else
			{
				sf->in.handle = ase_sio_open (
					ase_awk_getmmgr(sf->awk),
					0,
					sf->in.p.files[sf->in.index],
					ASE_SIO_READ
				);
				if (sf->in.handle == ASE_NULL) return -1;
			}

			/*
			ase_awk_setsinname ();
			*/
		}

		return 1;
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		if (sf->in.handle != ASE_NULL && 
		    sf->in.handle != ase_sio_in &&
		    sf->in.handle != ase_sio_out &&
		    sf->in.handle != ase_sio_err) 
		{
			ase_sio_close (sf->in.handle);
		}

		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		ase_ssize_t n = 0;

		if (sf->in.type == ASE_AWK_PARSE_FILES)
		{
			ase_sio_t* sio;

		retry:
			sio = sf->in.handle;

			n = ase_sio_getsx (sio, data, size);
			if (n == 0 && sf->in.p.files[++sf->in.index] != ASE_NULL)
			{
				if (sio != ase_sio_in) ase_sio_close (sio);
				if (sf->in.p.files[sf->in.index][0] == ASE_T('\0'))
				{
					sf->in.handle = ase_sio_in;
				}
				else
				{
					sf->in.handle = ase_sio_open (
						ase_awk_getmmgr(sf->awk),
						0,
						sf->in.p.files[sf->in.index],
						ASE_SIO_READ
					);
					if (sf->in.handle == ASE_NULL) return -1;
				}

				/* TODO: reset internal line counters...
					set new source name....
					ase_awk_setsinname ();
				*/

				goto retry;
			}
		}
		else
		{
			while (n < size && sf->in.p.str[sf->in.index] != ASE_T('\0'))
			{
				data[n++] = sf->in.p.str[sf->in.index++];
			}
		}

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
		if (sf->out.handle != ASE_NULL)
		{
			ase_sio_flush (sf->out.handle);
			if (sf->out.handle != ase_sio_in &&
			    sf->out.handle != ase_sio_out &&
			    sf->out.handle != ase_sio_err) 
			{
				ase_sio_close (sf->out.handle);
			}
		}

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
	ase_awk_t* awk, const void* isp, int ist, const ase_char_t* osf)
{
	sf_t sf;
	ase_awk_srcios_t sio;

	if (isp == ASE_NULL)
	{
		ase_awk_seterrnum (awk, ASE_AWK_EINVAL);
		return -1;
	}

	if (ist == ASE_AWK_PARSE_FILES) sf.in.p.files = isp;
	else if (ist == ASE_AWK_PARSE_STRING) sf.in.p.str = isp;
	else
	{
		ase_awk_seterrnum (awk, ASE_AWK_EINVAL);
		return -1;
	}

	sf.in.type = ist;
	sf.in.index = 0;
	sf.in.handle = ASE_NULL;

	sf.out.file = osf;
	sf.out.handle = ASE_NULL;
	sf.awk = awk;
	
        sio.in = sf_in;
        sio.out = (osf == ASE_NULL)? ASE_NULL: sf_out;
        sio.data = &sf;

	return ase_awk_parse (awk, &sio);
}

/*** RUNSIMPLE ***/

typedef struct runio_data_t
{
	struct
	{
		ase_char_t** files;
		ase_size_t   index;
	} ic; /* input console */
} runio_data_t;

static ase_ssize_t awk_extio_pipe (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	switch (cmd)
	{
		case ASE_AWK_IO_OPEN:
		{
			ase_sio_t* handle;
			int mode;

			if (epa->mode == ASE_AWK_EXTIO_PIPE_READ)
			{
				mode = ASE_SIO_READ;
			}
			else if (epa->mode == ASE_AWK_EXTIO_PIPE_WRITE)
			{
				mode = ASE_SIO_WRITE | 
				       ASE_SIO_TRUNCATE |
				       ASE_SIO_CREATE;
			}
			else return -1; /* TODO: any way to set the error number? */

			//dprint (ASE_T("opening %s of type %d (pipe)\n"),  epa->name, epa->type);

			// TOOD: pipe open....
			handle = ase_sio_open (ase_awk_getrunmmgr(epa->run), 0, epa->name, mode);
			if (handle == ASE_NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			//dprint (ASE_T("closing %s of type (pipe) %d\n"),  epa->name, epa->type);
			ase_sio_close ((ase_sio_t*)epa->handle);
			epa->handle = ASE_NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			return ase_sio_getsx (
				(ase_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case ASE_AWK_IO_WRITE:
		{
			return ase_sio_putsx (
				(ase_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case ASE_AWK_IO_FLUSH:
		{
			/*if (epa->mode == ASE_AWK_EXTIO_PIPE_READ) return -1;*/
			return ase_sio_flush ((ase_sio_t*)epa->handle);
		}

		case ASE_AWK_IO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

static ase_ssize_t awk_extio_file (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;

	switch (cmd)
	{
		case ASE_AWK_IO_OPEN:
		{
			ase_sio_t* handle;
			int mode;

			if (epa->mode == ASE_AWK_EXTIO_FILE_READ)
			{
				mode = ASE_SIO_READ;
			}
			else if (epa->mode == ASE_AWK_EXTIO_FILE_WRITE)
			{
				mode = ASE_SIO_WRITE |
				       ASE_SIO_CREATE |
				       ASE_SIO_TRUNCATE;
			}
			else if (epa->mode == ASE_AWK_EXTIO_FILE_APPEND)
			{
				mode = ASE_SIO_APPEND |
				       ASE_SIO_CREATE;
			}
			else return -1; /* TODO: any way to set the error number? */

			//dprint (ASE_T("opening %s of type %d (file)\n"), epa->name, epa->type);
			handle = ase_sio_open (
				ase_awk_getrunmmgr(epa->run),
				0,
				epa->name, 
				mode
			);
			if (handle == ASE_NULL) 
			{
				ase_cstr_t errarg;

				errarg.ptr = epa->name;
				errarg.len = ase_strlen(epa->name);

				ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
				return -1;
			}

			epa->handle = (void*)handle;
			return 1;
		}

		case ASE_AWK_IO_CLOSE:
		{
			//dprint (ASE_T("closing %s of type %d (file)\n"), epa->name, epa->type);
			ase_sio_close ((ase_sio_t*)epa->handle);
			epa->handle = ASE_NULL;
			return 0;
		}

		case ASE_AWK_IO_READ:
		{
			return ase_sio_getsx (
				(ase_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case ASE_AWK_IO_WRITE:
		{
			return ase_sio_putsx (
				(ase_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case ASE_AWK_IO_FLUSH:
		{
			return ase_sio_flush ((ase_sio_t*)epa->handle);
		}

		case ASE_AWK_IO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

static int open_extio_console (ase_awk_extio_t* epa)
{
	runio_data_t* rd = (runio_data_t*)epa->data;

	//dprint (ASE_T("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == ASE_AWK_EXTIO_CONSOLE_READ)
	{
		if (rd->ic.files[rd->ic.index] == ASE_NULL)
		{
			/* no more input file */
			//dprint (ASE_T("console - no more file\n"));;
			return 0;
		}

		if (rd->ic.files[rd->ic.index][0] == ASE_T('\0'))
		{
			//dprint (ASE_T("    console(r) - <standard input>\n"));
			epa->handle = ase_sio_in;
		}
		else
		{
			/* a temporary variable fp is used here not to change 
			 * any fields of epa when the open operation fails */
			ase_sio_t* fp;

			fp = ase_sio_open (
				ase_awk_getrunmmgr(epa->run),
				0,
				rd->ic.files[rd->ic.index],
				ASE_SIO_READ
			);
			if (fp == ASE_NULL)
			{
				ase_cstr_t errarg;

				errarg.ptr = rd->ic.files[rd->ic.index];
				errarg.len = ase_strlen(rd->ic.files[rd->ic.index]);

				ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
				return -1;
			}

			//dprint (ASE_T("    console(r) - %s\n"), rd->ic.files[rd->ic.index]);
			if (ase_awk_setfilename (
				epa->run, rd->ic.files[rd->ic.index], 
				ase_strlen(rd->ic.files[rd->ic.index])) == -1)
			{
				ase_sio_close (fp);
				return -1;
			}

			epa->handle = fp;
		}

		rd->ic.index++;
		return 1;
	}
	else if (epa->mode == ASE_AWK_EXTIO_CONSOLE_WRITE)
	{
		//dprint (ASE_T("    console(w) - <standard output>\n"));

		if (ase_awk_setofilename (epa->run, ASE_T(""), 0) == -1)
		{
			return -1;
		}

		epa->handle = ase_sio_out;
		return 1;
	}

	return -1;
}

static ase_ssize_t awk_extio_console (
	int cmd, void* arg, ase_char_t* data, ase_size_t size)
{
	ase_awk_extio_t* epa = (ase_awk_extio_t*)arg;
	runio_data_t* rd = (runio_data_t*)epa->data;

	if (cmd == ASE_AWK_IO_OPEN)
	{
		return open_extio_console (epa);
	}
	else if (cmd == ASE_AWK_IO_CLOSE)
	{
		//dprint (ASE_T("closing console of type %x\n"), epa->type);

		if (epa->handle != ASE_NULL &&
		    epa->handle != ase_sio_in && 
		    epa->handle != ase_sio_out && 
		    epa->handle != ase_sio_err)
		{
			ase_sio_close ((ase_sio_t*)epa->handle);
		}

		return 0;
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		ase_ssize_t n;

		while ((n = ase_sio_getsx((ase_sio_t*)epa->handle,data,size)) == 0)
		{
			/* it has reached the end of the current file.
			 * open the next file if available */
			if (rd->ic.files[rd->ic.index] == ASE_NULL) 
			{
				/* no more input console */
				return 0;
			}

			if (rd->ic.files[rd->ic.index][0] == ASE_T('\0'))
			{
				if (epa->handle != ASE_NULL &&
				    epa->handle != ase_sio_in &&
				    epa->handle != ase_sio_out &&
				    epa->handle != ase_sio_err) 
				{
					ase_sio_close ((ase_sio_t*)epa->handle);
				}

				epa->handle = ase_sio_in;
			}
			else
			{
				ase_sio_t* fp;

				fp = ase_sio_open (
					ase_awk_getrunmmgr(epa->run),
					0,
					rd->ic.files[rd->ic.index],
					ASE_SIO_READ
				);

				if (fp == ASE_NULL)
				{
					ase_cstr_t errarg;

					errarg.ptr = rd->ic.files[rd->ic.index];
					errarg.len = ase_strlen(rd->ic.files[rd->ic.index]);

					ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
					return -1;
				}

				if (ase_awk_setfilename (
					epa->run, rd->ic.files[rd->ic.index], 
					ase_strlen(rd->ic.files[rd->ic.index])) == -1)
				{
					ase_sio_close (fp);
					return -1;
				}

				if (ase_awk_setglobal (
					epa->run, ASE_AWK_GLOBAL_FNR, ase_awk_val_zero) == -1)
				{
					/* need to reset FNR */
					ase_sio_close (fp);
					return -1;
				}

				if (epa->handle != ASE_NULL &&
				    epa->handle != ase_sio_in &&
				    epa->handle != ase_sio_out &&
				    epa->handle != ase_sio_err) 
				{
					ase_sio_close ((ase_sio_t*)epa->handle);
				}

				//dprint (ASE_T("open the next console [%s]\n"), rd->ic.files[rd->ic.index]);
				epa->handle = fp;
			}

			rd->ic.index++;	
		}

		return n;
	}
	else if (cmd == ASE_AWK_IO_WRITE)
	{
		return ase_sio_putsx (	
			(ase_sio_t*)epa->handle,
			data,
			size
		);
	}
	else if (cmd == ASE_AWK_IO_FLUSH)
	{
		return ase_sio_flush ((ase_sio_t*)epa->handle);
	}
	else if (cmd == ASE_AWK_IO_NEXT)
	{
		int n;
		ase_sio_t* fp = (ase_sio_t*)epa->handle;

		//dprint (ASE_T("switching console[%s] of type %x\n"), epa->name, epa->type);

		n = open_extio_console(epa);
		if (n == -1) return -1;

		if (n == 0) 
		{
			/* if there is no more file, keep the previous handle */
			return 0;
		}

		if (fp != ASE_NULL && 
		    fp != ase_sio_in && 
		    fp != ase_sio_out &&
		    fp != ase_sio_err) 
		{
			ase_sio_close (fp);
		}

		return n;
	}

	return -1;
}

int ase_awk_runsimple (ase_awk_t* awk, ase_char_t** icf, ase_awk_runcbs_t* cbs)
{
	ase_awk_runios_t ios;
	runio_data_t rd;
	rxtn_t rxtn;
	ase_time_t now;

	rd.ic.files = icf;
	rd.ic.index = 0;

	ios.pipe = awk_extio_pipe;
	ios.file = awk_extio_file;
	ios.console = awk_extio_console;
	ios.data = &rd;

	if (ase_gettime (&now) == -1) rxtn.seed = 0;
	else rxtn.seed = (unsigned int)now;
	srand (rxtn.seed);

	return ase_awk_run (
		awk, 
		ASE_NULL/*mfn*/,
		&ios,
		cbs,
		ASE_NULL/*runarg*/,
		&rxtn/*ASE_NULL*/
	);
}


/*** EXTRA BUILTIN FUNCTIONS ***/
enum
{
	BFN_MATH_LD,
	BFN_MATH_D,
	BFN_MATH_F
};

static int bfn_math_1 (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl, int type, void* f)
{
	ase_size_t nargs;
	ase_awk_val_t* a0;
	ase_long_t lv;
	ase_real_t rv;
	ase_awk_val_t* r;
	int n;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);

	a0 = ase_awk_getarg (run, 0);

	n = ase_awk_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 0) rv = (ase_real_t)lv;

	if (type == BFN_MATH_LD)
	{
		long double (*rf) (long double) = 
			(long double(*)(long double))f;
		r = ase_awk_makerealval (run, rf(rv));
	}
	else if (type == BFN_MATH_D)
	{
		double (*rf) (double) = (double(*)(double))f;
		r = ase_awk_makerealval (run, rf(rv));
	}
	else 
	{
		ASE_ASSERT (type == BFN_MATH_F);
		float (*rf) (float) = (float(*)(float))f;
		r = ase_awk_makerealval (run, rf(rv));
	}
	
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

static int bfn_math_2 (
	ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl, int type, void* f)
{
	ase_size_t nargs;
	ase_awk_val_t* a0, * a1;
	ase_long_t lv0, lv1;
	ase_real_t rv0, rv1;
	ase_awk_val_t* r;
	int n;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 2);

	a0 = ase_awk_getarg (run, 0);
	a1 = ase_awk_getarg (run, 1);

	n = ase_awk_valtonum (run, a0, &lv0, &rv0);
	if (n == -1) return -1;
	if (n == 0) rv0 = (ase_real_t)lv0;

	n = ase_awk_valtonum (run, a1, &lv1, &rv1);
	if (n == -1) return -1;
	if (n == 0) rv1 = (ase_real_t)lv1;

	if (type == BFN_MATH_LD)
	{
		long double (*rf) (long double,long double) = 
			(long double(*)(long double,long double))f;
		r = ase_awk_makerealval (run, rf(rv0,rv1));
	}
	else if (type == BFN_MATH_D)
	{
		double (*rf) (double,double) = (double(*)(double,double))f;
		r = ase_awk_makerealval (run, rf(rv0,rv1));
	}
	else 
	{
		ASE_ASSERT (type == BFN_MATH_F);
		float (*rf) (float,float) = (float(*)(float,float))f;
		r = ase_awk_makerealval (run, rf(rv0,rv1));
	}
	
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

static int bfn_sin (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_1 (run, fnm, fnl, 
	#if defined(HAVE_SINL)
		BFN_MATH_LD, sinl
	#elif defined(HAVE_SIN)
		BFN_MATH_D, sin
	#elif defined(HAVE_SINF)
		BFN_MATH_F, sinf
	#else
		#error ### no sin function available ###
	#endif
	);
}

static int bfn_cos (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_1 (run, fnm, fnl, 
	#if defined(HAVE_COSL)
		BFN_MATH_LD, cosl
	#elif defined(HAVE_COS)
		BFN_MATH_D, cos
	#elif defined(HAVE_COSF)
		BFN_MATH_F, cosf
	#else
		#error ### no cos function available ###
	#endif
	);
}

static int bfn_tan (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_1 (run, fnm, fnl, 
	#if defined(HAVE_TANL)
		BFN_MATH_LD, tanl
	#elif defined(HAVE_TAN)
		BFN_MATH_D, tan
	#elif defined(HAVE_TANF)
		BFN_MATH_F, tanf
	#else
		#error ### no tan function available ###
	#endif
	);
}

static int bfn_atan (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_1 (run, fnm, fnl, 
	#if defined(HAVE_ATANL)
		BFN_MATH_LD, atanl
	#elif defined(HAVE_ATAN)
		BFN_MATH_D, atan
	#elif defined(HAVE_ATANF)
		BFN_MATH_F, atanf
	#else
		#error ### no atan function available ###
	#endif
	);
}

static int bfn_atan2 (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_2 (run, fnm, fnl, 
	#if defined(HAVE_ATAN2L)
		BFN_MATH_LD, atan2l
	#elif defined(HAVE_ATAN2)
		BFN_MATH_D, atan2
	#elif defined(HAVE_ATAN2F)
		BFN_MATH_F, atan2f
	#else
		#error ### no atan2 function available ###
	#endif
	);
}

static int bfn_log (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_1 (run, fnm, fnl, 
	#if defined(HAVE_LOGL)
		BFN_MATH_LD, logl
	#elif defined(HAVE_LOG)
		BFN_MATH_D, log
	#elif defined(HAVE_LOGF)
		BFN_MATH_F, logf
	#else
		#error ### no log function available ###
	#endif
	);
}

static int bfn_exp (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_1 (run, fnm, fnl, 
	#if defined(HAVE_EXPL)
		BFN_MATH_LD, expl
	#elif defined(HAVE_EXP)
		BFN_MATH_D, exp
	#elif defined(HAVE_EXPF)
		BFN_MATH_F, expf
	#else
		#error ### no exp function available ###
	#endif
	);
}

static int bfn_sqrt (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	return bfn_math_1 (run, fnm, fnl, 
	#if defined(HAVE_SQRTL)
		BFN_MATH_LD, sqrtl
	#elif defined(HAVE_SQRT)
		BFN_MATH_D, sqrt
	#elif defined(HAVE_SQRTF)
		BFN_MATH_F, sqrtf
	#else
		#error ### no sqrt function available ###
	#endif
	);
}

static int bfn_int (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0;
	ase_long_t lv;
	ase_real_t rv;
	ase_awk_val_t* r;
	int n;

	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 1);

	a0 = ase_awk_getarg (run, 0);

	n = ase_awk_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (ase_long_t)rv;

	r = ase_awk_makeintval (run, lv);
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

static int bfn_rand (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_awk_val_t* r;

	r = ase_awk_makeintval (run, rand());
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

static int bfn_srand (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_size_t nargs;
	ase_awk_val_t* a0;
	ase_long_t lv;
	ase_real_t rv;
	ase_awk_val_t* r;
	int n;
	unsigned int prev;
	rxtn_t* rxtn;

	rxtn = ase_awk_getrundata (run);
	nargs = ase_awk_getnargs (run);
	ASE_ASSERT (nargs == 0 || nargs == 1);

	prev = rxtn->seed;

	if (nargs == 1)
	{
		a0 = ase_awk_getarg (run, 0);

		n = ase_awk_valtonum (run, a0, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (ase_long_t)rv;

		rxtn->seed = lv;
	}
	else
	{
		ase_time_t now;

		if (ase_gettime(&now) == -1) rxtn->seed >>= 1;
		else rxtn->seed = (unsigned int)now;
	}

        srand (rxtn->seed);

	r = ase_awk_makeintval (run, prev);
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

static int bfn_systime (ase_awk_run_t* run, const ase_char_t* fnm, ase_size_t fnl)
{
	ase_awk_val_t* r;
	ase_time_t now;
	int n;
	
	if (ase_gettime(&now) == -1) now = 0;

	r = ase_awk_makeintval (run, now / ASE_MSEC_IN_SEC);
	if (r == ASE_NULL)
	{
		ase_awk_setrunerrnum (run, ASE_AWK_ENOMEM);
		return -1;
	}

	ase_awk_setretval (run, r);
	return 0;
}

#define ADD_FUNC(awk,name,min,max,bfn) \
        if (ase_awk_addfunc (\
		(awk), (name), ase_strlen(name), \
		0, (min), (max), ASE_NULL, (bfn)) == ASE_NULL) return -1;

static int add_functions (ase_awk_t* awk)
{
        ADD_FUNC (awk, ASE_T("sin"),        1, 1, bfn_sin);
        ADD_FUNC (awk, ASE_T("cos"),        1, 1, bfn_cos);
        ADD_FUNC (awk, ASE_T("tan"),        1, 1, bfn_tan);
        ADD_FUNC (awk, ASE_T("atan"),       1, 1, bfn_atan);
        ADD_FUNC (awk, ASE_T("atan2"),      2, 2, bfn_atan2);
        ADD_FUNC (awk, ASE_T("log"),        1, 1, bfn_log);
        ADD_FUNC (awk, ASE_T("exp"),        1, 1, bfn_exp);
        ADD_FUNC (awk, ASE_T("sqrt"),       1, 1, bfn_sqrt);
        ADD_FUNC (awk, ASE_T("int"),        1, 1, bfn_int);
        ADD_FUNC (awk, ASE_T("rand"),       0, 0, bfn_rand);
        ADD_FUNC (awk, ASE_T("srand"),      0, 1, bfn_srand);
        ADD_FUNC (awk, ASE_T("systime"),    0, 0, bfn_systime);
/*
        ADD_FUNC (awk, ASE_T("strftime"),   0, 2, bfn_strftime);
        ADD_FUNC (awk, ASE_T("strfgmtime"), 0, 2, bfn_strfgmtime);
        ADD_FUNC (awk, ASE_T("system"),     1, 1, bfn_system);
*/

	return 0;
}
