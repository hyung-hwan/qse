/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_XLI_XLI_H_
#define _QSE_XLI_XLI_H_

/** \file
 * This file defines functions and data types for handling key-value pairs
 * orgianized in the way shown below.
 *
 * \code
 *  key1 = "abc";
 *  key2 {
 *    key21 = "hello";
 *    key22 = "world";
 *    key23 {
 *       key231 = 99999;
 *    }
 *  }
 *  key3 = "12345";
 * \endcode
 *
 */

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
	QSE_XLI_ENOENT,  /**< '${0}' not found */
	QSE_XLI_EEXIST,  /**< '${0}' already exists */
	QSE_XLI_EIOFIL,  /**< io error with file '${0}' */
	QSE_XLI_EIOUSR,  /**< i/o handler error */

	QSE_XLI_ESYNTAX,  /**< syntax error */
	QSE_XLI_ESCOLON,  /**< semicolon expected in place of '${0}' */
	QSE_XLI_ELBREQ,   /**< { or = expected in place of '${0}' */
	QSE_XLI_ERBRCE,   /**< } expected in place of '${0}' */
	QSE_XLI_EPAVAL,   /**< pair value expected in place of '${0}' */
	QSE_XLI_ESTRNC,   /**< string not closed */
	QSE_XLI_ETAGNC,   /**< string tag not closed */
	QSE_XLI_EINCLSTR ,/**< '@include' not followed by a string */
	QSE_XLI_ELXCHR,   /**< invalid character '${0} */
	QSE_XLI_ETAGCHR,  /**< invalid tag character '${0} */
	QSE_XLI_EXKWNR,   /**< @word '${0}' not recognized */
	QSE_XLI_EXKWEM,   /**< @ not followed by a valid word  */
	QSE_XLI_EIDENT,   /**< invalid identifier '${0}' */
	QSE_XLI_ENOKEY,   /**< missing key after key tag */
	QSE_XLI_EUDKEY,   /**< undefined key '${0}' */
	QSE_XLI_ENOALI,   /**< no alias for '${0}' */
	QSE_XLI_EILVAL,   /**< illegal value for '${0}' */
	QSE_XLI_ENOVAL,   /**< no value for '${0}' */
	QSE_XLI_ESTRSEG   /**< too many string segments for '${0}' */
};
typedef enum qse_xli_errnum_t qse_xli_errnum_t;

/**
 * The qse_xli_errstr_t type defines an error string getter. It should return 
 * an error formatting string for an error number requested. A new string
 * should contain the same number of positional parameters (${X}) as in the
 * default error formatting string. You can set a new getter into a stream
 * editor with the qse_xli_seterrstr() function to customize an error string.
 */
typedef const qse_char_t* (*qse_xli_errstr_t) (
	const qse_xli_t* xli,   /**< stream editor */
	qse_xli_errnum_t num    /**< error number */
);

enum qse_xli_opt_t
{
	QSE_XLI_TRAIT,

	QSE_XLI_PAIRXTNSIZE,

	/** 
	 *  The size of extension area associated with the root list node.
	 *  you can get the pointer to the extension with qse_getxlirootxtn().
	 *  the new size set takes effect after the call to qse_xli_yieldroot().
	 */
	QSE_XLI_ROOTXTNSIZE,

	QSE_XLI_KEYSPLITTER
};
typedef enum qse_xli_opt_t qse_xli_opt_t;

enum qse_xli_trait_t
{
	QSE_XLI_KEYNODUP  = (1 << 1),
	QSE_XLI_KEYALIAS  = (1 << 2),

	QSE_XLI_KEEPTEXT  = (1 << 3), /**< keep comment text */
	QSE_XLI_KEEPFILE  = (1 << 4), /**< keep inclusion file info */

	QSE_XLI_MULSEGSTR = (1 << 5), /**< support multi-segmented string */
	QSE_XLI_LEADDIGIT = (1 << 6), /**< allow a leading digit in an identifier */

	QSE_XLI_NONIL     = (1 << 7), 
	QSE_XLI_NOLIST    = (1 << 8),

	/** enable a pair key tag. a pair key tag is a bracketed 
	 *  word placed in front of a pair key. for example,
	 *    [tg] A = "abc";
	 *  "tg" is stored into the tag field of qse_xli_pair_t. */
	QSE_XLI_KEYTAG    = (1 << 9),

	/** enable a string tag. a string tag is a bracketed word 
	 *  placed in front of a string value. for example, 
	 *    A = [tg] "abc"; 
	 *  "tg" is stored into the tag field of qse_xli_str_t. */
	QSE_XLI_STRTAG    = (1 << 10), 

