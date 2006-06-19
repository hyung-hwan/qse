/* 
 * $Id: awk.c,v 1.54 2006-06-19 15:43:27 bacon Exp $ 
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

static void __free_func (void* awk, void* func);

xp_awk_t* xp_awk_open (void)
{	
	xp_awk_t* awk;
	xp_size_t i;

	awk = (xp_awk_t*) xp_malloc (xp_sizeof(xp_awk_t));
	if (awk == XP_NULL) return XP_NULL;

	if (xp_str_open (&awk->token.name, 128) == XP_NULL) 
	{
		xp_free (awk);
		return XP_NULL;	
	}

	/* TODO: initial map size?? */
	if (xp_awk_map_open (
		&awk->tree.funcs, awk, 256, __free_func) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_free (awk);
		return XP_NULL;	
	}

	if (xp_awk_tab_open (&awk->parse.globals) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_awk_map_close (&awk->tree.funcs);
		xp_free (awk);
		return XP_NULL;	
	}

	if (xp_awk_tab_open (&awk->parse.locals) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_awk_map_close (&awk->tree.funcs);
		xp_awk_tab_close (&awk->parse.globals);
		xp_free (awk);
		return XP_NULL;	
	}

	if (xp_awk_tab_open (&awk->parse.params) == XP_NULL) 
	{
		xp_str_close (&awk->token.name);
		xp_awk_map_close (&awk->tree.funcs);
		xp_awk_tab_close (&awk->parse.globals);
		xp_awk_tab_close (&awk->parse.locals);
		xp_free (awk);
		return XP_NULL;	
	}

	awk->opt.parse = 0;
	awk->opt.run = 0;
	awk->errnum = XP_AWK_ENOERR;
	awk->srcio = XP_NULL;
	awk->srcio_arg = XP_NULL;

	awk->parse.nlocals_max = 0;

	awk->tree.nglobals = 0;
	awk->tree.begin = XP_NULL;
	awk->tree.end = XP_NULL;
	awk->tree.chain = XP_NULL;
	awk->tree.chain_tail = XP_NULL;

	awk->token.line = 1;
	awk->token.column = 1;

	awk->lex.curc = XP_CHAR_EOF;
	awk->lex.ungotc_count = 0;
	awk->lex.buf_pos = 0;
	awk->lex.buf_len = 0;
	awk->lex.line = 1;
	awk->lex.column = 1;

	for (i = 0; i < xp_countof(awk->extio); i++) awk->extio[i] = XP_NULL;

	return awk;
}

int xp_awk_close (xp_awk_t* awk)
{
	xp_awk_clear (awk);

	if (xp_awk_detsrc(awk) == -1) return -1;

	xp_awk_map_close (&awk->tree.funcs);
	xp_awk_tab_close (&awk->parse.globals);
	xp_awk_tab_close (&awk->parse.locals);
	xp_awk_tab_close (&awk->parse.params);
	xp_str_close (&awk->token.name);

	xp_free (awk);
	return 0;
}

/* TODO: write a function to clear awk->parse data structure.
         this would be need either as a separate function or as a part of xp_awk_clear...
         do i have to pass an option to xp_awk_clear to do this??? */
void xp_awk_clear (xp_awk_t* awk)
{
/* TODO: kill all associated run instances... */

	xp_awk_tab_clear (&awk->parse.globals);
	xp_awk_tab_clear (&awk->parse.locals);
	xp_awk_tab_clear (&awk->parse.params);

	awk->parse.nlocals_max = 0; 

	/* clear parse trees */	
	awk->tree.nglobals = 0;	
	xp_awk_map_clear (&awk->tree.funcs);

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
}

void xp_awk_setparseopt (xp_awk_t* awk, int opt)
{
	awk->opt.parse = opt;
}

void xp_awk_setrunopt (xp_awk_t* awk, int opt)
{
	awk->opt.run = opt;
}

int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg)
{
	if (xp_awk_detsrc(awk) == -1) return -1;

	xp_assert (awk->srcio == XP_NULL);
	if (src(XP_AWK_INPUT_OPEN, arg, XP_NULL, 0) == -1) 
	{
		awk->errnum = XP_AWK_ESRCINOPEN;
		return -1;
	}

	awk->srcio = src;
	awk->srcio_arg = arg;
	awk->lex.curc = XP_CHAR_EOF;
	awk->lex.ungotc_count = 0;
	awk->lex.buf_pos = 0;
	awk->lex.buf_len = 0;
	awk->lex.line = 1;
	awk->lex.column = 1;
	return 0;
}

int xp_awk_detsrc (xp_awk_t* awk)
{
	if (awk->srcio != XP_NULL) 
	{
		xp_ssize_t n;

		n = awk->srcio (XP_AWK_INPUT_CLOSE, awk->srcio_arg, XP_NULL, 0);
		if (n == -1)
		{
			awk->errnum = XP_AWK_ESRCINCLOSE;
			return -1;
		}

		awk->srcio = XP_NULL;
		awk->srcio_arg = XP_NULL;
		awk->lex.curc = XP_CHAR_EOF;
		awk->lex.ungotc_count = 0;
		awk->lex.buf_pos = 0;
		awk->lex.buf_len = 0;
		awk->lex.line = 1;
		awk->lex.column = 1;
	}

	return 0;
}

static void __free_func (void* owner, void* func)
{
	xp_awk_func_t* f = (xp_awk_func_t*)func;

	/* f->name doesn't have to be freed */
	/*xp_free (f->name);*/

	xp_awk_clrpt (f->body);
	xp_free (f);
}

xp_size_t xp_awk_getsrcline (xp_awk_t* awk)
{
	return awk->token.line;
}


/* TODO: imrove this... should it close io when it is overridden with a new handler??? */
int xp_awk_setextio (xp_awk_t* awk, int id, xp_awk_io_t handler, void* arg)
{
	if (id < 0 || id >= xp_countof(awk->extio)) 
	{
		awk->errnum = XP_AWK_EINVAL;
		return -1;
	}

	awk->extio[id] = handler;
	return 0;
}

