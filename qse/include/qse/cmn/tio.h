/*
 * $Id: tio.h,v 1.19 2006/01/01 13:50:24 bacon Exp $
 */

#ifndef _QSE_CMN_TIO_H_
#define _QSE_CMN_TIO_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>

enum
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
	qse_mmgr_t* mmgr;
	int errnum;

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

void* qse_tio_getxtn (
	qse_tio_t* tio
);

qse_mmgr_t* qse_tio_getmmgr (
	qse_tio_t* tio
);

void qse_tio_setmmgr (
	qse_tio_t* tio,
	qse_mmgr_t* mmgr
);

/*
 * FUNCTION: qse_tio_geterrnum
 *   Returns an error code
 *
 * PARAMETERS:
 *   grep - a grep object
 *
 * RETURNS:
 *   Error code set by the last tio function called
 */
int qse_tio_geterrnum (qse_tio_t* tio);

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
const qse_char_t* qse_tio_geterrstr (qse_tio_t* tio);

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


/*
 * FUNCTION: qse_tio_flush
 *   Flushes the output buffer
 *
 * PARAMETERS:
 *   tio - a tio object
 *
 * RETURNS:
 *   Number of bytes written on success, -1 on failure
 */
qse_ssize_t qse_tio_flush (qse_tio_t* tio);

/*
 * FUNCTION: qse_tio_purge
 *   Erases all input and output buffered 
 * 
 * PARAMETERS:
 *   tio - a tio object
 * 
 */
void qse_tio_purge (qse_tio_t* tio);

/* 
 * FUNCTION: qse_tio_getc
 *   Reads a single character
 */
qse_ssize_t qse_tio_getc (qse_tio_t* tio, qse_char_t* c);

/* 
 * FUNCTION: qse_tio_gets
 * 
 * DESCRIPTION:
 *   <qse_tio_gets> inserts a terminating null if there is a room
 */
qse_ssize_t qse_tio_gets (qse_tio_t* tio, qse_char_t* buf, qse_size_t size);

/* 
 * FUNCTION: qse_tio_getsx
 * 
 * DESCRIPTION:
 *   <qse_tio_getsx> doesn't insert a terminating null character 
 */
qse_ssize_t qse_tio_getsx (qse_tio_t* tio, qse_char_t* buf, qse_size_t size);

/*
 * FUNCTION: qse_tio_getstr
 */
qse_ssize_t qse_tio_getstr (qse_tio_t* tio, qse_str_t* buf);

/*
 * FUNCTION: qse_tio_putc
 */
qse_ssize_t qse_tio_putc (qse_tio_t* tio, qse_char_t c);

/*
 * FUNCTION: qse_tio_puts
 */
qse_ssize_t qse_tio_puts (qse_tio_t* tio, const qse_char_t* str);

/*
 * FUNCTION: qse_tio_putsx
 */
qse_ssize_t qse_tio_putsx (qse_tio_t* tio, const qse_char_t* str, qse_size_t size);

/*
 * FUNCTION: qse_tio_putsn
 */
qse_ssize_t qse_tio_putsn (qse_tio_t* tio, ...);

/*
 * FUNCTION: qse_tio_putsxn
 */
qse_ssize_t qse_tio_putsxn (qse_tio_t* tio, ...);

#ifdef __cplusplus
}
#endif

#endif
