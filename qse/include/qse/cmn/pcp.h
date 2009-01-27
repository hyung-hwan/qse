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
   limitapcpns under the License.
 */

#ifndef _QSE_CMN_PCP_H_
#define _QSE_CMN_PCP_H_

/* (P)ipe to a (C)hild (P)rocess */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/tio.h>

enum qse_pcp_open_flag_t
{
	/* enable ase_char_t based IO */
	QSE_PCP_TEXT       = (1 << 0),

	/* invoke the command through a system shell 
	 * (/bin/sh on *nix, command.com on windows) */
	QSE_PCP_SHELL      = (1 << 1),

	QSE_PCP_WRITEIN    = (1 << 8),
	QSE_PCP_READOUT    = (1 << 9),
	QSE_PCP_READERR    = (1 << 10),

	QSE_PCP_ERRTOOUT   = (1 << 11),	
	QSE_PCP_OUTTOERR   = (1 << 12),	

	QSE_PCP_INTONUL    = (1 << 13),
	QSE_PCP_ERRTONUL   = (1 << 14),
	QSE_PCP_OUTTONUL   = (1 << 15),

	QSE_PCP_DROPIN     = (1 << 16),
	QSE_PCP_DROPOUT    = (1 << 17),
	QSE_PCP_DROPERR    = (1 << 18)
};

enum qse_pcp_wait_flag_t
{
	QSE_PCP_NOHANG     = (1 << 0),
	QSE_PCP_IGNINTR    = (1 << 1)
};

enum qse_pcp_hid_t
{
	QSE_PCP_IN  = 0,
	QSE_PCP_OUT = 1,
	QSE_PCP_ERR = 2
};

enum qse_pcp_err_t
{
	QSE_PCP_ENOERR = 0,
	QSE_PCP_ENOMEM,     /* out of memory */
	QSE_PCP_ENOHND,     /* no handle available */
	QSE_PCP_ECHILD,     /* the child is not valid */
	QSE_PCP_EINTR,      /* interrupted */
	QSE_PCP_ESYSCALL    /* system call error */
};

typedef enum qse_pcp_hid_t qse_pcp_hid_t;
typedef enum qse_pcp_err_t qse_pcp_err_t;

#ifdef _WIN32
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_pcp_hnd_t;
	typedef void* qse_pcp_pid_t;
#	define  QSE_PCP_HND_NIL ((qse_pcp_hnd_t)QSE_NULL)
#	define  QSE_PCP_PID_NIL ((qse_pcp_pid_t)QSE_NULL)
#else
	typedef int qse_pcp_hnd_t;
	typedef int qse_pcp_pid_t;
#	define  QSE_PCP_HND_NIL ((qse_pcp_hnd_t)-1)
#	define  QSE_PCP_PID_NIL ((qse_pcp_hnd_t)-1)
#endif

typedef struct qse_pcp_t qse_pcp_t;
typedef struct qse_pcp_pip_t qse_pcp_pip_t;

struct qse_pcp_pip_t
{
	qse_pcp_hnd_t handle;
	qse_tio_t*    tio;
	qse_pcp_t*    self;	
};

struct qse_pcp_t
{
	QSE_DEFINE_STD_FIELDS(pcp)

	qse_pcp_err_t errnum;
	qse_pcp_pid_t child;
	qse_pcp_pip_t pip[3];
};

#define QSE_PCP_MMGR(pcp)       ((pcp)->mmgr)
#define QSE_PCP_XTN(pcp)        ((void*)(((qse_pcp_t*)pcp) + 1))
#define QSE_PCP_HANDLE(pcp,hid) ((pcp)->pip[3].handle)
#define QSE_PCP_CHILD(pcp)      ((pcp)->child)
#define QSE_PCP_ERRNUM(pcp)     ((pcp)->errnum)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_STD_FUNCTIONS (pcp)

/****f* qse.cmn.pcp/qse_pcp_open
 * NAME
 *  qse_pcp_open - open pipes to a child process
 *
 * DESCRIPTION
 *  QSE_PCP_SHELL drives the funcpcpn to execute the command via /bin/sh.
 *  If flags is clear of QSE_PCP_SHELL, you should pass the full program path.
 *
 * SYNOPSIS
 */
qse_pcp_t* qse_pcp_open (
	qse_mmgr_t*       mmgr,
	qse_size_t        ext,
	const qse_char_t* cmd,
	int               flags
);
/******/

/****f* qse.cmn.pcp/qse_pcp_close
 * NAME
 *  qse_pcp_close - close pipes to a child process
 *
 * SYNOPSIS
 */
void qse_pcp_close (
	qse_pcp_t* pcp
);
/******/

