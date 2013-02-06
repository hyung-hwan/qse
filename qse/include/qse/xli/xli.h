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

#ifndef _QSE_XLI_XLI_H_
#define _QSE_XLI_XLI_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_xli_t qse_xli_t;

enum qse_xli_errnum_t
{
	QSE_XLI_ENOERR,  /**< no error */
	QSE_XLI_EOTHER,  /**< other error */
	QSE_XLI_ENOIMPL, /**< not implemented */
	QSE_XLI_ESYSERR, /**< subsystem error */
	QSE_XLI_EINTERN, /**< internal error */
	QSE_XLI_ENOMEM,  /**< insufficient memory */
	QSE_XLI_EINVAL,  /**< invalid parameter or data */
	QSE_XLI_EIOUSR   /**< i/o handler error */
};
typedef enum qse_xli_errnum_t qse_xli_errnum_t;

enum qse_xli_opt_t
{
	QSE_XLI_TRAIT
};
typedef enum qse_xli_opt_t qse_xli_opt_t;

enum qse_xli_trait_t
{
	QSE_XLI_NOTEXT = (1 << 0)
};
typedef enum qse_xli_trait_t qse_xli_trait_t;

typedef struct qse_xli_val_t qse_xli_val_t;
typedef struct qse_xli_str_t qse_xli_str_t;
typedef struct qse_xli_list_t qse_xli_list_t;

typedef struct qse_xli_atom_t qse_xli_atom_t;
typedef struct qse_xli_pair_t qse_xli_pair_t;
typedef struct qse_xli_text_t qse_xli_text_t;
typedef struct qse_xli_file_t qse_xli_file_t;

enum qse_xli_val_type_t
{
	QSE_XLI_STR,
	QSE_XLI_LIST,
};
typedef enum qse_xli_val_type_t qse_xli_val_type_t;

enum qse_xli_atom_type_t
{
	QSE_XLI_PAIR,
	QSE_XLI_TEXT,
	QSE_XLI_FILE,
};
typedef enum qse_xli_atom_type_t qse_xli_atom_type_t;

#define QSE_XLI_VAL_HDR \
	qse_xli_val_type_t type;

struct qse_xli_val_t
{
	QSE_XLI_VAL_HDR;
};

struct qse_xli_list_t
{
	QSE_XLI_VAL_HDR;
	qse_xli_atom_t* head;
	qse_xli_atom_t* tail;
};

struct qse_xli_str_t
{
	QSE_XLI_VAL_HDR;
	int verbatim;
	const qse_char_t* ptr;
	qse_size_t len;
};

#define QSE_XLI_ATOM_HDR \
	qse_xli_atom_type_t type; \
	qse_xli_atom_t* prev; \
	qse_xli_atom_t* next; \
	qse_xli_file_t* file

struct qse_xli_atom_t
{
	QSE_XLI_ATOM_HDR;
};

struct qse_xli_pair_t
{
	QSE_XLI_ATOM_HDR;
	const qse_char_t* key;
	const qse_char_t* name; 
	qse_xli_val_t* val;
};

struct qse_xli_text_t
{
	QSE_XLI_ATOM_HDR;
	const qse_char_t* ptr;
	qse_size_t len;
};

struct qse_xli_file_t
{
	QSE_XLI_ATOM_HDR;
	const qse_char_t* path;
};

/**
 * The qse_xli_ecb_close_t type defines the callback function
 * called when an xli object is cloxli.
 */
typedef void (*qse_xli_ecb_close_t) (
	qse_xli_t* xli  /**< xli */
);

typedef void (*qse_xli_ecb_clear_t) (
	qse_xli_t* xli  /**< xli */
);

/**
 * The qse_xli_ecb_t type defines an event callback set.
 * You can register a callback function set with
 * qse_xli_pushecb().  The callback functions in the registered
 * set are called in the reverse order of registration.
 */
typedef struct qse_xli_ecb_t qse_xli_ecb_t;
struct qse_xli_ecb_t
{
	/** called by qse_xli_close() */
	qse_xli_ecb_close_t close;
	/** called by qse_xli_clear() */
	qse_xli_ecb_clear_t clear;

	/* internal use only. don't touch this field */
	qse_xli_ecb_t* next;
};

typedef struct qse_xli_loc_t qse_xli_loc_t;

/** 
 * The qse_xli_loc_t defines a structure to store location information.
 */
struct qse_xli_loc_t
{
	const qse_char_t* file;
	qse_size_t line; /**< line  */
	qse_size_t colm; /**< column */
};

/**
 * The qse_xli_io_cmd_t type defines I/O command codes. The code indicates 
 * the action to take in an I/O handler.
 */
