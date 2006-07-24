/*
 * $Id: rex.c,v 1.12 2006-07-24 16:23:19 bacon Exp $
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

struct __match_t
{
	const xp_char_t* match_ptr;

	xp_bool_t matched;
	xp_size_t match_len;

	const xp_byte_t* branch;
	const xp_byte_t* branch_end;
};

typedef const xp_byte_t* (*atom_matcher_t) (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);

#define NCHARS_REMAINING(rex) ((rex)->ptn.end - (rex)->ptn.curp)
	
#define NEXT_CHAR(rex,level) \
	do { if (__next_char(rex,level) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (__add_code(rex,data,len) == -1) return -1; } while (0)

#define CODEAT(rex,pos,type) (*((type*)&(rex)->code.buf[pos]))

static int __compile_pattern (xp_awk_rex_t* rex);
static int __compile_branch (xp_awk_rex_t* rex);
static int __compile_atom (xp_awk_rex_t* rex);
static int __compile_charset (xp_awk_rex_t* rex, struct __code_t* cmd);
static int __compile_boundary (xp_awk_rex_t* rex, struct __code_t* cmd);
static int __compile_cclass (xp_awk_rex_t* rex, xp_char_t* cc);
static int __compile_range (xp_awk_rex_t* rex, struct __code_t* cmd);
static int __next_char (xp_awk_rex_t* rex, int level);
static int __add_code (xp_awk_rex_t* rex, void* data, xp_size_t len);

static xp_bool_t __begin_with (
	const xp_char_t* str, xp_size_t len, const xp_char_t* what);

static const xp_byte_t* __match_pattern (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_branch (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_branch_body (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_atom (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_bol (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_eol (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_any_char (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_ord_char (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_charset (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);
static const xp_byte_t* __match_group (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat);

static const xp_byte_t* __match_boundary (
	xp_awk_rex_t* rex, xp_size_t si, const xp_byte_t* p,
	xp_size_t lbound, xp_size_t ubound, struct __match_t* mat);

static xp_bool_t __test_charset (
	const xp_byte_t* p, xp_size_t csc, xp_char_t c);

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

static const xp_byte_t* __print_pattern (const xp_byte_t* p);
static const xp_byte_t* __print_branch (const xp_byte_t* p);
static const xp_byte_t* __print_atom (const xp_byte_t* p);

static struct __char_class_t
{
	const xp_char_t* name;
	xp_size_t name_len;
	xp_bool_t (*func) (xp_char_t c);
};

static struct __char_class_t __char_class [] =
{
	{ XP_T("alnum"),  5, __cc_isalnum },
	{ XP_T("alpha"),  5, __cc_isalpha },
	{ XP_T("blank"),  5, __cc_isblank },
	{ XP_T("cntrl"),  5, __cc_iscntrl },
	{ XP_T("digit"),  5, __cc_isdigit },
	{ XP_T("graph"),  5, __cc_isgraph },
	{ XP_T("lower"),  5, __cc_islower },
	{ XP_T("print"),  5, __cc_isprint },
	{ XP_T("punct"),  5, __cc_ispunct },
	{ XP_T("space"),  5, __cc_isspace },
	{ XP_T("upper"),  5, __cc_isupper },
	{ XP_T("xdigit"), 6, __cc_isxdigit },

	/*
	{ XP_T("arabic"),   6, __cc_isarabic },
	{ XP_T("chinese"),  7, __cc_ischinese },
	{ XP_T("english"),  7, __cc_isenglish },
	{ XP_T("japanese"), 8, __cc_isjapanese },
	{ XP_T("korean"),   6, __cc_iskorean }, 
	{ XP_T("thai"),     4, __cc_isthai }, 
	*/

	{ XP_NULL,        0, XP_NULL }
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

	rex->errnum = XP_AWK_REX_ENOERR;
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
	if (__compile_pattern (rex) == -1) return -1;

	if (rex->ptn.curc.type != CT_EOF)
	{
		/* garbage after the patter */
		rex->errnum = XP_AWK_REX_EGARBAGE;
		return -1;
	}

xp_printf (XP_T("code.capa = %u\n"), (unsigned int)rex->code.capa);
xp_printf (XP_T("code.size = %u\n"), (unsigned int)rex->code.size);
	return 0;
}

