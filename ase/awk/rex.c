/*
 * $Id: rex.c,v 1.7 2006-07-20 16:21:54 bacon Exp $
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
	CMD_BOL,
	CMD_EOL,
	CMD_ANY_CHAR,
	CMD_ORD_CHAR,
	CMD_CHAR_RANGE,
	CMD_CHAR_CLASS,
	CMD_GROUP
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
	//xp_byte_t cmd;
	int cmd;
	xp_size_t lbound;
	xp_size_t ubound;
};

#define NEXT_CHAR(rex,level) \
	do { if (__next_char(rex,level) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (__add_code(rex,data,len) == -1) return -1; } while (0)

static int __compile_expression (xp_awk_rex_t* rex);
static int __compile_branch (xp_awk_rex_t* rex);
static int __compile_atom (xp_awk_rex_t* rex);
static int __compile_charset (xp_awk_rex_t* rex);
static int __compile_bound (xp_awk_rex_t* rex, struct __code* cmd);
static int __compile_range (xp_awk_rex_t* rex, struct __code* cmd);
static int __next_char (xp_awk_rex_t* rex, int level);
static int __add_code (xp_awk_rex_t* rex, void* data, xp_size_t len);

static const xp_byte_t* __print_expression (const xp_byte_t* p);
static const xp_byte_t* __print_branch (const xp_byte_t* p);
static const xp_byte_t* __print_atom (const xp_byte_t* p);

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
	xp_size_t* nb, * el;
	xp_size_t old_size;
	int n;

	old_size = rex->code.size;

	/* secure space for header and set the header fields to zero */
	nb = (xp_size_t*)&rex->code.buf[rex->code.size];
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	el = (xp_size_t*)&rex->code.buf[rex->code.size];
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	/* handle the first branch */
	n = __compile_branch (rex);
	if (n == -1) return -1;
	if (n == 0) 
	{
		/* TODO: what if the expression starts with a vertical bar??? */
		return 0;
	}

	(*nb) += 1;

	/* handle subsequent branches if any */
	while (rex->ptn.curc.type == __SPECIAL && 
	       rex->ptn.curc.value == XP_T('|'))
	{
		NEXT_CHAR (rex, __TOP);

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
	}

	*el = rex->code.size - old_size;
	return 1;
}

static int __compile_branch (xp_awk_rex_t* rex)
{
	int n;
	xp_size_t* na, * bl;
	xp_size_t old_size;
	xp_size_t zero = 0;
	struct __code* cmd;

	old_size = rex->code.size;

	na = (xp_size_t*)&rex->code.buf[rex->code.size];
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	bl = (xp_size_t*)&rex->code.buf[rex->code.size];
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	while (1)
	{
		cmd = (struct __code*)&rex->code.buf[rex->code.size];

		n = __compile_atom (rex);
		if (n == -1) 
		{
			rex->code.size = old_size;
			return -1;
		}

		if (n == 0) break; /* no atom */

		n = __compile_bound (rex, cmd);
		if (n == -1)
		{
			rex->code.size = old_size;
			return -1;
		}

		/* n == 0  no bound character. just continue */
		/* n == 1  bound has been applied by compile_bound */

		(*na) += 1;
	}

	*bl = rex->code.size - old_size;
	return ((*na) == 0)? 0: 1;
}

