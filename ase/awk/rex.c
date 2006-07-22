/*
 * $Id: rex.c,v 1.9 2006-07-22 16:40:39 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>
#include <xp/bas/ctype.h>
#endif

enum
{
	CT_EOF,
	CT_SPECIAL,
	CT_NORMAL
};

enum
{
	LEVEL_TOP,
	LEVEL_CHARSET,
	LEVEL_RANGE,
};

enum
{
	CMD_BOL,
	CMD_EOL,
	CMD_ANY_CHAR,
	CMD_ORD_CHAR,
	CMD_CHARSET,
	CMD_GROUP
};

enum
{
	CHARSET_ONE,
	CHARSET_RANGE,
	CHARSET_CLASS
};

enum
{
	CHARSET_CLASS_PUNCT,
	CHARSET_CLASS_SPACE,
	CHARSET_CLASS_DIGIT,
	CHARSET_CLASS_ALNUM
};

#define BOUND_MIN 0
#define BOUND_MAX (XP_TYPE_MAX(xp_size_t))

struct __code_t
{
	//xp_byte_t cmd;
	short cmd;
	short negate; /* only for CMD_CHARSET */
	xp_size_t lbound;
	xp_size_t ubound;
};

#define NCHARS_REMAINING(rex) ((rex)->ptn.end - (rex)->ptn.curp)
	
#define NEXT_CHAR(rex,level) \
	do { if (__next_char(rex,level) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (__add_code(rex,data,len) == -1) return -1; } while (0)

#define CODEAT(rex,pos,type) (*((type*)&(rex)->code.buf[pos]))

static int __compile_expression (xp_awk_rex_t* rex);
static int __compile_branch (xp_awk_rex_t* rex);
static int __compile_atom (xp_awk_rex_t* rex);
static int __compile_charset (xp_awk_rex_t* rex, struct __code_t* cmd);
static int __compile_bound (xp_awk_rex_t* rex, struct __code_t* cmd);
static int __compile_cclass (xp_awk_rex_t* rex, xp_char_t* cc);
static int __compile_range (xp_awk_rex_t* rex, struct __code_t* cmd);
static int __next_char (xp_awk_rex_t* rex, int level);
static int __add_code (xp_awk_rex_t* rex, void* data, xp_size_t len);

static const xp_byte_t* __print_expression (const xp_byte_t* p);
static const xp_byte_t* __print_branch (const xp_byte_t* p);
static const xp_byte_t* __print_atom (const xp_byte_t* p);

static xp_bool_t __begin_with (
	const xp_char_t* str, xp_size_t len, const xp_char_t* what);

static xp_bool_t __cc_isalnum (xp_char_t c);
static xp_bool_t __cc_isalpha (xp_char_t c);
static xp_bool_t __cc_isblank (xp_char_t c);
static xp_bool_t __cc_iscntrl (xp_char_t c);
static xp_bool_t __cc_isdigit (xp_char_t c);
static xp_bool_t __cc_isgraph (xp_char_t c);
static xp_bool_t __cc_islower (xp_char_t c);
static xp_bool_t __cc_isprint (xp_char_t c);
static xp_bool_t __cc_ispunct (xp_char_t c);
static xp_bool_t __cc_isspace (xp_char_t c);
static xp_bool_t __cc_isupper (xp_char_t c);
static xp_bool_t __cc_isxdigit (xp_char_t c);

static struct __char_class_t
{
	const xp_char_t* name;
	xp_bool_t (*func) (xp_char_t c);
};

static struct __char_class_t __char_class [] =
{
	{ XP_T("alnum"),  __cc_isalnum },
	{ XP_T("alpha"),  __cc_isalpha },
	{ XP_T("blank"),  __cc_isblank },
	{ XP_T("cntrl"),  __cc_iscntrl },
	{ XP_T("digit"),  __cc_isdigit },
	{ XP_T("graph"),  __cc_isgraph },
	{ XP_T("lower"),  __cc_islower },
	{ XP_T("print"),  __cc_isprint },
	{ XP_T("punct"),  __cc_ispunct },
	{ XP_T("space"),  __cc_isspace },
	{ XP_T("upper"),  __cc_isupper },
	{ XP_T("xdigit"), __cc_isxdigit },

	/*
	{ XP_T("arabic"),   __cc_isarabic },
	{ XP_T("chinese"),  __cc_ischinese },
	{ XP_T("english"),  __cc_isenglish },
	{ XP_T("japanese"), __cc_isjapanese },
	{ XP_T("korean"),   __cc_iskorean }, 
	{ XP_T("thai"),     __cc_isthai }, 
	*/

	{ XP_NULL,        XP_NULL }
};

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

	rex->ptn.curc.type = CT_EOF;
	rex->ptn.curc.value = XP_T('\0');

	rex->code.size = 0;

	NEXT_CHAR (rex, LEVEL_TOP);
	if (__compile_expression (rex) == -1)
	{
		/* TODO: clear expression */
xp_printf (XP_T("fuck ........ \n"));
		return -1;
	}

	if (rex->ptn.curc.type != CT_EOF)
	{
		/* TODO: error handling */
		/* garbage after the expression */
xp_printf (XP_T("garbage after expression\n"));
		return -1;
	}

xp_printf (XP_T("code.capa = %u\n"), (unsigned int)rex->code.capa);
xp_printf (XP_T("code.size = %u\n"), (unsigned int)rex->code.size);
	return 0;
}

