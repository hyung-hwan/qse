/*
 * $Id$
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

#ifndef _QSE_LIB_SCM_SCM_H_
#define _QSE_LIB_SCM_SCM_H_

#include "../cmn/mem.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/scm/scm.h>

#define QSE_SCM_ISUPPER(scm,c)  QSE_ISUPPER(c)
#define QSE_SCM_ISLOWER(scm,c)  QSE_ISLOWER(c)
#define QSE_SCM_ISALPHA(scm,c)  QSE_ISALPHA(c)
#define QSE_SCM_ISDIGIT(scm,c)  QSE_ISDIGIT(c)
#define QSE_SCM_ISXDIGIT(scm,c) QSE_ISXDIGIT(c)
#define QSE_SCM_ISALNUM(scm,c)  QSE_ISALNUM(c)
#define QSE_SCM_ISSPACE(scm,c)  QSE_ISSPACE(c)
#define QSE_SCM_ISPRINT(scm,c)  QSE_ISPRINT(c)
#define QSE_SCM_ISGRAPH(scm,c)  QSE_ISGRAPH(c)
#define QSE_SCM_ISCNTRL(scm,c)  QSE_ISCNTRL(c)
#define QSE_SCM_ISPUNCT(scm,c)  QSE_ISPUNCT(c)
#define QSE_SCM_TOUPPER(scm,c)  QSE_TOUPPER(c)
#define QSE_SCM_TOLOWER(scm,c)  QSE_TOLOWER(c)

#define QSE_SCM_VAL_STRING         1    /* 0000000000000001 */
#define QSE_SCM_VAL_NUMBER         2    /* 0000000000000010 */
#define QSE_SCM_VAL_SYMBOL         4    /* 0000000000000100 */
#define QSE_SCM_VAL_SYNTAX         8    /* 0000000000001000 */
#define QSE_SCM_VAL_PROC          16    /* 0000000000010000 */

#define QSE_SCM_VAL_PAIR          32    /* 0000000000100000 */
#define QSE_SCM_VAL_CLOSURE       64    /* 0000000001000000 */
#define QSE_SCM_VAL_CONTINUATION 128    /* 0000000010000000 */
#define QSE_SCM_VAL_MACRO        256    /* 0000000100000000 */
#define QSE_SCM_VAL_PROMISE      512    /* 0000001000000000 */
#define QSE_SCM_VAL_ATOM        4096    /* 0001000000000000 */ /* only for gc */

typedef struct qse_scm_val_t qse_scm_val_t;
struct qse_scm_val_t
{
	qse_uint16_t dsw_count: 2;
	qse_uint16_t mark:      1;
	qse_uint16_t types:     13;

	union
	{
		struct
		{
			qse_char_t* ptr;
			qse_size_t  len;
		} str;

		struct
		{
			qse_long_t val;
		} num;
		
		struct
		{
			qse_scm_val_t* car;
			qse_scm_val_t* cdr;
		} cons;

		/* arrayed cons. cona must maintain the
		 * same size as cons */
		struct
		{
			qse_scm_val_t* val[2];
		} cona; 
	} u;
};


/**
 * The qse_scm_vbl_t type defines a value block. A value block is allocated
 * when more memory is requested and is chained to existing value blocks.
 */
typedef struct qse_scm_vbl_t qse_scm_vbl_t;
struct qse_scm_vbl_t
{
	qse_scm_val_t* ptr;
	qse_size_t     len;
	qse_scm_vbl_t* next;	
};

struct qse_scm_t 
{
	QSE_DEFINE_COMMON_FIELDS (scm)

	/** error information */
	struct 
	{
		qse_scm_errstr_t str;      /**< error string getter */
		qse_scm_errnum_t num;      /**< stores an error number */
		qse_char_t       msg[128]; /**< error message holder */
		qse_scm_loc_t    loc;      /**< location of the last error */
	} err; 

	/** I/O functions */
	struct
	{
		qse_scm_io_t fns;

		struct
		{
			qse_scm_io_arg_t in;
			qse_scm_io_arg_t out;
		} arg;
	} io;

	/** data for reading */
	struct
	{
		qse_cint_t curc; 
		qse_scm_loc_t curloc;

		/** token */
		struct
		{
			int           type;
			qse_scm_loc_t loc;
			qse_long_t    ival;
			qse_real_t    rval;
			qse_str_t     name;
		} t;
	} r;

	/* common values */
	qse_scm_val_t* nil;
	qse_scm_val_t* t;
	qse_scm_val_t* f;

	/* global environment */
	qse_scm_val_t* genv;

	/* registers */
	struct
	{
		qse_scm_val_t* arg; /* function arguments */
		qse_scm_val_t* env; /* current environment */
		qse_scm_val_t* cod; /* current code */
		qse_scm_val_t* dmp; /* stack register for next evaluation */
	} reg;

	struct
	{
		qse_scm_vbl_t* vbl;  /* value block list */
		qse_scm_val_t* free;
	} mem;
};

#ifdef __cplusplus
extern "C" {
#endif

/* err.c */
const qse_char_t* qse_scm_dflerrstr (qse_scm_t* scm, qse_scm_errnum_t errnum);

#ifdef __cplusplus
}
#endif
#endif
