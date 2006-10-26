/* 
 * $Id: awk.c,v 1.87 2006-10-26 09:27:15 bacon Exp $ 
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include <ase/awk/awk_i.h>

static void __free_afn (void* awk, void* afn);

ase_awk_t* ase_awk_open (const ase_awk_syscas_t* syscas)
{
	ase_awk_t* awk;

	if (syscas == ASE_NULL) return ASE_NULL;

	if (syscas->malloc == ASE_NULL || 
	    syscas->free == ASE_NULL) return ASE_NULL;

	if (syscas->is_upper  == ASE_NULL ||
	    syscas->is_lower  == ASE_NULL ||
	    syscas->is_alpha  == ASE_NULL ||
	    syscas->is_digit  == ASE_NULL ||
	    syscas->is_xdigit == ASE_NULL ||
	    syscas->is_alnum  == ASE_NULL ||
	    syscas->is_space  == ASE_NULL ||
	    syscas->is_print  == ASE_NULL ||
	    syscas->is_graph  == ASE_NULL ||
	    syscas->is_cntrl  == ASE_NULL ||
	    syscas->is_punct  == ASE_NULL ||
	    syscas->to_upper  == ASE_NULL ||
	    syscas->to_lower  == ASE_NULL) return ASE_NULL;

	if (syscas->sprintf == ASE_NULL || 
	    syscas->dprintf == ASE_NULL || 
	    syscas->abort == ASE_NULL) return ASE_NULL;

#if defined(_WIN32) && defined(_DEBUG)
	awk = (ase_awk_t*) malloc (ase_sizeof(ase_awk_t));
#else
	awk = (ase_awk_t*) syscas->malloc (
		ase_sizeof(ase_awk_t), syscas->custom_data);
#endif
	if (awk == ASE_NULL) return ASE_NULL;

	/* it uses the built-in ase_awk_memset because awk is not 
	 * fully initialized yet */
	ase_awk_memset (awk, 0, ase_sizeof(ase_awk_t));

	if (syscas->memcpy == ASE_NULL)
	{
		ase_awk_memcpy (&awk->syscas, syscas, ase_sizeof(awk->syscas));
		awk->syscas.memcpy = ase_awk_memcpy;
	}
	else syscas->memcpy (&awk->syscas, syscas, ase_sizeof(awk->syscas));
	if (syscas->memset == ASE_NULL) awk->syscas.memset = ase_awk_memset;

	if (ase_awk_str_open (&awk->token.name, 128, awk) == ASE_NULL) 
	{
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	/* TODO: initial map size?? */
	if (ase_awk_map_open (
		&awk->tree.afns, awk, 256, __free_afn, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.globals, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ase_awk_map_close (&awk->tree.afns);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.locals, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ase_awk_map_close (&awk->tree.afns);
		ase_awk_tab_close (&awk->parse.globals);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	if (ase_awk_tab_open (&awk->parse.params, awk) == ASE_NULL) 
	{
		ase_awk_str_close (&awk->token.name);
		ase_awk_map_close (&awk->tree.afns);
		ase_awk_tab_close (&awk->parse.globals);
		ase_awk_tab_close (&awk->parse.locals);
		ASE_AWK_FREE (awk, awk);
		return ASE_NULL;	
	}

	awk->option = 0;
	awk->errnum = ASE_AWK_ENOERR;

	awk->parse.nlocals_max = 0;
	awk->parse.nl_semicolon = 0;

	awk->tree.nglobals = 0;
	awk->tree.nbglobals = 0;
	awk->tree.begin = ASE_NULL;
	awk->tree.end = ASE_NULL;
	awk->tree.chain = ASE_NULL;
	awk->tree.chain_tail = ASE_NULL;
	awk->tree.chain_size = 0;

	awk->token.prev = 0;
	awk->token.type = 0;
	awk->token.line = 0;
	awk->token.column = 0;

	awk->src.ios = ASE_NULL;
	awk->src.lex.curc = ASE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	awk->bfn.sys = ASE_NULL;
	awk->bfn.user = ASE_NULL;

	awk->run.count = 0;
	awk->run.ptr = ASE_NULL;

	return awk;
}

int ase_awk_close (ase_awk_t* awk)
{
	if (ase_awk_clear (awk) == -1) return -1;

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
/* TODO: can i stop all instances??? */
	if (awk->run.ptr != ASE_NULL)
	{
		awk->errnum = ASE_AWK_ERUNNING;
		return -1;
	}

/* TOOD: clear bfns when they can be added dynamically 
	awk->bfn.sys 
	awk->bfn.user
*/

	awk->src.ios = ASE_NULL;
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
	awk->parse.depth.loop = 0;

	/* clear parse trees */	
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

int ase_awk_getopt (ase_awk_t* awk)
{
	return awk->option;
}

void ase_awk_setopt (ase_awk_t* awk, int opt)
{
	awk->option = opt;
}

static void __free_afn (void* owner, void* afn)
{
	ase_awk_afn_t* f = (ase_awk_afn_t*)afn;

	/* f->name doesn't have to be freed */
	/*ASE_AWK_FREE ((ase_awk_t*)owner, f->name);*/

	ase_awk_clrpt ((ase_awk_t*)owner, f->body);
	ASE_AWK_FREE ((ase_awk_t*)owner, f);
}

ase_size_t ase_awk_getsrcline (ase_awk_t* awk)
{
	return awk->token.line;
}

