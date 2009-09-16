/*
 * $Id: tio.h 287 2009-09-15 10:01:02Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>

enum qse_tio_err_t
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

typedef enum qse_tio_err_t qse_tio_err_t;

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

#define QSE_TIO_ERRNUM(tio) ((tio)->errnum)

typedef struct qse_tio_t qse_tio_t;

/****t* Common/qse_tio_io_t
 * NAME
 *  qse_tio_io_t - define a text IO handler type
 * SYNOPSIS
 */
typedef qse_ssize_t (*qse_tio_io_t) (
	int        cmd, 
	void*      arg, 
	void*      data, 
	qse_size_t size
);
/******/

/****s* Common/qse_tio_t
 * NAME
 *  qse_tio_t - define a text IO type
 * DESCRIPTION
 *   The qse_tio_t type defines a generic type for text IO. When qse_char_t is
 *   qse_mchar_t, it handles any byte streams. When qse_char_t is qse_wchar_t,
 *   it handles a multi-byte stream converted to a wide character stream.
 * SYNOPSIS
 */
struct qse_tio_t
{
	QSE_DEFINE_COMMON_FIELDS (tio)
	qse_tio_err_t errnum;

	/* io functions */
	qse_tio_io_t input_func;
	qse_tio_io_t output_func;
	void* input_arg;
	void* output_arg;

	/* for housekeeping */
	int         input_status;
        qse_size_t  inbuf_curp;
        qse_size_t  inbuf_len;
        qse_size_t  outbuf_len;

        qse_mchar_t inbuf[QSE_TIO_MAX_INBUF_LEN];
        qse_mchar_t outbuf[QSE_TIO_MAX_OUTBUF_LEN];
};
/******/

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (tio)

/****f* Common/qse_tio_open
 * NAME
 *  qse_tio_open - create an text IO processor
 * SYNOPSIS
 */
qse_tio_t* qse_tio_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext
);
/******/

/****f* Common/qse_tio_close
 * NAME
 *  qse_tio_close - destroy an text IO processor
 * SYNOPSIS
 */
int qse_tio_close (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_init
 * NAME
 *  qse_tio_init - initialize an text IO processor
 * SYNOPSIS
 */
qse_tio_t* qse_tio_init (
	qse_tio_t*  tip,
	qse_mmgr_t* mmgr
);
/******/

/****f* Common/qse_tio_fini
 * NAME
 *  qse_tio_fini - finalize an text IO processor
 * SYNOPSIS
 */
int qse_tio_fini (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_geterrnum
 * NAME
 *  qse_tio_geterrnum - get an error code
 *
 * SYNOPSIS
 */
qse_tio_err_t qse_tio_geterrnum (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_geterrmsg
 * NAME
 *  qse_tio_geterrmsg - translate an error code to a string
 * RETURN
 *   A pointer to a constant string describing the last error occurred.
 */
const qse_char_t* qse_tio_geterrmsg (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_attachin
 * NAME
 *  qse_tio_attachin - attaches an input handler 
 * RETURN
 *   0 on success, -1 on failure
 * SYNOPSIS
 */
int qse_tio_attachin (
	qse_tio_t*   tio,
	qse_tio_io_t input,
	void*        arg
);
/******/

/****f* Common/qse_tio_detachin
 * NAME
 *  qse_tio_detachin - detach an input handler 
 * RETURN
 *   0 on success, -1 on failure
 * SYNOPSIS
 */
int qse_tio_detachin (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_attachout
 * NAME
 *  qse_tio_attachout - attaches an output handler 
 * RETURN
 *   0 on success, -1 on failure
 * SYNOPSIS
 */
int qse_tio_attachout (
	qse_tio_t* tio,
	qse_tio_io_t output,
	void* arg
);
/******/

/****f* Common/qse_tio_detachout
 * NAME
 *  qse_tio_detachout - detaches an output handler 
 * RETURN
 *   0 on success, -1 on failure
 * SYNOPSIS
 */
int qse_tio_detachout (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_flush
 * NAME
 *  qse_tio_flush - flush the output buffer
 * RETURNS
 *  The qse_tio_flush() function return the number of bytes written on 
 *  success, -1 on failure.
 * SYNOPSIS
 */
qse_ssize_t qse_tio_flush (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_purge
 * NAME 
 *  qse_tio_purge - empty input and output buffers
 * SYNOPSIS
 */
void qse_tio_purge (
	qse_tio_t* tio
);
/******/

/****f* Common/qse_tio_read
 * NAME
 *  qse_tio_read - read text
 * SYNOPSIS
 */
qse_ssize_t qse_tio_read (
	qse_tio_t*  tio, 
	qse_char_t* buf, 
	qse_size_t  size
);
/******/

/****f* Common/qse_tio_write
 * NAME
 *  qse_tio_write - write text
 * DESCRIPTION
 *  If the size paramenter is (qse_size_t)-1, the function treats the data 
 *  parameter as a pointer to a null-terminated string.
 * SYNOPSIS
 */
qse_ssize_t qse_tio_write (
	qse_tio_t*        tio,
	const qse_char_t* data,
	qse_size_t        size
);
/******/

#ifdef __cplusplus
}
#endif

#endif
