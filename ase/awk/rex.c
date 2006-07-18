/*
 * $Id: rex.c,v 1.3 2006-07-18 15:28:26 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

enum
{
	CMD_ORD_CHAR,
	CMD_ANY_CHAR,
	CMD_CHAR_RANGE,
	CMD_CHAR_CLASS
};

enum
{
	CMD_CHAR_CLASS_PUNCT,
	CMD_CHAR_CLASS_SPACE,
	CMD_CHAR_CLASS_DIGIT,
	CMD_CHAR_CLASS_ALNUM
};

struct __code
{
	unsigned char cmd; 
	unsigned char bflag; /* bound flag */
	xp_char_t lbound;
	xp_char_t ubound;
};

#define PC_CMD(rex,base)    (rex)->code[(base)].dc.cmd
#define PC_BFLAG(rex,base)  (rex)->code[(base)].dc.bflag
#define PC_LBOUND(rex,base) (rex)->code[(base)].dc.lbound
#define PC_UBOUND(rex,base) (rex)->code[(base)].dc.ubound
#define PC_VALUE(rex,base)  (rex)->code[(base)].cc

xp_awk_rex_t* xp_awk_rex_open (xp_awk_rex_t* rex)
{
	if (rex == XP_NULL)
	{
		rex = (xp_awk_rex_t*) xp_malloc (xp_sizeof(xp_awk_rex_t));
		if (rex == XP_NULL) return XP_NULL;
		rex->__dynamic = xp_true;
	}
	else rex->__dynamic = xp_false;

	return rex;
}

void xp_awk_rex_close (xp_awk_rex_t* rex)
{
	if (rex->__dynamic) xp_free (rex);
}

int xp_awk_rex_compile (xp_awk_rex_t* rex, const xp_char_t* ptn)
{
	const xp_char_t* p = ptn;
	xp_char_t c;

	rex->ptn = ptn;

	while (*p != XP_T('\0'))
	{
		c = *p++; // TODO: backspace escaping...

		if (c == XP_T('|'))
		{
		}

	}

	return -1;
}

int __compile_expression (xp_awk_rex_t* rex)
{
	if (__compile_branch (rex) == -1) return -1;

	while (rex->curc == VBAR)
	{
		GET_NEXT_CHAR (rex);

		branch_base = rex->code_size;
		if (compie_branch(rex) == -1) return -1;

		rex->code[branch_base]++;
		rex->code[len_base] += xxxxx;
	}
}

int __compile_branch (xp_awk_rex_t* rex)
{

	while (1)
	{
		atom_base = rex->code_size;

		n = compile_atom ();
		if (n == -1) return -1;
		if (n == 1) break;

		c = rex->curc;
		if (c == PLUS) /* + */
		{
			__apply_bound (1, MAX);
			get_next_char ();
		}
		else if (c == STAR) /* * */
		{
			__apply_bound (0, MAX);
			get_next_char ();
		}
		else if (c == QUEST) /* ? */
		{
			__apply_bound (0, 1);
			get_next_char ();
		}
		else if (c == LBRACE) /* { */
		{
			if (__compile_bound(rex) == -1) return -1;
		}
	}
}

int __compile_atom (xp_awk_rex_t* rex)
{
	xp_char_t c;

	c = rex->curc;

	if (c == LPAREN)
	{
	}
	else 
	{
		if (c == CARET)
		else if (c == DOLLAR)
		else if (c == PERIOD)
		else if (c == LBRACKET)
		else if (....) 
	}

	return n;
}

int __compile_bound (xp_awk_rex_t* rex)
{
}