enum qse_xli_io_cmd_t
{
	QSE_XLI_IO_OPEN  = 0,
	QSE_XLI_IO_CLOSE = 1,
	QSE_XLI_IO_READ  = 2,
	QSE_XLI_IO_WRITE = 3
};
typedef enum qse_xli_io_cmd_t qse_xli_io_cmd_t;

typedef struct qse_xli_io_lxc_t qse_xli_io_lxc_t;
struct qse_xli_io_lxc_t
{
	qse_cint_t        c;    /**< character */
	qse_size_t        line; /**< line */
	qse_size_t        colm; /**< column */
	const qse_char_t* file; /**< file */
};

enum qse_xli_io_arg_flag_t
{
	QSE_XLI_IO_INCLUDED = (1 << 0)
};

typedef struct qse_xli_io_arg_t qse_xli_io_arg_t;
struct qse_xli_io_arg_t 
{
	/** 
	 * [IN] bitwise-ORed of #qse_xli_io_arg_flag_t.
	 * The field is set with #QSE_XLI_SIO_INCLUDED if an included file
	 * is handled. 
	 */
	int flags;  

	/** 
	 * [IN/OUT] name of I/O object. 
	 * if #QSE_XLI_SIO_INCLUDED is not set, the name is set to #QSE_NULL.
	 * the source stream handler(#qse_xli_io_impl_t) can change this field
	 * to give useful information back to the parser.
	 *
	 * if #QSE_XLI_SIO_INCLUDED is set in the flags field,  
	 * the name field is set to the name of the included file.
	 */
	const qse_char_t* name;   

	/** 
	 * [OUT] I/O handle set by a handler. 
	 * The source stream handler can set this field when it opens a stream.
	 * All subsequent operations on the stream see this field as set
	 * during opening.
	 */
	void* handle;

	/*-- from here down, internal use only --*/
	struct
	{
		qse_char_t buf[1024];
		qse_size_t pos;
		qse_size_t len;
	} b;

	qse_size_t line;
	qse_size_t colm;

	qse_xli_io_lxc_t last;
	qse_xli_io_arg_t* next;
};

/** 
 * The qse_xli_io_impl_t type defines an I/O handler. I/O handlers are called by
 * qse_xli_exec().
 */
typedef qse_ssize_t (*qse_xli_io_impl_t) (
	qse_xli_t*        xli,
	qse_xli_io_cmd_t  cmd,
	qse_xli_io_arg_t* arg,
	qse_char_t*       data,
	qse_size_t        count
);

#if defined(__cplusplus)
extern "C" {
#endif


QSE_EXPORT qse_xli_t* qse_xli_open (
	qse_mmgr_t* mmgr,
	qse_size_t xtnsize
);

QSE_EXPORT void qse_xli_close (
	qse_xli_t* xli
);


QSE_EXPORT qse_mmgr_t* qse_xli_getmmgr (
	qse_xli_t* xli
);

QSE_EXPORT void* qse_xli_getxtn (
	qse_xli_t* xli
);

QSE_EXPORT void qse_xli_pushecb (
	qse_xli_t*     xli,
	qse_xli_ecb_t* ecb
);

QSE_EXPORT qse_xli_ecb_t* qse_xli_popecb (
	qse_xli_t* xli
);

QSE_EXPORT void* qse_xli_allocmem (
	qse_xli_t* xli,
	qse_size_t size
);

QSE_EXPORT void* qse_xli_callocmem (
	qse_xli_t* xli,
	qse_size_t size
);

QSE_EXPORT void qse_xli_freemem (
	qse_xli_t* xli, 
	void*      ptr
);

QSE_EXPORT qse_xli_pair_t* qse_xli_insertpairwithemptylist (
     qse_xli_t*        xli,
	qse_xli_list_t*   parent,
	qse_xli_atom_t*   peer,
     const qse_char_t* key,
	const qse_char_t* name
);

QSE_EXPORT qse_xli_pair_t* qse_xli_insertpairwithstr (
     qse_xli_t*        xli, 
	qse_xli_list_t*   parent,
	qse_xli_atom_t*   peer,
     const qse_char_t* key,
	const qse_char_t* name,
	const qse_char_t* value,
	int               verbatim
);


QSE_EXPORT void qse_xli_clear (
	qse_xli_t* xli
);

QSE_EXPORT int qse_xli_read (
	qse_xli_t*        xli,
	qse_xli_io_impl_t io
);

QSE_EXPORT int qse_xli_write (
	qse_xli_t*        xli,
	qse_xli_io_impl_t io
);

#if defined(__cplusplus)
}
#endif

#endif
