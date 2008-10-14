/*
 * $Id: tio.c,v 1.13 2006/01/01 13:50:24 bacon Exp $
 */

#include <ase/cmn/tio.h>
#include "mem.h"

ase_tio_t* ase_tio_open (ase_mmgr_t* mmgr, ase_size_t ext)
{
	ase_tio_t* tio;

	if (mmgr == ASE_NULL)
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

	tio = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_tio_t) + ext);
	if (tio == ASE_NULL) return ASE_NULL;

	if (ase_tio_init (tio, mmgr) == ASE_NULL)
	{
		ASE_MMGR_FREE (mmgr, tio);
		return ASE_NULL;
	}

	return tio;
}

int ase_tio_close (ase_tio_t* tio)
{
	int n = ase_tio_fini (tio);
	ASE_MMGR_FREE (tio->mmgr, tio);
	return n;
}

ase_tio_t* ase_tio_init (ase_tio_t* tio, ase_mmgr_t* mmgr)
{
	ASE_MEMSET (tio, 0, ASE_SIZEOF(*tio));

	tio->mmgr = mmgr;

	/*
	tio->input_func = ASE_NULL;
	tio->input_arg = ASE_NULL;
	tio->output_func = ASE_NULL;
	tio->output_arg = ASE_NULL;

        tio->input_status = 0;
        tio->inbuf_curp = 0;
        tio->inbuf_len = 0;
        tio->outbuf_len = 0;
	*/

	tio->errnum = ASE_TIO_ENOERR;

	return tio;
}

int ase_tio_fini (ase_tio_t* tio)
{
	ase_tio_flush (tio); /* don't care about the result */
	if (ase_tio_detin(tio) == -1) return -1;
	if (ase_tio_detout(tio) == -1) return -1;
	return 0;
}

int ase_tio_geterrnum (ase_tio_t* tio)
{
	return tio->errnum;
}

const ase_char_t* ase_tio_geterrstr (ase_tio_t* tio)
{
	static const ase_char_t* __errstr[] =
	{
		ASE_T("no error"),
		ASE_T("out of memory"),
		ASE_T("no more space"),
		ASE_T("illegal utf-8 sequence"),
		ASE_T("no input function attached"),
		ASE_T("input function returned an error"),
		ASE_T("input function failed to open"),
		ASE_T("input function failed to closed"),
		ASE_T("no output function attached"),
		ASE_T("output function returned an error"),
		ASE_T("output function failed to open"),
		ASE_T("output function failed to closed"),
		ASE_T("unknown error")
	};

	return __errstr[
		(tio->errnum < 0 || tio->errnum >= ase_countof(__errstr))? 
		ase_countof(__errstr) - 1: tio->errnum];
}

int ase_tio_attin (ase_tio_t* tio, ase_tio_io_t input, void* arg)
{
	if (ase_tio_detin(tio) == -1) return -1;

	ASE_ASSERT (tio->input_func == ASE_NULL);

	if (input(ASE_TIO_IO_OPEN, arg, ASE_NULL, 0) == -1) 
	{
		tio->errnum = ASE_TIO_EINPOP;
		return -1;
	}

	tio->input_func = input;
	tio->input_arg = arg;

        tio->input_status = 0;
        tio->inbuf_curp = 0;
        tio->inbuf_len = 0;

	return 0;
}

int ase_tio_detin (ase_tio_t* tio)
{
	if (tio->input_func != ASE_NULL) 
	{
		if (tio->input_func (
			ASE_TIO_IO_CLOSE, tio->input_arg, ASE_NULL, 0) == -1) 
		{
			tio->errnum = ASE_TIO_EINPCL;
			return -1;
		}

		tio->input_func = ASE_NULL;
		tio->input_arg = ASE_NULL;
	}
		
	return 0;
}

int ase_tio_attout (ase_tio_t* tio, ase_tio_io_t output, void* arg)
{
	if (ase_tio_detout(tio) == -1) return -1;

	ASE_ASSERT (tio->output_func == ASE_NULL);

	if (output(ASE_TIO_IO_OPEN, arg, ASE_NULL, 0) == -1) 
	{
		tio->errnum = ASE_TIO_EOUTOP;
		return -1;
	}

	tio->output_func = output;
	tio->output_arg = arg;
        tio->outbuf_len = 0;

	return 0;
}

int ase_tio_detout (ase_tio_t* tio)
{
	if (tio->output_func != ASE_NULL) 
	{
		ase_tio_flush (tio); /* don't care about the result */

		if (tio->output_func (
			ASE_TIO_IO_CLOSE, tio->output_arg, ASE_NULL, 0) == -1) 
		{
			tio->errnum = ASE_TIO_EOUTCL;
			return -1;
		}

		tio->output_func = ASE_NULL;
		tio->output_arg = ASE_NULL;
	}
		
	return 0;
}

ase_ssize_t ase_tio_flush (ase_tio_t* tio)
{
	ase_size_t left, count;

	if (tio->output_func == ASE_NULL) 
	{
		tio->errnum = ASE_TIO_ENOUTF;
		return (ase_ssize_t)-1;
	}

	left = tio->outbuf_len;
	while (left > 0) 
	{
		ase_ssize_t n;

		n = tio->output_func (
			ASE_TIO_IO_DATA, tio->output_arg, tio->outbuf, left);
		if (n <= -1) 
		{
			tio->outbuf_len = left;
			tio->errnum = ASE_TIO_EOUTPT;
			return -1;
		}
		if (n == 0) break;
	
		left -= n;
		ase_memcpy (tio->outbuf, &tio->inbuf[n], left);
	}

	count = tio->outbuf_len - left;
	tio->outbuf_len = left;

	return (ase_ssize_t)count;
}

void ase_tio_purge (ase_tio_t* tio)
{
        tio->input_status = 0;
        tio->inbuf_curp = 0;
        tio->inbuf_len = 0;
        tio->outbuf_len = 0;
	tio->errnum = ASE_TIO_ENOERR;
}
