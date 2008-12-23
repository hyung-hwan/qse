/*
 * $Id$
 */

#include "awk.h"
#include <qse/cmn/sio.h>
#include <qse/cmn/str.h>
#include <qse/cmn/time.h>

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <qse/utl/stdio.h>

typedef struct xtn_t
{
	qse_awk_prmfns_t prmfns;
} xtn_t;

typedef struct rxtn_t
{
	unsigned int seed;	
} rxtn_t;

static qse_real_t custom_awk_pow (void* custom, qse_real_t x, qse_real_t y)
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
	void* custom, qse_char_t* buf, qse_size_t size, 
	const qse_char_t* fmt, ...)
{
	int n;

	va_list ap;
	va_start (ap, fmt);
	n = qse_vsprintf (buf, size, fmt, ap);
	va_end (ap);

	return n;
}

static int add_functions (qse_awk_t* awk);

qse_awk_t* qse_awk_opensimple (qse_size_t xtnsize)
{
	qse_awk_t* awk;
	xtn_t* xtn;

	awk = qse_awk_open (QSE_MMGR_GETDFL(), xtnsize + QSE_SIZEOF(xtn_t));
	qse_awk_setccls (awk, QSE_CCLS_GETDFL());

	xtn = (xtn_t*)((qse_byte_t*)qse_awk_getxtn(awk) + xtnsize);

	xtn->prmfns.pow     = custom_awk_pow;
	xtn->prmfns.sprintf = custom_awk_sprintf;
	xtn->prmfns.data    = QSE_NULL;
	qse_awk_setprmfns (awk, &xtn->prmfns);

	qse_awk_setoption (awk, 
		QSE_AWK_IMPLICIT | QSE_AWK_EXTIO | QSE_AWK_NEWLINE | 
		QSE_AWK_BASEONE | QSE_AWK_PABLOCK);

	if (add_functions (awk) == -1)
	{
		qse_awk_close (awk);
		return QSE_NULL;
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
			const qse_char_t*const* files; 
			const qse_char_t* str;
		} p;
		qse_size_t              index;  /* current file index */
		qse_sio_t*              handle; /* the handle to an open file */
	} in;

	struct
	{
		const qse_char_t*       file;
		qse_sio_t*              handle;
	} out;

	qse_awk_t* awk;
};

static qse_ssize_t sf_in (int cmd, void* arg, qse_char_t* data, qse_size_t size)
{
	sf_t* sf = (sf_t*)arg;
	qse_cint_t c;

	if (cmd == QSE_AWK_IO_OPEN)
	{
		if (sf->in.type == QSE_AWK_PARSE_FILES)
		{
			if (sf->in.p.files[sf->in.index] == QSE_NULL) return 0;

			if (sf->in.p.files[sf->in.index][0] == QSE_T('\0'))
			{
				sf->in.handle = qse_sio_in;
			}
			else
			{
				sf->in.handle = qse_sio_open (
					qse_awk_getmmgr(sf->awk),
					0,
					sf->in.p.files[sf->in.index],
					QSE_SIO_READ
				);
				if (sf->in.handle == QSE_NULL) return -1;
			}

			/*
			qse_awk_setsinname ();
			*/
		}

		return 1;
	}
	else if (cmd == QSE_AWK_IO_CLOSE)
	{
		if (sf->in.handle != QSE_NULL && 
		    sf->in.handle != qse_sio_in &&
		    sf->in.handle != qse_sio_out &&
		    sf->in.handle != qse_sio_err) 
		{
			qse_sio_close (sf->in.handle);
		}

		return 0;
	}
	else if (cmd == QSE_AWK_IO_READ)
	{
		qse_ssize_t n = 0;

		if (sf->in.type == QSE_AWK_PARSE_FILES)
		{
			qse_sio_t* sio;

		retry:
			sio = sf->in.handle;

			n = qse_sio_getsx (sio, data, size);
			if (n == 0 && sf->in.p.files[++sf->in.index] != QSE_NULL)
			{
				if (sio != qse_sio_in) qse_sio_close (sio);
				if (sf->in.p.files[sf->in.index][0] == QSE_T('\0'))
				{
					sf->in.handle = qse_sio_in;
				}
				else
				{
					sf->in.handle = qse_sio_open (
						qse_awk_getmmgr(sf->awk),
						0,
						sf->in.p.files[sf->in.index],
						QSE_SIO_READ
					);
					if (sf->in.handle == QSE_NULL) return -1;
				}

				/* TODO: reset internal line counters...
					set new source name....
					qse_awk_setsinname ();
				*/

				goto retry;
			}
		}
		else
		{
			while (n < size && sf->in.p.str[sf->in.index] != QSE_T('\0'))
			{
				data[n++] = sf->in.p.str[sf->in.index++];
			}
		}

		return n;
	}

	return -1;
}