/****f* qse.cmn/pcp/qse_pcp_init
 * NAME
 *  qse_pcp_init - initialize pipes to a child process
 *
 * SYNOPSIS
 */
qse_pcp_t* qse_pcp_init (
	qse_pcp_t*        pcp,
	qse_mmgr_t*       mmgr,
	const qse_char_t* path,
	int               flags
);
/******/

/****f* qse.cmn/pcp/qse_pcp_fini
 * NAME
 *  qse_pcp_fini - finalize pipes to a child process
 *
 * SYNOPSIS
 */
void qse_pcp_fini (
	qse_pcp_t* pcp
);
/******/

/****f* qse.cmn.pcp/qse_pcp_geterrnum
 * NAME
 *  qse_pcp_geterrnum - get an error code
 *
 * SYNOPSIS
 */
qse_pcp_err_t qse_pcp_geterrnum (qse_pcp_t* pcp);
/******/

/****f* qse.cmn.pcp/qse_pcp_geterrstr
 * NAME
 *  qse_pcp_geterrstr - transllate an error code to a string
 *
 * DESCRIPTION
 *  The qse_pcp_geterrstr() funcpcpn returns the pointer to a constant string 
 *  describing the last error occurred.
 *
 * SYNOPSIS
 */
const qse_char_t* qse_pcp_geterrstr (qse_pcp_t* pcp);
/******/

/****f* qse.cmn.pcp/qse_pcp_gethandle
 * NAME
 *  qse_pcp_gethandle - get native handle
 *
 * SYNOPSIS
 */
qse_pcp_hnd_t qse_pcp_gethandle (
	qse_pcp_t*    pcp,
	qse_pcp_hid_t hid
);
/******/

/****f* qse.cmn.pcp/qse_pcp_getchild
 * NAME
 *  qse_pcp_getchild - get the PID of a child process
 *
 * SYNOPSIS
 */
qse_pcp_pid_t qse_pcp_getchild (
	qse_pcp_t*    pcp
);
/******/

/****f* qse.cmn.pcp/qse_pcp_read
 * NAME
 *  qse_pcp_read - read data
 * 
 * SYNOPSIS
 */
qse_ssize_t qse_pcp_read (
	qse_pcp_t*    pcp,
	void*         buf,
	qse_size_t    size,
	qse_pcp_hid_t hid
);
/******/

/****f* qse.cmn.pcp/qse_pcp_write
 * NAME 
 *  qse_pcp_write - write data
 *
 * DESCRIPTION
 *  If the parameter 'size' is zero, qse_pcp_write() closes the the writing
 *  stream causing the child process reach the end of the stream.
 *
 * SYNOPSIS
 */
qse_ssize_t qse_pcp_write (
	qse_pcp_t*    pcp,
	const void*   data,
	qse_size_t    size,
	qse_pcp_hid_t hid
);
/******/

/****f* qse.cmn.pcp/qse_pcp_flush
 * NAME
 *  qse_pcp_flush - flush data
 *
 * SYNOPSIS
 */
qse_ssize_t qse_pcp_flush (
	qse_pcp_t*    pcp,
	qse_pcp_hid_t hid
);
/*****/

/****f* qse.cmn.pcp/qse_pcp_end
 * NAME
 *  qse_pcp_end - close native handle
 *
 * SYNOPSIS
 */
void qse_pcp_end (
	qse_pcp_t*    pcp,
	qse_pcp_hid_t hid
);
/******/

/****f* qse.cmn.pcp/qse_pcp_wait
 * NAME
 *  qse_pcp_wait - wait for a child process 
 *
 * DESCRIPTION
 *  QSE_PCP_IGNINTR causes the function to retry when the underlying system
 *  call is interrupted. 
 *  When you specify QSE_PCP_NOHANG, the return value of 256 indicates that the
 *  child process has not terminated. If the flag is not specified, 256 will 
 *  never be returned.
 * 
 * RETURN
 *  -1 on error, 256 if the child is alive and QSE_PCP_NOHANG is specified,
 *  a number between 0 and 255 inclusive if the child process ends normally,
 *  256 + signal number if the child process is terminated by a signal.
 *   
 * SYNOPSIS
 */
int qse_pcp_wait (
	qse_pcp_t* pcp,
	int        flags
);
/******/

/****f* qse.cmn.pcp/qse_pcp_kill
 * NAME
 *  qse_pcp_kill - terminate the child process
 *
 * NOTES
 *  You should know the danger of calling this function as the function can
 *  kill a process that is not your child process if it has terminated but
 *  there is a new process with the same process handle.
 *
 * SYNOPSIS
 */ 
int qse_pcp_kill (
	qse_pcp_t* pcp
);
/******/

#ifdef __cplusplus
}
#endif

#endif
