/*
 * $Id: awk.c 385 2008-09-25 11:06:33Z baconevi $ 
 *
 * {License}
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include "awk.h"

#define SETERR(awk,code) ase_awk_seterrnum(awk,code)

#define SETERRARG(awk,code,line,arg,leng) \
	do { \
		ase_cstr_t errarg; \
		errarg.len = (leng); \
		errarg.ptr = (arg); \
		ase_awk_seterror ((awk), (code), (line), &errarg, 1); \
	} while (0)

static void free_afn (ase_map_t* map, void* vptr, ase_size_t vlen)
{
	ase_awk_t* awk = *(ase_awk_t**)ASE_MAP_EXTENSION(map);
	ase_awk_afn_t* f = (ase_awk_afn_t*)vptr;

	/* f->name doesn't have to be freed */
	/*ASE_AWK_FREE (awk, f->name);*/

	ase_awk_clrpt (awk, f->body);
	ASE_AWK_FREE (awk, f);
}

static void free_bfn (ase_map_t* map, void* vptr, ase_size_t vlen)
{
	ase_awk_t* awk = *(ase_awk_t**)ASE_MAP_EXTENSION(map);
	ase_awk_bfn_t* f = (ase_awk_bfn_t*)vptr;

	ASE_AWK_FREE (awk, f);
}

