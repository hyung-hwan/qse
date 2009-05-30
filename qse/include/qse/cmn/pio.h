/*
 * $Id: pio.h 168 2009-05-30 01:19:46Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitapions under the License.
 */

#ifndef _QSE_CMN_PIO_H_
#define _QSE_CMN_PIO_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/tio.h>

/** @file 
 * Pipe I/O
 * @todo 
 * - rename flags to option
 * - write code for win32
 */

enum qse_pio_open_flag_t
{
	/* enable ase_char_t based IO */
	QSE_PIO_TEXT       = (1 << 0),

	/* invoke the command through a system shell 
	 * (/bin/sh on *nix, command.com on windows) */
	QSE_PIO_SHELL      = (1 << 1),

	QSE_PIO_WRITEIN    = (1 << 8),
	QSE_PIO_READOUT    = (1 << 9),
	QSE_PIO_READERR    = (1 << 10),

	QSE_PIO_ERRTOOUT   = (1 << 11),	
	QSE_PIO_OUTTOERR   = (1 << 12),	

	QSE_PIO_INTONUL    = (1 << 13),
	QSE_PIO_ERRTONUL   = (1 << 14),
	QSE_PIO_OUTTONUL   = (1 << 15),

	QSE_PIO_DROPIN     = (1 << 16),
	QSE_PIO_DROPOUT    = (1 << 17),
	QSE_PIO_DROPERR    = (1 << 18),
};

enum qse_pio_hid_t
{
	QSE_PIO_IN  = 0,
	QSE_PIO_OUT = 1,
	QSE_PIO_ERR = 2
};

enum qse_pio_io_flag_t
{
	/*QSE_PIO_READ_NOBLOCK   = (1 << 0),*/
	QSE_PIO_READ_NORETRY   = (1 << 1),
	/*QSE_PIO_WRITE_NOBLOCK  = (1 << 2),*/
	QSE_PIO_WRITE_NORETRY  = (1 << 3),
	QSE_PIO_WAIT_NOBLOCK   = (1 << 4),
	QSE_PIO_WAIT_NORETRY   = (1 << 5)
};

enum qse_pio_errnum_t
{
	QSE_PIO_ENOERR = 0,
	QSE_PIO_ENOMEM,     /* out of memory */
	QSE_PIO_ENOHND,     /* no handle available */
	QSE_PIO_ECHILD,     /* the child is not valid */
	QSE_PIO_EINTR,      /* interrupted */
	QSE_PIO_ESUBSYS     /* subsystem(system call) error */
};

typedef enum qse_pio_hid_t qse_pio_hid_t;
typedef enum qse_pio_errnum_t qse_pio_errnum_t;

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
typedef struct qse_pio_pin_t qse_pio_pin_t;

struct qse_pio_pin_t
{
	qse_pio_hnd_t handle;
	qse_tio_t*    tio;
	qse_pio_t*    self;	
};

/**
 * The qse_pio_t type defines a pipe I/O type
 */
struct qse_pio_t
{
	QSE_DEFINE_COMMON_FIELDS(pio)

	int              flags;
	qse_pio_errnum_t errnum;
	qse_pio_pid_t    child;
	qse_pio_pin_t    pin[3];
};

#define QSE_PIO_ERRNUM(pio)     ((pio)->errnum)
#define QSE_PIO_FLAGS(pio)      ((pio)->flags)
#define QSE_PIO_CHILD(pio)      ((pio)->child)
#define QSE_PIO_HANDLE(pio,hid) ((pio)->pin[hid].handle)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (pio)

/**
 * The qse_pio_open() function opens pipes to a child process.
 * QSE_PIO_SHELL drives the function to execute the command via /bin/sh.
 * If flags is clear of QSE_PIO_SHELL, you should pass the full program path.
 */
qse_pio_t* qse_pio_open (
	qse_mmgr_t*       mmgr,  /**< a memory manager */
	qse_size_t        ext,   /**< extension size */
	const qse_char_t* cmd,   /**< a command to execute */
	int               flags  /**< options */
);

/**
 * The qse_pio_close() function closes pipes to a child process.
 */