static qse_ssize_t sf_out (int cmd, void* arg, qse_char_t* data, qse_size_t size)
{
	sf_t* sf = (sf_t*)arg;

	if (cmd == QSE_AWK_IO_OPEN) 
	{
		if (sf->out.file[0] == QSE_T('\0'))
		{
			sf->out.handle = qse_sio_out;
		}
		else 
		{
			sf->out.handle = qse_sio_open (
				qse_awk_getmmgr(sf->awk), 
				0,
				sf->out.file, 
				QSE_SIO_WRITE|QSE_SIO_CREATE|QSE_SIO_TRUNCATE
			);
			if (sf->out.handle == QSE_NULL) return -1;
		}

		return 1;
	}
	else if (cmd == QSE_AWK_IO_CLOSE) 
	{
		if (sf->out.handle != QSE_NULL)
		{
			qse_sio_flush (sf->out.handle);
			if (sf->out.handle != qse_sio_in &&
			    sf->out.handle != qse_sio_out &&
			    sf->out.handle != qse_sio_err) 
			{
				qse_sio_close (sf->out.handle);
			}
		}

		return 0;
	}
	else if (cmd == QSE_AWK_IO_WRITE)
	{
		/*
		qse_size_t left = size;

		while (left > 0)
		{
			if (*data == QSE_T('\0')) 
			{
				if (qse_fputc (*data, sf->out.handle) == QSE_CHAR_EOF) return -1;
				left -= 1; data += 1;
			}
			else
			{
				int chunk = (left > QSE_TYPE_MAX(int))? QSE_TYPE_MAX(int): (int)left;
				int n = qse_fprintf (sf->out.handle, QSE_T("%.*s"), chunk, data);
				if (n < 0) return -1;
				left -= n; data += n;
			}

		}
		*/

		return qse_sio_putsx (sf->out.handle, data, size);
	}

	return -1;
}

int qse_awk_parsesimple (
	qse_awk_t* awk, const void* isp, int ist, const qse_char_t* osf)
{
	sf_t sf;
	qse_awk_srcios_t sio;

	if (isp == QSE_NULL)
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
		return -1;
	}

	if (ist == QSE_AWK_PARSE_FILES) sf.in.p.files = isp;
	else if (ist == QSE_AWK_PARSE_STRING) sf.in.p.str = isp;
	else
	{
		qse_awk_seterrnum (awk, QSE_AWK_EINVAL);
		return -1;
	}

	sf.in.type = ist;
	sf.in.index = 0;
	sf.in.handle = QSE_NULL;

	sf.out.file = osf;
	sf.out.handle = QSE_NULL;
	sf.awk = awk;
	
        sio.in = sf_in;
        sio.out = (osf == QSE_NULL)? QSE_NULL: sf_out;
        sio.data = &sf;

	return qse_awk_parse (awk, &sio);
}

/*** RUNSIMPLE ***/

typedef struct runio_data_t
{
	struct
	{
		qse_char_t** files;
		qse_size_t   index;
	} ic; /* input console */
} runio_data_t;

