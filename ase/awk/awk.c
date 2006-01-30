/*
 * $Id: awk.c,v 1.13 2006-01-30 14:34:47 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

static void __free_func (void* func);

xp_awk_t* xp_awk_open (xp_awk_t* awk)
{
	if (awk == XP_NULL) {
		awk = (xp_awk_t*) xp_malloc (xp_sizeof(awk));
		if (awk == XP_NULL) return XP_NULL;
		awk->__dynamic = xp_true;
	}
	else awk->__dynamic = xp_false;

	if (xp_str_open(&awk->token.name, 128) == XP_NULL) {
		if (awk->__dynamic) xp_free (awk);
		return XP_NULL;
	}

	if (xp_awk_hash_open(&awk->tree.funcs, 256, __free_func) == XP_NULL) {
		xp_str_close (&awk->token.name);
		if (awk->__dynamic) xp_free (awk);
		return XP_NULL;
	}

	if (xp_awk_tab_open(&awk->parse.funcs) == XP_NULL) {
		xp_str_close (&awk->token.name);
		xp_awk_hash_close (&awk->tree.funcs);
		if (awk->__dynamic) xp_free (awk);
		return XP_NULL;
	}

	awk->opt = 0;
	awk->errnum = XP_AWK_ENOERR;

	awk->src_func = XP_NULL;
	awk->in_func = XP_NULL;
	awk->out_func = XP_NULL;

	awk->src_arg = XP_NULL;
	awk->in_arg = XP_NULL;
	awk->out_arg = XP_NULL;

	awk->tree.begin = XP_NULL;
	awk->tree.end = XP_NULL;
	awk->tree.unnamed = XP_NULL;
	//awk->tree.funcs = XP_NULL;


	awk->lex.curc = XP_CHAR_EOF;
	awk->lex.ungotc_count = 0;

	return awk;
}

int xp_awk_close (xp_awk_t* awk)
{
	xp_awk_clear (awk);
	if (xp_awk_detsrc(awk) == -1) return -1;

	xp_awk_hash_close (&awk->tree.funcs);
	xp_awk_tab_close (&awk->parse.funcs);
	xp_str_close (&awk->token.name);

	if (awk->__dynamic) xp_free (awk);
	return 0;
}

// TODO: write a function to clear awk->parse data structure.
//       this would be need either as a separate function or as a part of xp_awk_clear...
//       do i have to pass an option to xp_awk_clear to do this???

void xp_awk_clear (xp_awk_t* awk)
{
	/* clear parse trees */
	xp_awk_hash_clear (&awk->tree.funcs);

	if (awk->tree.begin != XP_NULL) {
		xp_assert (awk->tree.begin->next == XP_NULL);
		xp_awk_clrpt (awk->tree.begin);
		awk->tree.begin = XP_NULL;
	}

	if (awk->tree.end != XP_NULL) {
		xp_assert (awk->tree.end->next == XP_NULL);
		xp_awk_clrpt (awk->tree.end);
		awk->tree.end = XP_NULL;
	}

	while (awk->tree.unnamed != XP_NULL) {
		xp_awk_node_t* next = awk->tree.unnamed->next;
		xp_awk_clrpt (awk->tree.unnamed);
		awk->tree.unnamed = next;
	}

	/* TODO: destroy pattern-actions pairs */
	/* TODO: destroy function list */
}

int xp_awk_attsrc (xp_awk_t* awk, xp_awk_io_t src, void* arg)
{
	if (xp_awk_detsrc(awk) == -1) return -1;

	xp_assert (awk->src_func == XP_NULL);

	if (src(XP_AWK_IO_OPEN, arg, XP_NULL, 0) == -1) {
		awk->errnum = XP_AWK_ESRCOP;
		return -1;
	}

	awk->src_func = src;
	awk->src_arg = arg;
	awk->lex.curc = XP_CHAR_EOF;
	awk->lex.ungotc_count = 0;
	return 0;
}

int xp_awk_detsrc (xp_awk_t* awk)
{
	if (awk->src_func != XP_NULL) {
		if (awk->src_func(XP_AWK_IO_CLOSE, awk->src_arg, XP_NULL, 0) == -1) {
			awk->errnum = XP_AWK_ESRCCL;
			return -1;
		}

		awk->src_func = XP_NULL;
		awk->src_arg = XP_NULL;
		awk->lex.curc = XP_CHAR_EOF;
		awk->lex.ungotc_count = 0;
	}

	return 0;
}

static void __free_func (void* func)
{
	xp_awk_func_t* f = (xp_awk_func_t*) func;

	/* f->name doesn't have to be freed */
	/*xp_free (f->name);*/

	xp_awk_clrpt (f->body);
	xp_free (f);
}
