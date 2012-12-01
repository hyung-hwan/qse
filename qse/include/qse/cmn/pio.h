/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_CMN_PIO_H_
#define _QSE_CMN_PIO_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/tio.h>
#include <qse/cmn/env.h>

/** @file 
 * This file defines a piped interface to a child process. You can execute
 * a child process, read and write to its stdin, stdout, stderr, and terminate
 * it. It provides more advanced interface than popen() and pclose().
 */

/**
 * The qse_pio_flag_t defines enumerators to compose flags to qse_pio_open().
 */
enum qse_pio_flag_t
{
	/** enable text based I/O. */
	QSE_PIO_TEXT          = (1 << 0),
     QSE_PIO_IGNOREMBWCERR = (1 << 1),
	QSE_PIO_NOAUTOFLUSH   = (1 << 2),

	/** execute the command via a system shell 
	 * (/bin/sh on unix/linux, cmd.exe on windows and os2) */
	QSE_PIO_SHELL         = (1 << 3),

	/** indicate that the command to qse_pio_open() is a multi-byte string.
	 *  it is useful if #QSE_CHAR_IS_WCHAR is defined. */
	QSE_PIO_MBSCMD        = (1 << 4),

	/** don't attempt to close open file descriptors unknown to pio.
	 *  it is useful only on a unix-like systems where file descriptors
	 *  not set with FD_CLOEXEC are inherited by a child process.
	 *  you're advised to set this option if all normal file descriptors 
	 *  in your application are open with FD_CLOEXEC set. it can skip 
	 *  checking a bunch of file descriptors and arranging to close
	 *  them to prevent inheritance. */
	QSE_PIO_NOCLOEXEC     = (1 << 5),

	/** write to stdin of a child process */
	QSE_PIO_WRITEIN       = (1 << 8),
	/** read stdout of a child process */
	QSE_PIO_READOUT       = (1 << 9),
	/** read stderr of a child process */
	QSE_PIO_READERR       = (1 << 10),

	/** redirect stderr to stdout (2>&1, require #QSE_PIO_READOUT) */
	QSE_PIO_ERRTOOUT      = (1 << 11),	
	/** redirect stdout to stderr (1>&2, require #QSE_PIO_READERR) */
	QSE_PIO_OUTTOERR      = (1 << 12),

	/** redirect stdin to the null device (</dev/null, <NUL) */
	QSE_PIO_INTONUL       = (1 << 13),
	/** redirect stdin to the null device (>/dev/null, >NUL) */
	QSE_PIO_ERRTONUL      = (1 << 14),
	/** redirect stderr to the null device (2>/dev/null, 2>NUL) */
	QSE_PIO_OUTTONUL      = (1 << 15),

	/** drop stdin */
	QSE_PIO_DROPIN        = (1 << 16), 
	/** drop stdout */
	QSE_PIO_DROPOUT       = (1 << 17),
	/** drop stderr */
	QSE_PIO_DROPERR       = (1 << 18),

	/** do not reread if read has been interrupted */
	QSE_PIO_READNORETRY   = (1 << 21), 
	/** do not rewrite if write has been interrupted */
	QSE_PIO_WRITENORETRY  = (1 << 22),
	/** return immediately from qse_pio_wait() if a child has not exited */
	QSE_PIO_WAITNOBLOCK   = (1 << 23),
	/** do not wait again if waitpid has been interrupted */
	QSE_PIO_WAITNORETRY   = (1 << 24)
};

/**
 * The qse_pio_hid_t type defines pipe IDs established to a child process.
 */
enum qse_pio_hid_t
{
	QSE_PIO_IN  = 0, /**< stdin of a child process */ 
	QSE_PIO_OUT = 1, /**< stdout of a child process */
	QSE_PIO_ERR = 2  /**< stderr of a child process */
};
typedef enum qse_pio_hid_t qse_pio_hid_t;

/**
 * The qse_pio_errnum_t type defines error numbers.
 */
enum qse_pio_errnum_t
{
	QSE_PIO_ENOERR = 0, /**< no error */
	QSE_PIO_EOTHER,     /**< unknown error */
	QSE_PIO_ENOIMPL,    /**< not implemented */
	QSE_PIO_ESYSERR,    /**< subsystem error */
	QSE_PIO_EINTERN,    /**< internal error */

	QSE_PIO_ENOMEM,     /**< out of memory */
	QSE_PIO_EINVAL,     /**< invalid parameter */
	QSE_PIO_EACCES,     /**< access denied */
	QSE_PIO_ENOENT,     /**< no such file */
	QSE_PIO_EEXIST,     /**< already exist */
	QSE_PIO_EINTR,      /**< interrupted */
	QSE_PIO_ENOHND,     /**< no handle available */
	QSE_PIO_ECHILD,     /**< the child is not valid */
	QSE_PIO_EPIPE,      /**< broken pipe */
	QSE_PIO_EILSEQ,     /**< illegal sequence */
	QSE_PIO_EICSEQ,     /**< incomplete sequence */
	QSE_PIO_EILCHR      /**< illegal character */
};
typedef enum qse_pio_errnum_t qse_pio_errnum_t;

