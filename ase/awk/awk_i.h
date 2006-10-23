/*
 * $Id: awk_i.h,v 1.71 2006-10-23 14:44:42 bacon Exp $
 */

#ifndef _SSE_AWK_AWKI_H_
#define _SSE_AWK_AWKI_H_

typedef struct sse_awk_chain_t sse_awk_chain_t;
typedef struct sse_awk_tree_t sse_awk_tree_t;

#include <sse/awk/awk.h>
#include <sse/awk/str.h>
#include <sse/awk/rex.h>
#include <sse/awk/map.h>
#include <sse/awk/tree.h>
#include <sse/awk/val.h>
#include <sse/awk/func.h>
#include <sse/awk/tab.h>
#include <sse/awk/parse.h>
#include <sse/awk/run.h>
#include <sse/awk/extio.h>
#include <sse/awk/misc.h>

#ifdef NDEBUG
	#define sse_awk_assert(awk,expr) ((void)0)
#else
	#define sse_awk_assert(awk,expr) (void)((expr) || \
		(sse_awk_abort(awk, SSE_T(#expr), SSE_T(__FILE__), __LINE__), 0))
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#define SSE_AWK_MAX_GLOBALS 9999
#define SSE_AWK_MAX_LOCALS  9999
#define SSE_AWK_MAX_PARAMS  9999

#if defined(_WIN32) && defined(_DEBUG)
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>

	#define SSE_AWK_MALLOC(awk,size) malloc (size)
	#define SSE_AWK_REALLOC(awk,ptr,size) realloc (ptr, size)
	#define SSE_AWK_FREE(awk,ptr) free (ptr)
#else
	#define SSE_AWK_MALLOC(awk,size) \
		(awk)->syscas.malloc (size, (awk)->syscas.custom_data)
	#define SSE_AWK_REALLOC(awk,ptr,size) \
		(awk)->syscas.realloc (ptr, size, (awk)->syscas.custom_data)
	#define SSE_AWK_FREE(awk,ptr) \
		(awk)->syscas.free (ptr, (awk)->syscas.custom_data)
#endif

#define SSE_AWK_LOCK(awk) \
	do { \
		if ((awk)->syscas.lock != SSE_NULL) \
			(awk)->syscas.lock (awk, (awk)->syscas.custom_data); \
	} while (0) 

#define SSE_AWK_UNLOCK(awk) \
	do { \
		if ((awk)->syscas.unlock != SSE_NULL) \
			(awk)->syscas.unlock (awk, (awk)->syscas.custom_data); \
	} while (0) 

#define SSE_AWK_ISUPPER(awk,c)  (awk)->syscas.is_upper(c)
#define SSE_AWK_ISLOWER(awk,c)  (awk)->syscas.is_lower(c)
#define SSE_AWK_ISALPHA(awk,c)  (awk)->syscas.is_alpha(c)
#define SSE_AWK_ISDIGIT(awk,c)  (awk)->syscas.is_digit(c)
#define SSE_AWK_ISXDIGIT(awk,c) (awk)->syscas.is_xdigit(c)
#define SSE_AWK_ISALNUM(awk,c)  (awk)->syscas.is_alnum(c)
#define SSE_AWK_ISSPACE(awk,c)  (awk)->syscas.is_space(c)
#define SSE_AWK_ISPRINT(awk,c)  (awk)->syscas.is_print(c)
#define SSE_AWK_ISGRAPH(awk,c)  (awk)->syscas.is_graph(c)
#define SSE_AWK_ISCNTRL(awk,c)  (awk)->syscas.is_cntrl(c)
#define SSE_AWK_ISPUNCT(awk,c)  (awk)->syscas.is_punct(c)
#define SSE_AWK_TOUPPER(awk,c)  (awk)->syscas.to_upper(c)
#define SSE_AWK_TOLOWER(awk,c)  (awk)->syscas.to_lower(c)

#define SSE_AWK_MEMCPY(awk,dst,src,len) (awk)->syscas.memcpy (dst, src, len)
#define SSE_AWK_MEMSET(awk,dst,val,len) (awk)->syscas.memset (dst, val, len)

struct sse_awk_tree_t
{
	sse_size_t nglobals; /* total number of globals */
	sse_size_t nbglobals; /* number of builtin globals */
	sse_awk_map_t afns; /* awk function map */
	sse_awk_nde_t* begin;
	sse_awk_nde_t* end;
	sse_awk_chain_t* chain;
	sse_awk_chain_t* chain_tail;
	sse_size_t chain_size; /* number of nodes in the chain */
};

struct sse_awk_t
{
	sse_awk_syscas_t syscas;

	/* options */
	int option;

	/* parse tree */
	sse_awk_tree_t tree;
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
			sse_size_t loop;
		} depth;

		sse_awk_tab_t globals;
		sse_awk_tab_t locals;
		sse_awk_tab_t params;
		sse_size_t nlocals_max;

		int nl_semicolon;
	} parse;

	/* source code management */
	struct
	{
		sse_awk_srcios_t* ios;

		struct
		{
			sse_cint_t curc;
			sse_cint_t ungotc[5];
			sse_size_t ungotc_count;

			sse_size_t line;
			sse_size_t column;
		} lex;

		struct
		{
			sse_char_t buf[512];
			sse_size_t buf_pos;
			sse_size_t buf_len;
		} shared;	
	} src;

	/* token */
	struct 
	{
		int          prev;
		int          type;
		sse_awk_str_t name;
		sse_size_t    line;
		sse_size_t    column;
	} token;

	/* builtin functions */
	struct
	{
		sse_awk_bfn_t* sys;
		sse_awk_bfn_t* user;
	} bfn;

	struct
	{
		sse_size_t count;
		sse_awk_run_t* ptr;
	} run;

	/* housekeeping */
	int errnum;
};

struct sse_awk_chain_t
{
	sse_awk_nde_t* pattern;
	sse_awk_nde_t* action;
	sse_awk_chain_t* next;	
};

struct sse_awk_run_t
{
	int id;
	sse_awk_map_t named;

	void** stack;
	sse_size_t stack_top;
	sse_size_t stack_base;
	sse_size_t stack_limit;
	int exit_level;

	sse_awk_val_int_t* icache[100]; /* TODO: choose the optimal size  */
	sse_awk_val_real_t* rcache[100]; /* TODO: choose the optimal size  */
	sse_awk_val_ref_t* fcache[100]; /* TODO: choose the optimal size */
	sse_size_t icache_count;
	sse_size_t rcache_count;
	sse_size_t fcache_count;

	sse_awk_nde_blk_t* active_block;
	sse_byte_t* pattern_range_state;

	struct
	{
		sse_char_t buf[1024];
		sse_size_t buf_pos;
		sse_size_t buf_len;
		sse_bool_t eof;

		sse_awk_str_t line;
		sse_awk_val_t* d0; /* $0 */

		sse_size_t maxflds;
		sse_size_t nflds; /* NF */
		struct
		{
			sse_char_t*    ptr;
			sse_size_t     len;
			sse_awk_val_t* val; /* $1 .. $NF */
		}* flds;

	} inrec;

	struct
	{
		void* rs;
		void* fs;
		int ignorecase;
		sse_size_t fnr;

		struct 
		{
			sse_char_t* ptr;
			sse_size_t len;
		} convfmt;
		struct
		{
			sse_char_t* ptr;
			sse_size_t len;
		} ofmt;
		struct
		{
			sse_char_t* ptr;
			sse_size_t len;
		} ofs;
		struct
		{
			sse_char_t* ptr;
			sse_size_t len;
		} ors;
		struct
		{
			sse_char_t* ptr;
			sse_size_t len;
		} subsep;
	} global;

	/* extio chain */
	struct
	{
		sse_awk_io_t handler[SSE_AWK_EXTIO_NUM];
		void* custom_data;
		sse_awk_extio_t* chain;
	} extio;

	int errnum;

	sse_awk_t* awk;
	sse_awk_run_t* prev;
	sse_awk_run_t* next;
};

#endif
