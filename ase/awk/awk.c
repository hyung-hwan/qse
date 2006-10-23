/* 
 * $Id: awk.c,v 1.85 2006-10-23 14:44:42 bacon Exp $ 
 */

#if defined(__BORLANDC__)
#pragma hdrstop
#define Library
#endif

#include <sse/awk/awk_i.h>

static void __free_afn (void* awk, void* afn);

sse_awk_t* sse_awk_open (const sse_awk_syscas_t* syscas)
{
	sse_awk_t* awk;

	if (syscas == SSE_NULL) return SSE_NULL;

	if (syscas->malloc == SSE_NULL || 
	    syscas->free == SSE_NULL) return SSE_NULL;

	if (syscas->is_upper  == SSE_NULL ||
	    syscas->is_lower  == SSE_NULL ||
	    syscas->is_alpha  == SSE_NULL ||
	    syscas->is_digit  == SSE_NULL ||
	    syscas->is_xdigit == SSE_NULL ||
	    syscas->is_alnum  == SSE_NULL ||
	    syscas->is_space  == SSE_NULL ||
	    syscas->is_print  == SSE_NULL ||
	    syscas->is_graph  == SSE_NULL ||
	    syscas->is_cntrl  == SSE_NULL ||
	    syscas->is_punct  == SSE_NULL ||
	    syscas->to_upper  == SSE_NULL ||
	    syscas->to_lower  == SSE_NULL) return SSE_NULL;

	if (syscas->sprintf == SSE_NULL || 
	    syscas->dprintf == SSE_NULL || 
	    syscas->abort == SSE_NULL) return SSE_NULL;

#if defined(_WIN32) && defined(_DEBUG)
	awk = (sse_awk_t*) malloc (sse_sizeof(sse_awk_t));
#else
	awk = (sse_awk_t*) syscas->malloc (
		sse_sizeof(sse_awk_t), syscas->custom_data);
#endif
	if (awk == SSE_NULL) return SSE_NULL;

	/* it uses the built-in sse_awk_memset because awk is not 
	 * fully initialized yet */
	sse_awk_memset (awk, 0, sse_sizeof(sse_awk_t));

	if (syscas->memcpy == SSE_NULL)
	{
		sse_awk_memcpy (&awk->syscas, syscas, sse_sizeof(awk->syscas));
		awk->syscas.memcpy = sse_awk_memcpy;
	}
	else syscas->memcpy (&awk->syscas, syscas, sse_sizeof(awk->syscas));
	if (syscas->memset == SSE_NULL) awk->syscas.memset = sse_awk_memset;

	if (sse_awk_str_open (&awk->token.name, 128, awk) == SSE_NULL) 
	{
		SSE_AWK_FREE (awk, awk);
		return SSE_NULL;	
	}

	/* TODO: initial map size?? */
	if (sse_awk_map_open (
		&awk->tree.afns, awk, 256, __free_afn, awk) == SSE_NULL) 
	{
		sse_awk_str_close (&awk->token.name);
		SSE_AWK_FREE (awk, awk);
		return SSE_NULL;	
	}

	if (sse_awk_tab_open (&awk->parse.globals, awk) == SSE_NULL) 
	{
		sse_awk_str_close (&awk->token.name);
		sse_awk_map_close (&awk->tree.afns);
		SSE_AWK_FREE (awk, awk);
		return SSE_NULL;	
	}

	if (sse_awk_tab_open (&awk->parse.locals, awk) == SSE_NULL) 
	{
		sse_awk_str_close (&awk->token.name);
		sse_awk_map_close (&awk->tree.afns);
		sse_awk_tab_close (&awk->parse.globals);
		SSE_AWK_FREE (awk, awk);
		return SSE_NULL;	
	}

	if (sse_awk_tab_open (&awk->parse.params, awk) == SSE_NULL) 
	{
		sse_awk_str_close (&awk->token.name);
		sse_awk_map_close (&awk->tree.afns);
		sse_awk_tab_close (&awk->parse.globals);
		sse_awk_tab_close (&awk->parse.locals);
		SSE_AWK_FREE (awk, awk);
		return SSE_NULL;	
	}

	awk->option = 0;
	awk->errnum = SSE_AWK_ENOERR;

	awk->parse.nlocals_max = 0;
	awk->parse.nl_semicolon = 0;

	awk->tree.nglobals = 0;
	awk->tree.nbglobals = 0;
	awk->tree.begin = SSE_NULL;
	awk->tree.end = SSE_NULL;
	awk->tree.chain = SSE_NULL;
	awk->tree.chain_tail = SSE_NULL;
	awk->tree.chain_size = 0;

	awk->token.prev = 0;
	awk->token.type = 0;
	awk->token.line = 0;
	awk->token.column = 0;

	awk->src.ios = SSE_NULL;
	awk->src.lex.curc = SSE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	awk->bfn.sys = SSE_NULL;
	awk->bfn.user = SSE_NULL;

	awk->run.count = 0;
	awk->run.ptr = SSE_NULL;

	return awk;
}