void qse_pio_close (
	qse_pio_t* pio
);

/**
 * The qse_pio_init() function initializes pipes to a child process.
 */
qse_pio_t* qse_pio_init (
	qse_pio_t*        pio,
	qse_mmgr_t*       mmgr,
	const qse_char_t* path,
	int               flags
);

/**
 * The qse_pio_fini() function finalizes pipes to a child process.
 */
void qse_pio_fini (
	qse_pio_t* pio
);

int qse_pio_getflags (
	qse_pio_t* pio
);

void qse_pio_setflags (
	qse_pio_t* pio,
	int        flags,
	int        opt
);

/****f* Common/qse_pio_geterrnum
 * NAME
 *  qse_pio_geterrnum - get an error code
 *
 * SYNOPSIS
 */
qse_pio_errnum_t qse_pio_geterrnum (
	qse_pio_t* pio
);
/******/

/****f* Common/qse_pio_geterrmsg
 * NAME
 *  qse_pio_geterrstr - transllate an error code to a string
 *
 * DESCRIPTION
 *  The qse_pio_geterrstr() funcpion returns the pointer to a constant string 
 *  describing the last error occurred.
 *
 * SYNOPSIS
 */
const qse_char_t* qse_pio_geterrstr (
	qse_pio_t* pio
);
/******/

/****f* Common/qse_pio_gethandle
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

/****f* Common/qse_pio_getchild
 * NAME
 *  qse_pio_getchild - get the PID of a child process
 *
 * SYNOPSIS
 */
qse_pio_pid_t qse_pio_getchild (
	qse_pio_t*    pio
);
/******/

/****f* Common/qse_pio_read
 * NAME
 *  qse_pio_read - read data
 * SYNOPSIS
 */
qse_ssize_t qse_pio_read (
	qse_pio_t*    pio,
	void*         buf,
	qse_size_t    size,
	qse_pio_hid_t hid
);
/******/

/****f* Common/qse_pio_write
 * NAME 
 *  qse_pio_write - write data
 * DESCRIPTION
 *  If the parameter 'size' is zero, qse_pio_write() closes the the writing
 *  stream causing the child process reach the end of the stream.
 * SYNOPSIS
 */
qse_ssize_t qse_pio_write (
	qse_pio_t*    pio,
	const void*   data,
	qse_size_t    size,
	qse_pio_hid_t hid
);
/******/

/****f* Common/qse_pio_flush
 * NAME
 *  qse_pio_flush - flush data
 *
 * SYNOPSIS
 */
qse_ssize_t qse_pio_flush (
	qse_pio_t*    pio,
	qse_pio_hid_t hid
);
/*****/

/****f* Common/qse_pio_end
 * NAME
 *  qse_pio_end - close native handle
 *
 * SYNOPSIS
 */
void qse_pio_end (
	qse_pio_t*    pio,
	qse_pio_hid_t hid
);
/******/

/****f* Common/qse_pio_wait
 * NAME
 *  qse_pio_wait - wait for a child process 
 * DESCRIPTION
 *  QSE_PIO_WAIT_NORETRY causes the function to return an error and set the 
 *  errnum field to QSE_PIO_EINTR if the underlying system call is interrupted.
 *
 *  When QSE_PIO_WAIT_NOBLOCK is used, the return value of 256 indicates that 
 *  the child process has not terminated. If the flag is not used, 256 is never 
 *  returned.
 * RETURN
 *  -1 on error, 256 if the child is alive and QSE_PIO_NOBLOCK is used,
 *  a number between 0 and 255 inclusive if the child process ends normally,
 *  256 + signal number if the child process is terminated by a signal.
 * SYNOPSIS
 */
int qse_pio_wait (
	qse_pio_t* pio
);
/******/

/****f* Common/qse_pio_kill
 * NAME
 *  qse_pio_kill - terminate the child process
 * NOTES
 *  You should know the danger of calling this function as the function can
 *  kill a process that is not your child process if it has terminated but
 *  there is a new process with the same process handle.
 * SYNOPSIS
 */ 
int qse_pio_kill (
	qse_pio_t* pio
);
/******/

#ifdef __cplusplus
}
#endif

#endif