#if defined(_WIN32)
	/* <winnt.h> => typedef PVOID HANDLE; */
	typedef void* qse_pio_hnd_t; /**< defines a pipe handle type */
	typedef void* qse_pio_pid_t; /**< defines a process handle type */
#	define  QSE_PIO_HND_NIL ((qse_pio_hnd_t)QSE_NULL)
#	define  QSE_PIO_PID_NIL ((qse_pio_pid_t)QSE_NULL)
#elif defined(__OS2__)
	/* <os2def.h> => typedef LHANDLE HFILE;
	                 typedef LHANDLE PID;
	                 typedef unsigned long LHANDLE; */
	typedef unsigned long qse_pio_hnd_t; /**< defines a pipe handle type */
	typedef unsigned long qse_pio_pid_t; /**< defined a process handle type */
#	define  QSE_PIO_HND_NIL ((qse_pio_hnd_t)-1)
#	define  QSE_PIO_PID_NIL ((qse_pio_pid_t)-1)
#elif defined(__DOS__)
	typedef int qse_pio_hnd_t; /**< defines a pipe handle type */
	typedef int qse_pio_pid_t; /**< defines a process handle type */
#	define  QSE_PIO_HND_NIL ((qse_pio_hnd_t)-1)
#	define  QSE_PIO_PID_NIL ((qse_pio_pid_t)-1)
#else
	typedef int qse_pio_hnd_t; /**< defines a pipe handle type */
	typedef int qse_pio_pid_t; /**< defines a process handle type */
#	define  QSE_PIO_HND_NIL ((qse_pio_hnd_t)-1)
#	define  QSE_PIO_PID_NIL ((qse_pio_pid_t)-1)
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
 * The qse_pio_t type defines a structure to store status for piped I/O
 * to a child process. The qse_pio_xxx() funtions are written around this
 * type. Do not change the value of each field directly. 
 */
struct qse_pio_t
{
	qse_mmgr_t*      mmgr;
	int              flags;  /**< options */
	qse_pio_errnum_t errnum;  /**< error number */
	qse_pio_pid_t    child;   /**< handle to a child process */
	qse_pio_pin_t    pin[3];
};

/** access the @a errnum field of the #qse_pio_t structure */
#define QSE_PIO_ERRNUM(pio)    ((pio)->errnum)
/** access the @a child field of the #qse_pio_t structure */
#define QSE_PIO_CHILD(pio)     ((pio)->child)
/** get the native handle from the #qse_pio_t structure */
#define QSE_PIO_HANDLE(pio,hid) ((pio)->pin[hid].handle)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_pio_open() function executes a command @a cmd and establishes
 * pipes to it. #QSE_PIO_SHELL causes the function to execute @a cmd via 
 * the default shell of an underlying system: /bin/sh on *nix, cmd.exe on win32.
 * On *nix systems, a full path to the command is needed if it is not specified.
 * If @a env is #QSE_NULL, the environment of @a cmd inherits that of the 
 * calling process. If you want to pass an empty environment, you can pass
 * an empty @a env object with no items inserted. If #QSE_PIO_MBSCMD is 
 * specified in @a flags, @a cmd is treated as a multi-byte string whose 
 * character type is #qse_mchar_t.
 * @return #qse_pio_t object on success, #QSE_NULL on failure
 */
QSE_EXPORT qse_pio_t* qse_pio_open (
	qse_mmgr_t*       mmgr,   /**< memory manager */
	qse_size_t        ext,    /**< extension size */
	const qse_char_t* cmd,    /**< command to execute */
	qse_env_t*        env,    /**< environment */
	int               flags   /**< 0 or a number OR'ed of the
	                               #qse_pio_flag_t enumerators*/
);

/**
 * The qse_pio_close() function closes pipes to a child process and waits for
 * the child process to exit.
 */
QSE_EXPORT void qse_pio_close (
	qse_pio_t* pio /**< pio object */
);

/**
 * The qse_pio_init() functions performs the same task as the qse_pio_open()
 * except that you need to allocate a #qse_pio_t structure and pass it to the
 * function.
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_pio_init (
	qse_pio_t*        pio,    /**< pio object */
	qse_mmgr_t*       mmgr,   /**< memory manager */
	const qse_char_t* cmd,    /**< command to execute */
	qse_env_t*        env,    /**< environment */
	int               flags   /**< 0 or a number OR'ed of the
	                               #qse_pio_flag_t enumerators*/
);

/**
 * The qse_pio_fini() function performs the same task as qse_pio_close()
 * except that it does not destroy a #qse_pio_t structure pointed to by @a pio.
 */
QSE_EXPORT void qse_pio_fini (
	qse_pio_t* pio /**< pio object */
);

QSE_EXPORT qse_mmgr_t* qse_pio_getmmgr (
	qse_pio_t* pio
);

QSE_EXPORT void* qse_pio_getxtn (
	qse_pio_t* pio
);

