/*
 * $Id: sio.c,v 1.30 2006/01/15 06:51:35 bacon Ease $
 */

#include <ase/cmn/sio.h>
#include "mem.h"

static ase_ssize_t __sio_input (int cmd, void* arg, void* buf, ase_size_t size);
static ase_ssize_t __sio_output (int cmd, void* arg, void* buf, ase_size_t size);

static ase_sio_t __sio_in = 
{
	ASE_NULL, /* mmgr */

	/* fio */
	{
		ASE_NULL,
	#ifdef _WIN32
		(HANDLE)STD_INPUT_HANDLE,
	#else
		0,
	#endif
	},

	/* tio */
	{
		ASE_NULL,
		0,

		__sio_input,
		__sio_output,
		&__sio_in,	
		&__sio_in,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

static ase_sio_t __sio_out = 
{
	ASE_NULL, /* mmgr */

	/* fio */
	{
		ASE_NULL,
	#ifdef _WIN32
		(HANDLE)STD_OUTPUT_HANDLE,
	#else
		1
	#endif
	},

	/* tio */
	{
		ASE_NULL,
		0,

		__sio_input,
		__sio_output,
		&__sio_out,	
		&__sio_out,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

static ase_sio_t __sio_err = 
{
	ASE_NULL, /* mmgr */

	/* fio */
	{
		ASE_NULL,
	#ifdef _WIN32
		(HANDLE)STD_ERROR_HANDLE,
	#else
		2
	#endif
	},

	/* tio */
	{
		ASE_NULL,
		0,

		__sio_input,
		__sio_output,
		&__sio_err,	
		&__sio_err,

		0,
		0,
		0,
		0,

		{ 0 },
		{ 0 }
	}
};

ase_sio_t* ase_sio_in = &__sio_in;
ase_sio_t* ase_sio_out = &__sio_out;
ase_sio_t* ase_sio_err = &__sio_err;

ase_sio_t* ase_sio_open (
	ase_mmgr_t* mmgr, ase_size_t ext, const ase_char_t* file, int flags)
{
	ase_sio_t* sio;

	if (mmgr == ASE_NULL)
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

	sio = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_sio_t) + ext);
	if (sio == ASE_NULL) return ASE_NULL;

	if (ase_sio_init (sio, mmgr, file, flags) == ASE_NULL)
	{
		ASE_MMGR_FREE (mmgr, sio);
		return ASE_NULL;
	}

	return sio;
}

void ase_sio_close (ase_sio_t* sio)
{
	ase_sio_fini (sio);
	ASE_MMGR_FREE (sio->mmgr, sio);
}

ase_sio_t* ase_sio_init (
	ase_sio_t* sio, ase_mmgr_t* mmgr, const ase_char_t* file, int flags)
{
	ASE_MEMSET (sio, 0, ASE_SIZEOF(*sio));
	sio->mmgr = mmgr;

	if (ase_fio_init (&sio->fio, mmgr, file, flags, 0644) == ASE_NULL) 
	{
		return ASE_NULL;
	}

	if (ase_tio_init(&sio->tio, mmgr) == ASE_NULL) 
	{
		ase_fio_fini (&sio->fio);
		return ASE_NULL;
	}

	if (ase_tio_attachin(&sio->tio, __sio_input, sio) == -1 ||
	    ase_tio_attachout(&sio->tio, __sio_output, sio) == -1) 
	{
		ase_tio_fini (&sio->tio);	
		ase_fio_fini (&sio->fio);
		return ASE_NULL;
	}

	return sio;
}

void ase_sio_fini (ase_sio_t* sio)
{
	/*if (ase_sio_flush (sio) == -1) return -1;*/
	ase_sio_flush (sio);
	ase_tio_fini (&sio->tio);
	ase_fio_fini (&sio->fio);

	if (sio == ase_sio_in) ase_sio_in = ASE_NULL;
	else if (sio == ase_sio_out) ase_sio_out = ASE_NULL;
	else if (sio == ase_sio_err) ase_sio_err = ASE_NULL;
}

ase_sio_hnd_t ase_sio_gethandle (ase_sio_t* sio)
{
	/*return ase_fio_gethandle (&sio->fio);*/
	return ASE_FIO_HANDLE(&sio->fio);
}

ase_ssize_t ase_sio_flush (ase_sio_t* sio)
{
	return ase_tio_flush (&sio->tio);
}

void ase_sio_purge (ase_sio_t* sio)
{
	ase_tio_purge (&sio->tio);
}

ase_ssize_t ase_sio_getc (ase_sio_t* sio, ase_char_t* c)
{
	return ase_tio_getc (&sio->tio, c);
}

ase_ssize_t ase_sio_gets (ase_sio_t* sio, ase_char_t* buf, ase_size_t size)
{
	return ase_tio_gets (&sio->tio, buf, size);
}

ase_ssize_t ase_sio_getsx (ase_sio_t* sio, ase_char_t* buf, ase_size_t size)
{
	return ase_tio_getsx (&sio->tio, buf, size);
}

ase_ssize_t ase_sio_getstr (ase_sio_t* sio, ase_str_t* buf)
{
	return ase_tio_getstr (&sio->tio, buf);
}

ase_ssize_t ase_sio_putc (ase_sio_t* sio, ase_char_t c)
{
	return ase_tio_putc (&sio->tio, c);
}

ase_ssize_t ase_sio_puts (ase_sio_t* sio, const ase_char_t* str)
{
	return ase_tio_puts (&sio->tio, str);
}

ase_ssize_t ase_sio_putsx (ase_sio_t* sio, const ase_char_t* str, ase_size_t size)
{
	return ase_tio_putsx (&sio->tio, str, size);
}

#if 0
ase_ssize_t ase_sio_putsn (ase_sio_t* sio, ...)
{
	ase_ssize_t n;
	ase_va_list ap;

	ase_va_start (ap, sio);
	n = ase_tio_putsv (&sio->tio, ap);
	ase_va_end (ap);

	return n;
}

ase_ssize_t ase_sio_putsxn (ase_sio_t* sio, ...)
{
	ase_ssize_t n;
	ase_va_list ap;

	ase_va_start (ap, sio);
	n = ase_tio_putsxv (&sio->tio, ap);
	ase_va_end (ap);

	return n;
}

ase_ssize_t ase_sio_putsv (ase_sio_t* sio, ase_va_list ap)
{
	return ase_tio_putsv (&sio->tio, ap);
}

ase_ssize_t ase_sio_putsxv (ase_sio_t* sio, ase_va_list ap)
{
	return ase_tio_putsxv (&sio->tio, ap);
}

int ase_sio_getpos (ase_sio_t* sio, ase_siopos_t* pos)
{
	ase_off_t off;

	off = ase_fio_seek (&sio->fio, 0, ASE_FIO_SEEK_CURRENT);
	if (off == (ase_off_t)-1) return -1;

	*pos = off;
	return 0;
}

int ase_sio_setpos (ase_sio_t* sio, ase_siopos_t pos)
{
	if (ase_sio_flush(sio) == -1) return -1;
	return (ase_fio_seek (&sio->fio,
		pos, ASE_FIO_SEEK_BEGIN) == (ase_off_t)-1)? -1: 0;
}

int ase_sio_rewind (ase_sio_t* sio)
{
	if (ase_sio_flush(sio) == -1) return -1;
	return (ase_fio_seek (&sio->fio, 
		0, ASE_FIO_SEEK_BEGIN) == (ase_off_t)-1)? -1: 0;
}

int ase_sio_movetoend (ase_sio_t* sio)
{
	if (ase_sio_flush(sio) == -1) return -1;
	return (ase_fio_seek (&sio->fio, 
		0, ASE_FIO_SEEK_END) == (ase_off_t)-1)? -1: 0;
}
#endif

static ase_ssize_t __sio_input (int cmd, void* arg, void* buf, ase_size_t size)
{
	ase_sio_t* sio = (ase_sio_t*)arg;

	ASE_ASSERT (sio != ASE_NULL);
	if (cmd == ASE_TIO_IO_DATA) 
	{
		return ase_fio_read (&sio->fio, buf, size);
	}

	return 0;
}

static ase_ssize_t __sio_output (int cmd, void* arg, void* buf, ase_size_t size)
{
	ase_sio_t* sio = (ase_sio_t*)arg;

	ASE_ASSERT (sio != ASE_NULL);

	if (cmd == ASE_TIO_IO_DATA) 
	{
		return ase_fio_write (&sio->fio, buf, size);
	}

	return 0;
}