static int __compile_expression (xp_awk_rex_t* rex)
{
	xp_size_t zero = 0;
	xp_size_t old_size;
	xp_size_t pos_nb, pos_el;
	int n;

	old_size = rex->code.size;

	/* secure space for header and set the header fields to zero */
	pos_nb = rex->code.size;
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	pos_el = rex->code.size;
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	/* handle the first branch */
	n = __compile_branch (rex);
	if (n == -1) return -1;
	if (n == 0) 
	{
		/* if the expression is empty, the control reaches here */
		return 0;
	}

	CODEAT(rex,pos_nb,xp_size_t) += 1;

	/* handle subsequent branches if any */
	while (rex->ptn.curc.type == CT_SPECIAL && 
	       rex->ptn.curc.value == XP_T('|'))
	{
		NEXT_CHAR (rex, LEVEL_TOP);

		n = __compile_branch(rex);
		if (n == -1) return -1;
		if (n == 0) 
		{
			/* if the pattern ends with a vertical bar(|),
			 * this block can be reached. however, such a 
			 * pattern is highly discouraged */
			break;
		}

		CODEAT(rex,pos_nb,xp_size_t) += 1;
	}

	CODEAT(rex,pos_el,xp_size_t) = rex->code.size - old_size;
	return 1;
}

static int __compile_branch (xp_awk_rex_t* rex)
{
	int n;
	xp_size_t zero = 0;
	xp_size_t old_size;
	xp_size_t pos_na, pos_bl;
	struct __code_t* cmd;

	old_size = rex->code.size;

	pos_na = rex->code.size;
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	pos_bl = rex->code.size;
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	while (1)
	{
		cmd = (struct __code_t*)&rex->code.buf[rex->code.size];

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

		CODEAT(rex,pos_na,xp_size_t) += 1;
	}

	CODEAT(rex,pos_bl,xp_size_t) = rex->code.size - old_size;
	return (rex->code.size == old_size)? 0: 1;
}

static int __compile_atom (xp_awk_rex_t* rex)
{
	int n;
	struct __code_t tmp;

	if (rex->ptn.curc.type == CT_EOF) return 0;

	if (rex->ptn.curc.type == CT_SPECIAL)
	{
		if (rex->ptn.curc.value == XP_T('('))
		{
			tmp.cmd = CMD_GROUP;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (rex, &tmp, xp_sizeof(tmp));

			NEXT_CHAR (rex, LEVEL_TOP);

			n = __compile_expression (rex);
			if (n == -1) return -1;

			if (rex->ptn.curc.type != CT_SPECIAL || 
			    rex->ptn.curc.value != XP_T(')')) 
			{
				// rex->errnum = XP_AWK_REX_ERPAREN;
				return -1;
			}
		}
		else if (rex->ptn.curc.value == XP_T('^'))
		{
			tmp.cmd = CMD_BOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
		}
		else if (rex->ptn.curc.value == XP_T('$'))
		{
			tmp.cmd = CMD_EOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
		}
		else if (rex->ptn.curc.value == XP_T('.'))
		{
			tmp.cmd = CMD_ANY_CHAR;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (rex, &tmp, xp_sizeof(tmp));
		}
		else if (rex->ptn.curc.value == XP_T('['))
		{
			struct __code_t* cmd;

			cmd = (struct __code_t*)&rex->code.buf[rex->code.size];

			tmp.cmd = CMD_CHARSET;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (rex, &tmp, xp_sizeof(tmp));

			NEXT_CHAR (rex, LEVEL_CHARSET);

			n = __compile_charset (rex, cmd);
			if (n == -1) return -1;

			xp_assert (n != 0);

			if (rex->ptn.curc.type != CT_SPECIAL ||
			    rex->ptn.curc.value != XP_T(']'))
			{
				// TODO	
				/*rex->errnum = XP_AWK_REX_ERBRACKET;*/
				return -1;
			}

		}
		else return 0;

		NEXT_CHAR (rex, LEVEL_TOP);
		return 1;
	}
	else 
	{
		xp_assert (rex->ptn.curc.type == CT_NORMAL);

		tmp.cmd = CMD_ORD_CHAR;
		tmp.negate = 0;
		tmp.lbound = 1;
		tmp.ubound = 1;
		ADD_CODE (rex, &tmp, xp_sizeof(tmp));

		ADD_CODE (rex, &rex->ptn.curc.value, xp_sizeof(rex->ptn.curc.value));
		NEXT_CHAR (rex, LEVEL_TOP);

		return 1;
	}
}

