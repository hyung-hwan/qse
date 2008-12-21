/*
 * $Id: awk_i.h 332 2008-08-18 11:21:48Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LIB_AWK_AWK_H_
#define _QSE_LIB_AWK_AWK_H_

#include "../cmn/mem.h"
#include "../cmn/chr.h"
#include <qse/cmn/str.h>
#include <qse/cmn/map.h>
#include <qse/cmn/lda.h>
#include <qse/cmn/rex.h>

typedef struct qse_awk_chain_t qse_awk_chain_t;
typedef struct qse_awk_tree_t qse_awk_tree_t;

#include <qse/awk/awk.h>
#include "tree.h"
#include "func.h"
#include "parse.h"
#include "run.h"
#include "extio.h"
#include "misc.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#pragma warning (disable: 4296)
#endif

#define QSE_AWK_MAX_GLOBALS 9999
#define QSE_AWK_MAX_LOCALS  9999
#define QSE_AWK_MAX_PARAMS  9999

#define QSE_AWK_ALLOC(awk,size)       QSE_MMGR_ALLOC((awk)->mmgr,size)
#define QSE_AWK_REALLOC(awk,ptr,size) QSE_MMGR_REALLOC((awk)->mmgr,ptr,size)
#define QSE_AWK_FREE(awk,ptr)         QSE_MMGR_FREE((awk)->mmgr,ptr)

#define QSE_AWK_ISUPPER(awk,c)  QSE_CCLS_ISUPPER((awk)->ccls,c)
#define QSE_AWK_ISLOWER(awk,c)  QSE_CCLS_ISLOWER((awk)->ccls,c)
#define QSE_AWK_ISALPHA(awk,c)  QSE_CCLS_ISALPHA((awk)->ccls,c)
#define QSE_AWK_ISDIGIT(awk,c)  QSE_CCLS_ISDIGIT((awk)->ccls,c)
#define QSE_AWK_ISXDIGIT(awk,c) QSE_CCLS_ISXDIGIT((awk)->ccls,c)
#define QSE_AWK_ISALNUM(awk,c)  QSE_CCLS_ISALNUM((awk)->ccls,c)
#define QSE_AWK_ISSPACE(awk,c)  QSE_CCLS_ISSPACE((awk)->ccls,c)
#define QSE_AWK_ISPRINT(awk,c)  QSE_CCLS_ISPRINT((awk)->ccls,c)
#define QSE_AWK_ISGRAPH(awk,c)  QSE_CCLS_ISGRAPH((awk)->ccls,c)
#define QSE_AWK_ISCNTRL(awk,c)  QSE_CCLS_ISCNTRL((awk)->ccls,c)
#define QSE_AWK_ISPUNCT(awk,c)  QSE_CCLS_ISPUNCT((awk)->ccls,c)
#define QSE_AWK_TOUPPER(awk,c)  QSE_CCLS_TOUPPER((awk)->ccls,c)
#define QSE_AWK_TOLOWER(awk,c)  QSE_CCLS_TOLOWER((awk)->ccls,c)

#define QSE_AWK_STRDUP(awk,str) (qse_strdup(str,(awk)->mmgr))
#define QSE_AWK_STRXDUP(awk,str,len) (qse_strxdup(str,len,(awk)->mmgr))

struct qse_awk_tree_t
{
	qse_size_t nglobals; /* total number of globals */
	qse_size_t nbglobals; /* number of intrinsic globals */
	qse_cstr_t cur_afn;
	qse_map_t* afns; /* awk function map */

	qse_awk_nde_t* begin;
	qse_awk_nde_t* begin_tail;

	qse_awk_nde_t* end;
	qse_awk_nde_t* end_tail;

	qse_awk_chain_t* chain;
	qse_awk_chain_t* chain_tail;
	qse_size_t chain_size; /* number of nodes in the chain */

	int ok;
};

struct qse_awk_t
{
	qse_mmgr_t* mmgr;
	qse_ccls_t* ccls;
	qse_awk_prmfns_t* prmfns;

	void* assoc_data;

	/* options */
	int option;

	/* word table */
	qse_map_t* wtab;
	/* reverse word table */
	qse_map_t* rwtab;

	/* regular expression processing routines */
	qse_awk_rexfns_t* rexfns;

	/* parse tree */
	qse_awk_tree_t tree;

	/* temporary information that the parser needs */
	struct
	{
		struct
		{
			int block;
			int loop;
			int stmnt; /* statement */
		} id;

		struct
		{
			struct
			{
				qse_size_t block;
				qse_size_t loop;
				qse_size_t expr; /* expression */
			} cur;

			struct
			{
				qse_size_t block;
				qse_size_t expr;
			} max;
		} depth;

		/* function calls */
		qse_map_t* afns;

		/* named variables */
		qse_map_t* named;

		/* global variables */
		qse_lda_t* globals;

		/* local variables */
		qse_lda_t* locals;

		/* parameters to a function */
		qse_lda_t* params;

		/* maximum number of local variables */
		qse_size_t nlocals_max;

