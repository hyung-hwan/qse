/*
 * $Id: awk_i.h,v 1.62 2006-10-03 14:38:26 bacon Exp $
 */

#ifndef _XP_AWK_AWKI_H_
#define _XP_AWK_AWKI_H_

typedef struct xp_awk_chain_t xp_awk_chain_t;
typedef struct xp_awk_tree_t xp_awk_tree_t;

#include <xp/awk/awk.h>

#ifdef XP_AWK_STAND_ALONE
	#if !defined(XP_CHAR_IS_MCHAR) && !defined(XP_CHAR_IS_WCHAR)
	#error Neither XP_CHAR_IS_MCHAR nor XP_CHAR_IS_WCHAR is defined.
	#endif

	#include <assert.h>
	#define xp_assert assert
#else
	#include <xp/bas/assert.h>
#endif

#include <xp/awk/str.h>
#include <xp/awk/rex.h>
#include <xp/awk/map.h>
#include <xp/awk/tree.h>
#include <xp/awk/val.h>
#include <xp/awk/func.h>
#include <xp/awk/tab.h>
#include <xp/awk/parse.h>
#include <xp/awk/run.h>
#include <xp/awk/extio.h>
#include <xp/awk/misc.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#if defined(_WIN32) && defined(XP_AWK_STAND_ALONE) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define XP_AWK_MAX_GLOBALS 9999
#define XP_AWK_MAX_LOCALS  9999
#define XP_AWK_MAX_PARAMS  9999

#if defined(_WIN32) && defined(_DEBUG)
	#define XP_AWK_MALLOC(awk,size) malloc (size)
	#define XP_AWK_REALLOC(awk,ptr,size) realloc (ptr, size)
	#define XP_AWK_FREE(awk,ptr) free (ptr)
#else
	#define XP_AWK_MALLOC(awk,size) \
		(awk)->syscas->malloc (size, (awk)->syscas->custom_data)
	#define XP_AWK_REALLOC(awk,ptr,size) \
		(awk)->syscas->realloc (ptr, size, (awk)->syscas->custom_data)
	#define XP_AWK_FREE(awk,ptr) \
		(awk)->syscas->free (ptr, (awk)->syscas->custom_data)
#endif

#define XP_AWK_LOCK(awk) \
	do { \
		if ((awk)->syscas != XP_NULL && (awk)->syscas->lock != XP_NULL) \
			(awk)->syscas->lock (awk, (awk)->syscas->custom_data); \
	} while (0) 

#define XP_AWK_UNLOCK(awk) \
	do { \
		if ((awk)->syscas != XP_NULL && (awk)->syscas->unlock != XP_NULL) \
			(awk)->syscas->unlock (awk, (awk)->syscas->custom_data); \
	} while (0) 

#define XP_AWK_ISUPPER(awk,c)  (awk)->syscas->is_upper(c)
#define XP_AWK_ISLOWER(awk,c)  (awk)->syscas->is_lower(c)
#define XP_AWK_ISALPHA(awk,c)  (awk)->syscas->is_alpha(c)
#define XP_AWK_ISDIGIT(awk,c)  (awk)->syscas->is_digit(c)
#define XP_AWK_ISXDIGIT(awk,c) (awk)->syscas->is_xdigit(c)
#define XP_AWK_ISALNUM(awk,c)  (awk)->syscas->is_alnum(c)
#define XP_AWK_ISSPACE(awk,c)  (awk)->syscas->is_space(c)
#define XP_AWK_ISPRINT(awk,c)  (awk)->syscas->is_print(c)
#define XP_AWK_ISGRAPH(awk,c)  (awk)->syscas->is_graph(c)
#define XP_AWK_ISCNTRL(awk,c)  (awk)->syscas->is_cntrl(c)
#define XP_AWK_ISPUNCT(awk,c)  (awk)->syscas->is_punct(c)
#define XP_AWK_TOUPPER(awk,c)  (awk)->syscas->to_upper(c)
#define XP_AWK_TOLOWER(awk,c)  (awk)->syscas->to_lower(c)

#define XP_AWK_MEMCPY(awk,dst,src,len) \
	do { \
		if ((awk)->syscas->memcpy == XP_NULL) \
			xp_awk_memcpy (dst, src, len); \
		else (awk)->syscas->memcpy (dst, src, len); \
	} while (0)