static int __compile_charset (xp_awk_rex_t* rex, struct __code_t* cmd)
{
	xp_size_t zero = 0;
	xp_size_t old_size;
	xp_size_t pos_csc, pos_csl;

	old_size = rex->code.size;

	pos_csc = rex->code.size;
	ADD_CODE (rex, &zero, xp_sizeof(zero));
	pos_csl = rex->code.size;
	ADD_CODE (rex, &zero, xp_sizeof(zero));

	if (rex->ptn.curc.type == CT_NORMAL &&
	    rex->ptn.curc.value == XP_T('^')) 
	{
		cmd->negate = 1;
		NEXT_CHAR (rex, LEVEL_CHARSET);
	}

	while (rex->ptn.curc.type == CT_NORMAL)
	{
		xp_char_t c0, c1, c2;
		int cc = 0;

		c1 = rex->ptn.curc.value;
		NEXT_CHAR(rex, LEVEL_CHARSET);

		if (c1 == XP_T('[') &&
		    rex->ptn.curc.type == CT_NORMAL &&
		    rex->ptn.curc.value == XP_T(':'))
		{
			if (__compile_cclass (rex, &c1) == -1)
			{
				return -1;
			}

			cc = cc | 1;
		}

		c2 = c1;
		if (rex->ptn.curc.type == CT_NORMAL &&
		    rex->ptn.curc.value == XP_T('-'))
		{
			NEXT_CHAR (rex, LEVEL_CHARSET);

			if (rex->ptn.curc.type == CT_NORMAL)
			{
				c2 = rex->ptn.curc.value;
				NEXT_CHAR (rex, LEVEL_CHARSET);

				if (c2 == XP_T('[') &&
				    rex->ptn.curc.type == CT_NORMAL &&
				    rex->ptn.curc.value == XP_T(':'))
				{
					if (__compile_cclass (rex, &c2) == -1)
					{
						return -1;
					}

					cc = cc | 2;
				}
			}	
			else cc = cc | 4;
		}


		if (cc == 0 || cc == 4)
		{
			if (c1 == c2)
			{
				c0 = CHARSET_ONE;
				ADD_CODE (rex, &c0, xp_sizeof(c0));
				ADD_CODE (rex, &c1, xp_sizeof(c1));
			}
			else
			{
				c0 = CHARSET_RANGE;
				ADD_CODE (rex, &c0, xp_sizeof(c0));
				ADD_CODE (rex, &c1, xp_sizeof(c1));
				ADD_CODE (rex, &c2, xp_sizeof(c2));
			}
		}
		else if (cc == 1)
		{
			c0 = CHARSET_CLASS;
			ADD_CODE (rex, &c0, xp_sizeof(c0));
			ADD_CODE (rex, &c1, xp_sizeof(c1));
		}
		else
		{
			/* invalid range */
xp_printf (XP_T("invalid character set range\n"));
			return -1;
		}

		CODEAT(rex,pos_csc,xp_size_t) += 1;
	}

	CODEAT(rex,pos_csl,xp_size_t) = rex->code.size - old_size;
	return 1;
}

