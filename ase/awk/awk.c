/* 
 * $Id: awk.c,v 1.106 2007-02-01 08:38:22 bacon Exp $ 
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include <ase/awk/awk_i.h>

static void __free_afn (void* awk, void* afn);

ase_awk_t* ase_awk_open (
	const ase_awk_prmfns_t* prmfns, void* custom_data, int* errnum)
{
	ase_awk_t* awk;

	if (prmfns            == ASE_NULL ||
	    prmfns->malloc    == ASE_NULL || 
	    prmfns->free      == ASE_NULL ||
	    prmfns->is_upper  == ASE_NULL ||
	    prmfns->is_lower  == ASE_NULL ||
	    prmfns->is_alpha  == ASE_NULL ||
	    prmfns->is_digit  == ASE_NULL ||
	    prmfns->is_xdigit == ASE_NULL ||
	    prmfns->is_alnum  == ASE_NULL ||
	    prmfns->is_space  == ASE_NULL ||
	    prmfns->is_print  == ASE_NULL ||
	    prmfns->is_graph  == ASE_NULL ||
	    prmfns->is_cntrl  == ASE_NULL ||
	    prmfns->is_punct  == ASE_NULL ||
	    prmfns->to_upper  == ASE_NULL ||
	    prmfns->to_lower  == ASE_NULL ||
	    prmfns->pow       == ASE_NULL ||
	    prmfns->sprintf   == ASE_NULL || 
	    prmfns->aprintf   == ASE_NULL || 
	    prmfns->dprintf   == ASE_NULL || 
	    prmfns->abort     == ASE_NULL) 
	{
		*errnum = ASE_AWK_ESYSFNS;
		return ASE_NULL;
	}

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
	awk = (ase_awk_t*) malloc (ASE_SIZEOF(ase_awk_t));
#else
	awk = (ase_awk_t*) prmfns->malloc (
		ASE_SIZEOF(ase_awk_t), prmfns->custom_data);
#endif
	if (awk == ASE_NULL) 
	{
		*errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;
	}

	/* it uses the built-in ase_awk_memset because awk is not 
	 * fully initialized yet */
	ase_awk_memset (awk, 0, ASE_SIZEOF(ase_awk_t));

	if (prmfns->memcpy == ASE_NULL)
	{
		ase_awk_memcpy (&awk->prmfns, prmfns, ASE_SIZEOF(awk->prmfns));
		awk->prmfns.memcpy = ase_awk_memcpy;
	}
	else prmfns->memcpy (&awk->prmfns, prmfns, ASE_SIZEOF(awk->prmfns));
	if (prmfns->memset == ASE_NULL) awk->prmfns.memset = ase_awk_memset;

	if (ase_awk_str_open (&awk->token.name, 128, awk) == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, awk);
		*errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;	
	}

	/* TODO: initial map size?? */
	if (ase_awk_map_open (
		&awk->tree.afns, awk, 256, __free_afn, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		*errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.globals, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ase_awk_map_close (&awk->tree.afns);
		ASE_AWK_FREE (awk, awk);
		*errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.locals, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ase_awk_map_close (&awk->tree.afns);
		ase_awk_tab_close (&awk->parse.globals);
		ASE_AWK_FREE (awk, awk);
		*errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.params, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ase_awk_map_close (&awk->tree.afns);
		ase_awk_tab_close (&awk->parse.globals);
		ase_awk_tab_close (&awk->parse.locals);
		ASE_AWK_FREE (awk, awk);
		*errnum = ASE_AWK_ENOMEM;
		return ASE_NULL;	
	}

	awk->option = 0;
	awk->errnum = ASE_AWK_ENOERR;
	awk->errlin = 0;

	awk->parse.nlocals_max = 0;

	awk->tree.nglobals = 0;
	awk->tree.nbglobals = 0;
	awk->tree.begin = ASE_NULL;
	awk->tree.end = ASE_NULL;
	awk->tree.chain = ASE_NULL;
	awk->tree.chain_tail = ASE_NULL;
	awk->tree.chain_size = 0;

	awk->token.prev.type = 0;
	awk->token.prev.line = 0;
	awk->token.prev.column = 0;
	awk->token.type = 0;
	awk->token.line = 0;
	awk->token.column = 0;

	awk->src.lex.curc = ASE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	awk->bfn.sys = ASE_NULL;
	awk->bfn.user = ASE_NULL;

	awk->parse.depth.cur.block = 0;
	awk->parse.depth.cur.loop = 0;
	awk->parse.depth.cur.expr = 0;

	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_BLOCK_PARSE, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_BLOCK_RUN, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_EXPR_PARSE, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_EXPR_RUN, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_REX_BUILD, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_REX_MATCH, 0);

	awk->run.count = 0;
	awk->run.ptr = ASE_NULL;

	awk->custom_data = custom_data;
	return awk;
}

