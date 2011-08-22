/*
 * $Id: tio.h 554 2011-08-22 05:26:26Z hyunghwan.chung $
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
	QSE_TIO_ENOERR = 0,
	QSE_TIO_ENOMEM, /* out of memory */
	QSE_TIO_ENOSPC, /* no more space */
	QSE_TIO_EILSEQ, /* illegal sequence */
	QSE_TIO_EICSEQ, /* incomplete sequence */
	QSE_TIO_EILCHR, /* illegal character */
	QSE_TIO_ENOINF, /* no input function attached */
	QSE_TIO_EINPUT, /* input function returned an error */
	QSE_TIO_EINPOP, /* input function failed to open */
	QSE_TIO_EINPCL, /* input function failed to close */
	QSE_TIO_ENOUTF, /* no output function attached */
	QSE_TIO_EOUTPT, /* output function returned an error */
	QSE_TIO_EOUTOP, /* output function failed to open */
	QSE_TIO_EOUTCL  /* output function failed to close */
};

typedef enum qse_tio_errnum_t qse_tio_errnum_t;

enum
{
	/* the size of input buffer should be at least equal to or greater
	 * than the maximum sequence length of the qse_mchar_t string.
	 * (i.e. 6 for utf8)
	 */
	QSE_TIO_MAX_INBUF_LEN = 4096,
	QSE_TIO_MAX_OUTBUF_LEN = 4096
};

enum 
{
	QSE_TIO_IO_OPEN,
	QSE_TIO_IO_CLOSE,
	QSE_TIO_IO_DATA
};

#define QSE_TIO_ERRNUM(tio) ((const qse_tio_errnum_t)(tio)->errnum)

typedef struct qse_tio_t qse_tio_t;

/**
 * The qse_tio_io_t types define a text I/O handler.
 */
typedef qse_ssize_t (*qse_tio_io_t) (
	int        cmd, 
	void*      arg, 
	void*      data, 
	qse_size_t size
);

/**
 * The qse_tio_t type defines a generic type for text IO. If #qse_char_t is
 * #qse_mchar_t, it handles any byte streams. If qse_char_t is #qse_wchar_t,
 * it handles a multi-byte stream converted to a wide character stream.
 */
struct qse_tio_t
{
	QSE_DEFINE_COMMON_FIELDS (tio)
	qse_tio_errnum_t errnum;

	/* io functions */
	qse_tio_io_t input_func;
	qse_tio_io_t output_func;
	void* input_arg;
	void* output_arg;

	/* for housekeeping */
	int           input_status;
	qse_size_t    inbuf_curp;
	qse_size_t    inbuf_len;
	qse_size_t    outbuf_len;

	qse_mchar_t inbuf[QSE_TIO_MAX_INBUF_LEN];
	qse_mchar_t outbuf[QSE_TIO_MAX_OUTBUF_LEN];

#ifdef QSE_CHAR_IS_WCHAR
	struct
	{
		qse_mbstate_t in;
		qse_mbstate_t out;
	} mbstate;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (tio)

/**
 * The qse_tio_open() function creates an text stream processoor.
 */
qse_tio_t* qse_tio_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext
);

/**
 * The qse_tio_close() function destroys an text stream processor.
 */
int qse_tio_close (
	qse_tio_t* tio
);

/**
 * The qse_tio_init() function  initialize a statically declared 
 * text stream processor.
 */
qse_tio_t* qse_tio_init (
	qse_tio_t*  tip,
	qse_mmgr_t* mmgr
);

/**
 * The qse_tio_fini() function finalizes a text stream processor
 */
int qse_tio_fini (
	qse_tio_t* tio
);

/**
 * The qse_tio_geterrnum() function return an error code.
 */
qse_tio_errnum_t qse_tio_geterrnum (
	qse_tio_t* tio
);

/**
 * The qse_tio_geterrmsg() function translates an error code to a string.
 * @return pointer to a constant string describing the last error occurred.
 */
const qse_char_t* qse_tio_geterrmsg (
	qse_tio_t* tio
);

/**
 * The qse_tio_attachin() function attachs an input handler .
 * @return 0 on success, -1 on failure
 */
int qse_tio_attachin (
	qse_tio_t*   tio,
	qse_tio_io_t input,
	void*        arg
);

/**
 * The qse_tio_detachin() function detaches an input handler .
 * @return 0 on success, -1 on failure
 */
int qse_tio_detachin (
	qse_tio_t* tio
);

/**
 * The qse_tio_attachout() function attaches an output handler.
 * @return 0 on success, -1 on failure
 */
int qse_tio_attachout (
	qse_tio_t* tio,
	qse_tio_io_t output,
	void* arg
);

/**
 * The qse_tio_detachout() function detaches an output handler .
 * @return 0 on success, -1 on failure
 */
int qse_tio_detachout (
	qse_tio_t* tio
);

/**
 * The qse_tio_flush() function flushes the output buffer. It returns the 
 * number of bytes written on success, -1 on failure.
 */
qse_ssize_t qse_tio_flush (
	qse_tio_t* tio
);

/**
 * The qse_tio_purge() function empties input and output buffers.
 */
void qse_tio_purge (
	qse_tio_t* tio
);

/**
 * The qse_tio_read() functio reads text.
 */
qse_ssize_t qse_tio_read (
	qse_tio_t*  tio, 
	qse_char_t* buf, 
	qse_size_t  size
);

/**
 * The qse_tio_write() function writes text.
 * If the size paramenter is (qse_size_t)-1, the function treats the data 
 * parameter as a pointer to a null-terminated string.
 */
qse_ssize_t qse_tio_write (
	qse_tio_t*        tio,
	const qse_char_t* data,
	qse_size_t        size
);

#ifdef __cplusplus
}
#endif

#endif
