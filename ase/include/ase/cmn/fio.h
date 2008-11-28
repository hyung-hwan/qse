/*
 * $Id$
 */

#ifndef _ASE_CMN_FIO_H_
#define _ASE_CMN_FIO_H_

#include <ase/types.h>
#include <ase/macros.h>

enum ase_fio_open_flag_t
{
	/* treat the file name pointer as a handle pointer */
	ASE_FIO_HANDLE    = (1 << 0),

	ASE_FIO_READ      = (1 << 1),
	ASE_FIO_WRITE     = (1 << 2),
	ASE_FIO_CREATE    = (1 << 3),
	ASE_FIO_TRUNCATE  = (1 << 4),
	ASE_FIO_EXCLUSIVE = (1 << 5),
	ASE_FIO_APPEND    = (1 << 6),
	ASE_FIO_SYNC      = (1 << 7),

	/* for ms windows only */
	ASE_FIO_NOSHRD    = (1 << 16),
	ASE_FIO_NOSHWR    = (1 << 17)
};

/* seek origin */
enum ase_fio_seek_origin_t
{
	ASE_FIO_BEGIN   = 0,
	ASE_FIO_CURRENT = 1,
	ASE_FIO_END     = 2
};

#ifdef _WIN32
/* <winnt.h> typedef PVOID HANDLE; */
typedef void* aes_fio_hnd_t; 
#else
typedef int ase_fio_hnd_t;
#endif

/* file offset */
typedef ase_int64_t ase_fio_off_t;
typedef enum ase_fio_seek_origin_t ase_fio_ori_t;

typedef struct ase_fio_t ase_fio_t;

struct ase_fio_t
{
	ase_mmgr_t* mmgr;
	ase_fio_hnd_t handle;
};

#define ASE_FIO_MMGR(fio)   ((fio)->mmgr)
#define ASE_FIO_HANDLE(fio) ((fio)->handle)

#ifdef __cplusplus
extern "C" {
#endif

ase_fio_t* ase_fio_open (
	ase_mmgr_t* mmgr,
	ase_size_t ext,
	const ase_char_t* path,
	int flags,
	int mode
);

void ase_fio_close (
	ase_fio_t* fio
);

ase_fio_t* ase_fio_init (
	ase_fio_t* fio,
	ase_mmgr_t* mmgr,
	const ase_char_t* path,
	int flags,
	int mode
);

void ase_fio_fini (
	ase_fio_t* fio
);

ase_fio_hnd_t ase_fio_gethandle (
	ase_fio_t* fio
);

ase_fio_off_t ase_fio_seek (
	ase_fio_t* fio,
	ase_fio_off_t offset,
	ase_fio_ori_t origin
);

ase_ssize_t ase_fio_read (
	ase_fio_t* fio,
	void* buf,
	ase_size_t size
);

ase_ssize_t ase_fio_write (
	ase_fio_t* fio,
	const void* buf,
	ase_size_t size
);

#ifdef __cplusplus
}
#endif

#endif
