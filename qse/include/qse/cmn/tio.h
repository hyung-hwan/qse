/*
 * $Id: tio.h,v 1.19 2006/01/01 13:50:24 bacon Exp $
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


#define QSE_TIO_MMGR(tio)   ((tio)->mmgr)
#define QSE_TIO_XTN(s)      ((void*)(((qse_tio_t*)s) + 1))
#define QSE_TIO_ERRNUM(tio) ((tio)->errnum)

/*
 * TYPE: qse_tio_t
 *   Defines the tio type
 *
 * DESCRIPTION:
 *   <qse_tio_t> defines a generic type for text-based IO. When qse_char_t is
 *   qse_mchar_t, it handles any byte streams. When qse_char_t is qse_wchar_t,
 *   it handles utf-8 byte streams.
 */
typedef struct qse_tio_t qse_tio_t;

/*
 * TYPE: qse_tio_io_t
 *   Defines a user-defines IO handler
 */
typedef qse_ssize_t (*qse_tio_io_t) (
	int cmd, void* arg, void* data, qse_size_t size);

struct qse_tio_t
{
	QSE_DEFINE_STD_FIELDS (tio)
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

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_STD_FUNCTIONS (tio)

/*
 * FUNCTION: qse_tio_open
 */
qse_tio_t* qse_tio_open (
	qse_mmgr_t* mmgr,
	qse_size_t  ext
);

/*
 * FUNCTION: qse_tio_close
 */
int qse_tio_close (
	qse_tio_t* tio
);

qse_tio_t* qse_tio_init (
	qse_tio_t*  tip,
	qse_mmgr_t* mmgr
);

int qse_tio_fini (
	qse_tio_t* tio
);

/****f* qse.cmn.tio/qse_tio_geterrnum
 * NAME
 *  qse_tio_geterrnum - get an error code
 *
 * SYNOPSIS
 */
qse_tio_err_t qse_tio_geterrnum (
	qse_tio_t* tio
);
/******/

/*
 * FUNCTION: qse_tio_geterrstr
 *   Translates an error code to a string
 *
 * PARAMETERS:
 *   tio - a tio object
 *
 * RETURNS:
 *   A pointer to a constant string describing the last error occurred
 */
const qse_char_t* qse_tio_geterrstr (
	qse_tio_t* tio
);

/*
 * FUNCTION: qse_tio_attachin
 *   Attaches an input handler function
 *
 * PARAMETERS:
 *   tio - a tio object
 *   input - input handler function
 *   arg - user data to be passed to the input handler
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int qse_tio_attachin (
	qse_tio_t*   tio,
	qse_tio_io_t input,
	void*        arg
);

/*
 * FUNCTION: qse_tio_detachin
 *   Detaches an input handler function
 *
 * PARAMETERS:
 *   tio - a tio object
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int qse_tio_detachin (
	qse_tio_t* tio
);

/*
 * FUNCTION: qse_tio_attachout
 *   Attaches an output handler function
 *
 * PARAMETERS:
 *   tio - a tio object
 *   output - output handler function
 *   arg - user data to be passed to the output handler
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int qse_tio_attachout (
	qse_tio_t* tio,
	qse_tio_io_t output,
	void* arg
);

/*
 * FUNCTION: qse_tio_detachout
 *   Detaches an output handler function
 * 
 * PARAMETERS:
 *   tio - a tio object
 *
 * RETURNS:
 *   0 on success, -1 on failure
 */
int qse_tio_detachout (
	qse_tio_t* tio
);

/****f* qse.cmn.tio/qse_tio_flush
 * NAME
 *  qse_tio_flush - flush the output buffer
 *
 * RETURNS
 *  The qse_tio_flush() function return the number of bytes written on 
 *  success, -1 on failure.
 * 
 * SYNOPSIS
 */
qse_ssize_t qse_tio_flush (
	qse_tio_t* tio
);
/******/

/****f* qse.cmn.tio/qse_tio_purge
 * NAME 
 *  qse_tio_purge - erase input and output buffered 
 *
 * SYNOPSIS
 */
void qse_tio_purge (
	qse_tio_t* tio
);
/******/

/****f* qse.cmn.tio/qse_tio_read
 * NAME
 *  qse_tio_read - read text
 * 
 * SYNOPSIS
 */
qse_ssize_t qse_tio_read (
	qse_tio_t*  tio, 
	qse_char_t* buf, 
	qse_size_t  size
);
/******/

/****f* qse.cmn.tio/qse_tio_write
 * NAME
 *  qse_tio_write - write text
 * 
 * DESCRIPTION
 *  If the size paramenter is (qse_size_t)-1, the function treats the data 
 *  parameter as a pointer to a null-terminated string.
 *
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
