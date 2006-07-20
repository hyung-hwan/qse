/*
 * $Id: rex.c,v 1.6 2006-07-20 03:41:00 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>
#endif

enum
{
	__EOF,
	__NORMAL,
	__SPECIAL,
};

enum
{
	__TOP,
	__IN_CHARSET,
	__IN_RANGE,
};

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

#define NEXT_CHAR(rex,level) \
	do { if (__next_char(rex,level) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (__add_code(rex,data,len) == -1) return -1; } while (0)

static int __compile_expression (xp_awk_rex_t* rex);
static int __compile_branch (xp_awk_rex_t* rex);
static int __compile_atom (xp_awk_rex_t* rex);
static int __compile_charset (xp_awk_rex_t* rex);
static int __compile_bound (xp_awk_rex_t* rex);
static int __compile_range (xp_awk_rex_t* rex);
static int __next_char (xp_awk_rex_t* rex, int level);
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

	rex->ptn.curc.type = __EOF;
	rex->ptn.curc.value = XP_T('\0');

	rex->code.size = 0;

	NEXT_CHAR (rex, __TOP);
	if (__compile_expression (rex) == -1)
	{
		/* TODO: clear expression */
xp_printf (XP_T("fuck ........ \n"));
		return -1;
	}

	if (rex->ptn.curc.type != __EOF)
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
	xp_size_t zero = 0;
	xp_size_t* nb, * el, * bl;
	int n;

	/* secure space for header and set the header fields to zero */
	nb = (xp_size_t*)&rex->code.buf[rex->code.size];
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	el = (xp_size_t*)&rex->code.buf[rex->code.size];
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	/* handle the first branch */
	bl = (xp_size_t*)&rex->code.buf[rex->code.size];
	n = __compile_branch (rex);
	if (n == -1) return -1;
	if (n == 0) 
	{
		/* TODO: what if the expression starts with a vertical bar??? */
		return 0;
	}

	(*nb) += 1;
	(*el) += *bl + xp_sizeof(*bl);

	/* handle subsequent branches if any */
	while (rex->ptn.curc.type == __SPECIAL && 
	       rex->ptn.curc.value == XP_T('|'))
	{
		NEXT_CHAR (rex, __TOP);

		bl = (xp_size_t*)&rex->code.buf[rex->code.size];
		n = __compile_branch(rex);
		if (n == -1) return -1;
		if (n == 0) 
		{
			/* if the pattern ends with a vertical bar(|),
			 * this block can be reached. however, the use
			 * of such an expression is highly discouraged */

			/* TODO: should it return an error???? */
			break;
		}

		(*nb) += 1;
		(*el) += *bl + xp_sizeof(*bl);
	}

	return 1;
}

static int __compile_branch (xp_awk_rex_t* rex)
{
	int n;
	xp_size_t* bl;
	xp_size_t old_size;
	xp_size_t zero = 0;

	old_size = rex->code.size;

	bl = (xp_size_t*)&rex->code.buf[rex->code.size];
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	while (1)
	{
		n = __compile_atom (rex);
		if (n == -1) 
		{
			rex->code.size = old_size;
			return -1;
		}

		if (n == 0) break; /* no atom */

		n = __compile_bound (rex);
		if (n == -1)
		{
			rex->code.size = old_size;
			return -1;
		}

		/* n == 0  no bound character. just continue */
		/* n == 1  bound has been applied by compile_bound */
	}

	return 0;
}

static int __compile_atom (xp_awk_rex_t* rex)
{
	int n = 0;

	if (rex->ptn.curc.type == __EOF)
	{
		/* no atom */
		return 0;
	}
	else if (rex->ptn.curc.type == __SPECIAL)
	{
		if (rex->ptn.curc.value == XP_T('('))
		{
			// GROUP
			NEXT_CHAR (rex, __TOP);
	
			n = __compile_expression (rex);
			if (n == -1) return -1;

			if (rex->ptn.curc.type != __SPECIAL || 
			    rex->ptn.curc.value != XP_T(')')) 
			{
				// rex->errnum = XP_AWK_REX_ERPAREN;
				return -1;
			}
	
			NEXT_CHAR (rex, __TOP);
		}
		else if (rex->ptn.curc.value == XP_T('^'))
		{
			struct __code tmp;

			tmp.cmd = CMD_BOL_CHAR;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			NEXT_CHAR (rex, __TOP);
		}
		else if (rex->ptn.curc.value == XP_T('$'))
		{
			struct __code tmp;

			tmp.cmd = CMD_EOL_CHAR;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			NEXT_CHAR (rex, __TOP);
		}
		else if (rex->ptn.curc.value == XP_T('.'))
		{
			struct __code tmp;

			tmp.cmd = CMD_ANY_CHAR;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			NEXT_CHAR (rex, __TOP);
		}
		else if (rex->ptn.curc.value == XP_T('['))
		{
			if (__compile_charset (rex) == -1) return -1;
		}
		else
		{
			/*invalid special character....*/
			return -1;
		}

		return 1;
	}
	else 
	{
		/* normal characters */
		struct __code tmp;

		tmp.cmd = CMD_ORD_CHAR;
		tmp.lbound = 1;
		tmp.ubound = 1;

		ADD_CODE (rex, &tmp, xp_sizeof(tmp));
		ADD_CODE (rex, &rex->ptn.curc, xp_sizeof(rex->ptn.curc));
		NEXT_CHAR (rex, __TOP);

		return 1;
	}

}

