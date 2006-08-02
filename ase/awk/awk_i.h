/*
 * $Id: awk_i.h,v 1.37 2006-08-02 14:36:22 bacon Exp $
 */

#ifndef _XP_AWK_AWKI_H_
#define _XP_AWK_AWKI_H_

typedef struct xp_awk_chain_t xp_awk_chain_t;
typedef struct xp_awk_run_t xp_awk_run_t;
typedef struct xp_awk_tree_t xp_awk_tree_t;

#include <xp/awk/awk.h>

#ifdef XP_AWK_STAND_ALONE
#include <xp/awk/sa.h>
#else
#include <xp/bas/str.h>
#endif

#include <xp/awk/rex.h>
#include <xp/awk/map.h>
#include <xp/awk/val.h>
#include <xp/awk/func.h>
#include <xp/awk/tree.h>
#include <xp/awk/tab.h>
#include <xp/awk/run.h>
#include <xp/awk/extio.h>

#ifdef _WIN32
#pragma warning (disable: 4996)
#endif

#if defined(_WIN32) && defined(XP_AWK_STAND_ALONE) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/*
 *
struct xp_awk_parse_t
{
	int opt;
};

struct xp_awk_run_t
{
	int opt;
};

awk = xp_awk_open ();
xp_awk_parse (awk, source_input, source_output);
thr = create_thread (5);

thr[0]->xp_awk_run (awk, input_stream1, output_stream1);
thr[1]->xp_awk_run (awk, input_stream2, output_stream2);
thr[2]->xp_awk_run (awk, input_stream3, output_stream3);

xp_awk_setcallback (void* __command_callback (int cmd, void* arg), void* arg);
xp_awk_run (awk)
{
run_stack = malloc (run_stack_size);
while ()
{
if (command_callback) if (command_callback (XP_AWK_ABORT) == yes) break;
run with run_stack
}
}

*/

struct xp_awk_tree_t
{
	xp_size_t nglobals;
	xp_awk_map_t afns; /* awk function map */
	xp_awk_nde_t* begin;
	xp_awk_nde_t* end;
	xp_awk_chain_t* chain;
	xp_awk_chain_t* chain_tail;
};

struct xp_awk_t
{
	/* options */
	struct
	{
		int parse;
		int run;	
	} opt;

	/* io functions */
	xp_awk_io_t srcio;
	void* srcio_arg;

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

	/* source buffer management */
	struct 
	{
		xp_cint_t curc;
		xp_cint_t ungotc[5];
		xp_size_t ungotc_count;

		xp_char_t buf[512];
		xp_size_t buf_pos;
		xp_size_t buf_len;

		xp_size_t line;
		xp_size_t column;
	} lex;

	xp_awk_io_t extio[XP_AWK_EXTIO_NUM];

	/* token */
	struct 
	{
		int       prev;
		int       type;
		xp_str_t  name;
		xp_size_t line;
		xp_size_t column;
	} token;

	/* builtin functions */
	struct
	{
		xp_awk_bfn_t* sys;
		xp_awk_bfn_t* user;
	} bfn;

	/* housekeeping */
	short errnum;
	short suberrnum;
};

struct xp_awk_chain_t
{
	xp_awk_nde_t* pattern;
	int pattern_range_state; /* used when pattern is a range */
	xp_awk_nde_t* action;
	xp_awk_chain_t* next;	
};

struct xp_awk_run_t
{
	xp_awk_map_t named;

	void** stack;
	xp_size_t stack_top;
	xp_size_t stack_base;
	xp_size_t stack_limit;
	int exit_level;

	xp_awk_val_int_t* icache[100]; /* TODO: choose the optimal size  */
	xp_awk_val_real_t* rcache[100]; /* TODO: choose the optimal size  */
	xp_size_t icache_count;
	xp_size_t rcache_count;

	struct
	{
		xp_char_t buf[1024];
		xp_size_t buf_pos;
		xp_size_t buf_len;
		xp_bool_t eof;

		xp_str_t line;
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

	/* extio chain */
	xp_awk_extio_t* extio;

	int opt;
	short errnum;
	short suberrnum;

	/*xp_awk_tree_t* tree;
	xp_size_t nglobals;*/
	xp_awk_t* awk;
};

#endif
