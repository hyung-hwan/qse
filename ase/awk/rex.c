/*
 * $Id: rex.c,v 1.4 2006-07-19 11:45:23 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#endif

enum
{
	CMD_BOL_CHAR,
	CMD_EOL_CHAR,
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

#define PC_CMD(rex,base)    (rex)->code[(base)].dc.cmd
#define PC_BFLAG(rex,base)  (rex)->code[(base)].dc.bflag
#define PC_LBOUND(rex,base) (rex)->code[(base)].dc.lbound
#define PC_UBOUND(rex,base) (rex)->code[(base)].dc.ubound
#define PC_VALUE(rex,base)  (rex)->code[(base)].cc

#define BOUND_MIN 0
#define BOUND_MAX (XP_TYPE_MAX(xp_size_t))

struct __code
{
	xp_byte_t cmd;
	xp_size_t lbound;
	xp_size_t ubound;
	xp_char_t cc; /* optional */
};

#define AT_END(rex) ((rex)->ptn.curp >= (rex)->ptn.end)

#define NEXT_CHAR(rex) \
	do { if (__next_char(rex) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (__add_code(rex,data,len) == -1) return -1; } while (0)

static int __compile_expression (xp_awk_rex_t* rex);
static int __compile_branch (xp_awk_rex_t* rex);
static int __compile_atom (xp_awk_rex_t* rex);
static int __compile_charset (xp_awk_rex_t* rex);
static int __compile_bound (xp_awk_rex_t* rex);
static int __next_char (xp_awk_rex_t* rex);
static int __add_code (xp_awk_rex_t* rex, void* data, xp_size_t len);

xp_awk_rex_t* xp_awk_rex_open (xp_awk_rex_t* rex)
{
	if (rex == XP_NULL)
	{
		rex = (xp_awk_rex_t*) xp_malloc (xp_sizeof(xp_awk_rex_t));
		if (rex == XP_NULL) return XP_NULL;
		rex->__dynamic = xp_true;
	}
	else rex->__dynamic = xp_false;

	rex->code.capa = 512;
	rex->code.size = 0;
	rex->code.buf = (xp_byte_t*) xp_malloc (rex->code.capa);
	if (rex->code.buf == XP_NULL)
	{
		if (rex->__dynamic) xp_free (rex);
		return XP_NULL;
	}

	return rex;
}

void xp_awk_rex_close (xp_awk_rex_t* rex)
{
	xp_free (rex->code.buf);
	if (rex->__dynamic) xp_free (rex);
}

int xp_awk_rex_compile (xp_awk_rex_t* rex, const xp_char_t* ptn, xp_size_t len)
{
	rex->ptn.ptr = ptn;
	rex->ptn.end = rex->ptn.ptr + len;
	rex->ptn.curp = rex->ptn.ptr;
	rex->ptn.curc = XP_CHAR_EOF;

	rex->code.size = 0;

	NEXT_CHAR (rex);
	if (AT_END(rex)) return 0; /* empty pattern */

	if (__compile_expression (rex) == -1)
	{
		/* TODO: clear expression */
xp_printf (XP_T("fuck ........ \n"));
		return -1;
	}

	if (!AT_END(rex))
	{
		/* TODO: error handling */
		/* garbage after the expression */
xp_printf (XP_T("garbage after expression\n"));
		return -1;
	}

	return 0;
}

static int __compile_expression (xp_awk_rex_t* rex)
{
	if (__compile_branch (rex) == -1) return -1;

	while (!AT_END(rex) && rex->ptn.curc == XP_T('|'))
	{
		NEXT_CHAR (rex);

		//branch_base = rex->code_size;
		if (__compile_branch(rex) == -1) return -1;

		/*
		rex->code[branch_base]++;
		rex->code[len_base] += xxxxx;
		*/
	}

	return 0;
}

static int __compile_branch (xp_awk_rex_t* rex)
{
	int n;

	while (!AT_END(rex))
	{
		//atom_base = rex->code_size;

		n = __compile_atom (rex);
		if (n == -1) return -1;
		if (n == 1) break;

		if (AT_END(rex)) break;

		switch (rex->ptn.curc)
		{
			case XP_T('+'):
			{
				//__apply_bound (1, MAX);
				NEXT_CHAR (rex);
				break;
			}

			case XP_T('*'):
			{
				//__apply_bound (0, MAX);
				NEXT_CHAR (rex);
				break;
			}

			case XP_T('?'):
			{
				//__apply_bound (0, 1);
				NEXT_CHAR (rex);
				break;
			}

			case XP_T('{'):
			{
				if (__compile_bound(rex) == -1) return -1;
				break;
			}
		}
	}

	return 0;
}

static int __compile_atom (xp_awk_rex_t* rex)
{
	int n = 0;

	if (rex->ptn.curc == XP_T('('))
	{
		// GROUP
		NEXT_CHAR (rex);

		n = __compile_expression (rex);
		if (n == -1) return -1;
		if (rex->ptn.curc != ')') 
		{
			// rex->errnum = XP_AWK_REX_ERPAREN;
			return -1;
		}

		NEXT_CHAR (rex);
	}
	else 
	{
		xp_size_t index = rex->code.size;

		if (rex->ptn.curc == XP_T('^'))
		{
			struct __code tmp;

			tmp.cmd = CMD_BOL_CHAR;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			NEXT_CHAR (rex);
		}
		else if (rex->ptn.curc == XP_T('$'))
		{
			struct __code tmp;

			tmp.cmd = CMD_EOL_CHAR;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			NEXT_CHAR (rex);
		}
		else if (rex->ptn.curc == XP_T('.'))
		{
			struct __code tmp;

			tmp.cmd = CMD_ANY_CHAR;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			NEXT_CHAR (rex);
		}
		else if (rex->ptn.curc == XP_T('['))
		{
			if (__compile_charset (rex) == -1) return -1;
		}
		else 
		{
			struct __code tmp;

			tmp.cmd = CMD_ORD_CHAR;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			ADD_CODE (rex, &rex->ptn.curc, xp_sizeof(rex->ptn.curc));
			NEXT_CHAR (rex);
		}

	}

	return 0;
}

static int __compile_charset (xp_awk_rex_t* rex)
{
	return -1;
}

static int __compile_bound (xp_awk_rex_t* rex)
{
	return -1;
}

static int __next_char (xp_awk_rex_t* rex)
{
	if (AT_END(rex))
	{
xp_printf (XP_T("XP_AWK_REX_EEOF\n"));
		//rex->errnum = XP_AWK_REX_EEOF;
		return -1;
	}

	rex->ptn.curc = *rex->ptn.curp++;
xp_printf (XP_T("[%c]\n"), rex->ptn.curc);
	if (rex->ptn.curc == XP_T('\\'))
	{	       
		if (rex->ptn.curp >= rex->ptn.end)
		{
			/* unexpected end of expression */
			//rex->errnum = XP_AWK_REX_EEND;
			return -1;	
		}

		rex->ptn.curc = *rex->ptn.curp++;

		/* TODO: verify this part */
		if (rex->ptn.curc == XP_T('n')) rex->ptn.curc = XP_T('\n');
		else if (rex->ptn.curc == XP_T('r')) rex->ptn.curc = XP_T('\r');
		else if (rex->ptn.curc == XP_T('t')) rex->ptn.curc = XP_T('\t');
	}

	return 0;
}

static int __add_code (xp_awk_rex_t* rex, void* data, xp_size_t len)
{
	if (len > rex->code.capa - rex->code.size)
	{
		xp_size_t capa = rex->code.capa * 2;
		xp_byte_t* tmp;
		
		if (capa == 0) capa = 1;
		while (len > capa - rex->code.size) { capa = capa * 2; }

		tmp = (xp_byte_t*) xp_realloc (rex->code.buf, capa);
		if (tmp == XP_NULL)
		{
			/* TODO: */
			/*rex->errnum = XP_AWK_REX_ENOMEM;*/
			return -1;
		}

		rex->code.buf = tmp;
		rex->code.capa = capa;
	}

	xp_memcpy (&rex->code.buf[rex->code.size], data, len);
	rex->code.size += len;

	return 0;
}