static int __compile_pattern (xp_awk_rex_t* rex)
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
		/* if the pattern is empty, the control reaches here */
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

		n = __compile_boundary (rex, cmd);
		if (n == -1)
		{
			rex->code.size = old_size;
			return -1;
		}

		/* n == 0  no bound character. just continue */
		/* n == 1  bound has been applied by compile_boundary */

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

			n = __compile_pattern (rex);
			if (n == -1) return -1;

			if (rex->ptn.curc.type != CT_SPECIAL || 
			    rex->ptn.curc.value != XP_T(')')) 
			{
				rex->errnum = XP_AWK_REX_ERPAREN;
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
				rex->errnum = XP_AWK_REX_ERBRACKET;
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
			if (__compile_cclass (rex, &c1) == -1) return -1;
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
//xp_printf (XP_T("invalid character set range\n"));
			rex->errnum = XP_AWK_REX_ECRANGE;
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
//xp_printf (XP_T("wrong class name\n"));
		rex->errnum = XP_AWK_REX_ECCLASS;
		return -1;
	}

	rex->ptn.curp += ccp->name_len;

	NEXT_CHAR (rex, LEVEL_CHARSET);
	if (rex->ptn.curc.type != CT_NORMAL ||
	    rex->ptn.curc.value != XP_T(':'))
	{
//xp_printf (XP_T(": expected\n"));
		rex->errnum = XP_AWK_REX_ECOLON;
		return -1;
	}

	NEXT_CHAR (rex, LEVEL_CHARSET); 
	
	/* ] happens to be the charset ender ] */
	if (rex->ptn.curc.type != CT_SPECIAL ||
	    rex->ptn.curc.value != XP_T(']'))
	{
//xp_printf (XP_T("] expected\n"));
		rex->errnum = XP_AWK_REX_ERBRACKET;	
		return -1;
	}

	NEXT_CHAR (rex, LEVEL_CHARSET);

	*cc = (xp_char_t)(ccp - __char_class);
	return 1;
}

static int __compile_boundary (xp_awk_rex_t* rex, struct __code_t* cmd)
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
				rex->errnum = XP_AWK_REX_ERBRACE;
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

	if (cmd->lbound > cmd->ubound)
	{
		/* invalid boundary range */
		rex->errnum = XP_AWK_REX_EBRANGE;
		return -1;
	}

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
			rex->errnum = XP_AWK_REX_EEND;
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
			rex->errnum = XP_AWK_REX_ENOMEM;
			return -1;
		}

		rex->code.buf = tmp;
		rex->code.capa = capa;
	}

	xp_memcpy (&rex->code.buf[rex->code.size], data, len);
	rex->code.size += len;

	return 0;
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
	const xp_char_t** match_ptr, xp_size_t* match_len)
{
	xp_size_t offset = 0;
	struct __match_t mat;

	if (rex->code.size == 0)
	{
		/* no pattern has been compiled */
		rex->errnum = XP_AWK_REX_ENOPTN;
		return -1;
	}

	mat.matched = xp_false;

	/* store the source string */
	rex->match.str.ptr = str;
	rex->match.str.end = str + len;

	mat.match_ptr = str + offset;

	while (mat.match_ptr < rex->match.str.end)
	{
		__match_pattern (rex, rex->code.buf, &mat);
		if (mat.matched)
		{
			*match_ptr = mat.match_ptr;
			*match_len = mat.match_len;
			break;
		}

		mat.match_ptr++;
	}

	return (mat.matched)? 0: -1;
}

static const xp_byte_t* __match_pattern (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p;
	struct __match_t mat2;
	xp_size_t nb, el, i;

	p = base;
	nb = *(xp_size_t*)p; p += xp_sizeof(nb);
	el = *(xp_size_t*)p; p += xp_sizeof(el);

//xp_printf (XP_T("NB = %u, EL = %u\n"), (unsigned)nb, (unsigned)el);
	mat->matched = xp_false;
	mat->match_len = 0;

	for (i = 0; i < nb; i++)
	{
		mat2.match_ptr = mat->match_ptr;

		p = __match_branch (rex, p, &mat2);
		if (mat2.matched)
		{
			mat->matched = xp_true;
			mat->match_len = mat2.match_len;
			break;
		}
	}

	return base + el;
}

