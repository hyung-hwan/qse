/*
 * $Id: sio.h 569 2011-09-19 06:51:02Z hyunghwan.chung $
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
 
#ifndef _QSE_CMN_SIO_H_
#define _QSE_CMN_SIO_H_

/** @file
 * This file defines a simple stream I/O interface.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/fio.h>
#include <qse/cmn/tio.h>

enum qse_sio_flag_t
{
	/* ensure that these enumerators never overlap with
	 * qse_fio_flag_t enumerators. you can use values between
	 * (1<<0) and (1<<7) inclusive reserved in qse_fio_flag_t.
	 * the range is represented by QSE_FIO_RESERVED. */
	QSE_SIO_URI           = (1 << 0),
	QSE_SIO_IGNOREMBWCERR = (1 << 1),
	QSE_SIO_NOAUTOFLUSH   = (1 << 2),

	/* ensure that the following enumerators are one of
	 * qse_fio_flags_t enumerators */
	QSE_SIO_HANDLE        = QSE_FIO_HANDLE,
	QSE_SIO_TEMPORARY     = QSE_FIO_TEMPORARY,
	QSE_SIO_NOCLOSE       = QSE_FIO_NOCLOSE,
	QSE_SIO_READ          = QSE_FIO_READ,
	QSE_SIO_WRITE         = QSE_FIO_WRITE,
	QSE_SIO_APPEND        = QSE_FIO_APPEND,
	QSE_SIO_CREATE        = QSE_FIO_CREATE,
	QSE_SIO_TRUNCATE      = QSE_FIO_TRUNCATE,
	QSE_SIO_EXCLUSIVE     = QSE_FIO_EXCLUSIVE,
	QSE_SIO_SYNC          = QSE_FIO_SYNC,
	QSE_SIO_NOFOLLOW      = QSE_FIO_NOFOLLOW,
	QSE_SIO_NOSHREAD      = QSE_FIO_NOSHREAD,
	QSE_SIO_NOSHWRITE     = QSE_FIO_NOSHWRITE,
	QSE_SIO_NOSHDELETE    = QSE_FIO_NOSHDELETE,
	QSE_SIO_RANDOM        = QSE_FIO_RANDOM,
	QSE_SIO_SEQUENTIAL    = QSE_FIO_SEQUENTIAL
};

enum qse_sio_errnum_t
{
	QSE_SIO_ENOERR = 0, /**< no error */

	QSE_SIO_ENOMEM,     /**< out of memory */
	QSE_SIO_EINVAL,     /**< invalid parameter */
	QSE_SIO_EACCES,     /**< access denied */
	QSE_SIO_ENOENT,     /**< no such file */
	QSE_SIO_EEXIST,     /**< already exist */
	QSE_SIO_EINTR,      /**< interrupted */
	QSE_SIO_EILSEQ,     /**< illegal sequence */
	QSE_SIO_EICSEQ,     /**< incomplete sequence */
	QSE_SIO_EILCHR,     /**< illegal character */
	QSE_SIO_ESYSERR,    /**< subsystem(system call) error */
	QSE_SIO_ENOIMPL,    /**< not implemented */

	QSE_SIO_EOTHER      /**< other error */
};
typedef enum qse_sio_errnum_t qse_sio_errnum_t;

typedef qse_fio_off_t qse_sio_pos_t;
typedef qse_fio_hnd_t qse_sio_hnd_t;
typedef qse_fio_std_t qse_sio_std_t;

#define QSE_SIO_STDIN  QSE_FIO_STDIN
#define QSE_SIO_STDOUT QSE_FIO_STDOUT
#define QSE_SIO_STDERR QSE_FIO_STDERR

/**
 * The qse_sio_t type defines a simple text stream over a file. It also
 * provides predefined streams for standard input, output, and error.
 */
typedef struct qse_sio_t qse_sio_t;

struct qse_sio_t
{
	QSE_DEFINE_COMMON_FIELDS (sio)
	qse_sio_errnum_t errnum;

	qse_fio_t file;

	struct
	{
		qse_tio_t  io;
		qse_sio_t* xtn; /* static extension for tio */
	} tio;

	qse_mchar_t inbuf[2048];
	qse_mchar_t outbuf[2048];

#if defined(_WIN32)
	int status;
#endif
};

/** access the @a errnum field of the #qse_sio_t structure */
#define QSE_SIO_ERRNUM(sio)    ((sio)->errnum)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_sio_open() fucntion creates a stream object.
 */
qse_sio_t* qse_sio_open (
	qse_mmgr_t*       mmgr,    /**< memory manager */
	qse_size_t        xtnsize, /**< extension size in bytes */
	const qse_char_t* file,    /**< file name */
	int               flags   /**< number OR'ed of #qse_sio_flag_t */
);

