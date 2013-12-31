/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_TIO_H_
#define _QSE_CMN_TIO_H_

/** @file
 * This file provides an interface to a text stream processor.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>

enum qse_tio_errnum_t
{
	QSE_TIO_ENOERR = 0, /**< no error */
	QSE_TIO_EOTHER, /**< other error */
	QSE_TIO_ENOIMPL, /**< not implmeneted */
	QSE_TIO_ESYSERR, /**< subsystem error */
	QSE_TIO_EINTERN, /**< internal error */

	QSE_TIO_ENOMEM, /**< out of memory */
	QSE_TIO_EINVAL, /**< invalid parameter */
	QSE_TIO_EACCES, /**< access denied */
	QSE_TIO_ENOENT, /**< no such file */
	QSE_TIO_ENOSPC, /**< no more space */
	QSE_TIO_EILSEQ, /**< illegal sequence */
	QSE_TIO_EICSEQ, /**< incomplete sequence */
	QSE_TIO_EILCHR, /**< illegal character */
	QSE_TIO_ENINPF, /**< no input function attached */
	QSE_TIO_ENOUTF  /**< no output function attached */
};

typedef enum qse_tio_errnum_t qse_tio_errnum_t;

enum qse_tio_cmd_t
{
	QSE_TIO_OPEN,
	QSE_TIO_CLOSE,
	QSE_TIO_DATA
};
typedef enum qse_tio_cmd_t qse_tio_cmd_t;

enum qse_tio_flag_t
{
	/**< ignore multibyte/wide-character conversion error by
	 *   inserting a question mark for each error occurrence */
	QSE_TIO_IGNOREMBWCERR = (1 << 0),

	/**< do not flush data in the buffer until the buffer gets full. */
	QSE_TIO_NOAUTOFLUSH   = (1 << 1)
};

enum qse_tio_misc_t
{
	QSE_TIO_MININBUFCAPA  = 32,
	QSE_TIO_MINOUTBUFCAPA = 32
};

#define QSE_TIO_ERRNUM(tio) ((const qse_tio_errnum_t)(tio)->errnum)

typedef struct qse_tio_t qse_tio_t;

/**
 * The qse_tio_io_impl_t types define a text I/O handler.
 */
typedef qse_ssize_t (*qse_tio_io_impl_t) (
	qse_tio_t*    tio,
	qse_tio_cmd_t cmd, 
	void*         data, 
	qse_size_t    size
);

struct qse_tio_io_t
{
	qse_tio_io_impl_t fun;
	struct
	{
		qse_size_t   capa;
		qse_mchar_t* ptr;
	} buf;
};

typedef struct qse_tio_io_t qse_tio_io_t;

/**
 * The qse_tio_t type defines a generic type for text IO. If #qse_char_t is
 * #qse_mchar_t, it handles any byte streams. If qse_char_t is #qse_wchar_t,
 * it handles a multi-byte stream converted to a wide character stream.
 */
struct qse_tio_t
{
	qse_mmgr_t*      mmgr;
	qse_tio_errnum_t errnum;
	int              flags;
	qse_cmgr_t*      cmgr;

	qse_tio_io_t     in;
	qse_tio_io_t     out;