static const xp_byte_t* __match_branch (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p;
	xp_size_t na, bl;

	p = base;

	na = *(xp_size_t*)p; p += xp_sizeof(na);
	bl = *(xp_size_t*)p; p += xp_sizeof(bl);
//xp_printf (XP_T("NA = %u, BL = %u\n"), (unsigned)na, (unsigned)bl);

	/* remember the current branch to work on */
	mat->branch = base;
	mat->branch_end = base + bl;

	return __match_branch_body (rex, p, mat);
}

static const xp_byte_t* __match_branch_body (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p;
	struct __match_t mat2;

	mat->matched = xp_false;
	mat->match_len = 0;

	mat2.match_ptr = mat->match_ptr;
	mat2.branch = mat->branch;
	mat2.branch_end = mat->branch_end;

	p = base;

	while (p < mat->branch_end)
	{
		p = __match_atom (rex, p, &mat2);

		if (!mat2.matched) 
		{
			mat->matched = xp_false;
			break; /* stop matching */
		}

		mat->matched = xp_true;
		mat->match_len += mat2.match_len;

		mat2.match_ptr = &mat2.match_ptr[mat2.match_len];
	}

	return mat->branch_end;
}

static const xp_byte_t* __match_atom (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	static atom_matcher_t matchers[] =
	{
		__match_bol,
		__match_eol,
		__match_any_char,
		__match_ord_char,
		__match_charset,
		__match_group
	};
       
	xp_assert (((struct __code_t*)base)->cmd >= 0 && 
	           ((struct __code_t*)base)->cmd < xp_countof(matchers));

	return matchers[((struct __code_t*)base)->cmd] (rex, base, mat);
}

static const xp_byte_t* __match_bol (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_BOL);

	mat->matched = (mat->match_ptr == rex->match.str.ptr ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const xp_byte_t* __match_eol (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_EOL);

	mat->matched = (mat->match_ptr == rex->match.str.end ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const xp_byte_t* __match_any_char (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;
	xp_size_t si = 0, lbound, ubound;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_ANY_CHAR);

	lbound = cp->lbound;
	ubound = cp->ubound;

	mat->matched = xp_false;
	mat->match_len = 0;

	/* merge the same consecutive codes */
	while (p < mat->branch_end &&
	       cp->cmd == ((const struct __code_t*)p)->cmd)
	{
		lbound += ((const struct __code_t*)p)->lbound;
		ubound += ((const struct __code_t*)p)->ubound;

		p += xp_sizeof(*cp);
	}

//xp_printf (XP_T("lbound = %u, ubound = %u\n"), 
//(unsigned int)lbound, (unsigned int)ubound);
	/* find the longest match */
	while (si < ubound)
	{
		if (&mat->match_ptr[si] >= rex->match.str.end) break;
		si++;
	}

//xp_printf (XP_T("max si = %d\n"), si);
	if (si >= lbound && si <= ubound)
	{
		p = __match_boundary (rex, si, p, lbound, ubound, mat);
	}

	return p;
}

static const xp_byte_t* __match_ord_char (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;
	xp_size_t si = 0, lbound, ubound;
	xp_char_t cc;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_ORD_CHAR);

	lbound = cp->lbound; 
	ubound = cp->ubound;

	cc = *(xp_char_t*)p; p += xp_sizeof(cc);

	/* merge the same consecutive codes 
	 * for example, a{1,10}a{0,10} is shortened to a{1,20} 
	 */
	while (p < mat->branch_end &&
	       cp->cmd == ((const struct __code_t*)p)->cmd)
	{
		if (*(xp_char_t*)(p+xp_sizeof(*cp)) != cc) break;

		lbound += ((const struct __code_t*)p)->lbound;
		ubound += ((const struct __code_t*)p)->ubound;

		p += xp_sizeof(*cp) + xp_sizeof(cc);
	}
	
//xp_printf (XP_T("lbound = %u, ubound = %u\n"), 
//(unsigned int)lbound, (unsigned int)ubound);

	mat->matched = xp_false;
	mat->match_len = 0;

	/* find the longest match */
	while (si < ubound)
	{
		if (&mat->match_ptr[si] >= rex->match.str.end) break;
		if (cc != mat->match_ptr[si]) break;
		si++;
	}

//xp_printf (XP_T("max si = %d\n"), si);
	if (si >= lbound && si <= ubound)
	{
		p = __match_boundary (rex, si, p, lbound, ubound, mat);
	}

	return p;
}