/**
 * The qse_pio_geterrnum() function returns the number of the last error 
 * occurred. 
 * @return error number
 */
QSE_EXPORT qse_pio_errnum_t qse_pio_geterrnum (
	const qse_pio_t* pio /**< pio object */
);

/**
 * The qse_pio_getcmgr() function returns the current character manager.
 * It returns #QSE_NULL is @a pio is not opened with #QSE_PIO_TEXT.
 */
QSE_EXPORT qse_cmgr_t* qse_pio_getcmgr (
	qse_pio_t*    pio,
	qse_pio_hid_t hid
);

/**
 * The qse_pio_setcmgr() function changes the character manager to @a cmgr.
 * The character manager is used only if @a pio is opened with #QSE_PIO_TEXT.
 */
QSE_EXPORT void qse_pio_setcmgr (
	qse_pio_t*    pio,
	qse_pio_hid_t hid,
	qse_cmgr_t*   cmgr
);

/**
 * The qse_pio_gethandle() function gets a pipe handle.
 * @return pipe handle
 */
QSE_EXPORT qse_pio_hnd_t qse_pio_gethandle (
	const qse_pio_t* pio, /**< pio object */
	qse_pio_hid_t    hid  /**< handle ID */
);

/**
 * The qse_pio_gethandleasubi() function gets a pipe handle wrapped 
 * in the #qse_ubi_t type.
 * @return pipe handle
 */
QSE_EXPORT qse_ubi_t qse_pio_gethandleasubi (
	const qse_pio_t* pio, /**< pio object */
	qse_pio_hid_t    hid  /**< handle ID */
);

/**
 * The qse_pio_getchild() function gets a process handle.
 * @return process handle
 */
QSE_EXPORT qse_pio_pid_t qse_pio_getchild (
	const qse_pio_t* pio /**< pio object */
);

/**
 * The qse_pio_read() fucntion reads at most @a size bytes/characters
 * and stores them to the buffer pointed to by @a buf.
 * @return -1 on failure, 0 on EOF, data length read on success
 */
QSE_EXPORT qse_ssize_t qse_pio_read (
	qse_pio_t*    pio,  /**< pio object */
	qse_pio_hid_t hid,  /**< handle ID */
	void*         buf,  /**< buffer to fill */
	qse_size_t    size  /**< buffer size */
);

/**
 * The qse_pio_write() function writes up @a size bytes/characters 
 * from the buffer pointed to by @a data. If #QSE_PIO_TEXT is used 
 * and the @a size parameter is (qse_size_t)-1, the function treats 
 * the @a data parameter as a pointer to a null-terminated string.
 * @return -1 on failure, data length written on success
 */
QSE_EXPORT qse_ssize_t qse_pio_write (
	qse_pio_t*    pio,   /**< pio object */
	qse_pio_hid_t hid,   /**< handle ID */
	const void*   data,  /**< data to write */
	qse_size_t    size   /**< data size */
);

/**
 * The qse_pio_flush() flushes buffered data if #QSE_PIO_TEXT has been 
 * specified to qse_pio_open() and qse_pio_init().
 */
QSE_EXPORT qse_ssize_t qse_pio_flush (
	qse_pio_t*    pio, /**< pio object */
	qse_pio_hid_t hid  /**< handle ID */
);

/**
 * The qse_pio_purge() drops unflushed input and output data in the 
 * buffer. 
 */
QSE_EXPORT void qse_pio_purge (
	qse_pio_t*    pio, /**< pio object */
	qse_pio_hid_t hid  /**< handle ID */
);

/**
 * The qse_pio_end() function closes a pipe to a child process
 */
QSE_EXPORT void qse_pio_end (
	qse_pio_t*    pio, /**< pio object */
	qse_pio_hid_t hid  /**< handle ID */
);

/**
 * The qse_pio_wait() function waits for a child process to terminate.
 * #QSE_PIO_WAIT_NORETRY causes the function to return an error and set the 
 * @a pio->errnum field to #QSE_PIO_EINTR if the underlying system call has
 * been interrupted. If #QSE_PIO_WAIT_NOBLOCK is used, the return value of 256
 * indicates that the child process has not terminated. Otherwise, 256 is never
 * returned.
 *
 * @return
 *  -1 on error, 256 if the child is alive and #QSE_PIO_WAIT_NOBLOCK is used,
 *  a number between 0 and 255 inclusive if the child process ends normally,
 *  256 + signal number if the child process is terminated by a signal.
 */
QSE_EXPORT int qse_pio_wait (
	qse_pio_t* pio /**< pio object */
);

/**
 * The qse_pio_kill() function terminates a child process by force.
 * You should know the danger of calling this function as the function can
 * kill a process that is not your child process if it has terminated but
 * there is a new process with the same process handle.
 * @return 0 on success, -1 on failure
 */ 
QSE_EXPORT int qse_pio_kill (
	qse_pio_t* pio /**< pio object */
);

#ifdef __cplusplus
}
#endif

#endif
