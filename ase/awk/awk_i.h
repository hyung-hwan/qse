/*
 * $Id: awk_i.h,v 1.78 2006-11-13 15:08:53 bacon Exp $
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
#endif

/* TODO: remove this */
#ifdef _WIN32
#include <tchar.h>
#define xp_printf _tprintf
#endif

#define ASE_AWK_MAX_GLOBALS 9999
#define ASE_AWK_MAX_LOCALS  9999
#define ASE_AWK_MAX_PARAMS  9999

#if defined(_WIN32) && defined(_DEBUG)
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>

	#define ASE_AWK_MALLOC(awk,size) malloc (size)
	#define ASE_AWK_REALLOC(awk,ptr,size) realloc (ptr, size)
	#define ASE_AWK_FREE(awk,ptr) free (ptr)
#else
	#define ASE_AWK_MALLOC(awk,size) \
		(awk)->syscas.malloc (size, (awk)->syscas.custom_data)
	#define ASE_AWK_REALLOC(awk,ptr,size) \
		(awk)->syscas.realloc (ptr, size, (awk)->syscas.custom_data)
	#define ASE_AWK_FREE(awk,ptr) \
		(awk)->syscas.free (ptr, (awk)->syscas.custom_data)
#endif

#define ASE_AWK_LOCK(awk) \
	do { \
		if ((awk)->syscas.lock != ASE_NULL) \
			(awk)->syscas.lock (awk, (awk)->syscas.custom_data); \
	} while (0) 

#define ASE_AWK_UNLOCK(awk) \
	do { \
		if ((awk)->syscas.unlock != ASE_NULL) \
			(awk)->syscas.unlock (awk, (awk)->syscas.custom_data); \
	} while (0) 

#define ASE_AWK_ISUPPER(awk,c)  (awk)->syscas.is_upper(c)
#define ASE_AWK_ISLOWER(awk,c)  (awk)->syscas.is_lower(c)
#define ASE_AWK_ISALPHA(awk,c)  (awk)->syscas.is_alpha(c)
#define ASE_AWK_ISDIGIT(awk,c)  (awk)->syscas.is_digit(c)
#define ASE_AWK_ISXDIGIT(awk,c) (awk)->syscas.is_xdigit(c)
#define ASE_AWK_ISALNUM(awk,c)  (awk)->syscas.is_alnum(c)
#define ASE_AWK_ISSPACE(awk,c)  (awk)->syscas.is_space(c)
#define ASE_AWK_ISPRINT(awk,c)  (awk)->syscas.is_print(c)
#define ASE_AWK_ISGRAPH(awk,c)  (awk)->syscas.is_graph(c)
#define ASE_AWK_ISCNTRL(awk,c)  (awk)->syscas.is_cntrl(c)
#define ASE_AWK_ISPUNCT(awk,c)  (awk)->syscas.is_punct(c)
#define ASE_AWK_TOUPPER(awk,c)  (awk)->syscas.to_upper(c)
#define ASE_AWK_TOLOWER(awk,c)  (awk)->syscas.to_lower(c)

#define ASE_AWK_MEMCPY(awk,dst,src,len) (awk)->syscas.memcpy (dst, src, len)
#define ASE_AWK_MEMSET(awk,dst,val,len) (awk)->syscas.memset (dst, val, len)

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
};

struct ase_awk_t
{
	ase_awk_syscas_t syscas;

	/* options */
	int option;

	/* parse tree */
	ase_awk_tree_t tree;
	int state;

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
			ase_size_t loop;
			ase_size_t expr; /* expression */
		} depth;

		ase_awk_tab_t globals;
		ase_awk_tab_t locals;
		ase_awk_tab_t params;
		ase_size_t nlocals_max;

		int nl_semicolon;
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
		int           prev;
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

	ase_awk_val_int_t* icache[100]; /* TODO: choose the optimal size  */
	ase_awk_val_real_t* rcache[100]; /* TODO: choose the optimal size  */
	ase_awk_val_ref_t* fcache[100]; /* TODO: choose the optimal size */
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
		ase_char_t tmp[4096];
	} sprintf;

	int errnum;

	ase_awk_t* awk;
	ase_awk_run_t* prev;
	ase_awk_run_t* next;
};

#endif