static const xp_byte_t* __match_charset (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;
	xp_size_t si = 0, lbound, ubound, csc, csl;
	xp_bool_t n;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_CHARSET);

	lbound = cp->lbound;
	ubound = cp->ubound;

	csc = *(xp_size_t*)p; p += xp_sizeof(csc);
	csl = *(xp_size_t*)p; p += xp_sizeof(csl);

	mat->matched = xp_false;
	mat->match_len = 0;

	while (si < ubound)
	{
		if (&mat->match_ptr[si] >= rex->match.str.end) break;

		n = __test_charset (p, csc, mat->match_ptr[si]);
		if (cp->negate) n = !n;
		if (!n) break;

		si++;
	}

	p = p + csl - (xp_sizeof(csc) + xp_sizeof(csl));

	if (si >= lbound && si <= ubound)
	{
		p = __match_boundary (rex, si, p, lbound, ubound, mat);
	}

	return p;
}

static const xp_byte_t* __match_group (
	xp_awk_rex_t* rex, const xp_byte_t* base, struct __match_t* mat)
{
	const xp_byte_t* p = base, * sub;
	const struct __code_t* cp;
	struct __match_t mat2;
	xp_size_t si = 0, nb, el;

xp_size_t grp_len[100];

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_GROUP);

	/* peep at the header of a subpattern */
	sub = p;
	nb = *(xp_size_t*)p; p += xp_sizeof(nb);
	el = *(xp_size_t*)p; p += xp_sizeof(el);

	mat->matched = xp_false;
	mat->match_len = 0;
	
	mat2.match_ptr = mat->match_ptr;

	/* 
	 * A grouped pattern, unlike other atoms, can match one or more 
	 * characters. When it is requested with a variable occurrences, 
	 * the number of characters that have matched at each occurrence 
	 * needs to be remembered for the backtracking purpose.
	 *
	 * An array "grp_len" is used to store the accumulated number of 
	 * characters. grp_len[0] is set to zero always for convenience.
	 * grp_len[1] holds the number of characters that have matched
	 * at the first occurrence, grp_len[2] at the second occurrence, 
	 * and so on.
	 *
	 * Look at the following example
	 *
	 *   pattern: (abc){1,3}x   string: abcabcabcxyz
	 *
	 *  grp_len[3] => 9 -----------+
	 *  grp_len[2] => 6 --------+  |
	 *  grp_len[1] => 3 -----+  |  |
	 *  grp_len[0] => 0 --+  |  |  |
	 *                    |  |  |  |
	 *                     abcabcabcxyz
	 */

// TODO: make this dynamic......
grp_len[si] = 0;
	while (si < cp->ubound)
	{
		if (mat2.match_ptr >= rex->match.str.end) break;

		__match_pattern (rex, sub, &mat2);
		if (!mat2.matched) break;

grp_len[si+1] = grp_len[si] + mat2.match_len;
		mat2.match_ptr += mat2.match_len;
		mat2.match_len = 0;
		mat2.matched = xp_false;

		si++;
	}

	p = sub + el;

	if (si >= cp->lbound && si <= cp->ubound)
	{
		if (cp->lbound == cp->ubound || p >= mat->branch_end)
		{
			mat->matched = xp_true;
			mat->match_len = grp_len[si];
		}
		else 
		{
			xp_assert (cp->ubound > cp->lbound);

			do
			{
				const xp_byte_t* tmp;
	
				mat2.match_ptr = &mat->match_ptr[grp_len[si]];
				mat2.branch = mat->branch;
				mat2.branch_end = mat->branch_end;
	
//xp_printf (XP_T("GROUP si = %d [%s]\n"), si, mat->match_ptr);
				tmp = __match_branch_body (rex, p, &mat2);

				if (mat2.matched)
				{
					mat->matched = xp_true;
					mat->match_len = grp_len[si] + mat2.match_len;
					p = tmp;
					break;
				}

				if (si <= cp->lbound) break;
				si--;
			} 
			while (1);
		}

	}

	return p;
}

