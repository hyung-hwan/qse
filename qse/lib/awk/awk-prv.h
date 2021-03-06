/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _QSE_LIB_AWK_AWK_PRV_H_
#define _QSE_LIB_AWK_AWK_PRV_H_

#include "../cmn/mem-prv.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/arr.h>
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

/* [NOTE] the function value support implemented is very limited.
 *        it supports very primitive way to call a function via a variable.
 *        only user-defined functions are supported. neither builtin functions
 *        nor module functions are not supported yet.
 *   -----------------------------------------------------
 *   function x(a,b,c) { print a, b, c; }
 *   BEGIN { q = x; q(1, 2, 3); } # this works
 *   BEGIN { q[1]=x; q[1](1,2,3); } # this doesn't work. same as q[1] %% (1, 2, 3) or q[1] %% 3
 *   BEGIN { q[1]=x; y=q[1]; y(1,2,3); } # this works.
 *   -----------------------------------------------------
 *   function __printer(a,b,c) { print a, b, c; }
 *   function show(printer, a,b,c) { printer(a, b, c); }  
 *   BEGIN { show(__printer, 10, 20, 30); } ## passing the function value as an argumnet is ok.
 */
#define ENABLE_FEATURE_FUN_AS_VALUE

#define QSE_AWK_MAX_GBLS 9999
#define QSE_AWK_MAX_LCLS  9999
#define QSE_AWK_MAX_PARAMS  9999

#define QSE_AWK_DFL_RTX_STACK_LIMIT 5120
#define QSE_AWK_MIN_RTX_STACK_LIMIT 512
#if (QSE_SIZEOF_VOID_P <= 4)
#	define QSE_AWK_MAX_RTX_STACK_LIMIT ((qse_size_t)1 << (QSE_SIZEOF_VOID_P * 4 + 1))
#else
#	define QSE_AWK_MAX_RTX_STACK_LIMIT ((qse_size_t)1 << (QSE_SIZEOF_VOID_P * 4))
#endif

#define QSE_AWK_ALLOC(awk,size)       QSE_MMGR_ALLOC(qse_awk_getmmgr(awk),size)
#define QSE_AWK_REALLOC(awk,ptr,size) QSE_MMGR_REALLOC(qse_awk_getmmgr(awk),ptr,size)
#define QSE_AWK_FREE(awk,ptr)         QSE_MMGR_FREE(qse_awk_getmmgr(awk),ptr)

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

#define QSE_AWK_ISMUPPER(awk,c)  QSE_ISMUPPER(c)
#define QSE_AWK_ISMLOWER(awk,c)  QSE_ISMLOWER(c)
#define QSE_AWK_ISMALPHA(awk,c)  QSE_ISMALPHA(c)
#define QSE_AWK_ISMDIGIT(awk,c)  QSE_ISMDIGIT(c)
#define QSE_AWK_ISMXDIGIT(awk,c) QSE_ISMXDIGIT(c)
#define QSE_AWK_ISMALNUM(awk,c)  QSE_ISMALNUM(c)
#define QSE_AWK_ISMSPACE(awk,c)  QSE_ISMSPACE(c)
#define QSE_AWK_ISMPRINT(awk,c)  QSE_ISMPRINT(c)
#define QSE_AWK_ISMGRAPH(awk,c)  QSE_ISMGRAPH(c)
#define QSE_AWK_ISMCNTRL(awk,c)  QSE_ISMCNTRL(c)
#define QSE_AWK_ISMPUNCT(awk,c)  QSE_ISMPUNCT(c)
#define QSE_AWK_TOMUPPER(awk,c)  QSE_TOMUPPER(c)
#define QSE_AWK_TOMLOWER(awk,c)  QSE_TOMLOWER(c)

#define QSE_AWK_ISWUPPER(awk,c)  QSE_ISWUPPER(c)
#define QSE_AWK_ISWLOWER(awk,c)  QSE_ISWLOWER(c)
#define QSE_AWK_ISWALPHA(awk,c)  QSE_ISWALPHA(c)
#define QSE_AWK_ISWDIGIT(awk,c)  QSE_ISWDIGIT(c)
#define QSE_AWK_ISWXDIGIT(awk,c) QSE_ISWXDIGIT(c)
#define QSE_AWK_ISWALNUM(awk,c)  QSE_ISWALNUM(c)
#define QSE_AWK_ISWSPACE(awk,c)  QSE_ISWSPACE(c)
#define QSE_AWK_ISWPRINT(awk,c)  QSE_ISWPRINT(c)
#define QSE_AWK_ISWGRAPH(awk,c)  QSE_ISWGRAPH(c)
#define QSE_AWK_ISWCNTRL(awk,c)  QSE_ISWCNTRL(c)
#define QSE_AWK_ISWPUNCT(awk,c)  QSE_ISWPUNCT(c)
#define QSE_AWK_TOWUPPER(awk,c)  QSE_TOWUPPER(c)
#define QSE_AWK_TOWLOWER(awk,c)  QSE_TOWLOWER(c)