static int __compile_cclass (xp_awk_rex_t* rex, xp_char_t* cc)
{
	const struct __char_class_t* ccp = __char_class;
	xp_size_t len = rex->ptn.end - rex->ptn.curp;

	while (ccp->name != XP_NULL)
	{
		if (__begin_with (rex->ptn.curp, len, ccp->name)) break;
		ccp++;
	}

	if (ccp->name == XP_NULL)
	{
		/* wrong class name */
xp_printf (XP_T("wrong class name\n"));
		return -1;
	}

	rex->ptn.curp += xp_strlen(ccp->name);

	NEXT_CHAR (rex, LEVEL_CHARSET);
	if (rex->ptn.curc.type != CT_NORMAL ||
	    rex->ptn.curc.value != XP_T(':'))
	{
xp_printf (XP_T(": expected\n"));
		return -1;
	}

	NEXT_CHAR (rex, LEVEL_CHARSET); 
	
	/* ] happens to be the charset ender ] */
	if (rex->ptn.curc.type != CT_SPECIAL ||
	    rex->ptn.curc.value != XP_T(']'))
	{
xp_printf (XP_T("] expected\n"));
		return -1;
	}

	NEXT_CHAR (rex, LEVEL_CHARSET);

	*cc = (xp_char_t)(ccp - __char_class);
	return 1;
}

static int __compile_bound (xp_awk_rex_t* rex, struct __code_t* cmd)
{
	if (rex->ptn.curc.type != CT_SPECIAL) return 0;

	switch (rex->ptn.curc.value)
	{
		case XP_T('+'):
		{
			cmd->lbound = 1;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(rex, LEVEL_TOP);
			return 1;
		}

		case XP_T('*'):
		{
			cmd->lbound = 0;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(rex, LEVEL_TOP);
			return 1;
		}

		case XP_T('?'):
		{
			cmd->lbound = 0;
			cmd->ubound = 1;
			NEXT_CHAR(rex, LEVEL_TOP);
			return 1;
		}

		case XP_T('{'):
		{
			NEXT_CHAR (rex, LEVEL_RANGE);

			if (__compile_range(rex, cmd) == -1) return -1;

			if (rex->ptn.curc.type != CT_SPECIAL || 
			    rex->ptn.curc.value != XP_T('}')) 
			{
				// rex->errnum = XP_AWK_REX_ERBRACE
				return -1;
			}

			NEXT_CHAR (rex, LEVEL_TOP);
			return 1;
		}
	}

	return 0;
}

