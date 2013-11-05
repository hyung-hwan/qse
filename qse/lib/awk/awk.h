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

#ifndef _QSE_LIB_AWK_AWK_H_
#define _QSE_LIB_AWK_AWK_H_

#include "../cmn/mem.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/lda.h>
#include <qse/cmn/rex.h>
#include <qse/cmn/rbt.h>

typedef struct qse_awk_chain_t qse_awk_chain_t;
typedef struct qse_awk_tree_t qse_awk_tree_t;

#include <qse/awk/awk.h>
#include <qse/cmn/chr.h>
#include "tree.h"
#include "fnc.h"
#include "parse.h"
#include "run.h"
#include "rio.h"
#include "val.h"
#include "err.h"
#include "misc.h"

#define ENABLE_FEATURE_SCACHE
#define FEATURE_SCACHE_NUM_BLOCKS 16
#define FEATURE_SCACHE_BLOCK_UNIT 16
#define FEATURE_SCACHE_BLOCK_SIZE 128

#define QSE_AWK_MAX_GBLS 9999
#define QSE_AWK_MAX_LCLS  9999
#define QSE_AWK_MAX_PARAMS  9999

#define QSE_AWK_ALLOC(awk,size)       QSE_MMGR_ALLOC((awk)->mmgr,size)
#define QSE_AWK_REALLOC(awk,ptr,size) QSE_MMGR_REALLOC((awk)->mmgr,ptr,size)
#define QSE_AWK_FREE(awk,ptr)         QSE_MMGR_FREE((awk)->mmgr,ptr)

#define QSE_AWK_ISUPPER(awk,c)  QSE_ISUPPER(c)
#define QSE_AWK_ISLOWER(awk,c)  QSE_ISLOWER(c)
#define QSE_AWK_ISALPHA(awk,c)  QSE_ISALPHA(c)
#define QSE_AWK_ISDIGIT(awk,c)  QSE_ISDIGIT(c)
#define QSE_AWK_ISXDIGIT(awk,c) QSE_ISXDIGIT(c)
#define QSE_AWK_ISALNUM(awk,c)  QSE_ISALNUM(c)
#define QSE_AWK_ISSPACE(awk,c)  QSE_ISSPACE(c)
#define QSE_AWK_ISPRINT(awk,c)  QSE_ISPRINT(c)
#define QSE_AWK_ISGRAPH(awk,c)  QSE_ISGRAPH(c)
#define QSE_AWK_ISCNTRL(awk,c)  QSE_ISCNTRL(c)
#define QSE_AWK_ISPUNCT(awk,c)  QSE_ISPUNCT(c)
#define QSE_AWK_TOUPPER(awk,c)  QSE_TOUPPER(c)
#define QSE_AWK_TOLOWER(awk,c)  QSE_TOLOWER(c)

#define QSE_AWK_STRDUP(awk,str) (qse_strdup(str,(awk)->mmgr))
#define QSE_AWK_STRXDUP(awk,str,len) (qse_strxdup(str,len,(awk)->mmgr))

enum qse_awk_rio_type_t
{
	/* rio types available */
	QSE_AWK_RIO_PIPE,
	QSE_AWK_RIO_FILE,
	QSE_AWK_RIO_CONSOLE,

	/* reserved for internal use only */
	QSE_AWK_RIO_NUM
};

struct qse_awk_tree_t
{
	qse_size_t ngbls; /* total number of globals */
	qse_size_t ngbls_base; /* number of intrinsic globals */
	qse_cstr_t cur_fun;
	qse_htb_t* funs; /* awk function map */

	qse_awk_nde_t* begin;
	qse_awk_nde_t* begin_tail;

	qse_awk_nde_t* end;
	qse_awk_nde_t* end_tail;

	qse_awk_chain_t* chain;
	qse_awk_chain_t* chain_tail;
	qse_size_t chain_size; /* number of nodes in the chain */

	int ok;
};

typedef struct qse_awk_tok_t qse_awk_tok_t;
struct qse_awk_tok_t
{
	int           type;
	qse_str_t*    name;
	qse_awk_loc_t loc;
};

struct qse_awk_t
{
	qse_mmgr_t* mmgr;

	/* primitive functions */
	qse_awk_prm_t  prm;

	/* options */
	struct
	{
		int trait;
		qse_xstr_t mod[2];
		qse_xstr_t incldirs;

		union
		{
			qse_size_t a[7];
			struct
			{
				qse_size_t incl;
				qse_size_t block_parse;
				qse_size_t block_run;
				qse_size_t expr_parse;
				qse_size_t expr_run;
				qse_size_t rex_build;
				qse_size_t rex_match;
			} s;
		} depth;
	} opt;

	/* parse tree */
	qse_awk_tree_t tree;

	/* temporary information that the parser needs */
	struct
	{
		struct
		{
			int block;
			int loop;
			int stmt; /* statement */
		} id;

		struct
		{
			qse_size_t block;
			qse_size_t loop;
			qse_size_t expr; /* expression */
			qse_size_t incl;
		} depth;

		/* function calls */
		qse_htb_t* funs;

		/* named variables */
		qse_htb_t* named;

		/* global variables */
		qse_lda_t* gbls;

		/* local variables */
		qse_lda_t* lcls;