	/* for house keeping from here */
	int              status;
	qse_size_t       inbuf_cur;
	qse_size_t       inbuf_len;
	qse_size_t       outbuf_len;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_tio_open() function creates an text stream processoor.
 */
QSE_EXPORT qse_tio_t* qse_tio_open (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize, /**< extension size in bytes */
	int         flags    /**< ORed of qse_tio_flag_t enumerators */
);

/**
 * The qse_tio_close() function destroys an text stream processor.
 */
QSE_EXPORT int qse_tio_close (
	qse_tio_t* tio
);

/**
 * The qse_tio_init() function  initialize a statically declared 
 * text stream processor.
 */
QSE_EXPORT int qse_tio_init (
	qse_tio_t*  tio,
	qse_mmgr_t* mmgr,
	int         flags
);

/**
 * The qse_tio_fini() function finalizes a text stream processor
 */
QSE_EXPORT int qse_tio_fini (
	qse_tio_t* tio
);

QSE_EXPORT qse_mmgr_t* qse_tio_getmmgr (
	qse_tio_t* tio
);

QSE_EXPORT void* qse_tio_getxtn (
	qse_tio_t* tio
);

/**
 * The qse_tio_geterrnum() function returns the current error code.
 */
QSE_EXPORT qse_tio_errnum_t qse_tio_geterrnum (
	const qse_tio_t* tio
);

/**
 * The qse_tio_geterrnum() function changes the current error code.
 * typically from within the I/O handler attached.
 */
QSE_EXPORT void qse_tio_seterrnum (
	qse_tio_t*       tio,
	qse_tio_errnum_t errnum
);

/**
 * The qse_tio_getcmgr() function returns the character manager.
 */
QSE_EXPORT qse_cmgr_t* qse_tio_getcmgr (
	qse_tio_t* tio
);

/**
 * The qse_tio_setcmgr() function changes the character manager.
 */
QSE_EXPORT void qse_tio_setcmgr (
	qse_tio_t*  tio,
	qse_cmgr_t* cmgr
);

/**
 * The qse_tio_attachin() function attachs an input handler .
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_tio_attachin (
	qse_tio_t*       tio,
	qse_tio_io_impl_t input,
	qse_mchar_t*     bufptr,
	qse_size_t       bufcapa
);

/**
 * The qse_tio_detachin() function detaches an input handler .
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_tio_detachin (
	qse_tio_t* tio
);

/**
 * The qse_tio_attachout() function attaches an output handler.
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_tio_attachout (
	qse_tio_t*       tio,
	qse_tio_io_impl_t output,
	qse_mchar_t*     bufptr,
	qse_size_t       bufcapa
);

/**
 * The qse_tio_detachout() function detaches an output handler .
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_tio_detachout (
	qse_tio_t* tio
);

/**
 * The qse_tio_flush() function flushes the output buffer. It returns the 
 * number of bytes written on success, -1 on failure.
 */
QSE_EXPORT qse_ssize_t qse_tio_flush (
	qse_tio_t* tio
);

/**
 * The qse_tio_purge() function empties input and output buffers.
 */
QSE_EXPORT void qse_tio_purge (
	qse_tio_t* tio
);

QSE_EXPORT qse_ssize_t qse_tio_readmbs (
	qse_tio_t*   tio, 
	qse_mchar_t* buf, 
	qse_size_t   size
);

QSE_EXPORT qse_ssize_t qse_tio_readwcs (
	qse_tio_t*   tio, 
	qse_wchar_t* buf, 
	qse_size_t   size
);

/**
 * The qse_tio_read() macro is character-type neutral. It maps
 * to qse_tio_readmbs() or qse_tio_readwcs().
 */
#ifdef QSE_CHAR_IS_MCHAR
#	define qse_tio_read(tio,buf,size) qse_tio_readmbs(tio,buf,size)
#else
#	define qse_tio_read(tio,buf,size) qse_tio_readwcs(tio,buf,size)
#endif

/**
 * The qse_tio_writembs() function writes the @a size characters 
 * from a multibyte string @a str. If @a size is (qse_size_t)-1,
 * it writes on until a terminating null is found. It doesn't 
 * write more than QSE_TYPE_MAX(qse_ssize_t) characters.
 * @return number of characters written on success, -1 on failure.
 */
QSE_EXPORT qse_ssize_t qse_tio_writembs (
	qse_tio_t*         tio,
	const qse_mchar_t* str,
	qse_size_t         size
);

/**
 * The qse_tio_writembs() function writes the @a size characters 
 * from a wide-character string @a str. If @a size is (qse_size_t)-1,
 * it writes on until a terminating null is found. It doesn't write 
 * more than QSE_TYPE_MAX(qse_ssize_t) characters.
 * @return number of characters written on success, -1 on failure.
 */
QSE_EXPORT qse_ssize_t qse_tio_writewcs (
	qse_tio_t*         tio,
	const qse_wchar_t* str,
	qse_size_t         size
);

/**
 * The qse_tio_write() macro is character-type neutral. It maps
 * to qse_tio_writembs() or qse_tio_writewcs().
 */
#ifdef QSE_CHAR_IS_MCHAR
#	define qse_tio_write(tio,str,size) qse_tio_writembs(tio,str,size)
#else
#	define qse_tio_write(tio,str,size) qse_tio_writewcs(tio,str,size)
#endif

#ifdef __cplusplus
}
#endif

#endif
