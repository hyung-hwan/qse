/*
 * $Id: sio.h,v 1.29 2005/12/26 05:38:24 bacon Ease $
 */
 
#ifndef _ASE_CMN_SIO_H_
#define _ASE_CMN_SIO_H_

#include <ase/types.h>
#include <ase/macros.h>
#include <ase/cmn/fio.h>
#include <ase/cmn/tio.h>

enum ase_sio_open_flag_t
{
        ASE_SIO_HANDLE    = ASE_FIO_HANDLE,

        ASE_SIO_READ      = ASE_FIO_READ,
        ASE_SIO_WRITE     = ASE_FIO_WRITE,
	ASE_SIO_APPEND    = ASE_FIO_APPEND,

	ASE_SIO_CREATE    = ASE_FIO_CREATE,
        ASE_SIO_TRUNCATE  = ASE_FIO_TRUNCATE,
	ASE_SIO_EXCLUSIVE = ASE_FIO_EXCLUSIVE,
	ASE_SIO_SYNC      = ASE_FIO_SYNC,

        ASE_SIO_NOSHRD    = ASE_FIO_NOSHRD,
        ASE_SIO_NOSHWR    = ASE_FIO_NOSHWR
};

typedef ase_fio_off_t ase_sio_off_t;
typedef ase_fio_hnd_t ase_sio_hnd_t;

typedef struct ase_sio_t ase_sio_t;

struct ase_sio_t
{
	ase_mmgr_t* mmgr;
	ase_fio_t   fio;
	ase_tio_t   tio;
};

#ifdef __cplusplus
extern "C" {
#endif

extern ase_sio_t* ase_sio_in;
extern ase_sio_t* ase_sio_out;
extern ase_sio_t* ase_sio_err;

ase_sio_t* ase_sio_open (
        ase_mmgr_t*       mmgr,
	ase_size_t        ext,
	const ase_char_t* file,
	int               flags
);

void ase_sio_close (
	ase_sio_t* sio
);

ase_sio_t* ase_sio_init (
        ase_sio_t*        sio,
	ase_mmgr_t*       mmgr,
	const ase_char_t* file,
	int flags
);

void ase_sio_fini (
	ase_sio_t* sio
);

ase_fio_hnd_t ase_sio_gethandle (
	ase_sio_t* sio
);

ase_ssize_t ase_sio_flush (
	ase_sio_t* sio
);

void ase_sio_purge (
	ase_sio_t* sio
);

ase_ssize_t ase_sio_getc (
	ase_sio_t* sio,
	ase_char_t* c
);

ase_ssize_t ase_sio_gets (
	ase_sio_t* sio,
	ase_char_t* buf,
	ase_size_t size
);

ase_ssize_t ase_sio_getsx (
	ase_sio_t* sio,
	ase_char_t* buf,
	ase_size_t size
);

ase_ssize_t ase_sio_getstr (
	ase_sio_t* sio, 
	ase_str_t* buf
);

ase_ssize_t ase_sio_putc (
	ase_sio_t* sio, 
	ase_char_t c
);

ase_ssize_t ase_sio_puts (
	ase_sio_t* sio,
	const ase_char_t* str
);

ase_ssize_t ase_sio_putsx (
	ase_sio_t* sio, 
	const ase_char_t* str,
	ase_size_t size
);

#if 0
ase_ssize_t ase_sio_putsn (ase_sio_t* sio, ...);
ase_ssize_t ase_sio_putsxn (ase_sio_t* sio, ...);
ase_ssize_t ase_sio_putsv (ase_sio_t* sio, ase_va_list ap);
ase_ssize_t ase_sio_putsxv (ase_sio_t* sio, ase_va_list ap);

/* WARNING:
 *   getpos may not return the desired postion because of the buffering 
 */
int ase_sio_getpos (ase_sio_t* sio, ase_sio_off_t* pos);
int ase_sio_setpos (ase_sio_t* sio, ase_sio_off_t pos);
int ase_sio_rewind (ase_sio_t* sio);
int ase_sio_movetoend (ase_sio_t* sio);
#endif


#ifdef __cplusplus
}
#endif

#endif
