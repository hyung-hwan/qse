/*
 * $Id: awk.c 192 2008-06-06 10:33:44Z baconevi $ 
 *
 * {License}
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include "awk_i.h"

static void free_word (void* awk, void* ptr);
static void free_afn (void* awk, void* afn);
static void free_bfn (void* awk, void* afn);

#define SETERR(awk,code) ase_awk_seterrnum(awk,code)
#define SETERRARG(awk,code,line,arg,leng) \
	do { \
		ase_cstr_t errarg; \
		errarg.len = (leng); \
		errarg.ptr = (arg); \
		ase_awk_seterror ((awk), (code), (line), &errarg, 1); \
	} while (0)

ase_awk_t* ase_awk_open (const ase_awk_prmfns_t* prmfns, void* custom_data)
{
	ase_awk_t* awk;

	ASE_ASSERT (prmfns != ASE_NULL);

	ASE_ASSERT (prmfns->mmgr.malloc != ASE_NULL &&
	            prmfns->mmgr.free   != ASE_NULL);

	ASE_ASSERT (prmfns->ccls.is_upper  != ASE_NULL &&
	            prmfns->ccls.is_lower  != ASE_NULL &&
	            prmfns->ccls.is_alpha  != ASE_NULL &&
	            prmfns->ccls.is_digit  != ASE_NULL &&
	            prmfns->ccls.is_xdigit != ASE_NULL &&
	            prmfns->ccls.is_alnum  != ASE_NULL &&
	            prmfns->ccls.is_space  != ASE_NULL &&
	            prmfns->ccls.is_print  != ASE_NULL &&
	            prmfns->ccls.is_graph  != ASE_NULL &&
	            prmfns->ccls.is_cntrl  != ASE_NULL &&
	            prmfns->ccls.is_punct  != ASE_NULL &&
	            prmfns->ccls.to_upper  != ASE_NULL &&
	            prmfns->ccls.to_lower  != ASE_NULL);
	
	ASE_ASSERT (prmfns->misc.pow     != ASE_NULL &&
	            prmfns->misc.sprintf != ASE_NULL &&
	            prmfns->misc.dprintf != ASE_NULL);

	/* use ASE_MALLOC instead of ASE_AWK_MALLOC because
	 * the awk object has not been initialized yet */
	awk = ASE_MALLOC (&prmfns->mmgr, ASE_SIZEOF(ase_awk_t));
	if (awk == ASE_NULL) return ASE_NULL;

	/* it uses the built-in ase_awk_memset because awk is not 
	 * fully initialized yet */
	ase_memset (awk, 0, ASE_SIZEOF(ase_awk_t));
	ase_memcpy (&awk->prmfns, prmfns, ASE_SIZEOF(awk->prmfns));

	if (ase_str_open (
		&awk->token.name, 128, &awk->prmfns.mmgr) == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->wtab = ase_map_open (
		awk, 512, 70, free_word, ASE_NULL, &awk->prmfns.mmgr);
	if (awk->wtab == ASE_NULL)
	{
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->rwtab = ase_map_open (
		awk, 512, 70, free_word, ASE_NULL, &awk->prmfns.mmgr);
	if (awk->rwtab == ASE_NULL)
	{
		ase_map_close (awk->wtab);
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	/* TODO: initial map size?? */
	awk->tree.afns = ase_map_open (
		awk, 512, 70, free_afn, ASE_NULL, &awk->prmfns.mmgr);
	if (awk->tree.afns == ASE_NULL) 
	{
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->parse.afns = ase_map_open (
		awk, 256, 70, ASE_NULL, ASE_NULL, &awk->prmfns.mmgr);
	if (awk->parse.afns == ASE_NULL)
	{
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->parse.named = ase_map_open (
		awk, 256, 70, ASE_NULL, ASE_NULL, &awk->prmfns.mmgr);
	if (awk->parse.named == ASE_NULL)
	{
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.globals, awk) == ASE_NULL) 
	{
		ase_map_close (awk->parse.named);
		ase_map_close (awk->parse.afns);
		ase_map_close (awk->tree.afns);
		ase_map_close (awk->rwtab);
		ase_map_close (awk->wtab);
		ase_str_close (&awk->token.name);
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
		ase_str_close (&awk->token.name);
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
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->option = 0;
	awk->errnum = ASE_AWK_ENOERR;
	awk->errlin = 0;
	awk->stopall = ase_false;

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
	awk->bfn.user = ase_map_open (
		awk, 512, 70, free_bfn, ASE_NULL, &awk->prmfns.mmgr);
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
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->parse.depth.cur.block = 0;
	awk->parse.depth.cur.loop = 0;
	awk->parse.depth.cur.expr = 0;

	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_BLOCK_PARSE, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_BLOCK_RUN, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_EXPR_PARSE, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_EXPR_RUN, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_REX_BUILD, 0);
	ase_awk_setmaxdepth (awk, ASE_AWK_DEPTH_REX_MATCH, 0);

	awk->custom_data = custom_data;

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
		ase_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	return awk;
}

static void free_word (void* owner, void* ptr)
{
	ASE_AWK_FREE ((ase_awk_t*)owner, ptr);
}

static void free_afn (void* owner, void* afn)
{
	ase_awk_afn_t* f = (ase_awk_afn_t*)afn;

	/* f->name doesn't have to be freed */
	/*ASE_AWK_FREE ((ase_awk_t*)owner, f->name);*/

	ase_awk_clrpt ((ase_awk_t*)owner, f->body);
	ASE_AWK_FREE ((ase_awk_t*)owner, f);
}

static void free_bfn (void* owner, void* bfn)
{
	ase_awk_bfn_t* f = (ase_awk_bfn_t*)bfn;
	ASE_AWK_FREE ((ase_awk_t*)owner, f);
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

	ase_str_close (&awk->token.name);

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
	awk->stopall = ase_false;

	ase_memset (&awk->src.ios, 0, ASE_SIZEOF(awk->src.ios));
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

int ase_awk_getoption (ase_awk_t* awk)
{
	return awk->option;
}

void ase_awk_setoption (ase_awk_t* awk, int opt)
{
	awk->option = opt;
}

ase_mmgr_t* ase_awk_getmmgr (ase_awk_t* awk)
{
	return &awk->prmfns.mmgr;
}

void* ase_awk_getcustomdata (ase_awk_t* awk)
{
	return awk->custom_data;
}

void ase_awk_stopall (ase_awk_t* awk)
{
	awk->stopall = ase_true;
}

int ase_awk_getword (ase_awk_t* awk, 
	const ase_char_t* okw, ase_size_t olen,
	const ase_char_t** nkw, ase_size_t* nlen)
{
	ase_pair_t* p;

	p = ase_map_get (awk->wtab, okw, olen);
	if (p == ASE_NULL) return -1;

	*nkw = ((ase_cstr_t*)p->val)->ptr;
	*nlen = ((ase_cstr_t*)p->val)->len;

	return 0;
}

int ase_awk_setword (ase_awk_t* awk, 
	const ase_char_t* okw, ase_size_t olen,
	const ase_char_t* nkw, ase_size_t nlen)
{
	ase_cstr_t* vn, * vo;

	if (nkw == ASE_NULL || nlen == 0)
	{
		ase_pair_t* p;

		if (okw == ASE_NULL || olen == 0)
		{
			/* clear the entire table */
			ase_map_clear (awk->wtab);
			ase_map_clear (awk->rwtab);
			return 0;
		}

		/* delete the word */
		p = ase_map_get (awk->wtab, okw, olen);
		if (p != ASE_NULL)
		{
			ase_cstr_t* s = (ase_cstr_t*)p->val;
			ase_map_remove (awk->rwtab, s->ptr, s->len);
			ase_map_remove (awk->wtab, okw, olen);
			return 0;
		}
		else 
		{
			SETERRARG (awk, ASE_AWK_ENOENT, 0, okw, olen);
			return -1;
		}
	}
	else if (okw == ASE_NULL || olen == 0)
	{
		SETERR (awk, ASE_AWK_EINVAL);
		return -1;
	}

	/* set the word */
	vn = (ase_cstr_t*) ASE_AWK_MALLOC (
		awk, ASE_SIZEOF(ase_cstr_t)+((nlen+1)*ASE_SIZEOF(*nkw)));
	if (vn == ASE_NULL) 
	{
		SETERR (awk, ASE_AWK_ENOMEM);
		return -1;
	}
	vn->len = nlen;
	vn->ptr = (const ase_char_t*)(vn + 1);
	ase_strncpy ((ase_char_t*)vn->ptr, nkw, nlen);

	vo = (ase_cstr_t*)ASE_AWK_MALLOC (
		awk, ASE_SIZEOF(ase_cstr_t)+((olen+1)*ASE_SIZEOF(*okw)));
	if (vo == ASE_NULL)
	{
		ASE_AWK_FREE (awk, vn);
		SETERR (awk, ASE_AWK_ENOMEM);
		return -1;
	}
	vo->len = olen;
	vo->ptr = (const ase_char_t*)(vo + 1);
	ase_strncpy ((ase_char_t*)vo->ptr, okw, olen);

	if (ase_map_put (awk->wtab, okw, olen, vn) == ASE_NULL)
	{
		ASE_AWK_FREE (awk, vo);
		ASE_AWK_FREE (awk, vn);
		SETERR (awk, ASE_AWK_ENOMEM);
		return -1;
	}

	if (ase_map_put (awk->rwtab, nkw, nlen, vo) == ASE_NULL)
	{
		ase_map_remove (awk->wtab, okw, olen);
		ASE_AWK_FREE (awk, vo);
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