static const xp_byte_t* __match_boundary (
	xp_awk_rex_t* rex, xp_size_t si, const xp_byte_t* p,
	xp_size_t lbound, xp_size_t ubound, struct __match_t* mat)
{
	xp_assert (si >= lbound && si <= ubound);
	/* the match has been found */

	if (lbound == ubound || p >= mat->branch_end)
	{
		/* if the match for fixed occurrences was 
		 * requested or no atoms remain unchecked in 
		 * the branch, the match is returned. */
		mat->matched = xp_true;
		mat->match_len = si;
	}
	else 
	{
		/* Otherwise, it checks if the remaining atoms 
		 * match the rest of the string 
		 * 
		 * Let's say the caller of this function was processing
		 * the first period character in the following example.
		 *
		 *     pattern: .{1,3}xx   string: xxxyy
		 * 
		 * It scans up to the third "x" in the string. si is set 
		 * to 3 and p points to the first "x" in the pattern. 
		 * It doesn't change mat.match_ptr so mat.match_ptr remains
		 * the same.
		 *
		 *     si = 3    p -----+    mat.match_ptr ---+
		 *                      |                     |
		 *                .{1,3}xx                    xxxyy
		 *                     
		 *  When the code reaches here, the string pointed at by
		 *  &mat.match_ptr[si] is tried to match against the remaining
		 *  pattern pointed at p.
		 *  
		 *     &mat.match_ptr[si] ---+
		 *                           |
		 *                        xxxyy
		 *
		 * If a match is found, the match and the previous match are
		 * merged and returned.
		 *
		 * If not, si is decremented by one and the match is performed
		 * from the string pointed at by &mat.match_ptr[si].
		 *
		 *     &mat.match_ptr[si] --+
		 *                          |
		 *                        xxxyy
		 *
		 * This process is repeated until a match is found or si 
		 * becomes less than lbound. (si never becomes less than
		 * lbound in the implementation below, though)
		 */

		xp_assert (ubound > lbound);

		do
		{
			struct __match_t mat2;
			const xp_byte_t* tmp;

			mat2.match_ptr = &mat->match_ptr[si];
			mat2.branch = mat->branch;
			mat2.branch_end = mat->branch_end;

//xp_printf (XP_T("si = %d [%s]\n"), si, mat->match_ptr);
			tmp = __match_branch_body (rex, p, &mat2);

			if (mat2.matched)
			{
				mat->matched = xp_true;
				mat->match_len = si + mat2.match_len;
				p = tmp;
				break;
			}

			if (si <= lbound) break;
			si--;
		} 
		while (1);
	}

	return p;
}

xp_bool_t __test_charset (const xp_byte_t* p, xp_size_t csc, xp_char_t c)
{
	xp_size_t i;

	for (i = 0; i < csc; i++)
	{
		xp_char_t c0, c1, c2;

		c0 = *(xp_char_t*)p;
		p += xp_sizeof(c0);
		if (c0 == CHARSET_ONE)
		{
			c1 = *(xp_char_t*)p;
			if (c == c1) return xp_true;
		}
		else if (c0 == CHARSET_RANGE)
		{
			c1 = *(xp_char_t*)p;
			p += xp_sizeof(c1);
			c2 = *(xp_char_t*)p;

			if (c >= c1 && c <= c2) return xp_true;
		}
		else if (c0 == CHARSET_CLASS)
		{
			c1 = *(xp_char_t*)p;
			if (__char_class[c1].func (c)) return xp_true;
		}
		else
		{
			xp_assert (!"should never happen - invalid charset code");
			break;
		}

		p += xp_sizeof(c1);
	}

	return xp_false;
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

void xp_awk_rex_print (xp_awk_rex_t* rex)
{
	const xp_byte_t* p;
	p = __print_pattern (rex->code.buf);
	xp_printf (XP_T("\n"));
	xp_assert (p == rex->code.buf + rex->code.size);
}

static const xp_byte_t* __print_pattern (const xp_byte_t* p)
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
	const struct __code_t* cp = (const struct __code_t*)p;

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
				xp_printf (XP_T("[:%s:]"), __char_class[c1].name);
			}
			else
			{
				xp_assert (!"should never happen - invalid charset code");
			}

			p += xp_sizeof(c1);
		}

		xp_printf (XP_T("]"));
	}
	else if (cp->cmd == CMD_GROUP)
	{
		p += xp_sizeof(*cp);
		xp_printf (XP_T("("));
		p = __print_pattern (p);
		xp_printf (XP_T(")"));
	}
	else 
	{
		xp_assert (!"should never happen - invalid atom code");
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
