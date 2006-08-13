/* 
 * $Id: awk.c,v 1.70 2006-08-13 16:04:32 bacon Exp $ 
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

static void __free_afn (void* awk, void* afn);

xp_awk_t* xp_awk_open (xp_awk_thrlks_t* thrlks)
{	
	xp_awk_t* awk;

	awk = (xp_awk_t*) xp_malloc (xp_sizeof(xp_awk_t));
	if (awk == XP_NULL) return XP_NULL;

	if (xp_str_open (&awk->token.name, 128) == XP_NULL) 
	{
		xp_free (awk);
		return XP_NULL;	
	}

	/* TODO: initial map size?? */
	if (xp_awk_map_open (
		&awk->tree.afns, awk, 256, __free_afn) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_free (awk);
		return XP_NULL;	
	}

	if (xp_awk_tab_open (&awk->parse.globals) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_awk_map_close (&awk->tree.afns);
		xp_free (awk);
		return XP_NULL;	
	}

	if (xp_awk_tab_open (&awk->parse.locals) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_awk_map_close (&awk->tree.afns);
		xp_awk_tab_close (&awk->parse.globals);
		xp_free (awk);
		return XP_NULL;	
	}

	if (xp_awk_tab_open (&awk->parse.params) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_awk_map_close (&awk->tree.afns);
		xp_awk_tab_close (&awk->parse.globals);
		xp_awk_tab_close (&awk->parse.locals);
		xp_free (awk);
		return XP_NULL;	
	}

	awk->option = 0;
	awk->errnum = XP_AWK_ENOERR;

	awk->parse.nlocals_max = 0;

	awk->tree.nglobals = 0;
	awk->tree.nbglobals = 0;
	awk->tree.begin = XP_NULL;
	awk->tree.end = XP_NULL;
	awk->tree.chain = XP_NULL;
	awk->tree.chain_tail = XP_NULL;
	awk->tree.chain_size = 0;

	awk->token.prev = 0;
	awk->token.type = 0;
	awk->token.line = 0;
	awk->token.column = 0;

	awk->src.ios = XP_NULL;
	awk->src.lex.curc = XP_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	awk->bfn.sys = XP_NULL;
	awk->bfn.user = XP_NULL;

	awk->run.count = 0;
	awk->run.ptr = XP_NULL;

	awk->thr.lks = thrlks;
	return awk;
}

int xp_awk_close (xp_awk_t* awk)
{
	if (xp_awk_clear (awk) == -1) return -1;

	xp_assert (awk->run.count == 0 && awk->run.ptr == XP_NULL);

	xp_awk_map_close (&awk->tree.afns);
	xp_awk_tab_close (&awk->parse.globals);
	xp_awk_tab_close (&awk->parse.locals);
	xp_awk_tab_close (&awk->parse.params);
	xp_str_close (&awk->token.name);

	xp_free (awk);
	return 0;
}

int xp_awk_clear (xp_awk_t* awk)
{
	/* you should stop all running instances beforehand */
/* TODO: can i stop all instances??? */
	if (awk->run.ptr != XP_NULL)
	{
		awk->errnum = XP_AWK_ERUNNING;
		return -1;
	}

/* TOOD: clear bfns when they can be added dynamically 
	awk->bfn.sys 
	awk->bfn.user
*/

	awk->src.ios = XP_NULL;
	awk->src.lex.curc = XP_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	xp_awk_tab_clear (&awk->parse.globals);
	xp_awk_tab_clear (&awk->parse.locals);
	xp_awk_tab_clear (&awk->parse.params);

	awk->parse.nlocals_max = 0; 
	awk->parse.depth.loop = 0;

	/* clear parse trees */	
	awk->tree.nbglobals = 0;
	awk->tree.nglobals = 0;	
	xp_awk_map_clear (&awk->tree.afns);

	if (awk->tree.begin != XP_NULL) 
	{
		xp_assert (awk->tree.begin->next == XP_NULL);
		xp_awk_clrpt (awk->tree.begin);
		awk->tree.begin = XP_NULL;
	}

	if (awk->tree.end != XP_NULL) 
	{
		xp_assert (awk->tree.end->next == XP_NULL);
		xp_awk_clrpt (awk->tree.end);
		awk->tree.end = XP_NULL;
	}

	while (awk->tree.chain != XP_NULL) 
	{
		xp_awk_chain_t* next = awk->tree.chain->next;
		if (awk->tree.chain->pattern != XP_NULL)
			xp_awk_clrpt (awk->tree.chain->pattern);
		if (awk->tree.chain->action != XP_NULL)
			xp_awk_clrpt (awk->tree.chain->action);
		xp_free (awk->tree.chain);
		awk->tree.chain = next;
	}

	awk->tree.chain_tail = XP_NULL;	
	awk->tree.chain_size = 0;
	return 0;
}

int xp_awk_getopt (xp_awk_t* awk)
{
	return awk->option;
}

void xp_awk_setopt (xp_awk_t* awk, int opt)
{
	awk->option = opt;
}

static void __free_afn (void* owner, void* afn)
{
	xp_awk_afn_t* f = (xp_awk_afn_t*)afn;

	/* f->name doesn't have to be freed */
	/*xp_free (f->name);*/

	xp_awk_clrpt (f->body);
	xp_free (f);
}

xp_size_t xp_awk_getsrcline (xp_awk_t* awk)
{
	return awk->token.line;
}