static qse_ssize_t awk_extio_pipe (
	int cmd, void* arg, qse_char_t* data, qse_size_t size)
{
	qse_awk_extio_t* epa = (qse_awk_extio_t*)arg;

	switch (cmd)
	{
		case QSE_AWK_IO_OPEN:
		{
			qse_sio_t* handle;
			int mode;

			if (epa->mode == QSE_AWK_EXTIO_PIPE_READ)
			{
				mode = QSE_SIO_READ;
			}
			else if (epa->mode == QSE_AWK_EXTIO_PIPE_WRITE)
			{
				mode = QSE_SIO_WRITE | 
				       QSE_SIO_TRUNCATE |
				       QSE_SIO_CREATE;
			}
			else return -1; /* TODO: any way to set the error number? */

			//dprint (QSE_T("opening %s of type %d (pipe)\n"),  epa->name, epa->type);

			// TOOD: pipe open....
			handle = qse_sio_open (qse_awk_getrunmmgr(epa->run), 0, epa->name, mode);
			if (handle == QSE_NULL) return -1;
			epa->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_IO_CLOSE:
		{
			//dprint (QSE_T("closing %s of type (pipe) %d\n"),  epa->name, epa->type);
			qse_sio_close ((qse_sio_t*)epa->handle);
			epa->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_IO_READ:
		{
			return qse_sio_getsx (
				(qse_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case QSE_AWK_IO_WRITE:
		{
			return qse_sio_putsx (
				(qse_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case QSE_AWK_IO_FLUSH:
		{
			/*if (epa->mode == QSE_AWK_EXTIO_PIPE_READ) return -1;*/
			return qse_sio_flush ((qse_sio_t*)epa->handle);
		}

		case QSE_AWK_IO_NEXT:
		{
			return -1;
		}
	}

	return -1;
}

static qse_ssize_t awk_extio_file (
	int cmd, void* arg, qse_char_t* data, qse_size_t size)
{
	qse_awk_extio_t* epa = (qse_awk_extio_t*)arg;

	switch (cmd)
	{
		case QSE_AWK_IO_OPEN:
		{
			qse_sio_t* handle;
			int mode;

			if (epa->mode == QSE_AWK_EXTIO_FILE_READ)
			{
				mode = QSE_SIO_READ;
			}
			else if (epa->mode == QSE_AWK_EXTIO_FILE_WRITE)
			{
				mode = QSE_SIO_WRITE |
				       QSE_SIO_CREATE |
				       QSE_SIO_TRUNCATE;
			}
			else if (epa->mode == QSE_AWK_EXTIO_FILE_APPEND)
			{
				mode = QSE_SIO_APPEND |
				       QSE_SIO_CREATE;
			}
			else return -1; /* TODO: any way to set the error number? */

			//dprint (QSE_T("opening %s of type %d (file)\n"), epa->name, epa->type);
			handle = qse_sio_open (
				qse_awk_getrunmmgr(epa->run),
				0,
				epa->name, 
				mode
			);
			if (handle == QSE_NULL) 
			{
				qse_cstr_t errarg;

				errarg.ptr = epa->name;
				errarg.len = qse_strlen(epa->name);

				qse_awk_setrunerror (epa->run, QSE_AWK_EOPEN, 0, &errarg, 1);
				return -1;
			}

			epa->handle = (void*)handle;
			return 1;
		}

		case QSE_AWK_IO_CLOSE:
		{
			//dprint (QSE_T("closing %s of type %d (file)\n"), epa->name, epa->type);
			qse_sio_close ((qse_sio_t*)epa->handle);
			epa->handle = QSE_NULL;
			return 0;
		}

		case QSE_AWK_IO_READ:
		{
			return qse_sio_getsx (
				(qse_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case QSE_AWK_IO_WRITE:
		{
			return qse_sio_putsx (
				(qse_sio_t*)epa->handle,
				data,	
				size
			);
		}

		case QSE_AWK_IO_FLUSH:
		{
			return qse_sio_flush ((qse_sio_t*)epa->handle);
		}

		case QSE_AWK_IO_NEXT:
		{
			return -1;
		}

	}

	return -1;
}

static int open_extio_console (qse_awk_extio_t* epa)
{
	runio_data_t* rd = (runio_data_t*)epa->data;

	//dprint (QSE_T("opening console[%s] of type %x\n"), epa->name, epa->type);

	if (epa->mode == QSE_AWK_EXTIO_CONSOLE_READ)
	{
		if (rd->ic.files[rd->ic.index] == QSE_NULL)
		{
			/* no more input file */
			//dprint (QSE_T("console - no more file\n"));;
			return 0;
		}

		if (rd->ic.files[rd->ic.index][0] == QSE_T('\0'))
		{
			//dprint (QSE_T("    console(r) - <standard input>\n"));
			epa->handle = qse_sio_in;
		}
		else
		{
			/* a temporary variable fp is used here not to change 
			 * any fields of epa when the open operation fails */
			qse_sio_t* fp;

			fp = qse_sio_open (
				qse_awk_getrunmmgr(epa->run),
				0,
				rd->ic.files[rd->ic.index],
				QSE_SIO_READ
			);
			if (fp == QSE_NULL)
			{
				qse_cstr_t errarg;

				errarg.ptr = rd->ic.files[rd->ic.index];
				errarg.len = qse_strlen(rd->ic.files[rd->ic.index]);

				qse_awk_setrunerror (epa->run, QSE_AWK_EOPEN, 0, &errarg, 1);
				return -1;
			}

			//dprint (QSE_T("    console(r) - %s\n"), rd->ic.files[rd->ic.index]);
			if (qse_awk_setfilename (
				epa->run, rd->ic.files[rd->ic.index], 
				qse_strlen(rd->ic.files[rd->ic.index])) == -1)
			{
				qse_sio_close (fp);
				return -1;
			}

			epa->handle = fp;
		}

		rd->ic.index++;
		return 1;
	}
	else if (epa->mode == QSE_AWK_EXTIO_CONSOLE_WRITE)
	{
		//dprint (QSE_T("    console(w) - <standard output>\n"));

		if (qse_awk_setofilename (epa->run, QSE_T(""), 0) == -1)
		{
			return -1;
		}

		epa->handle = qse_sio_out;
		return 1;
	}

	return -1;
}

static qse_ssize_t awk_extio_console (
	int cmd, void* arg, qse_char_t* data, qse_size_t size)
{
	qse_awk_extio_t* epa = (qse_awk_extio_t*)arg;
	runio_data_t* rd = (runio_data_t*)epa->data;

	if (cmd == QSE_AWK_IO_OPEN)
	{
		return open_extio_console (epa);
	}
	else if (cmd == QSE_AWK_IO_CLOSE)
	{
		//dprint (QSE_T("closing console of type %x\n"), epa->type);

		if (epa->handle != QSE_NULL &&
		    epa->handle != qse_sio_in && 
		    epa->handle != qse_sio_out && 
		    epa->handle != qse_sio_err)
		{
			qse_sio_close ((qse_sio_t*)epa->handle);
		}

		return 0;
	}
	else if (cmd == QSE_AWK_IO_READ)
	{
		qse_ssize_t n;

		while ((n = qse_sio_getsx((qse_sio_t*)epa->handle,data,size)) == 0)
		{
			/* it has reached the end of the current file.
			 * open the next file if available */
			if (rd->ic.files[rd->ic.index] == QSE_NULL) 
			{
				/* no more input console */
				return 0;
			}

			if (rd->ic.files[rd->ic.index][0] == QSE_T('\0'))
			{
				if (epa->handle != QSE_NULL &&
				    epa->handle != qse_sio_in &&
				    epa->handle != qse_sio_out &&
				    epa->handle != qse_sio_err) 
				{
					qse_sio_close ((qse_sio_t*)epa->handle);
				}

				epa->handle = qse_sio_in;
			}
			else
			{
				qse_sio_t* fp;

				fp = qse_sio_open (
					qse_awk_getrunmmgr(epa->run),
					0,
					rd->ic.files[rd->ic.index],
					QSE_SIO_READ
				);

				if (fp == QSE_NULL)
				{
					qse_cstr_t errarg;

					errarg.ptr = rd->ic.files[rd->ic.index];
					errarg.len = qse_strlen(rd->ic.files[rd->ic.index]);

					qse_awk_setrunerror (epa->run, QSE_AWK_EOPEN, 0, &errarg, 1);
					return -1;
				}

				if (qse_awk_setfilename (
					epa->run, rd->ic.files[rd->ic.index], 
					qse_strlen(rd->ic.files[rd->ic.index])) == -1)
				{
					qse_sio_close (fp);
					return -1;
				}

				if (qse_awk_setglobal (
					epa->run, QSE_AWK_GLOBAL_FNR, qse_awk_val_zero) == -1)
				{
					/* need to reset FNR */
					qse_sio_close (fp);
					return -1;
				}

				if (epa->handle != QSE_NULL &&
				    epa->handle != qse_sio_in &&
				    epa->handle != qse_sio_out &&
				    epa->handle != qse_sio_err) 
				{
					qse_sio_close ((qse_sio_t*)epa->handle);
				}

				//dprint (QSE_T("open the next console [%s]\n"), rd->ic.files[rd->ic.index]);
				epa->handle = fp;
			}

			rd->ic.index++;	
		}

		return n;
	}
	else if (cmd == QSE_AWK_IO_WRITE)
	{
		return qse_sio_putsx (	
			(qse_sio_t*)epa->handle,
			data,
			size
		);
	}
	else if (cmd == QSE_AWK_IO_FLUSH)
	{
		return qse_sio_flush ((qse_sio_t*)epa->handle);
	}
	else if (cmd == QSE_AWK_IO_NEXT)
	{
		int n;
		qse_sio_t* fp = (qse_sio_t*)epa->handle;

		//dprint (QSE_T("switching console[%s] of type %x\n"), epa->name, epa->type);

		n = open_extio_console(epa);
		if (n == -1) return -1;

		if (n == 0) 
		{
			/* if there is no more file, keep the previous handle */
			return 0;
		}

		if (fp != QSE_NULL && 
		    fp != qse_sio_in && 
		    fp != qse_sio_out &&
		    fp != qse_sio_err) 
		{
			qse_sio_close (fp);
		}

		return n;
	}

	return -1;
}

int qse_awk_runsimple (qse_awk_t* awk, qse_char_t** icf, qse_awk_runcbs_t* cbs)
{
	qse_awk_runios_t ios;
	runio_data_t rd;
	rxtn_t rxtn;
	qse_ntime_t now;

	rd.ic.files = icf;
	rd.ic.index = 0;

	ios.pipe = awk_extio_pipe;
	ios.file = awk_extio_file;
	ios.console = awk_extio_console;
	ios.data = &rd;

	if (qse_gettime (&now) == -1) rxtn.seed = 0;
	else rxtn.seed = (unsigned int)now;
	srand (rxtn.seed);

	return qse_awk_run (
		awk, 
		QSE_NULL/*mfn*/,
		&ios,
		cbs,
		QSE_NULL/*runarg*/,
		&rxtn/*QSE_NULL*/
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
	qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl, int type, void* f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_getarg (run, 0);

	n = qse_awk_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 0) rv = (qse_real_t)lv;

	if (type == BFN_MATH_LD)
	{
		long double (*rf) (long double) = 
			(long double(*)(long double))f;
		r = qse_awk_makerealval (run, rf(rv));
	}
	else if (type == BFN_MATH_D)
	{
		double (*rf) (double) = (double(*)(double))f;
		r = qse_awk_makerealval (run, rf(rv));
	}
	else 
	{
		QSE_ASSERT (type == BFN_MATH_F);
		float (*rf) (float) = (float(*)(float))f;
		r = qse_awk_makerealval (run, rf(rv));
	}
	
	if (r == QSE_NULL)
	{
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_setretval (run, r);
	return 0;
}

static int bfn_math_2 (
	qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl, int type, void* f)
{
	qse_size_t nargs;
	qse_awk_val_t* a0, * a1;
	qse_long_t lv0, lv1;
	qse_real_t rv0, rv1;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_getnargs (run);
	QSE_ASSERT (nargs == 2);

	a0 = qse_awk_getarg (run, 0);
	a1 = qse_awk_getarg (run, 1);

	n = qse_awk_valtonum (run, a0, &lv0, &rv0);
	if (n == -1) return -1;
	if (n == 0) rv0 = (qse_real_t)lv0;

	n = qse_awk_valtonum (run, a1, &lv1, &rv1);
	if (n == -1) return -1;
	if (n == 0) rv1 = (qse_real_t)lv1;

	if (type == BFN_MATH_LD)
	{
		long double (*rf) (long double,long double) = 
			(long double(*)(long double,long double))f;
		r = qse_awk_makerealval (run, rf(rv0,rv1));
	}
	else if (type == BFN_MATH_D)
	{
		double (*rf) (double,double) = (double(*)(double,double))f;
		r = qse_awk_makerealval (run, rf(rv0,rv1));
	}
	else 
	{
		QSE_ASSERT (type == BFN_MATH_F);
		float (*rf) (float,float) = (float(*)(float,float))f;
		r = qse_awk_makerealval (run, rf(rv0,rv1));
	}
	
	if (r == QSE_NULL)
	{
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_setretval (run, r);
	return 0;
}

static int bfn_sin (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_cos (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_tan (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_atan (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_atan2 (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_log (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_exp (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_sqrt (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
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

static int bfn_int (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_getarg (run, 0);

	n = qse_awk_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (qse_long_t)rv;

	r = qse_awk_makeintval (run, lv);
	if (r == QSE_NULL)
	{
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_setretval (run, r);
	return 0;
}

static int bfn_rand (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_awk_val_t* r;

	r = qse_awk_makeintval (run, rand());
	if (r == QSE_NULL)
	{
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_setretval (run, r);
	return 0;
}

static int bfn_srand (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_real_t rv;
	qse_awk_val_t* r;
	int n;
	unsigned int prev;
	rxtn_t* rxtn;

	rxtn = qse_awk_getrundata (run);
	nargs = qse_awk_getnargs (run);
	QSE_ASSERT (nargs == 0 || nargs == 1);

	prev = rxtn->seed;

	if (nargs == 1)
	{
		a0 = qse_awk_getarg (run, 0);

		n = qse_awk_valtonum (run, a0, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (qse_long_t)rv;

		rxtn->seed = lv;
	}
	else
	{
		qse_ntime_t now;

		if (qse_gettime(&now) == -1) rxtn->seed >>= 1;
		else rxtn->seed = (unsigned int)now;
	}

        srand (rxtn->seed);

	r = qse_awk_makeintval (run, prev);
	if (r == QSE_NULL)
	{
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_setretval (run, r);
	return 0;
}

static int bfn_systime (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	qse_awk_val_t* r;
	qse_ntime_t now;
	int n;
	
	if (qse_gettime(&now) == -1)
		r = qse_awk_makeintval (run, QSE_TYPE_MIN(qse_long_t));
	else
		r = qse_awk_makeintval (run, now / QSE_MSECS_PER_SEC);

	if (r == QSE_NULL)
	{
		qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
		return -1;
	}

	qse_awk_setretval (run, r);
	return 0;
}

static int bfn_gmtime (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
/* TODO: *********************** */
	qse_ntime_t nt;
	qse_btime_t bt;


	qse_gmtime (nt, &bt);
	/* TODO: create an array containing
	 *       .....
	 */
	return -1;
}

static int bfn_localtime (qse_awk_run_t* run, const qse_char_t* fnm, qse_size_t fnl)
{
	return -1;
}

#define ADD_FUNC(awk,name,min,max,bfn) \
        if (qse_awk_addfunc (\
		(awk), (name), qse_strlen(name), \
		0, (min), (max), QSE_NULL, (bfn)) == QSE_NULL) return -1;

static int add_functions (qse_awk_t* awk)
{
        ADD_FUNC (awk, QSE_T("sin"),        1, 1, bfn_sin);
        ADD_FUNC (awk, QSE_T("cos"),        1, 1, bfn_cos);
        ADD_FUNC (awk, QSE_T("tan"),        1, 1, bfn_tan);
        ADD_FUNC (awk, QSE_T("atan"),       1, 1, bfn_atan);
        ADD_FUNC (awk, QSE_T("atan2"),      2, 2, bfn_atan2);
        ADD_FUNC (awk, QSE_T("log"),        1, 1, bfn_log);
        ADD_FUNC (awk, QSE_T("exp"),        1, 1, bfn_exp);
        ADD_FUNC (awk, QSE_T("sqrt"),       1, 1, bfn_sqrt);
        ADD_FUNC (awk, QSE_T("int"),        1, 1, bfn_int);
        ADD_FUNC (awk, QSE_T("rand"),       0, 0, bfn_rand);
        ADD_FUNC (awk, QSE_T("srand"),      0, 1, bfn_srand);
        ADD_FUNC (awk, QSE_T("systime"),    0, 0, bfn_systime);
	ADD_FUNC (awk, QSE_T("gmtime"),     0, 0, bfn_gmtime);
	ADD_FUNC (awk, QSE_T("localtime"),  0, 0, bfn_localtime);
/*
        ADD_FUNC (awk, QSE_T("strftime"),   0, 2, bfn_strftime);
        ADD_FUNC (awk, QSE_T("strfgmtime"), 0, 2, bfn_strfgmtime);
        ADD_FUNC (awk, QSE_T("system"),     1, 1, bfn_system);
*/

	return 0;
}