static int __compile_range (xp_awk_rex_t* rex, struct __code_t* cmd)
{
	xp_size_t bound;

// TODO: should allow white spaces in the range???
//  what if it is not in the raight format? convert it to ordinary characters??
	bound = 0;
	while (rex->ptn.curc.type == CT_NORMAL &&
	       xp_isdigit(rex->ptn.curc.value))
	{
		bound = bound * 10 + rex->ptn.curc.value - XP_T('0');
		NEXT_CHAR (rex, LEVEL_RANGE);
	}

	cmd->lbound = bound;

	if (rex->ptn.curc.type == CT_SPECIAL &&
	    rex->ptn.curc.value == XP_T(',')) 
	{
		NEXT_CHAR (rex, LEVEL_RANGE);

		bound = 0;
		while (rex->ptn.curc.type == CT_NORMAL &&
		       xp_isdigit(rex->ptn.curc.value))
		{
			bound = bound * 10 + rex->ptn.curc.value - XP_T('0');
			NEXT_CHAR (rex, LEVEL_RANGE);
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
		rex->ptn.curc.type = CT_EOF;
		rex->ptn.curc.value = XP_T('\0');
		return 0;
	}

	rex->ptn.curc.type = CT_NORMAL;
	rex->ptn.curc.value = *rex->ptn.curp++;

	if (rex->ptn.curc.value == XP_T('\\'))
	{	       
		if (rex->ptn.curp >= rex->ptn.end)
		{
			/* unexpected end of expression */
			//rex->errnum = XP_AWK_REX_EEND;
			return -1;	
		}

		rex->ptn.curc.value = *rex->ptn.curp++;
		return 0;
	}
	else
	{
		if (level == LEVEL_TOP)
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
				rex->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_CHARSET)
		{
			if (rex->ptn.curc.value == XP_T(']')) 
			{
				rex->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_RANGE)
		{
			if (rex->ptn.curc.value == XP_T(',') ||
			    rex->ptn.curc.value == XP_T('}')) 
			{
				rex->ptn.curc.type = CT_SPECIAL;
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
//xp_printf (XP_T("NA = %u, EL = %u\n"), (unsigned int)nb, (unsigned int)el);

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
//xp_printf (XP_T("NA = %u, BL = %u\n"), (unsigned int) na, (unsigned int)bl);

	for (i = 0; i < na; i++)
	{
		p = __print_atom (p);
	}

	return p;
}

static const xp_byte_t* __print_atom (const xp_byte_t* p)
{
	struct __code_t* cp = (struct __code_t*)p;

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
	else if (cp->cmd == CMD_CHARSET)
	{
		xp_size_t csc, csl, i;

		p += xp_sizeof(*cp);
		xp_printf (XP_T("["));
		if (cp->negate) xp_printf (XP_T("^"));

		csc = *(xp_size_t*)p; p += xp_sizeof(csc);
		csl = *(xp_size_t*)p; p += xp_sizeof(csl);

		for (i = 0; i < csc; i++)
		{
			xp_char_t c0, c1, c2;

			c0 = *(xp_char_t*)p;
			p += xp_sizeof(c0);
			if (c0 == CHARSET_ONE)
			{
				c1 = *(xp_char_t*)p;
				xp_printf (XP_T("%c"), c1);
			}
			else if (c0 == CHARSET_RANGE)
			{
				c1 = *(xp_char_t*)p;
				p += xp_sizeof(c1);
				c2 = *(xp_char_t*)p;
				xp_printf (XP_T("%c-%c"), c1, c2);
			}
			else if (c0 == CHARSET_CLASS)
			{
				c1 = *(xp_char_t*)p;
				xp_printf (XP_T("[:%s:]"), __char_class[c1]);
			}
			else
			{
xp_printf (XP_T("FUCK: WRONG CHARSET CODE\n"));
			}

			p += xp_sizeof(c1);
		}

		xp_printf (XP_T("]"));
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

static xp_bool_t __begin_with (
	const xp_char_t* str, xp_size_t len, const xp_char_t* what)
{
	const xp_char_t* end = str + len;

	while (str < end)
	{
		if (*what == XP_T('\0')) return xp_true;
		if (*what != *str) return xp_false;

		str++; what++;
	}

	if (*what == XP_T('\0')) return xp_true;
	return xp_false;
}

int xp_awk_rex_match (xp_awk_rex_t* rex, 
	const xp_char_t* str, xp_size_t len, 
	const xp_char_t** match, xp_size_t* match_len)
{
	xp_size_t offset = 0;

	while (offset <= len)
	{
		__match_expression (rex);
	}
}

void __match_expression (xp_awk_rex_t* rex)
{
	xp_size_t nb, el, i;

	nb = *(xp_size_t*)p; p += xp_sizeof(nb);
	el = *(xp_size_t*)p; p += xp_sizeof(el);

	for (i = 0; i < nb; i++)
	{
		__match_branch (rex);
	}

	return p;
}

void __match_branch (xp_awk_rex_t* rex)
{
}

void __match_atom (xp_awk_rex_t* rex)
{
}

static const xp_byte_t* __print_branch (const xp_byte_t* p)
}

static xp_bool_t __cc_isalnum (xp_char_t c)
{
	return xp_isalnum (c);
}

static xp_bool_t __cc_isalpha (xp_char_t c)
{
	return xp_isalpha (c);
}

static xp_bool_t __cc_isblank (xp_char_t c)
{
	return c == XP_T(' ') || c == XP_T('\t');
}

static xp_bool_t __cc_iscntrl (xp_char_t c)
{
	return xp_iscntrl (c);
}

static xp_bool_t __cc_isdigit (xp_char_t c)
{
	return xp_isdigit (c);
}

static xp_bool_t __cc_isgraph (xp_char_t c)
{
	return xp_isgraph (c);
}

static xp_bool_t __cc_islower (xp_char_t c)
{
	return xp_islower (c);
}

static xp_bool_t __cc_isprint (xp_char_t c)
{
	return xp_isprint (c);
}

static xp_bool_t __cc_ispunct (xp_char_t c)
{
	return xp_ispunct (c);
}

static xp_bool_t __cc_isspace (xp_char_t c)
{
	return xp_isspace (c);
}

static xp_bool_t __cc_isupper (xp_char_t c)
{
	return xp_isupper (c);
}

static xp_bool_t __cc_isxdigit (xp_char_t c)
{
	return xp_isxdigit (c);
}