static int __compile_atom (xp_awk_rex_t* rex)
{
	int n;

	if (rex->ptn.curc.type == __EOF) return 0;

	if (rex->ptn.curc.type == __SPECIAL)
	{
		if (rex->ptn.curc.value == XP_T('('))
		{
			struct __code tmp;

			tmp.cmd = CMD_GROUP;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
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

			tmp.cmd = CMD_BOL;
			tmp.lbound = 1;
			tmp.ubound = 1;

			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
			NEXT_CHAR (rex, __TOP);
		}
		else if (rex->ptn.curc.value == XP_T('$'))
		{
			struct __code tmp;

			tmp.cmd = CMD_EOL;
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
		else return 0;

		return 1;
	}
	else 
	{
		struct __code tmp;

		xp_assert (rex->ptn.curc.type == __NORMAL);

		tmp.cmd = CMD_ORD_CHAR;
		tmp.lbound = 1;
		tmp.ubound = 1;

		ADD_CODE (rex, &tmp, xp_sizeof(tmp));
		ADD_CODE (rex, &rex->ptn.curc.value, xp_sizeof(rex->ptn.curc.value));
		NEXT_CHAR (rex, __TOP);

		return 1;
	}
}

static int __compile_charset (xp_awk_rex_t* rex)
{
	return -1;
}

static int __compile_bound (xp_awk_rex_t* rex, struct __code* cmd)
{
	if (rex->ptn.curc.type != __SPECIAL) return 0;

	switch (rex->ptn.curc.value)
	{
		case XP_T('+'):
		{
			cmd->lbound = 1;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(rex, __TOP);
			return 1;
		}

		case XP_T('*'):
		{
			cmd->lbound = 0;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(rex, __TOP);
			return 1;
		}

		case XP_T('?'):
		{
			cmd->lbound = 0;
			cmd->ubound = 1;
			NEXT_CHAR(rex, __TOP);
			return 1;
		}

		case XP_T('{'):
		{
			NEXT_CHAR (rex, __IN_RANGE);

			if (__compile_range(rex, cmd) == -1) return -1;

			if (rex->ptn.curc.type != __SPECIAL || 
			    rex->ptn.curc.value != XP_T('}')) 
			{
				// rex->errnum = XP_AWK_REX_ERBRACE
				return -1;
			}

			NEXT_CHAR (rex, __TOP);
			return 1;
		}
	}

	return 0;
}

static int __compile_range (xp_awk_rex_t* rex, struct __code* cmd)
{
	xp_size_t bound;

// TODO: should allow white spaces in the range???
//  what if it is not in the raight format? convert it to ordinary characters??
	bound = 0;
	while (rex->ptn.curc.type == __NORMAL &&
	       xp_isdigit(rex->ptn.curc.value))
	{
		bound = bound * 10 + rex->ptn.curc.value - XP_T('0');
		NEXT_CHAR (rex, __IN_RANGE);
	}

	cmd->lbound = bound;

	if (rex->ptn.curc.type == __SPECIAL &&
	    rex->ptn.curc.value == XP_T(',')) 
	{
		NEXT_CHAR (rex, __IN_RANGE);

		bound = 0;
		while (rex->ptn.curc.type == __NORMAL &&
		       xp_isdigit(rex->ptn.curc.value))
		{
			bound = bound * 10 + rex->ptn.curc.value - XP_T('0');
			NEXT_CHAR (rex, __IN_RANGE);
		}

		cmd->ubound = bound;
	}
	else cmd->ubound = BOUND_MAX;

	return 0;
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

void xp_awk_rex_print (xp_awk_rex_t* rex)
{
	const xp_byte_t* p;
	p = __print_expression (rex->code.buf);
	xp_printf (XP_T("\n"));
	xp_assert (p == rex->code.buf + rex->code.size);
}

static const xp_byte_t* __print_expression (const xp_byte_t* p)
{
	xp_size_t nb, el, i;

	nb = *(xp_size_t*)p; p += xp_sizeof(nb);
	el = *(xp_size_t*)p; p += xp_sizeof(el);
//xp_printf (XP_T("NA = %u, EL = %u\n"), 
//	(unsigned int)nb, (unsigned int)el);

	for (i = 0; i < nb; i++)
	{
		if (i != 0) xp_printf (XP_T("|"));
		p = __print_branch (p);
	}

	return p;
}

static const xp_byte_t* __print_branch (const xp_byte_t* p)
{
	xp_size_t na, bl, i;

	na = *(xp_size_t*)p; p += xp_sizeof(na);
	bl = *(xp_size_t*)p; p += xp_sizeof(bl);
//xp_printf (XP_T("NA = %u, BL = %u\n"), 
//	(unsigned int) na, (unsigned int)bl);

	for (i = 0; i < na; i++)
	{
		p = __print_atom (p);
	}

	return p;
}

static const xp_byte_t* __print_atom (const xp_byte_t* p)
{
	struct __code* cp = (struct __code*)p;

	if (cp->cmd == CMD_BOL)
	{
		xp_printf (XP_T("^"));
		p += xp_sizeof(*cp);
	}
	else if (cp->cmd == CMD_EOL)
	{
		xp_printf (XP_T("$"));
		p += xp_sizeof(*cp);
	}
	else if (cp->cmd == CMD_ANY_CHAR) 
	{
		xp_printf (XP_T("."));
		p += xp_sizeof(*cp);
	}
	else if (cp->cmd == CMD_ORD_CHAR) 
	{
		p += xp_sizeof(*cp);
		xp_printf (XP_T("%c"), *(xp_char_t*)p);
		p += xp_sizeof(xp_char_t);
	}
	else if (cp->cmd == CMD_GROUP)
	{
		p += xp_sizeof(*cp);
		xp_printf (XP_T("("));
		p = __print_expression (p);
		xp_printf (XP_T(")"));
	}
	else 
	{
xp_printf (XP_T("FUCK FUCK FUCK\n"));
	}

	if (cp->lbound == 0 && cp->ubound == BOUND_MAX)
		xp_printf (XP_T("*"));
	else if (cp->lbound == 1 && cp->ubound == BOUND_MAX)
		xp_printf (XP_T("+"));
	else if (cp->lbound == 0 && cp->ubound == 1)
		xp_printf (XP_T("?"));
	else if (cp->lbound != 1 || cp->ubound != 1)
	{
		xp_printf (XP_T("{%lu,%lu}"), 
			(unsigned long)cp->lbound, (unsigned long)cp->ubound);
	}


	return p;
}


