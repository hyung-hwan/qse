/*
 * $Id: awk.c,v 1.3 2005-11-14 15:23:53 bacon Exp $
 */

#include <xp/awk/awk.h>
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>

xp_awk_t* xp_awk_open (xp_awk_t* awk)
{
	if (awk == XP_NULL) {
		awk = (xp_awk_t*) xp_malloc (xp_sizeof(awk));
		if (awk == XP_NULL) return XP_NULL;
		awk->__malloced = xp_true;
	}
	else awk->__malloced = xp_false;

	if (xp_str_open(&awk->token.name, 128) == XP_NULL) {
		if (awk->__malloced) xp_free (awk);
		return XP_NULL;
	}

	awk->errnum = XP_AWK_ENOERR;

	awk->source_func = XP_NULL;
	awk->input_func = XP_NULL;
	awk->output_func = XP_NULL;

	awk->source_arg = XP_NULL;
	awk->input_arg = XP_NULL;
	awk->output_arg = XP_NULL;

	awk->lex.curc = XP_CHAR_EOF;
	awk->lex.ungotc_count = 0;

	return awk;
}

int xp_awk_close (xp_awk_t* awk)
{
	if (xp_awk_detach_source(awk) == -1) return -1;
	xp_str_close (&awk->token.name);
	if (awk->__malloced) xp_free (awk);
	return 0;
}

int xp_awk_attach_source (xp_awk_t* awk, xp_awk_io_t source, void* arg)
{
	if (xp_awk_detach_source(awk) == -1) return -1;

	xp_assert (awk->source_func == XP_NULL);

	if (source(XP_AWK_IO_OPEN, arg, XP_NULL, 0) == -1) {
		awk->errnum = XP_AWK_ESRCOP;
		return -1;
	}

	awk->source_func = source;
	awk->source_arg = arg;
	awk->lex.curc = XP_CHAR_EOF;
	awk->lex.ungotc_count = 0;
	return 0;
}

int xp_awk_detach_source (xp_awk_t* awk)
{
	if (awk->source_func != XP_NULL) {
		if (awk->source_func(XP_AWK_IO_CLOSE, awk->source_arg, XP_NULL, 0) == -1) {
			awk->errnum = XP_AWK_ESRCCL;
			return -1;
		}

		awk->source_func = XP_NULL;
		awk->source_arg = XP_NULL;
		awk->lex.curc = XP_CHAR_EOF;
		awk->lex.ungotc_count = 0;
	}

	return 0;
}