static void __free_afn (void* owner, void* afn)
{
	ase_awk_afn_t* f = (ase_awk_afn_t*)afn;

	/* f->name doesn't have to be freed */
	/*ASE_AWK_FREE ((ase_awk_t*)owner, f->name);*/

	ase_awk_clrpt ((ase_awk_t*)owner, f->body);
	ASE_AWK_FREE ((ase_awk_t*)owner, f);
}

int ase_awk_close (ase_awk_t* awk)
{
	if (ase_awk_clear (awk) == -1) return -1;
	ase_awk_clrbfn (awk);

	ASE_AWK_ASSERT (awk, awk->run.count == 0 && awk->run.ptr == ASE_NULL);

	ase_awk_map_close (&awk->tree.afns);
	ase_awk_tab_close (&awk->parse.globals);
	ase_awk_tab_close (&awk->parse.locals);
	ase_awk_tab_close (&awk->parse.params);
	ase_awk_str_close (&awk->token.name);

	/* ASE_AWK_ALLOC, ASE_AWK_FREE, etc can not be used 
	 * from the next line onwards */
	ASE_AWK_FREE (awk, awk);
	return 0;
}

int ase_awk_clear (ase_awk_t* awk)
{
	/* you should stop all running instances beforehand */
	if (awk->run.ptr != ASE_NULL)
	{
		awk->errnum = ASE_AWK_ERUNNING;
		return -1;
	}

	ASE_AWK_MEMSET (awk, &awk->src.ios, 0, ASE_SIZEOF(awk->src.ios));
	awk->src.lex.curc = ASE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	ase_awk_tab_clear (&awk->parse.globals);
	ase_awk_tab_clear (&awk->parse.locals);
	ase_awk_tab_clear (&awk->parse.params);

	awk->parse.nlocals_max = 0; 
	awk->parse.depth.cur.block = 0;
	awk->parse.depth.cur.loop = 0;
	awk->parse.depth.cur.expr = 0;

	/* clear parse trees */	
	awk->tree.ok = 0;
	awk->tree.nbglobals = 0;
	awk->tree.nglobals = 0;	
	ase_awk_map_clear (&awk->tree.afns);

	if (awk->tree.begin != ASE_NULL) 
	{
		ASE_AWK_ASSERT (awk, awk->tree.begin->next == ASE_NULL);
		ase_awk_clrpt (awk, awk->tree.begin);
		awk->tree.begin = ASE_NULL;
	}

	if (awk->tree.end != ASE_NULL) 
	{
		ASE_AWK_ASSERT (awk, awk->tree.end->next == ASE_NULL);
		ase_awk_clrpt (awk, awk->tree.end);
		awk->tree.end = ASE_NULL;
	}

	while (awk->tree.chain != ASE_NULL) 
	{
		ase_awk_chain_t* next = awk->tree.chain->next;
		if (awk->tree.chain->pattern != ASE_NULL)
			ase_awk_clrpt (awk, awk->tree.chain->pattern);
		if (awk->tree.chain->action != ASE_NULL)
			ase_awk_clrpt (awk, awk->tree.chain->action);
		ASE_AWK_FREE (awk, awk->tree.chain);
		awk->tree.chain = next;
	}

	awk->tree.chain_tail = ASE_NULL;	
	awk->tree.chain_size = 0;

	return 0;
}

int ase_awk_getoption (ase_awk_t* awk)
{
	return awk->option;
}

void ase_awk_setoption (ase_awk_t* awk, int opt)
{
	awk->option = opt;
}

void* ase_awk_getcustomdata (ase_awk_t* awk)
{
	return awk->custom_data;
}