	/** enable pair validation against pair definitions while reading */
	QSE_XLI_VALIDATE  = (1 << 11)
};
typedef enum qse_xli_trait_t qse_xli_trait_t;

typedef struct qse_xli_val_t  qse_xli_val_t;
typedef struct qse_xli_nil_t  qse_xli_nil_t;
typedef struct qse_xli_str_t  qse_xli_str_t;
typedef struct qse_xli_list_t qse_xli_list_t;

typedef struct qse_xli_atom_t qse_xli_atom_t;
typedef struct qse_xli_pair_t qse_xli_pair_t;
typedef struct qse_xli_text_t qse_xli_text_t;
typedef struct qse_xli_file_t qse_xli_file_t;
typedef struct qse_xli_eof_t  qse_xli_eof_t;

enum qse_xli_val_type_t
{
	QSE_XLI_NIL,
	QSE_XLI_STR,
	QSE_XLI_LIST,
};
typedef enum qse_xli_val_type_t qse_xli_val_type_t;

enum qse_xli_atom_type_t
{
	QSE_XLI_PAIR,
	QSE_XLI_TEXT,
	QSE_XLI_FILE,
	QSE_XLI_EOF 
};
typedef enum qse_xli_atom_type_t qse_xli_atom_type_t;

#define QSE_XLI_VAL_HDR \
	qse_xli_val_type_t type

struct qse_xli_val_t
{
	QSE_XLI_VAL_HDR;
};

struct qse_xli_nil_t
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
	const qse_char_t*  tag;
	const qse_char_t*  ptr;
	qse_size_t         len; 
	qse_xli_str_t*     next;
};

#define QSE_XLI_ATOM_HDR \
	qse_xli_atom_type_t type; \
	qse_xli_atom_t* prev; \
	qse_xli_atom_t* next; \
	qse_xli_file_t* file; \
	qse_xli_list_t* super

struct qse_xli_atom_t
{
	QSE_XLI_ATOM_HDR;
};

struct qse_xli_pair_t
{
	QSE_XLI_ATOM_HDR;
	const qse_char_t* key;
	const qse_char_t* alias; 
	const qse_char_t* tag;
	qse_xli_val_t*    val;
};

struct qse_xli_text_t
{
	QSE_XLI_ATOM_HDR;
	const qse_char_t* ptr;
};

struct qse_xli_file_t
{
	QSE_XLI_ATOM_HDR;
	const qse_char_t* path;
};

struct qse_xli_eof_t
{
	QSE_XLI_ATOM_HDR;
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

typedef struct qse_xli_io_arg_t qse_xli_io_arg_t;
struct qse_xli_io_arg_t 
{
	/** 
	 * [IN/OUT] name of I/O object. 
	 * It is #QSE_NULL for the top-level stream. It points to a stream name
	 * for an included stream. This can be changed by an I/O handler.
	 */
	const qse_char_t* name;   

	/** 
	 * [OUT] I/O handle set by a handler. 
	 * The source stream handler can set this field when it opens a stream.
	 * All subsequent operations on the stream see this field as set
	 * during opening.
	 */
	void* handle;

	/**
	 * [IN] points to the includer
	 */
	qse_xli_io_arg_t* prev;

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


enum qse_xli_scm_flag_t
{
	/*QSE_XLI_SCM_REQUIRED = (1 << 0), TODO: support this. */
	QSE_XLI_SCM_VALNIL   = (1 << 1),
	QSE_XLI_SCM_VALSTR   = (1 << 2),
	QSE_XLI_SCM_VALLIST  = (1 << 3),

	QSE_XLI_SCM_KEYNODUP = (1 << 4),
	QSE_XLI_SCM_KEYALIAS = (1 << 5),

