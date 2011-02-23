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

enum qse_scm_ent_type_t
{
	QSE_SCM_ENT_NIL     = (1 << 0),
	QSE_SCM_ENT_T       = (1 << 1),
	QSE_SCM_ENT_F       = (1 << 2),
	QSE_SCM_ENT_NUM     = (1 << 3),
	QSE_SCM_ENT_STR     = (1 << 4), 
	QSE_SCM_ENT_NAM     = (1 << 5),
	QSE_SCM_ENT_SYM     = (1 << 6),
	QSE_SCM_ENT_PAIR    = (1 << 7),
	QSE_SCM_ENT_PROC    = (1 << 8),
	QSE_SCM_ENT_SYNT    = (1 << 9)

};

#if 0
#define QSE_SCM_ENT_CLOSURE       64    /* 0000000001000000 */
#define QSE_SCM_ENT_CONTINUATION 128    /* 0000000010000000 */
#define QSE_SCM_ENT_MACRO        256    /* 0000000100000000 */
#define QSE_SCM_ENT_PROMISE      512    /* 0000001000000000 */
#endif

typedef struct qse_scm_ent_t qse_scm_ent_t;

/**
 * The qse_scm_ent_t type defines an entity that represents an individual
 * value in scheme.
 */
struct qse_scm_ent_t
{
	qse_uint16_t dswcount: 2;
	qse_uint16_t mark:     1;
	qse_uint16_t atom:     1;
	qse_uint16_t type:     12;

	union
	{
		struct
		{
			qse_long_t val;
		} num; /* number */

		struct
		{
			/* a string doesn't need to be null-terminated 
			 * as the length is remembered */
			qse_char_t* ptr; 
			qse_size_t  len;
		} str; /* string */

		struct
		{
			qse_char_t* ptr;  /* null-terminated string */
			int         code; /* used for syntax entities only */
		} lab; /* label */

		struct
		{
			int code;
		} proc;
		
		struct
		{
			qse_scm_ent_t* ent[2];
		} ref; 
	} u;
};

#define DSWCOUNT(v)       ((v)->dswcount)
#define MARK(v)           ((v)->mark)
#define TYPE(v)           ((v)->type)
#define ATOM(v)           ((v)->atom)
#define NUM_VALUE(v)      ((v)->u.num.val)
#define STR_PTR(v)        ((v)->u.str.ptr)
#define STR_LEN(v)        ((v)->u.str.len)
#define LAB_PTR(v)        ((v)->u.lab.ptr)
#define LAB_CODE(v)       ((v)->u.lab.code)
#define SYM_NAME(v)       ((v)->u.ref.ent[0])
#define SYM_PROP(v)       ((v)->u.ref.ent[1])
#define PAIR_CAR(v)       ((v)->u.ref.ent[0])
#define PAIR_CDR(v)       ((v)->u.ref.ent[1])
#define PROC_CODE(v)      ((v)->u.proc.code)
#define SYNT_CODE(v)      LAB_CODE(SYM_NAME(v))

/**
 * The qse_scm_enb_t type defines a value block. A value block is allocated
 * when more memory is requested and is chained to existing value blocks.
 */
typedef struct qse_scm_enb_t qse_scm_enb_t;
struct qse_scm_enb_t
{
	qse_scm_ent_t* ptr;
	qse_size_t     len;
	qse_scm_enb_t* next;	
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
	qse_scm_ent_t* nil;
	qse_scm_ent_t* t;
	qse_scm_ent_t* f;

	qse_scm_ent_t* gloenv; /* global environment */
	qse_scm_ent_t* symtab; /* symbol table */

	/* registers */
	struct
	{
		qse_scm_ent_t* arg; /* function arguments */
		qse_scm_ent_t* env; /* current environment */
		qse_scm_ent_t* cod; /* current code */
		qse_scm_ent_t* dmp; /* stack register for next evaluation */
	} reg;

	struct
	{
		qse_scm_enb_t* ebl;  /* entity block list */
		qse_scm_ent_t* free;
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