#define QSE_AWK_STRDUP(awk,str) (qse_strdup(str,qse_awk_getmmgr(awk)))
#define QSE_AWK_STRXDUP(awk,str,len) (qse_strxdup(str,len,qse_awk_getmmgr(awk)))

#define QSE_AWK_BYTE_PRINTABLE(x) ((x) <= 0x7F && (x) != '\\' && QSE_ISMPRINT(x))

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
	/* exposed fields via qse_awk_alt_t */
	QSE_AWK_HDR;

	/* primitive functions */
	qse_awk_prm_t  prm;

	/* options */
	struct
	{
		int trait;
		qse_cstr_t mod[2];
		qse_cstr_t incldirs;

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

		qse_size_t rtx_stack_limit;
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

		/* current pragma values */
		struct
		{
			int trait;
			qse_size_t rtx_stack_limit;
		} pragma;

		/* function calls */
		qse_htb_t* funs;

		/* named variables */
		qse_htb_t* named;

		/* global variables */
		qse_arr_t* gbls;

		/* local variables */
		qse_arr_t* lcls;

		/* parameters to a function */
		qse_arr_t* params;

		/* maximum number of local variables */
		qse_size_t nlcls_max;

		/* some data to find if an expression is
		 * enclosed in parentheses or not.
		 * see parse_primary_lparen() and parse_print() in parse.c
		 */
		qse_size_t lparen_seq;
		qse_size_t lparen_last_closed;

		struct
		{
			qse_uint8_t* ptr;
			qse_size_t count;
			qse_size_t capa;
		} incl_hist;
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
	qse_char_t errmsg_backup[QSE_AWK_ERRINF_MSG_SIZE];
#if defined(QSE_CHAR_IS_MCHAR)
	qse_wchar_t werrmsg[QSE_AWK_ERRINF_MSG_SIZE];
#else
	qse_mchar_t merrmsg[QSE_AWK_ERRINF_MSG_SIZE * 2];
#endif

	int haltall;
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
#define RTX_STACK_NARGS(rtx) RTX_STACK_AT(rtx,3)
#define RTX_STACK_ARG(rtx,n) RTX_STACK_AT(rtx,3+1+(n))
#define RTX_STACK_LCL(rtx,n) RTX_STACK_AT(rtx,3+(qse_size_t)RTX_STACK_NARGS(rtx)+1+(n))
#define RTX_STACK_RETVAL(rtx) RTX_STACK_AT(rtx,2)
#define RTX_STACK_GBL(rtx,n) ((rtx)->stack[(n)])
#define RTX_STACK_RETVAL_GBL(rtx) ((rtx)->stack[(rtx)->awk->tree.ngbls+2])

struct qse_awk_rtx_t
{
	QSE_AWK_RTX_HDR;

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
		int striprecspc;

		qse_awk_int_t nr;
		qse_awk_int_t fnr;

		qse_cstr_t convfmt;
		qse_cstr_t ofmt;
		qse_cstr_t ofs;
		qse_cstr_t ors;
		qse_cstr_t subsep;
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
		qse_mbs_t fmt;
		qse_mbs_t out;

		struct
		{
			qse_mchar_t* ptr;
			qse_size_t len; /* length */
			qse_size_t inc; /* increment */
		} tmp;
	} formatmbs;

	struct
	{
		qse_size_t block;
		qse_size_t expr; /* expression */
	} depth;

	qse_awk_errinf_t errinf;
	qse_char_t errmsg_backup[QSE_AWK_ERRINF_MSG_SIZE];
#if defined(QSE_CHAR_IS_MCHAR)
	qse_wchar_t werrmsg[QSE_AWK_ERRINF_MSG_SIZE];
#else
	qse_mchar_t merrmsg[QSE_AWK_ERRINF_MSG_SIZE * 2];
#endif

	qse_awk_rtx_ecb_t* ecb;
};

typedef struct qse_awk_mod_data_t qse_awk_mod_data_t;
struct qse_awk_mod_data_t
{
	void* handle;
	qse_awk_mod_t mod;
};


#define QSE_AWK_RTX_INIT_REF_VAL(refval, _id, _adr, _nrefs) \
	do { \
		(refval)->v_type = QSE_AWK_VAL_REF; \
		(refval)->ref = (_nrefs); \
		(refval)->stat = 0; \
		(refval)->nstr = 0; \
		(refval)->fcb = 0; \
		(refval)->id = (_id); \
		(refval)->adr = (_adr); \
	} while(0);

#if defined(__cplusplus)
extern "C" {
#endif

int qse_awk_init (qse_awk_t* awk, qse_mmgr_t* mmgr, const qse_awk_prm_t* prm);
void qse_awk_fini (qse_awk_t* awk);

#if defined(__cplusplus)
}
#endif

#endif