int sse_awk_close (sse_awk_t* awk)
{
	if (sse_awk_clear (awk) == -1) return -1;

	sse_awk_assert (awk, awk->run.count == 0 && awk->run.ptr == SSE_NULL);

	sse_awk_map_close (&awk->tree.afns);
	sse_awk_tab_close (&awk->parse.globals);
	sse_awk_tab_close (&awk->parse.locals);
	sse_awk_tab_close (&awk->parse.params);
	sse_awk_str_close (&awk->token.name);

	/* SSE_AWK_ALLOC, SSE_AWK_FREE, etc can not be used 
	 * from the next line onwards */
	SSE_AWK_FREE (awk, awk);
	return 0;
}

int sse_awk_clear (sse_awk_t* awk)
{
	/* you should stop all running instances beforehand */
/* TODO: can i stop all instances??? */
	if (awk->run.ptr != SSE_NULL)
	{
		awk->errnum = SSE_AWK_ERUNNING;
		return -1;
	}

/* TOOD: clear bfns when they can be added dynamically 
	awk->bfn.sys 
	awk->bfn.user
*/

	awk->src.ios = SSE_NULL;
	awk->src.lex.curc = SSE_CHAR_EOF;
	awk->src.lex.ungotc_count = 0;
	awk->src.lex.line = 1;
	awk->src.lex.column = 1;
	awk->src.shared.buf_pos = 0;
	awk->src.shared.buf_len = 0;

	sse_awk_tab_clear (&awk->parse.globals);
	sse_awk_tab_clear (&awk->parse.locals);
	sse_awk_tab_clear (&awk->parse.params);

	awk->parse.nlocals_max = 0; 
	awk->parse.depth.loop = 0;

	/* clear parse trees */	
	awk->tree.nbglobals = 0;
	awk->tree.nglobals = 0;	
	sse_awk_map_clear (&awk->tree.afns);

	if (awk->tree.begin != SSE_NULL) 
	{
		sse_awk_assert (awk, awk->tree.begin->next == SSE_NULL);
		sse_awk_clrpt (awk, awk->tree.begin);
		awk->tree.begin = SSE_NULL;
	}

	if (awk->tree.end != SSE_NULL) 
	{
		sse_awk_assert (awk, awk->tree.end->next == SSE_NULL);
		sse_awk_clrpt (awk, awk->tree.end);
		awk->tree.end = SSE_NULL;
	}

	while (awk->tree.chain != SSE_NULL) 
	{
		sse_awk_chain_t* next = awk->tree.chain->next;
		if (awk->tree.chain->pattern != SSE_NULL)
			sse_awk_clrpt (awk, awk->tree.chain->pattern);
		if (awk->tree.chain->action != SSE_NULL)
			sse_awk_clrpt (awk, awk->tree.chain->action);
		SSE_AWK_FREE (awk, awk->tree.chain);
		awk->tree.chain = next;
	}

	awk->tree.chain_tail = SSE_NULL;	
	awk->tree.chain_size = 0;
	return 0;
}

int sse_awk_getopt (sse_awk_t* awk)
{
	return awk->option;
}

void sse_awk_setopt (sse_awk_t* awk, int opt)
{
	awk->option = opt;
}

static void __free_afn (void* owner, void* afn)
{
	sse_awk_afn_t* f = (sse_awk_afn_t*)afn;

	/* f->name doesn't have to be freed */
	/*SSE_AWK_FREE ((sse_awk_t*)owner, f->name);*/

	sse_awk_clrpt ((sse_awk_t*)owner, f->body);
	SSE_AWK_FREE ((sse_awk_t*)owner, f);
}

sse_size_t sse_awk_getsrcline (sse_awk_t* awk)
{
	return awk->token.line;
}