	/** Indicates that the value is a list with uncertain definitions with
	 *  the following constraints:
	 *    no key aliases and duplicate keys are allowed.
	 *    only a single-segment string is allowed as a value.
	 *
	 *  Each pair in the list is treated as if { QSE_SLI_SCM_VALSTR | QSE_XLI_SCM_KEYNODUP, 1, 1 }
	 *  is specified.
	 *
	 *  Applies only if #QSE_XLI_SCM_VALLIST is set. */
	QSE_XLI_SCM_VALIFFY  = (1 << 6) 
};
typedef enum qse_xli_scm_flag_t qse_xli_scm_flag_t;

struct qse_xli_scm_t
{
	int flags;
	int str_minseg;
	int str_maxseg;
};

typedef struct qse_xli_scm_t qse_xli_scm_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_xli_t* qse_xli_open (
	qse_mmgr_t* mmgr,
	qse_size_t xtnsize,
	qse_size_t rootxtnsize
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


/**
 * The qse_xli_getopt() function gets the value of an option
 * specified by \a id into the buffer pointed to by \a value.
 *
 * The \a value field is dependent on \a id:
 *  - #QSE_XLI_TRAIT - int*, 0 or bitwixli-ORed of #qse_xli_trait_t values
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_xli_getopt (
	qse_xli_t*    xli,
	qse_xli_opt_t id,
	void*         value
);

/**
 * The qse_xli_setopt() function sets the value of an option 
 * specified by \a id to the value pointed to by \a value.
 *
 * The \a value field is dependent on \a id:
 *  - #QSE_XLI_TRAIT - const int*, 0 or bitwixli-ORed of #qse_xli_trait_t values
 *
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_xli_setopt (
	qse_xli_t*    xli,
	qse_xli_opt_t id,
	const void*   value
);

/**
 * The qse_xli_geterrstr() gets an error string getter.
 */
QSE_EXPORT qse_xli_errstr_t qse_xli_geterrstr (
	const qse_xli_t* xli    /**< stream editor */
);

/**
 * The qse_xli_seterrstr() sets an error string getter that is called to
 * compose an error message when its retrieval is requested.
 *
 * Here is an example of changing the formatting string for the #QSE_XLI_EIOFIL 
 * error.
 * @code
 * qse_xli_errstr_t orgerrstr;
 *
 * const qse_char_t* myerrstr (qse_xli_t* xli, qse_xli_errnum_t num)
 * {
 *   if (num == QSE_XLI_EIOFIL) return QSE_T("file I/O error in ${0}");
 *   return orgerrstr (xli, num);
 * }
 * int main ()
 * {
 *    qse_xli_t* xli;
 *    ...
 *    orgerrstr = qse_xli_geterrstr (xli);
 *    qse_xli_seterrstr (xli, myerrstr);
 *    ...
 * }
 * @endcode
 */
QSE_EXPORT void qse_xli_seterrstr (
	qse_xli_t*       xli,   /**< stream editor */
	qse_xli_errstr_t errstr /**< an error string getter */
);

/**
 * The qse_xli_geterrnum() function gets the number of the last error.
 * @return error number
 */
QSE_EXPORT qse_xli_errnum_t qse_xli_geterrnum (
	const qse_xli_t* xli /**< stream editor */
);

/**
 * The qse_xli_geterrloc() function gets the location where the last error 
 * has occurred.
 * @return error location
 */
QSE_EXPORT const qse_xli_loc_t* qse_xli_geterrloc (
	const qse_xli_t* xli /**< stream editor */
);

/**
 * The qse_xli_geterrmsg() function gets a string describing the last error.
 * @return error message pointer
 */
QSE_EXPORT const qse_char_t* qse_xli_geterrmsg (
	const qse_xli_t* xli /**< stream editor */
);

/**
 * The qse_xli_geterror() function gets an error number, an error location, 
 * and an error message. The information is set to the memory area pointed 
 * to by each parameter.
 */
QSE_EXPORT void qse_xli_geterror (
	const qse_xli_t*   xli,    /**< stream editor */
	qse_xli_errnum_t*  errnum, /**< error number */
	const qse_char_t** errmsg, /**< error message */
	qse_xli_loc_t*     errloc  /**< error location */
);

/**
 * The qse_xli_seterrnum() function sets error information omitting error
 * location.
 */
QSE_EXPORT void qse_xli_seterrnum (
	qse_xli_t*        xli,    /**< stream editor */
	qse_xli_errnum_t  errnum, /**< error number */
	const qse_cstr_t* errarg  /**< argument for formatting error message */
);

/**
 * The qse_xli_seterrmsg() function sets error information with a customized 
 * message for a given error number.
 */
QSE_EXPORT void qse_xli_seterrmsg (
	qse_xli_t*        xli,      /**< stream editor */
	qse_xli_errnum_t  errnum,   /**< error number */
	const qse_char_t* errmsg,   /**< error message */
	const qse_xli_loc_t* errloc /**< error location */
);

/**
 * The qse_xli_seterror() function sets an error number, an error location, and
 * an error message. An error string is compoxli of a formatting string
 * and an array of formatting parameters.
 */
QSE_EXPORT void qse_xli_seterror (
	qse_xli_t*           xli,    /**< stream editor */
	qse_xli_errnum_t     errnum, /**< error number */
	const qse_cstr_t*    errarg, /**< array of arguments for formatting 
	                              *   an error message */
	const qse_xli_loc_t* errloc  /**< error location */
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

QSE_EXPORT qse_xli_pair_t* qse_xli_insertpair (
	qse_xli_t*        xli,
	qse_xli_list_t*   list,
	qse_xli_atom_t*   peer,
	const qse_char_t* key,
	const qse_char_t* alias,
	const qse_char_t* keytag,
	qse_xli_val_t*    val
);

/**
 * The qse_xli_insertpairwithemptylist() function inserts a new pair
 * with an empty list as a value. You should call this function for adding
 * a new pair holding a list.
 *
 * \return pointer to an inserted pair on success, #QSE_NULL on failure.
 */
QSE_EXPORT qse_xli_pair_t* qse_xli_insertpairwithemptylist (
	qse_xli_t*        xli,
	qse_xli_list_t*   list,
	qse_xli_atom_t*   peer,
	const qse_char_t* key,
	const qse_char_t* alias,
	const qse_char_t* keytag
);

/**
 * The qse_xli_insertpairwithstr() function inserts a new pair with a key
 * named \a key with a string value \a value.
 */
QSE_EXPORT qse_xli_pair_t* qse_xli_insertpairwithstr (
	qse_xli_t*        xli, 
	qse_xli_list_t*   list,
	qse_xli_atom_t*   peer,
	const qse_char_t* key,
	const qse_char_t* alias,
	const qse_char_t* keytag,
	const qse_cstr_t* value,
	const qse_char_t* strtag
);

QSE_EXPORT qse_xli_pair_t* qse_xli_insertpairwithstrs (
	qse_xli_t*        xli, 
	qse_xli_list_t*   list,
	qse_xli_atom_t*   peer,
	const qse_char_t* key,
	const qse_char_t* alias,
	const qse_char_t* keytag,
	const qse_cstr_t  value[],
	qse_size_t        count
);

QSE_EXPORT qse_xli_text_t* qse_xli_inserttext (
	qse_xli_t* xli,
	qse_xli_list_t* parent,
	qse_xli_atom_t* peer,
	const qse_char_t* str
);

QSE_EXPORT qse_xli_file_t* qse_xli_insertfile (
	qse_xli_t* xli,
	qse_xli_list_t* parent,
	qse_xli_atom_t* peer,
	const qse_char_t* path
);

QSE_EXPORT qse_xli_eof_t* qse_xli_inserteof (
	qse_xli_t* xli,
	qse_xli_list_t* parent,
	qse_xli_atom_t* peer
);

QSE_EXPORT qse_xli_pair_t* qse_xli_findpair (
	qse_xli_t*            xli,
	const qse_xli_list_t* list,
	const qse_char_t*     fqpn
);

QSE_EXPORT qse_size_t qse_xli_countpairs (
	qse_xli_t*            xli,
	const qse_xli_list_t* list,
	const qse_char_t*     fqpn 
);


/**
 * The qse_xli_addsegtostr() function creates a new string segment made of
 * the character string pointed to by \a value and chains it to the XLI string
 * pointed to by \a str.
 */
QSE_EXPORT qse_xli_str_t* qse_xli_addsegtostr (
	qse_xli_t*        xli, 
	qse_xli_str_t*    str,
	const qse_char_t* tag,
	const qse_cstr_t* value
);

/**
 * The qse_xli_dupflatstr() function duplicates the character strings
 * found in the string list led by \a str and flattens them into a single
 * character string each of whose segment is delimited by '\0' and the last
 * segment is delimited by double '\0's. The string tags are not included.
 */
QSE_EXPORT qse_char_t* qse_xli_dupflatstr (
	qse_xli_t*     xli,
	qse_xli_str_t* str,
	qse_size_t*    len,
	qse_size_t*    nsegs
);

QSE_EXPORT qse_xli_list_t* qse_xli_getroot (
	qse_xli_t* xli
);

QSE_EXPORT void qse_xli_clearroot (
	qse_xli_t* xli
);

QSE_EXPORT qse_xli_list_t* qse_xli_yieldroot (
	qse_xli_t* xli
);

QSE_EXPORT void qse_xli_clear (
	qse_xli_t* xli
);

/**
 * The qse_xli_definepair() function defines a pair structure.
 */
QSE_EXPORT int qse_xli_definepair (
	qse_xli_t*           xli,
	const qse_char_t*    fqpn,
	const qse_xli_scm_t* scm
);

QSE_EXPORT int qse_xli_undefinepair (
	qse_xli_t*           xli,
	const qse_char_t*    fqpn
);

QSE_EXPORT void qse_xli_undefinepairs (
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

QSE_EXPORT void* qse_getxlipairxtn (
	qse_xli_pair_t* pair
);

QSE_EXPORT void* qse_getxlirootxtn (
	qse_xli_list_t* root
);

/**
 * The qse_freexliroot() function frees the root list acquired with qse_xli_yeildroot().
 */
QSE_EXPORT void qse_freexliroot (
	qse_xli_list_t* root
);

#if defined(__cplusplus)
}
#endif

#endif