		qse_awk_nde_t* (*parse_block) (
			qse_awk_t*,qse_size_t,qse_bool_t);

	} parse;

	/* source code management */
	struct
	{
		qse_awk_srcios_t ios;

		struct
		{
			qse_cint_t curc;
			qse_cint_t ungotc[5];
			qse_size_t ungotc_line[5];
			qse_size_t ungotc_column[5];
			qse_size_t ungotc_count;

			qse_size_t line;
			qse_size_t column;
		} lex;

		struct
		{
			qse_char_t buf[512];
			qse_size_t buf_pos;
			qse_size_t buf_len;
		} shared;	
	} src;

	/* token */
	struct 
	{
		struct
		{
			int type;
			qse_size_t line;
			qse_size_t column;
		} prev;

		int        type;
		qse_str_t* name;
		qse_size_t line;
		qse_size_t column;
	} token;

	/* intrinsic functions */
	struct
	{
		qse_awk_bfn_t* sys;
		qse_map_t* user;
	} bfn;

	struct
	{
		struct
		{
			struct
			{
				qse_size_t block;
				qse_size_t expr;
			} max;
		} depth;
	} run;

	struct
	{
		struct
		{
			struct
			{
				qse_size_t build;
				qse_size_t match;
			} max;
		} depth;
	} rex;

	struct
	{
		qse_char_t fmt[1024];
	} tmp;

	/* housekeeping */
	int errnum;
	qse_size_t errlin;
	qse_char_t errmsg[256];
	qse_char_t* errstr[QSE_AWK_NUMERRNUM];

	qse_bool_t stopall;
};

struct qse_awk_chain_t
{
	qse_awk_nde_t* pattern;
	qse_awk_nde_t* action;
	qse_awk_chain_t* next;	
};

struct qse_awk_run_t
{
	int id;
	qse_map_t* named;

	void** stack;
	qse_size_t stack_top;
	qse_size_t stack_base;
	qse_size_t stack_limit;
	int exit_level;

	qse_awk_val_ref_t* fcache[128];
	/*qse_awk_val_str_t* scache32[128];
	qse_awk_val_str_t* scache64[128];*/
	qse_size_t fcache_count;
	/*qse_size_t scache32_count;
	qse_size_t scache64_count;*/

	struct
	{
		qse_awk_val_int_t* ifree;
		qse_awk_val_chunk_t* ichunk;
		qse_awk_val_real_t* rfree;
		qse_awk_val_chunk_t* rchunk;
	} vmgr;

	qse_awk_nde_blk_t* active_block;
	qse_byte_t* pattern_range_state;

	struct
	{
		qse_char_t buf[1024];
		qse_size_t buf_pos;
		qse_size_t buf_len;
		qse_bool_t eof;

		qse_str_t line;
		qse_awk_val_t* d0; /* $0 */

		qse_size_t maxflds;
		qse_size_t nflds; /* NF */
		struct
		{
			qse_char_t*    ptr;
			qse_size_t     len;
			qse_awk_val_t* val; /* $1 .. $NF */
		}* flds;

	} inrec;

	struct
	{
		void* rs;
		void* fs;
		int ignorecase;

		qse_long_t nr;
		qse_long_t fnr;

		struct 
		{
			qse_char_t* ptr;
			qse_size_t len;
		} convfmt;
		struct
		{
			qse_char_t* ptr;
			qse_size_t len;
		} ofmt;
		struct
		{
			qse_char_t* ptr;
			qse_size_t len;
		} ofs;
		struct
		{
			qse_char_t* ptr;
			qse_size_t len;
		} ors;
		struct
		{
			qse_char_t* ptr;
			qse_size_t len;
		} subsep;
	} global;

	/* extio chain */
	struct
	{
		qse_awk_io_t handler[QSE_AWK_EXTIO_NUM];
		void* data;
		qse_awk_extio_t* chain;
	} extio;

	struct
	{
		qse_str_t fmt;
		qse_str_t out;

		struct
		{
			qse_char_t* ptr;
			qse_size_t len;	/* length */
			qse_size_t inc; /* increment */
		} tmp;
	} format;

	struct
	{
		struct
		{
			qse_size_t block;
			qse_size_t expr; /* expression */
		} cur;

		struct
		{
			qse_size_t block;
			qse_size_t expr;
		} max;
	} depth;

	int errnum;
	qse_size_t errlin;
	qse_char_t errmsg[256];

	void* data;

	qse_awk_t* awk;
	qse_awk_runcbs_t* cbs;
};


#define QSE_AWK_FREEREX(awk,code) qse_freerex((awk)->mmgr,code)
#define QSE_AWK_ISEMPTYREX(awk,code) qse_isemptyrex(code)
#define QSE_AWK_BUILDREX(awk,ptn,len,errnum) \
	qse_awk_buildrex(awk,ptn,len,errnum)
#define QSE_AWK_MATCHREX(awk,code,option,str,len,match_ptr,match_len,errnum) \
	qse_awk_matchrex(awk,code,option,str,len,match_ptr,match_len,errnum)

#endif