qse_sio_t* qse_sio_openstd (
	qse_mmgr_t*       mmgr,    /**< memory manager */
	qse_size_t        xtnsize, /**< extension size in bytes */
	qse_sio_std_t     std,     /**< standard I/O identifier */
	int               flags   /**< number OR'ed of #qse_sio_flag_t */
);

/**
 * The qse_sio_close() function destroys a stream object.
 */
void qse_sio_close (
	qse_sio_t* sio  /**< stream */
);

int qse_sio_init (
	qse_sio_t*        sio,
	qse_mmgr_t*       mmgr,
	const qse_char_t* file,
	int               flags
);

int qse_sio_initstd (
	qse_sio_t*        sio,
	qse_mmgr_t*       mmgr,
	qse_sio_std_t     std,
	int               flags
);

void qse_sio_fini (
	qse_sio_t* sio
);

qse_sio_errnum_t qse_sio_geterrnum (
	const qse_sio_t* sio
);

qse_cmgr_t* qse_sio_getcmgr (
	qse_sio_t* sio
);

void qse_sio_setcmgr (
	qse_sio_t*  sio,
	qse_cmgr_t* cmgr
);

qse_sio_hnd_t qse_sio_gethandle (
	const qse_sio_t* sio
);

qse_ubi_t qse_sio_gethandleasubi (
	const qse_sio_t* sio
);

qse_ssize_t qse_sio_flush (
	qse_sio_t* sio
);

void qse_sio_purge (
	qse_sio_t* sio
);

qse_ssize_t qse_sio_getmc (
	qse_sio_t*   sio,
	qse_mchar_t* c
);

qse_ssize_t qse_sio_getwc (
	qse_sio_t*   sio,
	qse_wchar_t* c
);

qse_ssize_t qse_sio_getmbs (
	qse_sio_t*   sio,
	qse_mchar_t* buf,
	qse_size_t   size
);

qse_ssize_t qse_sio_getmbsn (
	qse_sio_t*   sio,
	qse_mchar_t* buf,
	qse_size_t   size
);

qse_ssize_t qse_sio_getwcs (
	qse_sio_t*   sio,
	qse_wchar_t* buf,
	qse_size_t   size
);

qse_ssize_t qse_sio_getwcsn (
	qse_sio_t*   sio,
	qse_wchar_t* buf,
	qse_size_t   size
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_sio_getc(sio,c) qse_sio_getmb(sio,c)
#	define qse_sio_getstr(sio,buf,size) qse_sio_getmbs(sio,buf,size)
#	define qse_sio_getstrn(sio,buf,size) qse_sio_getmbsn(sio,buf,size)
#else
#	define qse_sio_getc(sio,c) qse_sio_getwc(sio,c)
#	define qse_sio_getstr(sio,buf,size) qse_sio_getwcs(sio,buf,size)
#	define qse_sio_getstrn(sio,buf,size) qse_sio_getwcsn(sio,buf,size)
#endif

qse_ssize_t qse_sio_putmb (
	qse_sio_t*  sio, 
	qse_mchar_t c
);

qse_ssize_t qse_sio_putwc (
	qse_sio_t*  sio, 
	qse_wchar_t c
);

qse_ssize_t qse_sio_putmbs (
	qse_sio_t*         sio,
	const qse_mchar_t* str
);

qse_ssize_t qse_sio_putwcs (
	qse_sio_t*         sio,
	const qse_wchar_t* str
);


qse_ssize_t qse_sio_putmbsn (
	qse_sio_t*         sio, 
	const qse_mchar_t* str,
	qse_size_t         size
);

qse_ssize_t qse_sio_putwcsn (
	qse_sio_t*         sio, 
	const qse_wchar_t* str,
	qse_size_t         size
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_sio_putc(sio,c) qse_sio_putmb(sio,c)
#	define qse_sio_putstr(sio,str) qse_sio_putmbs(sio,str)
#	define qse_sio_putstrn(sio,str,size) qse_sio_putmbsn(sio,str,size)
#else
#	define qse_sio_putc(sio,c) qse_sio_putwc(sio,c)
#	define qse_sio_putstr(sio,str) qse_sio_putwcs(sio,str)
#	define qse_sio_putstrn(sio,str,size) qse_sio_putwcsn(sio,str,size)
#endif

/**
 * The qse_sio_getpos() gets the current position in a stream.
 * Note that it may not return the desired postion due to buffering.
 * @return 0 on success, -1 on failure
 */
int qse_sio_getpos (
	qse_sio_t*     sio,  /**< stream */
	qse_sio_pos_t* pos   /**< position */
);

/**
 * The qse_sio_setpos() changes the current position in a stream.
 * @return 0 on success, -1 on failure
 */
int qse_sio_setpos (
	qse_sio_t*    sio,   /**< stream */
	qse_sio_pos_t pos    /**< position */
);

#if 0
int qse_sio_rewind (qse_sio_t* sio);
int qse_sio_movetoend (qse_sio_t* sio);
#endif


#ifdef __cplusplus
}
#endif

#endif
