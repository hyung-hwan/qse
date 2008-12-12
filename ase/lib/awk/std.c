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
		/*
		if (sf->in.files[sf->in.index] == ASE_NULL) return 0;
		*/

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
				set new source name....
				ase_awk_setsinname ();
			*/

			goto retry;
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
	ase_char_t** icf; /* input console files */
	ase_size_t icf_cur;
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
				       ASE_SIO_CREATE |
				       ASE_SIO_TRUNCATE;
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

static int open_extio_console (ase_awk_extio_t* epa);
static int close_extio_console (ase_awk_extio_t* epa);
static int next_extio_console (ase_awk_extio_t* epa);

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
		return close_extio_console (epa);
	}
	else if (cmd == ASE_AWK_IO_READ)
	{
		ase_ssize_t n;

		while ((n = ase_sio_getsx((ase_sio_t*)epa->handle,data,size)) == 0)
		{
			/* it has reached the end of the current file.
			 * open the next file if available */
			if (rd->icf[rd->icf_cur] == ASE_NULL) 
			{
				/* no more input console */
				return 0;
			}

			if (rd->icf[rd->icf_cur][0] == ASE_T('\0'))
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
					rd->icf[rd->icf_cur],
					ASE_SIO_READ
				);

				if (fp == ASE_NULL)
				{
					ase_cstr_t errarg;

					errarg.ptr = rd->icf[rd->icf_cur];
					errarg.len = ase_strlen(rd->icf[rd->icf_cur]);

					ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
					return -1;
				}

				if (ase_awk_setfilename (
					epa->run, rd->icf[rd->icf_cur], 
					ase_strlen(rd->icf[rd->icf_cur])) == -1)
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

				//dprint (ASE_T("open the next console [%s]\n"), rd->icf[rd->icf_cur]);
				epa->handle = fp;
			}

			rd->icf_cur++;	
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
		return next_extio_console (epa);
	}

	return -1;
}

static int open_extio_console (ase_awk_extio_t* epa)
{
	runio_data_t* rd = (runio_data_t*)epa->data;


	//dprint (ASE_T("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == ASE_AWK_EXTIO_CONSOLE_READ)
	{
		if (rd->icf[rd->icf_cur] == ASE_NULL)
		{
			/* no more input file */
			//dprint (ASE_T("console - no more file\n"));;
			return 0;
		}

		if (rd->icf[rd->icf_cur][0] == ASE_T('\0'))
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
				rd->icf[rd->icf_cur],
				ASE_SIO_READ
			);
			if (fp == ASE_NULL)
			{
				ase_cstr_t errarg;

				errarg.ptr = rd->icf[rd->icf_cur];
				errarg.len = ase_strlen(rd->icf[rd->icf_cur]);

				ase_awk_setrunerror (epa->run, ASE_AWK_EOPEN, 0, &errarg, 1);
				return -1;
			}

			//dprint (ASE_T("    console(r) - %s\n"), rd->icf[rd->icf_cur]);
			if (ase_awk_setfilename (
				epa->run, rd->icf[rd->icf_cur], 
				ase_strlen(rd->icf[rd->icf_cur])) == -1)
			{
				ase_sio_close (fp);
				return -1;
			}

			epa->handle = fp;
		}

		rd->icf_cur++;
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

static int close_extio_console (ase_awk_extio_t* epa)
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

static int next_extio_console (ase_awk_extio_t* epa)
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

int ase_awk_runsimple (ase_awk_t* awk, ase_char_t** icf)
{
	ase_awk_runcbs_t runcbs;
	ase_awk_runios_t runios;
	runio_data_t rd;

	rd.icf = icf;
	rd.icf_cur = 0;

	runios.pipe = awk_extio_pipe;
	runios.file = awk_extio_file;
	runios.console = awk_extio_console;
	runios.data = &rd;

	/*
	runcbs.on_start = on_run_start;
	runcbs.on_statement = on_run_statement;
	runcbs.on_return = on_run_return;
	runcbs.on_end = on_run_end;
	runcbs.data = ASE_NULL;
	*/

	return ase_awk_run (
		awk, 
		ASE_NULL/*mfn*/,
		&runios,
		ASE_NULL/*&runcbs*/,
		ASE_NULL/*runarg*/,
		ASE_NULL
	);
}