static int __compile_charset (xp_awk_rex_t* rex)
{
	return -1;
}

static int __compile_bound (xp_awk_rex_t* rex)
{
	if (rex->ptn.curc.type != __SPECIAL) return 0;

	switch (rex->ptn.curc.value)
	{
		case XP_T('+'):
		{
			//__apply_bound (1, MAX);
			NEXT_CHAR(rex, __TOP);
			return 1;
		}

		case XP_T('*'):
		{
			//__apply_bound (0, MAX);
			NEXT_CHAR(rex, __TOP);
			return 1;
		}

		case XP_T('?'):
		{
			//__apply_bound (0, 1);
			NEXT_CHAR(rex, __TOP);
			return 1;
		}

		case XP_T('{'):
		{
			if (__compile_range(rex) == -1) return -1;
			return 1;
		}
	}

	return 0;
}

static int __compile_range (xp_awk_rex_t* rex)
{
	return -1;
}

static int __next_char (xp_awk_rex_t* rex, int level)
{
	if (rex->ptn.curp >= rex->ptn.end)
	{
		rex->ptn.curc.type = __EOF;
		rex->ptn.curc.value = XP_T('\0');
		return 0;
	}

	rex->ptn.curc.type = __NORMAL;
	rex->ptn.curc.value = *rex->ptn.curp++;

xp_printf (XP_T("[%c]\n"), rex->ptn.curc.value);
	if (rex->ptn.curc.value == XP_T('\\'))
	{	       
		if (rex->ptn.curp >= rex->ptn.end)
		{
			/* unexpected end of expression */
			//rex->errnum = XP_AWK_REX_EEND;
			return -1;	
		}

		rex->ptn.curc.value = *rex->ptn.curp++;

		/* TODO: need this? */
		/*
		if (rex->ptn.curc.value == XP_T('n')) rex->ptn.curc = XP_T('\n');
		else if (rex->ptn.curc.value == XP_T('r')) rex->ptn.curc = XP_T('\r');
		else if (rex->ptn.curc.value == XP_T('t')) rex->ptn.curc = XP_T('\t');
		*/

		return 0;
	}
	else
	{
		if (level == __TOP)
		{
			if (rex->ptn.curc.value == XP_T('[') ||
			    rex->ptn.curc.value == XP_T('|') ||
			    rex->ptn.curc.value == XP_T('^') ||
			    rex->ptn.curc.value == XP_T('$') ||
			    rex->ptn.curc.value == XP_T('{') ||
			    rex->ptn.curc.value == XP_T('+') ||
			    rex->ptn.curc.value == XP_T('?') ||
			    rex->ptn.curc.value == XP_T('*') ||
			    rex->ptn.curc.value == XP_T('.') ||
			    rex->ptn.curc.value == XP_T('(') ||
			    rex->ptn.curc.value == XP_T(')')) 
			{
				rex->ptn.curc.type = __SPECIAL;
			}
		}
		else if (level == __IN_CHARSET)
		{
			if (rex->ptn.curc.value == XP_T('^') ||
			    rex->ptn.curc.value == XP_T('-') ||
			    rex->ptn.curc.value == XP_T(']')) 
			{
				rex->ptn.curc.type = __SPECIAL;
			}
		}
		else if (level == __IN_RANGE)
		{
			if (rex->ptn.curc.value == XP_T(',') ||
			    rex->ptn.curc.value == XP_T('}')) 
			{
				rex->ptn.curc.type = __SPECIAL;
			}
		}
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
