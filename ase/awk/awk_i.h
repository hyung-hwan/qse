/*
 * $Id: awk_i.h,v 1.93 2006-12-17 14:56:06 bacon Exp $
 */

#ifndef _ASE_AWK_AWKI_H_
#define _ASE_AWK_AWKI_H_

typedef struct ase_awk_chain_t ase_awk_chain_t;
typedef struct ase_awk_tree_t ase_awk_tree_t;

#include <ase/awk/awk.h>
#include <ase/awk/str.h>
#include <ase/awk/rex.h>
#include <ase/awk/map.h>
#include <ase/awk/tree.h>
#include <ase/awk/val.h>
#include <ase/awk/func.h>
#include <ase/awk/tab.h>
#include <ase/awk/parse.h>
#include <ase/awk/run.h>
#include <ase/awk/extio.h>
#include <ase/awk/misc.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#pragma warning (disable: 4296)
#endif

/* TODO: remove this */
#ifdef _WIN32
#include <tchar.h>
#define xp_printf _tprintf
#endif

#define ASE_AWK_MAX_GLOBALS 9999
#define ASE_AWK_MAX_LOCALS  9999
#define ASE_AWK_MAX_PARAMS  9999

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>

	#define ASE_AWK_MALLOC(awk,size) malloc (size)
	#define ASE_AWK_REALLOC(awk,ptr,size) realloc (ptr, size)
	#define ASE_AWK_FREE(awk,ptr) free (ptr)
#else
	#define ASE_AWK_MALLOC(awk,size) \
		(awk)->sysfns.malloc (size, (awk)->sysfns.custom_data)
	#define ASE_AWK_REALLOC(awk,ptr,size) \
		(awk)->sysfns.realloc (ptr, size, (awk)->sysfns.custom_data)
	#define ASE_AWK_FREE(awk,ptr) \
		(awk)->sysfns.free (ptr, (awk)->sysfns.custom_data)
#endif

#define ASE_AWK_LOCK(awk) \
	do { \
		if ((awk)->sysfns.lock != ASE_NULL) \
			(awk)->sysfns.lock ((awk)->sysfns.custom_data); \
	} while (0) 

#define ASE_AWK_UNLOCK(awk) \
	do { \
		if ((awk)->sysfns.unlock != ASE_NULL) \
			(awk)->sysfns.unlock ((awk)->sysfns.custom_data); \
	} while (0) 

#define ASE_AWK_ISUPPER(awk,c)  (awk)->sysfns.is_upper(c)
#define ASE_AWK_ISLOWER(awk,c)  (awk)->sysfns.is_lower(c)
#define ASE_AWK_ISALPHA(awk,c)  (awk)->sysfns.is_alpha(c)
#define ASE_AWK_ISDIGIT(awk,c)  (awk)->sysfns.is_digit(c)
#define ASE_AWK_ISXDIGIT(awk,c) (awk)->sysfns.is_xdigit(c)
#define ASE_AWK_ISALNUM(awk,c)  (awk)->sysfns.is_alnum(c)
#define ASE_AWK_ISSPACE(awk,c)  (awk)->sysfns.is_space(c)
#define ASE_AWK_ISPRINT(awk,c)  (awk)->sysfns.is_print(c)
#define ASE_AWK_ISGRAPH(awk,c)  (awk)->sysfns.is_graph(c)
#define ASE_AWK_ISCNTRL(awk,c)  (awk)->sysfns.is_cntrl(c)
#define ASE_AWK_ISPUNCT(awk,c)  (awk)->sysfns.is_punct(c)
#define ASE_AWK_TOUPPER(awk,c)  (awk)->sysfns.to_upper(c)
#define ASE_AWK_TOLOWER(awk,c)  (awk)->sysfns.to_lower(c)

#define ASE_AWK_MEMCPY(awk,dst,src,len) (awk)->sysfns.memcpy (dst, src, len)
#define ASE_AWK_MEMSET(awk,dst,val,len) (awk)->sysfns.memset (dst, val, len)

struct ase_awk_tree_t
{
	ase_size_t nglobals; /* total number of globals */
	ase_size_t nbglobals; /* number of builtin globals */
	ase_awk_map_t afns; /* awk function map */
	ase_awk_nde_t* begin;
	ase_awk_nde_t* end;
	ase_awk_chain_t* chain;
	ase_awk_chain_t* chain_tail;
	ase_size_t chain_size; /* number of nodes in the chain */
	int ok;
};

