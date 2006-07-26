/*
 * $Id: rex.c,v 1.16 2006-07-26 15:00:00 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
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

typedef struct __builder_t __builder_t;
typedef struct __matcher_t __matcher_t;
typedef struct __match_t __match_t;

struct __code_t
{
	//xp_byte_t cmd;
	short cmd;
	short negate; /* only for CMD_CHARSET */
	xp_size_t lbound;
	xp_size_t ubound;
};

struct __builder_t
{
	struct
	{
		const xp_char_t* ptr;
		const xp_char_t* end;
		const xp_char_t* curp;
		struct
		{
			int type;
			xp_char_t value;
		} curc;
	} ptn;

	struct
	{
		xp_byte_t* buf;
		xp_size_t  size;
		xp_size_t  capa;
	} code;	

	int errnum;
};

struct __matcher_t
{
	struct
	{
		struct
		{
			const xp_char_t* ptr;
			const xp_char_t* end;
		} str;
	} match;

	int errnum;
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
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);

#define NCHARS_REMAINING(rex) ((rex)->ptn.end - (rex)->ptn.curp)
	
#define NEXT_CHAR(rex,level) \
	do { if (__next_char(rex,level) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (__add_code(rex,data,len) == -1) return -1; } while (0)

#define CODEAT(rex,pos,type) (*((type*)&(rex)->code.buf[pos]))

static int __build_pattern (__builder_t* rex);
static int __build_branch (__builder_t* rex);
static int __build_atom (__builder_t* rex);
static int __build_charset (__builder_t* rex, struct __code_t* cmd);
static int __build_boundary (__builder_t* rex, struct __code_t* cmd);
static int __build_cclass (__builder_t* rex, xp_char_t* cc);
static int __build_range (__builder_t* rex, struct __code_t* cmd);
static int __next_char (__builder_t* rex, int level);
static int __add_code (__builder_t* rex, void* data, xp_size_t len);

static xp_bool_t __begin_with (
	const xp_char_t* str, xp_size_t len, const xp_char_t* what);

static const xp_byte_t* __match_pattern (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_branch (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_branch_body (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_atom (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_bol (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_eol (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_any_char (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_ord_char (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_charset (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);
static const xp_byte_t* __match_group (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat);

static const xp_byte_t* __match_boundary (
	__matcher_t* matcher, xp_size_t si, const xp_byte_t* p,
	xp_size_t lbound, xp_size_t ubound, __match_t* mat);

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

const xp_char_t* xp_awk_getrexerrstr (int errnum)
{
	static const xp_char_t* __errstr[] =
	{
		XP_T("no error"),
		XP_T("out of memory"),
		XP_T("no pattern built"),
		XP_T("a right parenthesis is expected"),
		XP_T("a right bracket is expected"),
		XP_T("a right brace is expected"),
		XP_T("a colon is expected"),
		XP_T("invalid character range"),
		XP_T("invalid character class"),
		XP_T("invalid boundary range"),
		XP_T("unexpected end of the pattern"),
		XP_T("garbage after the pattern")
	};

	if (errnum >= 0 && errnum < xp_countof(__errstr)) 
	{
		return __errstr[errnum];
	}

	return XP_T("unknown error");
}

void* xp_awk_buildrex (const xp_char_t* ptn, xp_size_t len)
{
	__builder_t builder;

	builder.code.capa = 512;
	builder.code.size = 0;
	builder.code.buf = (xp_byte_t*) xp_malloc (builder.code.capa);
	if (builder.code.buf == XP_NULL) return XP_NULL;

	builder.ptn.ptr = ptn;
	builder.ptn.end = builder.ptn.ptr + len;
	builder.ptn.curp = builder.ptn.ptr;

	builder.ptn.curc.type = CT_EOF;
	builder.ptn.curc.value = XP_T('\0');

	//NEXT_CHAR (&builder, LEVEL_TOP);
	if (__next_char (&builder, LEVEL_TOP) == -1) 
	{
		xp_free (builder.code.buf);
		return XP_NULL;
	}

	if (__build_pattern (&builder) == -1) 
	{
		xp_free (builder.code.buf);
		return XP_NULL;
	}

	if (builder.ptn.curc.type != CT_EOF)
	{
		/* garbage after the pattern */
		xp_free (builder.code.buf);
		return XP_NULL;
	}

	return builder.code.buf;
}

int xp_awk_matchrex (void* code,
	const xp_char_t* str, xp_size_t len, 
	const xp_char_t** match_ptr, xp_size_t* match_len)
{
	__matcher_t matcher;
	__match_t mat;
	xp_size_t offset = 0;

	mat.matched = xp_false;

	/* store the source string */
	matcher.match.str.ptr = str;
	matcher.match.str.end = str + len;

/* TODO: shoud it allow an offset here??? */
	mat.match_ptr = str + offset;

	while (mat.match_ptr < matcher.match.str.end)
	{
		if (__match_pattern (
			&matcher, code, &mat) == XP_NULL) return -1;

		if (mat.matched)
		{
			if (match_ptr != XP_NULL) *match_ptr = mat.match_ptr;
			if (match_len != XP_NULL) *match_len = mat.match_len;
			break;
		}

		mat.match_ptr++;
	}

	return (mat.matched)? 1: 0;
}

void xp_awk_printrex (void* rex)
{
	__print_pattern (rex);
	xp_printf (XP_T("\n"));
}

static int __build_pattern (__builder_t* builder)
{
	xp_size_t zero = 0;
	xp_size_t old_size;
	xp_size_t pos_nb, pos_el;
	int n;

	old_size = builder->code.size;

	/* secure space for header and set the header fields to zero */
	pos_nb = builder->code.size;
	ADD_CODE (builder, &zero, xp_sizeof(zero));

	pos_el = builder->code.size;
	ADD_CODE (builder, &zero, xp_sizeof(zero));

	/* handle the first branch */
	n = __build_branch (builder);
	if (n == -1) return -1;
	if (n == 0) 
	{
		/* if the pattern is empty, the control reaches here */
		return 0;
	}

	CODEAT(builder,pos_nb,xp_size_t) += 1;

	/* handle subsequent branches if any */
	while (builder->ptn.curc.type == CT_SPECIAL && 
	       builder->ptn.curc.value == XP_T('|'))
	{
		NEXT_CHAR (builder, LEVEL_TOP);

		n = __build_branch(builder);
		if (n == -1) return -1;
		if (n == 0) 
		{
			/* if the pattern ends with a vertical bar(|),
			 * this block can be reached. however, such a 
			 * pattern is highly discouraged */
			break;
		}

		CODEAT(builder,pos_nb,xp_size_t) += 1;
	}

	CODEAT(builder,pos_el,xp_size_t) = builder->code.size - old_size;
	return 1;
}

static int __build_branch (__builder_t* builder)
{
	int n;
	xp_size_t zero = 0;
	xp_size_t old_size;
	xp_size_t pos_na, pos_bl;
	struct __code_t* cmd;

	old_size = builder->code.size;

	pos_na = builder->code.size;
	ADD_CODE (builder, &zero, xp_sizeof(zero));

	pos_bl = builder->code.size;
	ADD_CODE (builder, &zero, xp_sizeof(zero));

	while (1)
	{
		cmd = (struct __code_t*)&builder->code.buf[builder->code.size];

		n = __build_atom (builder);
		if (n == -1) 
		{
			builder->code.size = old_size;
			return -1;
		}

		if (n == 0) break; /* no atom */

		n = __build_boundary (builder, cmd);
		if (n == -1)
		{
			builder->code.size = old_size;
			return -1;
		}

		/* n == 0  no bound character. just continue */
		/* n == 1  bound has been applied by build_boundary */

		CODEAT(builder,pos_na,xp_size_t) += 1;
	}

	CODEAT(builder,pos_bl,xp_size_t) = builder->code.size - old_size;
	return (builder->code.size == old_size)? 0: 1;
}

static int __build_atom (__builder_t* builder)
{
	int n;
	struct __code_t tmp;

	if (builder->ptn.curc.type == CT_EOF) return 0;

	if (builder->ptn.curc.type == CT_SPECIAL)
	{
		if (builder->ptn.curc.value == XP_T('('))
		{
			tmp.cmd = CMD_GROUP;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, xp_sizeof(tmp));

			NEXT_CHAR (builder, LEVEL_TOP);

			n = __build_pattern (builder);
			if (n == -1) return -1;

			if (builder->ptn.curc.type != CT_SPECIAL || 
			    builder->ptn.curc.value != XP_T(')')) 
			{
				builder->errnum = XP_AWK_REX_ERPAREN;
				return -1;
			}
		}
		else if (builder->ptn.curc.value == XP_T('^'))
		{
			tmp.cmd = CMD_BOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, xp_sizeof(tmp));
		}
		else if (builder->ptn.curc.value == XP_T('$'))
		{
			tmp.cmd = CMD_EOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, xp_sizeof(tmp));
		}
		else if (builder->ptn.curc.value == XP_T('.'))
		{
			tmp.cmd = CMD_ANY_CHAR;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, xp_sizeof(tmp));
		}
		else if (builder->ptn.curc.value == XP_T('['))
		{
			struct __code_t* cmd;

			cmd = (struct __code_t*)&builder->code.buf[builder->code.size];

			tmp.cmd = CMD_CHARSET;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, xp_sizeof(tmp));

			NEXT_CHAR (builder, LEVEL_CHARSET);

			n = __build_charset (builder, cmd);
			if (n == -1) return -1;

			xp_assert (n != 0);

			if (builder->ptn.curc.type != CT_SPECIAL ||
			    builder->ptn.curc.value != XP_T(']'))
			{
				builder->errnum = XP_AWK_REX_ERBRACKET;
				return -1;
			}

		}
		else return 0;

		NEXT_CHAR (builder, LEVEL_TOP);
		return 1;
	}
	else 
	{
		xp_assert (builder->ptn.curc.type == CT_NORMAL);

		tmp.cmd = CMD_ORD_CHAR;
		tmp.negate = 0;
		tmp.lbound = 1;
		tmp.ubound = 1;
		ADD_CODE (builder, &tmp, xp_sizeof(tmp));

		ADD_CODE (builder, &builder->ptn.curc.value, xp_sizeof(builder->ptn.curc.value));
		NEXT_CHAR (builder, LEVEL_TOP);

		return 1;
	}
}

static int __build_charset (__builder_t* builder, struct __code_t* cmd)
{
	xp_size_t zero = 0;
	xp_size_t old_size;
	xp_size_t pos_csc, pos_csl;

	old_size = builder->code.size;

	pos_csc = builder->code.size;
	ADD_CODE (builder, &zero, xp_sizeof(zero));
	pos_csl = builder->code.size;
	ADD_CODE (builder, &zero, xp_sizeof(zero));

	if (builder->ptn.curc.type == CT_NORMAL &&
	    builder->ptn.curc.value == XP_T('^')) 
	{
		cmd->negate = 1;
		NEXT_CHAR (builder, LEVEL_CHARSET);
	}

	while (builder->ptn.curc.type == CT_NORMAL)
	{
		xp_char_t c0, c1, c2;
		int cc = 0;

		c1 = builder->ptn.curc.value;
		NEXT_CHAR(builder, LEVEL_CHARSET);

		if (c1 == XP_T('[') &&
		    builder->ptn.curc.type == CT_NORMAL &&
		    builder->ptn.curc.value == XP_T(':'))
		{
			if (__build_cclass (builder, &c1) == -1) return -1;
			cc = cc | 1;
		}

		c2 = c1;
		if (builder->ptn.curc.type == CT_NORMAL &&
		    builder->ptn.curc.value == XP_T('-'))
		{
			NEXT_CHAR (builder, LEVEL_CHARSET);

			if (builder->ptn.curc.type == CT_NORMAL)
			{
				c2 = builder->ptn.curc.value;
				NEXT_CHAR (builder, LEVEL_CHARSET);

				if (c2 == XP_T('[') &&
				    builder->ptn.curc.type == CT_NORMAL &&
				    builder->ptn.curc.value == XP_T(':'))
				{
					if (__build_cclass (builder, &c2) == -1)
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
				ADD_CODE (builder, &c0, xp_sizeof(c0));
				ADD_CODE (builder, &c1, xp_sizeof(c1));
			}
			else
			{
				c0 = CHARSET_RANGE;
				ADD_CODE (builder, &c0, xp_sizeof(c0));
				ADD_CODE (builder, &c1, xp_sizeof(c1));
				ADD_CODE (builder, &c2, xp_sizeof(c2));
			}
		}
		else if (cc == 1)
		{
			c0 = CHARSET_CLASS;
			ADD_CODE (builder, &c0, xp_sizeof(c0));
			ADD_CODE (builder, &c1, xp_sizeof(c1));
		}
		else
		{
			/* invalid range */
//xp_printf (XP_T("invalid character set range\n"));
			builder->errnum = XP_AWK_REX_ECRANGE;
			return -1;
		}

		CODEAT(builder,pos_csc,xp_size_t) += 1;
	}

	CODEAT(builder,pos_csl,xp_size_t) = builder->code.size - old_size;
	return 1;
}

static int __build_cclass (__builder_t* builder, xp_char_t* cc)
{
	const struct __char_class_t* ccp = __char_class;
	xp_size_t len = builder->ptn.end - builder->ptn.curp;

	while (ccp->name != XP_NULL)
	{
		if (__begin_with (builder->ptn.curp, len, ccp->name)) break;
		ccp++;
	}

	if (ccp->name == XP_NULL)
	{
		/* wrong class name */
//xp_printf (XP_T("wrong class name\n"));
		builder->errnum = XP_AWK_REX_ECCLASS;
		return -1;
	}

	builder->ptn.curp += ccp->name_len;

	NEXT_CHAR (builder, LEVEL_CHARSET);
	if (builder->ptn.curc.type != CT_NORMAL ||
	    builder->ptn.curc.value != XP_T(':'))
	{
//xp_printf (XP_T(": expected\n"));
		builder->errnum = XP_AWK_REX_ECOLON;
		return -1;
	}

	NEXT_CHAR (builder, LEVEL_CHARSET); 
	
	/* ] happens to be the charset ender ] */
	if (builder->ptn.curc.type != CT_SPECIAL ||
	    builder->ptn.curc.value != XP_T(']'))
	{
//xp_printf (XP_T("] expected\n"));
		builder->errnum = XP_AWK_REX_ERBRACKET;	
		return -1;
	}

	NEXT_CHAR (builder, LEVEL_CHARSET);

	*cc = (xp_char_t)(ccp - __char_class);
	return 1;
}

static int __build_boundary (__builder_t* builder, struct __code_t* cmd)
{
	if (builder->ptn.curc.type != CT_SPECIAL) return 0;

	switch (builder->ptn.curc.value)
	{
		case XP_T('+'):
		{
			cmd->lbound = 1;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case XP_T('*'):
		{
			cmd->lbound = 0;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case XP_T('?'):
		{
			cmd->lbound = 0;
			cmd->ubound = 1;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case XP_T('{'):
		{
			NEXT_CHAR (builder, LEVEL_RANGE);

			if (__build_range(builder, cmd) == -1) return -1;

			if (builder->ptn.curc.type != CT_SPECIAL || 
			    builder->ptn.curc.value != XP_T('}')) 
			{
				builder->errnum = XP_AWK_REX_ERBRACE;
				return -1;
			}

			NEXT_CHAR (builder, LEVEL_TOP);
			return 1;
		}
	}

	return 0;
}

static int __build_range (__builder_t* builder, struct __code_t* cmd)
{
	xp_size_t bound;

// TODO: should allow white spaces in the range???
//  what if it is not in the raight format? convert it to ordinary characters??
	bound = 0;
	while (builder->ptn.curc.type == CT_NORMAL &&
	       xp_isdigit(builder->ptn.curc.value))
	{
		bound = bound * 10 + builder->ptn.curc.value - XP_T('0');
		NEXT_CHAR (builder, LEVEL_RANGE);
	}

	cmd->lbound = bound;

	if (builder->ptn.curc.type == CT_SPECIAL &&
	    builder->ptn.curc.value == XP_T(',')) 
	{
		NEXT_CHAR (builder, LEVEL_RANGE);

		bound = 0;
		while (builder->ptn.curc.type == CT_NORMAL &&
		       xp_isdigit(builder->ptn.curc.value))
		{
			bound = bound * 10 + builder->ptn.curc.value - XP_T('0');
			NEXT_CHAR (builder, LEVEL_RANGE);
		}

		cmd->ubound = bound;
	}
	else cmd->ubound = BOUND_MAX;

	if (cmd->lbound > cmd->ubound)
	{
		/* invalid boundary range */
		builder->errnum = XP_AWK_REX_EBRANGE;
		return -1;
	}

	return 0;
}

static int __next_char (__builder_t* builder, int level)
{
	if (builder->ptn.curp >= builder->ptn.end)
	{
		builder->ptn.curc.type = CT_EOF;
		builder->ptn.curc.value = XP_T('\0');
		return 0;
	}

	builder->ptn.curc.type = CT_NORMAL;
	builder->ptn.curc.value = *builder->ptn.curp++;

	if (builder->ptn.curc.value == XP_T('\\'))
	{	       
		if (builder->ptn.curp >= builder->ptn.end)
		{
			builder->errnum = XP_AWK_REX_EEND;
			return -1;	
		}

		builder->ptn.curc.value = *builder->ptn.curp++;
		return 0;
	}
	else
	{
		if (level == LEVEL_TOP)
		{
			if (builder->ptn.curc.value == XP_T('[') ||
			    builder->ptn.curc.value == XP_T('|') ||
			    builder->ptn.curc.value == XP_T('^') ||
			    builder->ptn.curc.value == XP_T('$') ||
			    builder->ptn.curc.value == XP_T('{') ||
			    builder->ptn.curc.value == XP_T('+') ||
			    builder->ptn.curc.value == XP_T('?') ||
			    builder->ptn.curc.value == XP_T('*') ||
			    builder->ptn.curc.value == XP_T('.') ||
			    builder->ptn.curc.value == XP_T('(') ||
			    builder->ptn.curc.value == XP_T(')')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_CHARSET)
		{
			if (builder->ptn.curc.value == XP_T(']')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_RANGE)
		{
			if (builder->ptn.curc.value == XP_T(',') ||
			    builder->ptn.curc.value == XP_T('}')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
	}

	return 0;
}

static int __add_code (__builder_t* builder, void* data, xp_size_t len)
{
	if (len > builder->code.capa - builder->code.size)
	{
		xp_size_t capa = builder->code.capa * 2;
		xp_byte_t* tmp;
		
		if (capa == 0) capa = 1;
		while (len > capa - builder->code.size) { capa = capa * 2; }

		tmp = (xp_byte_t*) xp_realloc (builder->code.buf, capa);
		if (tmp == XP_NULL)
		{
			builder->errnum = XP_AWK_REX_ENOMEM;
			return -1;
		}

		builder->code.buf = tmp;
		builder->code.capa = capa;
	}

	xp_memcpy (&builder->code.buf[builder->code.size], data, len);
	builder->code.size += len;

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

static const xp_byte_t* __match_pattern (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
{
	const xp_byte_t* p;
	__match_t mat2;
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

		p = __match_branch (matcher, p, &mat2);
		if (p == XP_NULL) return XP_NULL;

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
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
{
	/*
	 * branch body (base+sizeof(NA)+sizeof(BL)---+
	 * BL=base+sizeof(NA) ---------+             |
	 * base=NA ------+             |             |
	 *               |             |             |
	 *               |NA(xp_size_t)|BL(xp_size_t)|ATOMS.........|
	 */
	mat->branch = base;
	mat->branch_end = base + *((xp_size_t*)(base+xp_sizeof(xp_size_t)));

	return __match_branch_body (
		matcher, base+xp_sizeof(xp_size_t)*2, mat);
}

static const xp_byte_t* __match_branch_body (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
{
	const xp_byte_t* p;
//	__match_t mat2;
	xp_size_t match_len = 0;

	mat->matched = xp_false;
	mat->match_len = 0;

/* TODO: is mat2 necessary here ? */
/*
	mat2.match_ptr = mat->match_ptr;
	mat2.branch = mat->branch;
	mat2.branch_end = mat->branch_end;
*/

	p = base;

	while (p < mat->branch_end)
	{
		p = __match_atom (matcher, p, mat);
		if (p == XP_NULL) return XP_NULL;

		if (!mat->matched) break;

		mat->match_ptr = &mat->match_ptr[mat->match_len];
		match_len += mat->match_len;
#if 0
		p = __match_atom (matcher, p, &mat2);
		if (p == XP_NULL) return XP_NULL;

		if (!mat2.matched) 
		{
			mat->matched = xp_false;
			break; /* stop matching */
		}

		mat->matched = xp_true;
		mat->match_len += mat2.match_len;

		mat2.match_ptr = &mat2.match_ptr[mat2.match_len];
#endif
	}

	if (mat->matched) mat->match_len = match_len;
	return mat->branch_end;
}

static const xp_byte_t* __match_atom (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
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

	return matchers[((struct __code_t*)base)->cmd] (matcher, base, mat);
}

static const xp_byte_t* __match_bol (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_BOL);

	mat->matched = (mat->match_ptr == matcher->match.str.ptr ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const xp_byte_t* __match_eol (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_EOL);

	mat->matched = (mat->match_ptr == matcher->match.str.end ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const xp_byte_t* __match_any_char (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
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
		if (&mat->match_ptr[si] >= matcher->match.str.end) break;
		si++;
	}

//xp_printf (XP_T("max si = %d\n"), si);
	if (si >= lbound && si <= ubound)
	{
		p = __match_boundary (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const xp_byte_t* __match_ord_char (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
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
		if (&mat->match_ptr[si] >= matcher->match.str.end) break;
		if (cc != mat->match_ptr[si]) break;
		si++;
	}

//xp_printf (XP_T("max si = %d\n"), si);
	if (si >= lbound && si <= ubound)
	{
		p = __match_boundary (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const xp_byte_t* __match_charset (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
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
		if (&mat->match_ptr[si] >= matcher->match.str.end) break;

		n = __test_charset (p, csc, mat->match_ptr[si]);
		if (cp->negate) n = !n;
		if (!n) break;

		si++;
	}

	p = p + csl - (xp_sizeof(csc) + xp_sizeof(csl));

	if (si >= lbound && si <= ubound)
	{
		p = __match_boundary (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const xp_byte_t* __match_group (
	__matcher_t* matcher, const xp_byte_t* base, __match_t* mat)
{
	const xp_byte_t* p = base;
	const struct __code_t* cp;
	__match_t mat2;
	xp_size_t si = 0, grp_len_static[16], * grp_len;

	cp = (const struct __code_t*)p; p += xp_sizeof(*cp);
	xp_assert (cp->cmd == CMD_GROUP);

	mat->matched = xp_false;
	mat->match_len = 0;
	
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

	if (cp->ubound < xp_countof(grp_len_static))
	{
		grp_len = grp_len_static;
	}
	else 
	{
		grp_len = (xp_size_t*) xp_malloc (
			xp_sizeof(xp_size_t) * cp->ubound);
		if (grp_len == XP_NULL)
		{
			matcher->errnum = XP_AWK_REX_ENOMEM;
			return XP_NULL;
		}
	}

	grp_len[si] = 0;

	mat2.match_ptr = mat->match_ptr;
	while (si < cp->ubound)
	{
		if (mat2.match_ptr >= matcher->match.str.end) break;

		if (__match_pattern (matcher, p, &mat2) == XP_NULL) 
		{
			if (grp_len != grp_len_static) xp_free (grp_len);
			return XP_NULL;
		}
		if (!mat2.matched) break;

		grp_len[si+1] = grp_len[si] + mat2.match_len;

		mat2.match_ptr += mat2.match_len;
		mat2.match_len = 0;
		mat2.matched = xp_false;

		si++;
	}

	/* increment p by the length of the subpattern */
	p += *(xp_size_t*)(p+xp_sizeof(xp_size_t));

	/* check the boundary */
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
				tmp = __match_branch_body (matcher, p, &mat2);
				if (tmp == XP_NULL)
				{
					if (grp_len != grp_len_static) 
						xp_free (grp_len);
					return XP_NULL;
				}

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

	if (grp_len != grp_len_static) xp_free (grp_len);
	return p;
}

static const xp_byte_t* __match_boundary (
	__matcher_t* matcher, xp_size_t si, const xp_byte_t* p,
	xp_size_t lbound, xp_size_t ubound, __match_t* mat)
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
			__match_t mat2;
			const xp_byte_t* tmp;

			mat2.match_ptr = &mat->match_ptr[si];
			mat2.branch = mat->branch;
			mat2.branch_end = mat->branch_end;

//xp_printf (XP_T("si = %d [%s]\n"), si, mat->match_ptr);
			tmp = __match_branch_body (matcher, p, &mat2);

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
