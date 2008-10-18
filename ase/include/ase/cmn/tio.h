/*
 * $Id: tio.h,v 1.19 2006/01/01 13:50:24 bacon Exp $
 */

#ifndef _ASE_CMN_TIO_H_
#define _ASE_CMN_TIO_H_

#include <ase/types.h>
#include <ase/macros.h>
#include <ase/cmn/str.h>

enum
{
	ASE_TIO_ENOERR = 0,
	ASE_TIO_ENOMEM, /* out of memory */
	ASE_TIO_ENOSPC, /* no more space */
	ASE_TIO_EILSEQ, /* illegal sequence */
	ASE_TIO_EICSEQ, /* incomplete sequence */
	ASE_TIO_EILCHR, /* illegal character */
	ASE_TIO_ENOINF, /* no input function attached */
	ASE_TIO_EINPUT, /* input function returned an error */
	ASE_TIO_EINPOP, /* input function failed to open */
	ASE_TIO_EINPCL, /* input function failed to close */
	ASE_TIO_ENOUTF, /* no output function attached */
	ASE_TIO_EOUTPT, /* output function returned an error */
	ASE_TIO_EOUTOP, /* output function failed to open */
	ASE_TIO_EOUTCL  /* output function failed to close */
};

enum
{
        /* the size of input buffer should be at least equal to or greater
         * than the maximum sequence length of the ase_mchar_t string.
         * (i.e. 6 for utf8)
         */
        ASE_TIO_MAX_INBUF_LEN = 4096,
        ASE_TIO_MAX_OUTBUF_LEN = 4096
};

enum 
{
	ASE_TIO_IO_OPEN,
	ASE_TIO_IO_CLOSE,
	ASE_TIO_IO_DATA
};


#define ASE_TIO_MMGR(tio)   ((tio)->mmgr)
#define ASE_TIO_ERRNUM(tio) ((tio)->errnum)

/*
 * TYPE: ase_tio_t
 *   Defines the tio type
 *
 * DESCRIPTION:
 *   <ase_tio_t> defines a generic type for text-based IO. When ase_char_t is
 *   ase_mchar_t, it handles any byte streams. When ase_char_t is ase_wchar_t,
 *   it handles utf-8 byte streams.
 */
typedef struct ase_tio_t ase_tio_t;

/*
 * TYPE: ase_tio_io_t
 *   Defines a user-defines IO handler
 */
typedef ase_ssize_t (*ase_tio_io_t) (
	int cmd, void* arg, void* data, ase_size_t size);

struct ase_tio_t
{
	ase_mmgr_t* mmgr;
	int errnum;

	/* io functions */
	ase_tio_io_t input_func;
	ase_tio_io_t output_func;
	void* input_arg;
	void* output_arg;

	/* for housekeeping */
	int         input_status;
        ase_size_t  inbuf_curp;
        ase_size_t  inbuf_len;
        ase_size_t  outbuf_len;

        ase_mchar_t inbuf[ASE_TIO_MAX_INBUF_LEN];
        ase_mchar_t outbuf[ASE_TIO_MAX_OUTBUF_LEN];
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTION: ase_tio_open
 */
ase_tio_t* ase_tio_open (
	ase_mmgr_t* mmgr,
	ase_size_t  ext
);

/*
 * FUNCTION: ase_tio_close
 */
int ase_tio_close (
	ase_tio_t* tio
);

ase_tio_t* ase_tio_init (
	ase_tio_t*  tip,
	ase_mmgr_t* mmgr
);

int ase_tio_fini (
	ase_tio_t* tio
);

void* ase_tio_getextension (
	ase_tio_t* tio
);

ase_mmgr_t* ase_tio_getmmgr (
	ase_tio_t* tio
);

void ase_tio_setmmgr (
	ase_tio_t* tio,
	ase_mmgr_t* mmgr
);

/*
 * FUNCTION: ase_tio_geterrnum
 *   Returns an error code
 *
 * PARAMETERS:
 *   grep - a grep object
 *
 * RETURNS:
 *   Error code set by the last tio function called
 */
int ase_tio_geterrnum (ase_tio_t* tio);

/*
 * FUNCTION: ase_tio_geterrstr
 *   Translates an error code to a string
 *
 * PARAMETERS:
 *   tio - a tio object
 *
 * RETURNS:
 *   A pointer to a constant string describing the last error occurred
 */
const ase_char_t* ase_tio_geterrstr (ase_tio_t* tio);

/*
 * FUNCTION: ase_tio_attinp
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
int ase_tio_attinp (ase_tio_t* tio, ase_tio_io_t input, void* arg);

/*
 * FUNCTION: ase_tio_detinp
 *   Detaches an input handler function
 *
 * PARAMETERS:
 *   tio - a tio object
 * 
 * RETURNS:
 *   0 on success, -1 on failure
 */
int ase_tio_detinp (ase_tio_t* tio);

/*
 * FUNCTION: ase_tio_attoutp
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
int ase_tio_attoutp (ase_tio_t* tio, ase_tio_io_t output, void* arg);

/*
 * FUNCTION: ase_tio_detoutp
 *   Detaches an output handler function
 * 
 * PARAMETERS:
 *   tio - a tio object
 *
 * RETURNS:
 *   0 on success, -1 on failure
 */
int ase_tio_detoutp (ase_tio_t* tio);


/*
 * FUNCTION: ase_tio_flush
 *   Flushes the output buffer
 *
 * PARAMETERS:
 *   tio - a tio object
 *
 * RETURNS:
 *   Number of bytes written on success, -1 on failure
 */
ase_ssize_t ase_tio_flush (ase_tio_t* tio);

/*
 * FUNCTION: ase_tio_purge
 *   Erases all input and output buffered 
 * 
 * PARAMETERS:
 *   tio - a tio object
 * 
 */
void ase_tio_purge (ase_tio_t* tio);

/* 
 * FUNCTION: ase_tio_getc
 *   Reads a single character
 */
ase_ssize_t ase_tio_getc (ase_tio_t* tio, ase_char_t* c);

/* 
 * FUNCTION: ase_tio_gets
 * 
 * DESCRIPTION:
 *   <ase_tio_gets> inserts a terminating null if there is a room
 */
ase_ssize_t ase_tio_gets (ase_tio_t* tio, ase_char_t* buf, ase_size_t size);

/* 
 * FUNCTION: ase_tio_getsx
 * 
 * DESCRIPTION:
 *   <ase_tio_getsx> doesn't insert a terminating null character 
 */
ase_ssize_t ase_tio_getsx (ase_tio_t* tio, ase_char_t* buf, ase_size_t size);

/*
 * FUNCTION: ase_tio_getstr
 */
ase_ssize_t ase_tio_getstr (ase_tio_t* tio, ase_str_t* buf);

/*
 * FUNCTION: ase_tio_putc
 */
ase_ssize_t ase_tio_putc (ase_tio_t* tio, ase_char_t c);

/*
 * FUNCTION: ase_tio_puts
 */
ase_ssize_t ase_tio_puts (ase_tio_t* tio, const ase_char_t* str);

/*
 * FUNCTION: ase_tio_putsx
 */
ase_ssize_t ase_tio_putsx (ase_tio_t* tio, const ase_char_t* str, ase_size_t size);

/*
 * FUNCTION: ase_tio_putsn
 */
ase_ssize_t ase_tio_putsn (ase_tio_t* tio, ...);

/*
 * FUNCTION: ase_tio_putsxn
 */
ase_ssize_t ase_tio_putsxn (ase_tio_t* tio, ...);

#ifdef __cplusplus
}
#endif

#endif
