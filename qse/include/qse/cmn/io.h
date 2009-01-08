/*
 * $Id: sysapi.h,v 1.56 2006/03/21 16:15:16 bacon Ease $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_CMN_IO_H_
#define _QSE_CMN_IO_H_

#include <qse/types.h>
#include <qse/macros.h>

/* flags for qse_open */
enum 
{
	QSE_OPEN_READ      = (1 << 0),
	QSE_OPEN_WRITE     = (1 << 1),
	QSE_OPEN_CREATE    = (1 << 2),
	QSE_OPEN_TRUNCATE  = (1 << 3),
	QSE_OPEN_EXCLUSIVE = (1 << 4),
	QSE_OPEN_APPEND    = (1 << 5),
	QSE_OPEN_NONBLOCK  = (1 << 6)
};

/* origin for qse_seek */
enum
{
	QSE_SEEK_BEGIN   = 0,
	QSE_SEEK_CURRENT = 1,
	QSE_SEEK_END     = 2
};

#ifdef __cplusplus
extern "C" {
#endif

qse_hnd_t qse_open (
	const qse_char_t* path,
	int flag, 
	...
);

int qse_close (
	qse_hnd_t handle
);

qse_ssize_t qse_read (
	qse_hnd_t handle,
	void* buf,
	qse_size_t sz
);

qse_ssize_t qse_write (
	qse_hnd_t handle,
	const void* data,
	qse_size_t sz
);

qse_off_t qse_seek (
	qse_hnd_t handle,
	qse_off_t offset,
	int origin
);

/*
int qse_hstat (qse_hnd_t handle, qse_stat_t* buf);
int qse_hchmod (qse_hnd_t handle, qse_mode_t mode);
*/
int qse_htruncate (qse_hnd_t handle, qse_off_t size);

#ifdef __cplusplus
}
#endif

#endif

