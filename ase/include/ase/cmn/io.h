/*
 * $Id: sysapi.h,v 1.56 2006/03/21 16:15:16 bacon Ease $
 */

#ifndef _ASE_CMN_IO_H_
#define _ASE_CMN_IO_H_

#include <ase/types.h>
#include <ase/macros.h>

/* flags for ase_open */
enum 
{
	ASE_OPEN_READ      = (1 << 0),
	ASE_OPEN_WRITE     = (1 << 1),
	ASE_OPEN_CREATE    = (1 << 2),
	ASE_OPEN_TRUNCATE  = (1 << 3),
	ASE_OPEN_EXCLUSIVE = (1 << 4),
	ASE_OPEN_APPEND    = (1 << 5),
	ASE_OPEN_NONBLOCK  = (1 << 6)
};

/* origin for ase_seek */
enum
{
	ASE_SEEK_BEGIN   = 0,
	ASE_SEEK_CURRENT = 1,
	ASE_SEEK_END     = 2
};

#ifdef __cplusplus
extern "C" {
#endif

ase_hnd_t ase_open (
	const ase_char_t* path,
	int flag, 
	...
);

int ase_close (
	ase_hnd_t handle
);

ase_ssize_t ase_read (
	ase_hnd_t handle,
	void* buf,
	ase_size_t sz
);

ase_ssize_t ase_write (
	ase_hnd_t handle,
	const void* data,
	ase_size_t sz
);

ase_off_t ase_seek (
	ase_hnd_t handle,
	ase_off_t offset,
	int origin
);

/*
int ase_hstat (ase_hnd_t handle, ase_stat_t* buf);
int ase_hchmod (ase_hnd_t handle, ase_mode_t mode);
*/
int ase_htruncate (ase_hnd_t handle, ase_off_t size);

#ifdef __cplusplus
}
#endif

#endif