ase_awk_t* ase_awk_open (ase_mmgr_t* mmgr, ase_size_t ext)
{
	ase_awk_t* awk;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

	awk = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_awk_t) + ext);
	if (awk == ASE_NULL) return ASE_NULL;

	ASE_MEMSET (awk, 0, ASE_SIZEOF(ase_awk_t) + ext);
	awk->mmgr = mmgr;

	if (ase_str_init (&awk->token.name, mmgr, 128) == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->wtab = ase_map_open (mmgr, ASE_SIZEOF(awk), 512, 70);
	if (awk->wtab == ASE_NULL)
	{
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}
	*(ase_awk_t**)ASE_MAP_EXTENSION(awk->wtab) = awk;
	ase_map_setcopier (awk->wtab, ASE_MAP_KEY, ASE_MAP_COPIER_INLINE);
	ase_map_setcopier (awk->wtab, ASE_MAP_VAL, ASE_MAP_COPIER_INLINE);

	awk->rwtab = ase_map_open (mmgr, ASE_SIZEOF(awk), 512, 70);
	if (awk->rwtab == ASE_NULL)
	{
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}
	*(ase_awk_t**)ASE_MAP_EXTENSION(awk->rwtab) = awk;
	ase_map_setcopier (awk->rwtab, ASE_MAP_KEY, ASE_MAP_COPIER_INLINE);
	ase_map_setcopier (awk->rwtab, ASE_MAP_VAL, ASE_MAP_COPIER_INLINE);

	/* TODO: initial map size?? */
	/*awk->tree.afns = ase_map_open (awk, 512, 70, free_afn, ASE_NULL, mmgr);*/
	awk->tree.afns = ase_map_open (mmgr, ASE_SIZEOF(awk), 512, 70);
	if (awk->tree.afns == ASE_NULL) 
	{
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}
	*(ase_awk_t**)ASE_MAP_EXTENSION(awk->tree.afns) = awk;
	ase_map_setfreeer (awk->tree.afns, ASE_MAP_VAL, free_afn);

	/*awk->parse.afns = ase_map_open (awk, 256, 70, ASE_NULL, ASE_NULL, mmgr);*/
	awk->parse.afns = ase_map_open (mmgr, ASE_SIZEOF(awk), 256, 70);
	if (awk->parse.afns == ASE_NULL)
	{
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}
	*(ase_awk_t**)ASE_MAP_EXTENSION(awk->parse.afns) = awk;
	ase_map_setcopier (awk->parse.afns, ASE_MAP_VAL, ASE_MAP_COPIER_INLINE);

	/*awk->parse.named = ase_map_open (awk, 256, 70, ASE_NULL, ASE_NULL, mmgr);*/
	awk->parse.named = ase_map_open (mmgr, ASE_SIZEOF(awk), 256, 70);
	if (awk->parse.named == ASE_NULL)
	{
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}
	*(ase_awk_t**)ASE_MAP_EXTENSION(awk->parse.named) = awk;
	ase_map_setcopier (awk->parse.named, ASE_MAP_VAL, ASE_MAP_COPIER_INLINE);

	if (ase_awk_tab_open (&awk->parse.globals, awk) == ASE_NULL) 
	{
		ase_map_close (awk->parse.named);
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.locals, awk) == ASE_NULL) 
	{
		ase_awk_tab_close (&awk->parse.globals);
		ase_map_close (awk->parse.named);
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.params, awk) == ASE_NULL) 
	{
		ase_awk_tab_close (&awk->parse.locals);
		ase_awk_tab_close (&awk->parse.globals);
		ase_map_close (awk->parse.named);
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->option = 0;
	awk->errnum = ASE_AWK_ENOERR;
	awk->errlin = 0;
	awk->stopall = ASE_FALSE;

	awk->parse.nlocals_max = 0;

	awk->tree.nglobals = 0;
	awk->tree.nbglobals = 0;
	awk->tree.begin = ASE_NULL;
	awk->tree.begin_tail = ASE_NULL;
	awk->tree.end = ASE_NULL;
	awk->tree.end_tail = ASE_NULL;
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
	/*awk->bfn.user = ASE_NULL;*/
	/*awk->bfn.user = ase_map_open (awk, 512, 70, free_bfn, ASE_NULL, mmgr);*/
	awk->bfn.user = ase_map_open (mmgr, ASE_SIZEOF(awk), 512, 70);
	if (awk->bfn.user == ASE_NULL)
	{
		ase_awk_tab_close (&awk->parse.params);
		ase_awk_tab_close (&awk->parse.locals);
		ase_awk_tab_close (&awk->parse.globals);
		ase_map_close (awk->parse.named);
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}
	*(ase_awk_t**)ASE_MAP_EXTENSION(awk->bfn.user) = awk;
	ase_map_setcopier (awk->bfn.user, ASE_MAP_KEY, ASE_MAP_COPIER_INLINE);
	ase_map_setfreeer (awk->bfn.user, ASE_MAP_VAL, free_bfn); 

	awk->parse.depth.cur.block = 0;
	awk->parse.depth.cur.loop = 0;
	awk->parse.depth.cur.expr = 0;

	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_BLOCK_PARSE, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_BLOCK_RUN, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_EXPR_PARSE, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_EXPR_RUN, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_REX_BUILD, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_REX_MATCH, 0);

	awk->assoc_data = ASE_NULL;

	if (ase_awk_initglobals (awk) == -1)
	{
		ase_map_close (awk->bfn.user);
		ase_awk_tab_close (&awk->parse.params);
		ase_awk_tab_close (&awk->parse.locals);
		ase_awk_tab_close (&awk->parse.globals);
		ase_map_close (awk->parse.named);
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_fini (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	return awk;
}


int ase_awk_close (ase_awk_t* awk)
{
	ase_size_t i;

	if (ase_awk_clear (awk) == -1) return -1;
	/*ase_awk_clrbfn (awk);*/
	ase_map_close (awk->bfn.user);

	ase_awk_tab_close (&awk->parse.params);
	ase_awk_tab_close (&awk->parse.locals);
	ase_awk_tab_close (&awk->parse.globals);
	ase_map_close (awk->parse.named);
	ase_map_close (awk->parse.afns);

	ase_map_close (awk->tree.afns);
	ase_map_close (awk->rwtab);
	ase_map_close (awk->wtab);

	ase_str_fini (&awk->token.name);

	for (i = 0; i < ASE_COUNTOF(awk->errstr); i++)
	{
		if (awk->errstr[i] != ASE_NULL)
		{
			ASE_AWK_FREE (awk, awk->errstr[i]);
			awk->errstr[i] = ASE_NULL;
		}
	}

	/* ASE_AWK_ALLOC, ASE_AWK_FREE, etc can not be used 
	 * from the next line onwards */
	ASE_AWK_FREE (awk, awk);
	return 0;
}

int ase_awk_clear (ase_awk_t* awk)
{
	awk->stopall = ASE_FALSE;

	ASE_MEMSET (&awk->src.ios, 0, ASE_SIZEOF(awk->src.ios));
	awk->src.lex.curc = ASE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	/*ase_awk_tab_clear (&awk->parse.globals);*/
	ASE_ASSERT (awk->parse.globals.size == awk->tree.nglobals);
	ase_awk_tab_remove (
		&awk->parse.globals, awk->tree.nbglobals, 
		awk->parse.globals.size - awk->tree.nbglobals);

	ase_awk_tab_clear (&awk->parse.locals);
	ase_awk_tab_clear (&awk->parse.params);
	ase_map_clear (awk->parse.named);
	ase_map_clear (awk->parse.afns);

	awk->parse.nlocals_max = 0; 
	awk->parse.depth.cur.block = 0;
	awk->parse.depth.cur.loop = 0;
	awk->parse.depth.cur.expr = 0;

	/* clear parse trees */	
	awk->tree.ok = 0;
	/*awk->tree.nbglobals = 0;
	awk->tree.nglobals = 0;	 */
	awk->tree.nglobals = awk->tree.nbglobals;

	awk->tree.cur_afn.ptr = ASE_NULL;
	awk->tree.cur_afn.len = 0;
	ase_map_clear (awk->tree.afns);

	if (awk->tree.begin != ASE_NULL) 
	{
		ase_awk_nde_t* next = awk->tree.begin->next;
		/*ASE_ASSERT (awk->tree.begin->next == ASE_NULL);*/
		ase_awk_clrpt (awk, awk->tree.begin);
		awk->tree.begin = ASE_NULL;
		awk->tree.begin_tail = ASE_NULL;	
	}

	if (awk->tree.end != ASE_NULL) 
	{
		/*ASE_ASSERT (awk->tree.end->next == ASE_NULL);*/
		ase_awk_clrpt (awk, awk->tree.end);
		awk->tree.end = ASE_NULL;
		awk->tree.end_tail = ASE_NULL;	
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

void* ase_awk_getextension (ase_awk_t* awk)
{
	return (void*)(awk + 1);
}

ase_mmgr_t* ase_awk_getmmgr (ase_awk_t* awk)
{
	return awk->mmgr;
}

void ase_awk_setmmgr (ase_awk_t* awk, ase_mmgr_t* mmgr)
{
	awk->mmgr = mmgr;
}

ase_ccls_t* ase_awk_getccls (ase_awk_t* awk)
{
	return awk->ccls;
}

void ase_awk_setccls (ase_awk_t* awk, ase_ccls_t* ccls)
{
	ASE_ASSERT (ccls->is_upper  != ASE_NULL);
	ASE_ASSERT (ccls->is_lower  != ASE_NULL);
	ASE_ASSERT (ccls->is_alpha  != ASE_NULL);
	ASE_ASSERT (ccls->is_digit  != ASE_NULL);
	ASE_ASSERT (ccls->is_xdigit  != ASE_NULL);
	ASE_ASSERT (ccls->is_alnum  != ASE_NULL);
	ASE_ASSERT (ccls->is_space  != ASE_NULL);
	ASE_ASSERT (ccls->is_print  != ASE_NULL);
	ASE_ASSERT (ccls->is_graph  != ASE_NULL);
	ASE_ASSERT (ccls->is_cntrl  != ASE_NULL);
	ASE_ASSERT (ccls->is_punct  != ASE_NULL);
	ASE_ASSERT (ccls->to_upper  != ASE_NULL);
	ASE_ASSERT (ccls->to_lower  != ASE_NULL);

	awk->ccls = ccls;
}

void ase_awk_setprmfns (ase_awk_t* awk, ase_awk_prmfns_t* prmfns)
{
	ASE_ASSERT (prmfns->pow     != ASE_NULL);
	ASE_ASSERT (prmfns->sprintf != ASE_NULL);
	ASE_ASSERT (prmfns->dprintf != ASE_NULL);

	awk->prmfns = prmfns;
}

int ase_awk_getoption (ase_awk_t* awk)
{
	return awk->option;
}

void ase_awk_setoption (ase_awk_t* awk, int opt)
{
	awk->option = opt;
}


void ase_awk_stopall (ase_awk_t* awk)
{
	awk->stopall = ASE_TRUE;
}

int ase_awk_getword (ase_awk_t* awk, 
	const ase_char_t* okw, ase_size_t olen,
	const ase_char_t** nkw, ase_size_t* nlen)
{
	ase_map_pair_t* p;

	p = ase_map_search (awk->wtab, okw, olen);
	if (p == ASE_NULL) return -1;

	*nkw = ((ase_cstr_t*)p->vptr)->ptr;
	*nlen = ((ase_cstr_t*)p->vptr)->len;

	return 0;
}

int ase_awk_unsetword (ase_awk_t* awk, const ase_char_t* kw, ase_size_t len)
{
	ase_map_pair_t* p;

	p = ase_map_search (awk->wtab, kw, ASE_NCHARS_TO_NBYTES(len));
	if (p == ASE_NULL)
	{
		SETERRARG (awk, ASE_AWK_ENOENT, 0, kw, len);
		return -1;
	}

	ase_map_delete (awk->rwtab, ASE_MAP_VPTR(p), ASE_MAP_VLEN(p));
	ase_map_delete (awk->wtab, kw, ASE_NCHARS_TO_NBYTES(len));
	return 0;
}

void ase_awk_unsetallwords (ase_awk_t* awk)
{
	ase_map_clear (awk->wtab);
	ase_map_clear (awk->rwtab);
}

int ase_awk_setword (ase_awk_t* awk, 
	const ase_char_t* okw, ase_size_t olen,
	const ase_char_t* nkw, ase_size_t nlen)
{
	ase_cstr_t* vn, * vo;

	if (nkw == ASE_NULL || nlen == 0)
	{
		ase_map_pair_t* p;

		if (okw == ASE_NULL || olen == 0)
		{
			/* clear the entire table */
			ase_awk_unsetallwords (awk);
			return 0;
		}

		return ase_awk_unsetword (awk, okw, olen);
	}
	else if (okw == ASE_NULL || olen == 0)
	{
		SETERR (awk, ASE_AWK_EINVAL);
		return -1;
	}

	/* set the word */
	if (ase_map_upsert (awk->wtab, 
		(ase_char_t*)okw, ASE_NCHARS_TO_NBYTES(olen), 
		(ase_char_t*)nkw, ASE_NCHARS_TO_NBYTES(nlen)) == ASE_NULL)
	{
		SETERR (awk, ASE_AWK_ENOMEM);
		return -1;
	}

	if (ase_map_upsert (awk->rwtab, 
		(ase_char_t*)nkw, ASE_NCHARS_TO_NBYTES(nlen), 
		(ase_char_t*)okw, ASE_NCHARS_TO_NBYTES(olen)) == ASE_NULL)
	{
		ase_map_delete (awk->wtab, okw, ASE_NCHARS_TO_NBYTES(olen));
		SETERR (awk, ASE_AWK_ENOMEM);
		return -1;
	}
 
	return 0;
}

int ase_awk_setrexfns (ase_awk_t* awk, ase_awk_rexfns_t* rexfns)
{
	if (rexfns->build == ASE_NULL ||
	    rexfns->match == ASE_NULL ||
	    rexfns->free == ASE_NULL ||
	    rexfns->isempty == ASE_NULL)
	{
		SETERR (awk, ASE_AWK_EINVAL);
		return -1;
	}

	awk->rexfns = rexfns;
	return 0;
}