struct ase_awk_t
{
	ase_awk_sysfns_t sysfns;

	/* options */
	int option;

	/* parse tree */
	ase_awk_tree_t tree;

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
				ase_size_t block;
				ase_size_t loop;
				ase_size_t expr; /* expression */
			} cur;

			struct
			{
				ase_size_t block;
				ase_size_t expr;
			} max;
		} depth;

		ase_awk_tab_t globals;
		ase_awk_tab_t locals;
		ase_awk_tab_t params;
		ase_size_t nlocals_max;

		ase_awk_nde_t* (*parse_block) (ase_awk_t*,ase_bool_t);
	} parse;

	/* source code management */
	struct
	{
		ase_awk_srcios_t ios;

		struct
		{
			ase_cint_t curc;
			ase_cint_t ungotc[5];
			ase_size_t ungotc_count;

			ase_size_t line;
			ase_size_t column;
		} lex;

		struct
		{
			ase_char_t buf[512];
			ase_size_t buf_pos;
			ase_size_t buf_len;
		} shared;	
	} src;

	/* token */
	struct 
	{
		struct
		{
			int type;
			ase_size_t line;
			ase_size_t column;
		} prev;

		int           type;
		ase_awk_str_t name;
		ase_size_t    line;
		ase_size_t    column;
	} token;

	/* builtin functions */
	struct
	{
		ase_awk_bfn_t* sys;
		ase_awk_bfn_t* user;
	} bfn;

	struct
	{
		ase_size_t count;
		ase_awk_run_t* ptr;

		struct
		{
			struct
			{
				ase_size_t block;
				ase_size_t expr; 
			} cur;

			struct
			{
				ase_size_t block;
				ase_size_t expr;
			} max;
		} depth;
	} run;

	/* housekeeping */
	int errnum;
};

struct ase_awk_chain_t
{
	ase_awk_nde_t* pattern;
	ase_awk_nde_t* action;
	ase_awk_chain_t* next;	
};

struct ase_awk_run_t
{
	int id;
	ase_awk_map_t named;

	void** stack;
	ase_size_t stack_top;
	ase_size_t stack_base;
	ase_size_t stack_limit;
	int exit_level;

	ase_awk_val_int_t* icache[200]; /* TODO: choose the optimal size  */
	ase_awk_val_real_t* rcache[200]; /* TODO: choose the optimal size  */
	ase_awk_val_ref_t* fcache[200]; /* TODO: choose the optimal size */
	ase_size_t icache_count;
	ase_size_t rcache_count;
	ase_size_t fcache_count;

	ase_awk_nde_blk_t* active_block;
	ase_byte_t* pattern_range_state;

	struct
	{
		ase_char_t buf[1024];
		ase_size_t buf_pos;
		ase_size_t buf_len;
		ase_bool_t eof;

		ase_awk_str_t line;
		ase_awk_val_t* d0; /* $0 */

		ase_size_t maxflds;
		ase_size_t nflds; /* NF */
		struct
		{
			ase_char_t*    ptr;
			ase_size_t     len;
			ase_awk_val_t* val; /* $1 .. $NF */
		}* flds;

	} inrec;

	struct
	{
		void* rs;
		void* fs;
		int ignorecase;
		ase_size_t fnr;

		struct 
		{
			ase_char_t* ptr;
			ase_size_t len;
		} convfmt;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} ofmt;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} ofs;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} ors;
		struct
		{
			ase_char_t* ptr;
			ase_size_t len;
		} subsep;
	} global;

	/* extio chain */
	struct
	{
		ase_awk_io_t handler[ASE_AWK_EXTIO_NUM];
		void* custom_data;
		ase_awk_extio_t* chain;
	} extio;

	struct
	{
		ase_awk_str_t fmt;
		ase_awk_str_t out;

		struct
		{
			ase_char_t* ptr;
			ase_size_t len;	
			ase_size_t inc;
		} tmp;
	} format;

	struct
	{
		struct
		{
			ase_size_t block;
			ase_size_t expr; /* expression */
		} cur;

		struct
		{
			ase_size_t block;
			ase_size_t expr;
		} max;
	} depth;

	int errnum;
	ase_size_t errlin;
	ase_char_t errmsg[256];

	void* custom_data;
	ase_awk_t* awk;

	ase_awk_run_t* prev;
	ase_awk_run_t* next;
};

#endif
