/*
 * $Id: awk_i.h,v 1.1 2006-03-31 16:35:37 bacon Exp $
 */

#ifndef _XP_AWK_AWKI_H_
#define _XP_AWK_AWKI_H_

#include <xp/awk/awk.h>
#include <xp/awk/err.h>
#include <xp/awk/tree.h>
#include <xp/awk/tab.h>
#include <xp/awk/map.h>
#include <xp/awk/val.h>
#include <xp/awk/run.h>

#ifdef __STAND_ALONE
#include <xp/awk/sa.h>
#else
#include <xp/bas/str.h>
#endif

typedef struct xp_awk_chain_t xp_awk_chain_t;

struct xp_awk_t
{
	/* options */
	struct
	{
		int parse;
		int run;	
	} opt;

	/* io functions */
	xp_awk_io_t src_func;
	xp_awk_io_t in_func;
	xp_awk_io_t out_func;

	void* src_arg;
	void* in_arg;
	void* out_arg;

	/* parse tree */
	struct 
	{
		xp_size_t nglobals;
		xp_awk_map_t funcs;
		xp_awk_nde_t* begin;
		xp_awk_nde_t* end;
		xp_awk_chain_t* chain;
		xp_awk_chain_t* chain_tail;
	} tree;

	/* temporary information that the parser needs */
	struct
	{
		xp_awk_tab_t globals;
		xp_awk_tab_t locals;
		xp_awk_tab_t params;
		xp_size_t nlocals_max;
	} parse;

	/* run-time data structure */
	struct
	{
		xp_awk_map_t named;

		void** stack;
		xp_size_t stack_top;
		xp_size_t stack_base;
		xp_size_t stack_limit;
		int exit_level;

		xp_awk_val_int_t* icache[100]; // TODO: ... 
		xp_awk_val_real_t* rcache[100]; // TODO: ... 
		xp_size_t icache_count;
		xp_size_t rcache_count;
	} run;

	/* source buffer management */
	struct 
	{
		xp_cint_t curc;
		xp_cint_t ungotc[5];
		xp_size_t ungotc_count;
	} lex;

	/* token */
	struct 
	{
		int       type;
		xp_str_t  name;
	} token;

	/* housekeeping */
	int errnum;
	xp_bool_t __dynamic;
};

struct xp_awk_chain_t
{
	xp_awk_nde_t* pattern;
	xp_awk_nde_t* action;
	xp_awk_chain_t* next;	
};

#endif