		/* parameters to a function */
		qse_lda_t* params;

		/* maximum number of local variables */
		qse_size_t nlcls_max;

		/* some data to find if an expression is
		 * enclosed in parentheses or not.
		 * see parse_primary_lparen() and parse_print() in parse.c
		 */
		qse_size_t lparen_seq;
		qse_size_t lparen_last_closed;
	} parse;

	/* source code management */
	struct
	{
		qse_awk_sio_impl_t inf;
		qse_awk_sio_impl_t outf;

		qse_awk_sio_lxc_t last; 

		qse_size_t nungots;
		qse_awk_sio_lxc_t ungot[5];

		qse_awk_sio_arg_t arg; /* for the top level source */
		qse_awk_sio_arg_t* inp; /* current input argument. */
	} sio;
	qse_link_t* sio_names;

	/* previous token */
	qse_awk_tok_t ptok;
	/* current token */
	qse_awk_tok_t tok;
	/* look-ahead token */
	qse_awk_tok_t ntok;

	/* intrinsic functions */
	struct
	{
		qse_awk_fnc_t* sys;
		qse_htb_t* user;
	} fnc;

	struct
	{
		qse_char_t fmt[1024];
	} tmp;

	/* housekeeping */
	qse_awk_errstr_t errstr;
	qse_awk_errinf_t errinf;

	int stopall;
	qse_awk_ecb_t* ecb;

	qse_rbt_t* modtab;
};

struct qse_awk_chain_t
{
	qse_awk_nde_t* pattern;
	qse_awk_nde_t* action;
	qse_awk_chain_t* next;	
};

#define RTX_STACK_AT(rtx,n) ((rtx)->stack[(rtx)->stack_base+(n)])
#define RTX_STACK_NARGS(rtx) (RTX_STACK_AT(rtx,3))
#define RTX_STACK_ARG(rtx,n) RTX_STACK_AT(rtx,3+1+(n))
#define RTX_STACK_LCL(rtx,n) RTX_STACK_AT(rtx,3+(qse_size_t)RTX_STACK_NARGS(rtx)+1+(n))
#define RTX_STACK_RETVAL(rtx) RTX_STACK_AT(rtx,2)
#define RTX_STACK_GBL(rtx,n) ((rtx)->stack[(n)])
#define RTX_STACK_RETVAL_GBL(rtx) ((rtx)->stack[(rtx)->awk->tree.ngbls+2])

struct qse_awk_rtx_t
{
	int id;
	qse_htb_t* named;

	void** stack;
	qse_size_t stack_top;
	qse_size_t stack_base;
	qse_size_t stack_limit;
	int exit_level;

	qse_awk_val_ref_t* rcache[128];
	qse_size_t rcache_count;

#ifdef ENABLE_FEATURE_SCACHE
	qse_awk_val_str_t* scache
		[FEATURE_SCACHE_NUM_BLOCKS][FEATURE_SCACHE_BLOCK_SIZE];
	qse_size_t scache_count[FEATURE_SCACHE_NUM_BLOCKS];
#endif

	struct
	{
		qse_awk_val_int_t* ifree;
		qse_awk_val_chunk_t* ichunk;
		qse_awk_val_flt_t* rfree;
		qse_awk_val_chunk_t* rchunk;
	} vmgr;

	qse_awk_nde_blk_t* active_block;
	qse_byte_t* pattern_range_state;

	struct
	{
		qse_char_t buf[1024];
		qse_size_t buf_pos;
		qse_size_t buf_len;
		int        eof;

		qse_str_t line; /* entire line */
		qse_str_t linew; /* line for manipulation, if necessary */
		qse_str_t lineg; /* line buffer for getline */

		qse_awk_val_t* d0; /* $0 */

		qse_size_t maxflds;
		qse_size_t nflds; /* NF */
		struct
		{
			const qse_char_t* ptr;
			qse_size_t        len;
			qse_awk_val_t*    val; /* $1 .. $NF */
		}* flds;
	} inrec;

	qse_awk_nrflt_t nrflt;

	struct
	{
		void* rs[2];
		void* fs[2]; 
		int ignorecase;

		qse_awk_int_t nr;
		qse_awk_int_t fnr;

		qse_xstr_t convfmt;
		qse_xstr_t ofmt;
		qse_xstr_t ofs;
		qse_xstr_t ors;
		qse_xstr_t subsep;
	} gbl;

	/* rio chain */
	struct
	{
		qse_awk_rio_impl_t handler[QSE_AWK_RIO_NUM];
		qse_awk_rio_arg_t* chain;
	} rio;

	struct
	{
		qse_str_t fmt;
		qse_str_t out;

		struct
		{
			qse_char_t* ptr;
			qse_size_t len; /* length */
			qse_size_t inc; /* increment */
		} tmp;
	} format;

	struct
	{
		qse_size_t block;
		qse_size_t expr; /* expression */
	} depth;

	qse_awk_errinf_t errinf;

	qse_awk_t* awk;
	qse_awk_rtx_ecb_t* ecb;
};

typedef struct qse_awk_mod_data_t qse_awk_mod_data_t;
struct qse_awk_mod_data_t
{
	void* handle;
	qse_awk_mod_t mod;
};

#endif
