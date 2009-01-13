/*
 * $Id$
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

#ifndef _QSE_CMN_PIO_H_
#define _QSE_CMN_PIO_H_

#include <qse/types.h>
#include <qse/macros.h>

enum qse_pio_open_flag_t
{
	QSE_PIO_WRITEIN    = (1 << 0),
	QSE_PIO_READOUT    = (1 << 1),
	QSE_PIO_READERR    = (1 << 2),

	QSE_PIO_ERRTOOUT   = (1 << 3),	
	QSE_PIO_OUTTOERR   = (1 << 4),	

	QSE_PIO_INTONUL    = (1 << 5),
	QSE_PIO_ERRTONUL   = (1 << 6),
	QSE_PIO_OUTTONUL   = (1 << 7),

	QSE_PIO_DROPIN     = (1 << 8),
	QSE_PIO_DROPOUT    = (1 << 9),
	QSE_PIO_DROPERR    = (1 << 10),

	/* invoke the command through a default system shell */
	QSE_PIO_SHELL      = (1 << 11)
};

enum qse_pio_wait_flag_t
{
	QSE_PIO_NOWAIT     = (1 << 0),
	QSE_PIO_IGNINTR    = (1 << 1)
};

enum qse_pio_hid_t
{
	QSE_PIO_IN  = 0,
	QSE_PIO_OUT = 1,
	QSE_PIO_ERR = 2
};

typedef enum qse_pio_hid_t qse_pio_hid_t;

#ifdef _WIN32
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_pio_hnd_t;
	typedef void* qse_pio_pid_t;
#	define  QSE_PIO_HND_NIL ((qse_pio_hnd_t)QSE_NULL)
#	define  QSE_PIO_PID_NIL ((qse_pio_pid_t)QSE_NULL)
#else
	typedef int qse_pio_hnd_t;
	typedef int qse_pio_pid_t;
#	define  QSE_PIO_HND_NIL ((qse_pio_hnd_t)-1)
#	define  QSE_PIO_PID_NIL ((qse_pio_hnd_t)-1)
#endif

typedef struct qse_pio_t qse_pio_t;

struct qse_pio_t
{
	qse_mmgr_t*   mmgr;
	qse_pio_pid_t child;
	qse_pio_hnd_t handle[3];
};

#define QSE_PIO_MMGR(pio)       ((pio)->mmgr)
#define QSE_PIO_HANDLE(pio,hid) ((pio)->handle[hid])
#define QSE_PIO_CHILD(pio)      ((pio)->child)

#ifdef __cplusplus
extern "C" {
#endif

/****f* qse.cmn.pio/qse_pio_open
 * NAME
 *  qse_pio_open - open pipes to a child process
 *
 * DESCRIPTION
 *  QSE_PIO_SHELL drives the function to execute the command via /bin/sh.
 *  If flags is clear of QSE_PIO_SHELL, you should pass the full program path.
 *
 * SYNOPSIS
 */
qse_pio_t* qse_pio_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_char_t* cmd,
	int               flags
);
/******/

/****f* qse.cmn.pio/qse_pio_close
 * NAME
 *  qse_pio_close - close pipes to a child process
 *
 * SYNOPSIS
 */
void qse_pio_close (
	qse_pio_t* pio
);
/******/

/****f* qse.cmn/pio/qse_pio_init
 * NAME
 *  qse_pio_init - initialize pipes to a child process
 *
 * SYNOPSIS
 */
qse_pio_t* qse_pio_init (
	qse_pio_t*        pio,
	qse_mmgr_t*       mmgr,
	const qse_char_t* path,
	int               flags
);
/******/

/****f* qse.cmn/pio/qse_pio_fini
 * NAME
 *  qse_pio_fini - finalize pipes to a child process
 *
 * SYNOPSIS
 */
void qse_pio_fini (
	qse_pio_t* pio
);
/******/

/****f* qse.cmn.pio/qse_pio_gethandle
 * NAME
 *  qse_pio_gethandle - get native handle
 *
 * SYNOPSIS
 */
qse_pio_hnd_t qse_pio_gethandle (
	qse_pio_t*    pio,
	qse_pio_hid_t hid
);
/******/

/****f* qse.cmn.pio/qse_pio_getchild
 * NAME
 *  qse_pio_getchild - get the PID of a child process
 *
 * SYNOPSIS
 */
qse_pio_pid_t qse_pio_getchild (
	qse_pio_t*    pio
);
/******/

/****f* qse.cmn.pio/qse_pio_read
 * NAME
 *  qse_pio_read - read data
 * 
 * SYNOPSIS
 */
qse_ssize_t qse_pio_read (
	qse_pio_t*    pio,
	void*         buf,
	qse_size_t    size,
	qse_pio_hid_t hid
);
/******/

/****f* qse.cmn.pio/qse_pio_write
 * NAME 
 *  qse_pio_write - write data
 *
 * DESCRIPTION
 *  If the parameter 'size' is zero, qse_pio_write() closes the the writing
 *  stream causing the child process reach the end of the stream.
 *
 * SYNOPSIS
 */
qse_ssize_t qse_pio_write (
	qse_pio_t*    pio,
	const void*   data,
	qse_size_t    size,
	qse_pio_hid_t hid
);
/******/

/****f* qse.cmn.pio/qse_pio_end
 * NAME
 *  qse_pio_end
 *
 * SYNOPSIS
 */
void qse_pio_end (
	qse_pio_t*    pio,
	qse_pio_hid_t hid
);
/******/

/****f* qse.cmn.pio/qse_pio_wait
 * NAME
 *  qse_pio_wait - wait for a child process 
 *
 * SYNOPSIS
 */
int qse_pio_wait (
	qse_pio_t* pio,
	int        flags
);
/******/

#ifdef __cplusplus
}
#endif

#endif
