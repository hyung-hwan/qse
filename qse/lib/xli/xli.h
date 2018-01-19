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

#ifndef _QSE_LIB_XLI_XLI_H_
#define _QSE_LIB_XLI_XLI_H_

#include <qse/xli/xli.h>
#include <qse/cmn/str.h>
#include <qse/cmn/rbt.h>
#include "../cmn/mem-prv.h"

enum qse_xli_tok_type_t
{
	QSE_XLI_TOK_EOF,
	QSE_XLI_TOK_XINCLUDE,
	QSE_XLI_TOK_TRUE,
	QSE_XLI_TOK_FALSE,
	QSE_XLI_TOK_NIL,
	QSE_XLI_TOK_COLON,
	QSE_XLI_TOK_SEMICOLON,
	QSE_XLI_TOK_LBRACE,
	QSE_XLI_TOK_RBRACE,
	QSE_XLI_TOK_LBRACK,
	QSE_XLI_TOK_RBRACK,
	QSE_XLI_TOK_LPAREN,
	QSE_XLI_TOK_RPAREN,
	QSE_XLI_TOK_EQ,
	QSE_XLI_TOK_COMMA,
	QSE_XLI_TOK_DQSTR,
	QSE_XLI_TOK_SQSTR,
	QSE_XLI_TOK_NSTR,
	QSE_XLI_TOK_IDENT,
	QSE_XLI_TOK_TEXT,
	QSE_XLI_TOK_TAG,
	QSE_XLI_TOK_NL
};

typedef enum qse_xli_tok_type_t qse_xli_tok_type_t;

typedef struct qse_xli_tok_t qse_xli_tok_t;

struct qse_xli_tok_t
{
	qse_xli_tok_type_t type;
	qse_str_t*         name;
	qse_xli_loc_t      loc;
};

typedef struct qse_xli_list_link_t qse_xli_list_link_t;

struct qse_xli_list_link_t
{
	qse_xli_list_link_t* next;
	qse_xli_list_t* list;
};

typedef struct qse_xli_root_list_t qse_xli_root_list_t;

struct qse_xli_root_list_t
{
	qse_xli_list_t list;
	qse_xli_nil_t xnil;
	qse_xli_true_t xtrue;
	qse_xli_false_t xfalse;
	qse_mmgr_t* mmgr;
};

struct qse_xli_t
{
	qse_mmgr_t* mmgr;

	qse_xli_errstr_t errstr; /**< error string getter */
	qse_xli_errnum_t errnum; /**< stores an error number */
	qse_char_t errmsg[128];  /**< error message holder */
	qse_xli_loc_t errloc;    /**< location of the last error */

	struct
	{
		int trait;          
		qse_size_t pair_xtnsize;
		qse_size_t root_xtnsize;
		qse_char_t key_splitter; /**< character to use to split a key in the fqpn format */
		qse_xli_cbs_t cbs;

		int (*strcmp) (const qse_char_t* s1, const qse_char_t* s2);
		int (*strxcmp) (const qse_char_t* s1, qse_size_t len1, const qse_char_t* s2);
	} opt;

	qse_xli_ecb_t* ecb;

	qse_xli_root_list_t* root;
	qse_xli_list_link_t* parlink; /* internal use only - link that points to the list being read currently */

	qse_str_t* dotted_curkey;
	qse_rbt_t* schema;

	qse_xli_tok_t tok;
	int tok_status;
	struct
	{
		qse_xli_io_impl_t impl; /* input handler */
		qse_xli_io_lxc_t  last;
		qse_xli_io_arg_t  top; /* for top level */
		qse_xli_io_arg_t* inp; /* current */
	} rio;
	qse_link_t* rio_names;

	struct 
	{
		qse_xli_io_impl_t impl; /* output handler */
		qse_xli_io_arg_t  top; /* for top level */
		qse_xli_io_arg_t* inp; /* current */
	} wio;
	qse_link_t* wio_names;
};


#if defined(__cplusplus)
extern "C" {
#endif

int qse_xli_init (qse_xli_t* xli, qse_mmgr_t* mmgr, qse_size_t rootxtnsize);

void qse_xli_fini (qse_xli_t* xli);

const qse_char_t* qse_xli_dflerrstr (
     const qse_xli_t* xli, qse_xli_errnum_t errnum);

void qse_xli_clearrionames (qse_xli_t* xli);
void qse_xli_clearwionames (qse_xli_t* xli);

int qse_xli_getchar (qse_xli_t* xli);
int qse_xli_openrstream (qse_xli_t* xli, qse_xli_io_arg_t* arg);
int qse_xli_closeactiverstream (qse_xli_t* xli);

int qse_xli_openwstream (qse_xli_t* xli, const qse_char_t* path, int old_depth);
int qse_xli_closeactivewstream (qse_xli_t* xli, int* org_depth);
int qse_xli_flushwstream (qse_xli_t* xli, qse_xli_io_arg_t* arg);

qse_xli_list_link_t* qse_xli_makelistlink (qse_xli_t* xli, qse_xli_list_t* parlist);
void qse_xli_freelistlink (qse_xli_t* xli, qse_xli_list_link_t* link);


qse_xli_str_t* qse_xli_makestrval (qse_xli_t* xli, const qse_cstr_t* value, const qse_char_t* strtag);
qse_xli_list_t* qse_xli_makelistval (qse_xli_t* xli);
void qse_xli_freeval (qse_xli_t* xli, qse_xli_val_t* val);

#if defined(__cplusplus)
}
#endif



#endif