#define XP_AWK_MEMSET(awk,dst,val,len) \
	do { \
		if ((awk)->syscas->memset == XP_NULL) \
			xp_awk_memset (dst, val, len); \
		else (awk)->syscas->memset (dst, val, len); \
	} while (0)

struct xp_awk_tree_t
{
	xp_size_t nglobals; /* total number of globals */
	xp_size_t nbglobals; /* number of builtin globals */
	xp_awk_map_t afns; /* awk function map */
	xp_awk_nde_t* begin;
	xp_awk_nde_t* end;
	xp_awk_chain_t* chain;
	xp_awk_chain_t* chain_tail;
	xp_size_t chain_size; /* number of nodes in the chain */
};

struct xp_awk_t
{
	xp_awk_syscas_t* syscas;

	/* options */
	int option;

	/* parse tree */
	xp_awk_tree_t tree;
	int state;

	/* temporary information that the parser needs */
	struct
	{
		struct
		{
			int block;
			int loop;
		} id;

		struct
		{
			xp_size_t loop;
		} depth;

		xp_awk_tab_t globals;
		xp_awk_tab_t locals;
		xp_awk_tab_t params;
		xp_size_t nlocals_max;
	} parse;

	/* source code management */
	struct
	{
		xp_awk_srcios_t* ios;

		struct
		{
			xp_cint_t curc;
			xp_cint_t ungotc[5];
			xp_size_t ungotc_count;

			xp_size_t line;
			xp_size_t column;
		} lex;

		struct
		{
			xp_char_t buf[512];
			xp_size_t buf_pos;
			xp_size_t buf_len;
		} shared;	
	} src;

	/* token */
	struct 
	{
		int          prev;
		int          type;
		xp_awk_str_t name;
		xp_size_t    line;
		xp_size_t    column;
	} token;

	/* builtin functions */
	struct
	{
		xp_awk_bfn_t* sys;
		xp_awk_bfn_t* user;
	} bfn;

	struct
	{
		xp_size_t count;
		xp_awk_run_t* ptr;
	} run;

	/* housekeeping */
	int errnum;
};

struct xp_awk_chain_t
{
	xp_awk_nde_t* pattern;
	xp_awk_nde_t* action;
	xp_awk_chain_t* next;	
};

struct xp_awk_run_t
{
	int id;
	xp_awk_map_t named;

	void** stack;
	xp_size_t stack_top;
	xp_size_t stack_base;
	xp_size_t stack_limit;
	int exit_level;

	xp_awk_val_int_t* icache[100]; /* TODO: choose the optimal size  */
	xp_awk_val_real_t* rcache[100]; /* TODO: choose the optimal size  */
	xp_awk_val_ref_t* fcache[100]; /* TODO: choose the optimal size */
	xp_size_t icache_count;
	xp_size_t rcache_count;
	xp_size_t fcache_count;

	xp_awk_nde_blk_t* active_block;
	xp_byte_t* pattern_range_state;

	struct
	{
		xp_char_t buf[1024];
		xp_size_t buf_pos;
		xp_size_t buf_len;
		xp_bool_t eof;

		xp_awk_str_t line;
		xp_awk_val_t* d0; /* $0 */

		xp_size_t maxflds;
		xp_size_t nflds; /* NF */
		struct
		{
			xp_char_t*    ptr;
			xp_size_t     len;
			xp_awk_val_t* val; /* $1 .. $NF */
		}* flds;

	} inrec;

	struct
	{
		void* rs;
		void* fs;
		int ignorecase;

		struct
		{
			xp_char_t* ptr;
			xp_size_t len;
		} ofmt;
		struct
		{
			xp_char_t* ptr;
			xp_size_t len;
		} ofs;
		struct
		{
			xp_char_t* ptr;
			xp_size_t len;
		} ors;
		struct
		{
			xp_char_t* ptr;
			xp_size_t len;
		} subsep;
	} global;

	/* extio chain */
	struct
	{
		xp_awk_io_t handler[XP_AWK_EXTIO_NUM];
		xp_awk_extio_t* chain;
	} extio;

	int errnum;

	xp_awk_t* awk;
	xp_awk_run_t* prev;
	xp_awk_run_t* next;
};

#endif
