/*
 * $Id: rex.c,v 1.37 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

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

#ifdef _MSC_VER
#pragma warning (disable: 4296)
#endif

#define DEF_CODE_CAPA 512
#define BOUND_MIN 0
#define BOUND_MAX (SSE_TYPE_MAX(sse_size_t))

typedef struct __builder_t __builder_t;
typedef struct __matcher_t __matcher_t;
typedef struct __match_t __match_t;

struct __code_t
{
	/*sse_byte_t cmd;*/
	short cmd;
	short negate; /* only for CMD_CHARSET */
	sse_size_t lbound;
	sse_size_t ubound;
};

struct __builder_t
{
	sse_awk_t* awk;

	struct
	{
		const sse_char_t* ptr;
		const sse_char_t* end;
		const sse_char_t* curp;
		struct
		{
			int type;
			sse_char_t value;
		} curc;
	} ptn;

	struct
	{
		sse_byte_t* buf;
		sse_size_t  size;
		sse_size_t  capa;
	} code;	

	struct
	{
		int max;
		int cur;
	} depth;

	int errnum;
};

struct __matcher_t
{
	sse_awk_t* awk;

	struct
	{
		struct
		{
			const sse_char_t* ptr;
			const sse_char_t* end;
		} str;
	} match;

	struct
	{
		int max;
		int cur;
	} depth;

	int ignorecase;
	int errnum;
};

struct __match_t
{
	const sse_char_t* match_ptr;

	sse_bool_t matched;
	sse_size_t match_len;

	const sse_byte_t* branch;
	const sse_byte_t* branch_end;
};

typedef const sse_byte_t* (*atom_matcher_t) (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);

#define NCHARS_REMAINING(rex) ((rex)->ptn.end - (rex)->ptn.curp)
	
#define NEXT_CHAR(rex,level) \
	do { if (__next_char(rex,level) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (__add_code(rex,data,len) == -1) return -1; } while (0)

#if defined(__sparc) || defined(__sparc__)
	#define GET_CODE(rex,pos,type) __get_code(rex,pos)
	#define SET_CODE(rex,pos,type,code) __set_code(rex,pos,code)
#else
	#define GET_CODE(rex,pos,type) (*((type*)&(rex)->code.buf[pos]))
	#define SET_CODE(rex,pos,type,code) (GET_CODE(rex,pos,type) = (code))
#endif

static int __build_pattern (__builder_t* rex);
static int __build_pattern0 (__builder_t* rex);
static int __build_branch (__builder_t* rex);
static int __build_atom (__builder_t* rex);
static int __build_charset (__builder_t* rex, struct __code_t* cmd);
static int __build_occurrences (__builder_t* rex, struct __code_t* cmd);
static int __build_cclass (__builder_t* rex, sse_char_t* cc);
static int __build_range (__builder_t* rex, struct __code_t* cmd);
static int __next_char (__builder_t* rex, int level);
static int __add_code (__builder_t* rex, void* data, sse_size_t len);

#if defined(__sparc) || defined(__sparc__)

static sse_size_t __get_code (__builder_t* builder, sse_size_t pos)
{
	sse_size_t code;
	SSE_AWK_MEMCPY (builder->awk, 
		&code, &builder->code.buf[pos], sse_sizeof(code));
	return code;
}

static void __set_code (__builder_t* builder, sse_size_t pos, sse_size_t code)
{
	SSE_AWK_MEMCPY (builder->awk, 
		&builder->code.buf[pos], &code, sse_sizeof(code));
}

#endif

static sse_bool_t __begin_with (
	const sse_char_t* str, sse_size_t len, const sse_char_t* what);

static const sse_byte_t* __match_pattern (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_branch (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_branch_body (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_branch_body0 (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_atom (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_bol (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_eol (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_any_char (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_ord_char (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_charset (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);
static const sse_byte_t* __match_group (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat);

static const sse_byte_t* __match_occurrences (
	__matcher_t* matcher, sse_size_t si, const sse_byte_t* p,
	sse_size_t lbound, sse_size_t ubound, __match_t* mat);

static sse_bool_t __test_charset (
	__matcher_t* matcher, const sse_byte_t* p, sse_size_t csc, sse_char_t c);

static sse_bool_t __cc_isalnum (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isalpha (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isblank (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_iscntrl (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isdigit (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isgraph (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_islower (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isprint (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_ispunct (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isspace (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isupper (sse_awk_t* awk, sse_char_t c);
static sse_bool_t __cc_isxdigit (sse_awk_t* awk, sse_char_t c);

static const sse_byte_t* __print_pattern (const sse_byte_t* p);
static const sse_byte_t* __print_branch (const sse_byte_t* p);
static const sse_byte_t* __print_atom (const sse_byte_t* p);

struct __char_class_t
{
	const sse_char_t* name;
	sse_size_t name_len;
	sse_bool_t (*func) (sse_awk_t* awk, sse_char_t c);
}; 

static struct __char_class_t __char_class[] =
{
	{ SSE_T("alnum"),  5, __cc_isalnum },
	{ SSE_T("alpha"),  5, __cc_isalpha },
	{ SSE_T("blank"),  5, __cc_isblank },
	{ SSE_T("cntrl"),  5, __cc_iscntrl },
	{ SSE_T("digit"),  5, __cc_isdigit },
	{ SSE_T("graph"),  5, __cc_isgraph },
	{ SSE_T("lower"),  5, __cc_islower },
	{ SSE_T("print"),  5, __cc_isprint },
	{ SSE_T("punct"),  5, __cc_ispunct },
	{ SSE_T("space"),  5, __cc_isspace },
	{ SSE_T("upper"),  5, __cc_isupper },
	{ SSE_T("xdigit"), 6, __cc_isxdigit },

	/*
	{ SSE_T("arabic"),   6, __cc_isarabic },
	{ SSE_T("chinese"),  7, __cc_ischinese },
	{ SSE_T("english"),  7, __cc_isenglish },
	{ SSE_T("japanese"), 8, __cc_isjapanese },
	{ SSE_T("korean"),   6, __cc_iskorean }, 
	{ SSE_T("thai"),     4, __cc_isthai }, 
	*/

	{ SSE_NULL,        0, SSE_NULL }
};

void* sse_awk_buildrex (
	sse_awk_t* awk, const sse_char_t* ptn, sse_size_t len, int* errnum)
{
	__builder_t builder;

	builder.awk = awk;
	builder.code.capa = DEF_CODE_CAPA;
	builder.code.size = 0;
	builder.code.buf = (sse_byte_t*) 
		SSE_AWK_MALLOC (builder.awk, builder.code.capa);
	if (builder.code.buf == SSE_NULL) 
	{
		*errnum = SSE_AWK_ENOMEM;
		return SSE_NULL;
	}

	builder.ptn.ptr = ptn;
	builder.ptn.end = builder.ptn.ptr + len;
	builder.ptn.curp = builder.ptn.ptr;

	builder.ptn.curc.type = CT_EOF;
	builder.ptn.curc.value = SSE_T('\0');

/* TODO: implement the maximum depth 
	builder.depth.max = awk->max_depth; */
	builder.depth.max = 0;
	builder.depth.cur = 0;

	if (__next_char (&builder, LEVEL_TOP) == -1) 
	{
		if (errnum != SSE_NULL) *errnum = builder.errnum;
		SSE_AWK_FREE (builder.awk, builder.code.buf);
		return SSE_NULL;
	}

	if (__build_pattern (&builder) == -1) 
	{
		if (errnum != SSE_NULL) *errnum = builder.errnum;
		SSE_AWK_FREE (builder.awk, builder.code.buf);
		return SSE_NULL;
	}

	if (builder.ptn.curc.type != CT_EOF)
	{
		if (errnum != SSE_NULL) *errnum = SSE_AWK_EREXGARBAGE;
		SSE_AWK_FREE (builder.awk, builder.code.buf);
		return SSE_NULL;
	}

	return builder.code.buf;
}

int sse_awk_matchrex (
	sse_awk_t* awk, void* code, int option,
	const sse_char_t* str, sse_size_t len, 
	const sse_char_t** match_ptr, sse_size_t* match_len, int* errnum)
{
	__matcher_t matcher;
	__match_t mat;
	sse_size_t offset = 0;
	/*const sse_char_t* match_ptr_zero = SSE_NULL;*/

	matcher.awk = awk;

	/* store the source string */
	matcher.match.str.ptr = str;
	matcher.match.str.end = str + len;

/* TODO: implement the maximum depth 
	matcher.depth.max = awk->max_depth; */
	matcher.depth.max = 0;
	matcher.depth.cur = 0;
	matcher.ignorecase = (option & SSE_AWK_REX_IGNORECASE)? 1: 0;

	mat.matched = sse_false;
/* TODO: should it allow an offset here??? */
	mat.match_ptr = str + offset;

	while (mat.match_ptr < matcher.match.str.end)
	{
		if (__match_pattern (&matcher, code, &mat) == SSE_NULL) 
		{
			if (errnum != SSE_NULL) *errnum = matcher.errnum;
			return -1;
		}

		if (mat.matched)
		{
			/*
			if (mat.match_len == 0)
			{
				if (match_ptr_zero == SSE_NULL)
					match_ptr_zero = mat.match_ptr;
				mat.match_ptr++;
				continue;
			}
			*/

			if (match_ptr != SSE_NULL) *match_ptr = mat.match_ptr;
			if (match_len != SSE_NULL) *match_len = mat.match_len;

			/*match_ptr_zero = SSE_NULL;*/
			break;
		}

		mat.match_ptr++;
	}

	/*
	if (match_ptr_zero != SSE_NULL) 
	{
		if (match_ptr != SSE_NULL) *match_ptr = match_ptr_zero;
		if (match_len != SSE_NULL) *match_len = 0;
		return 1;
	}
	*/

	return (mat.matched)? 1: 0;
}

void sse_awk_freerex (sse_awk_t* awk, void* code)
{
	sse_awk_assert (awk, code != SSE_NULL);
	SSE_AWK_FREE (awk, code);
}

sse_bool_t sse_awk_isemptyrex (sse_awk_t* awk, void* code)
{
	const sse_byte_t* p = code;
	sse_size_t nb, el;

	sse_awk_assert (awk, p != SSE_NULL);

	nb = *(sse_size_t*)p; p += sse_sizeof(nb);
	el = *(sse_size_t*)p; p += sse_sizeof(el);

	/* an empty regular esseression look like:
	 *  | esseression                     | 
	 *  | header         | branch        |
	 *  |                | branch header |
	 *  | NB(1) | EL(16) | NA(1) | BL(8) | */
	return (nb == 1 && el == sse_sizeof(sse_size_t)*4)? sse_true: sse_false;
}

static int __build_pattern (__builder_t* builder)
{
	int n;

	if (builder->depth.max > 0 && builder->depth.cur >= builder->depth.max)
	{
		builder->errnum = SSE_AWK_ERECURSION;
		return -1;
	}

	builder->depth.cur++;
	n = __build_pattern0 (builder);
	builder->depth.cur--;

	return n;
}

static int __build_pattern0 (__builder_t* builder)
{
	sse_size_t zero = 0;
	sse_size_t old_size;
	sse_size_t pos_nb, pos_el;
	int n;


	old_size = builder->code.size;

	/* secure space for header and set the header fields to zero */
	pos_nb = builder->code.size;
	ADD_CODE (builder, &zero, sse_sizeof(zero));

	pos_el = builder->code.size;
	ADD_CODE (builder, &zero, sse_sizeof(zero));

	/* handle the first branch */
	n = __build_branch (builder);
	if (n == -1) return -1;
	if (n == 0) 
	{
		/* if the pattern is empty, the control reaches here */
		return 0;
	}

	/*CODEAT(builder,pos_nb,sse_size_t) += 1;*/
	SET_CODE (builder, pos_nb, sse_size_t,
		GET_CODE (builder, pos_nb, sse_size_t) + 1);

	/* handle subsequent branches if any */
	while (builder->ptn.curc.type == CT_SPECIAL && 
	       builder->ptn.curc.value == SSE_T('|'))
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

		/*CODEAT(builder,pos_nb,sse_size_t) += 1;*/
		SET_CODE (builder, pos_nb, sse_size_t, 
			GET_CODE (builder, pos_nb, sse_size_t) + 1);
	}

	/*CODEAT(builder,pos_el,sse_size_t) = builder->code.size - old_size;*/
	SET_CODE (builder, pos_el, sse_size_t, builder->code.size - old_size);
	return 1;
}

static int __build_branch (__builder_t* builder)
{
	int n;
	sse_size_t zero = 0;
	sse_size_t old_size;
	sse_size_t pos_na, pos_bl;
	struct __code_t* cmd;

	old_size = builder->code.size;

	pos_na = builder->code.size;
	ADD_CODE (builder, &zero, sse_sizeof(zero));

	pos_bl = builder->code.size;
	ADD_CODE (builder, &zero, sse_sizeof(zero));

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

		n = __build_occurrences (builder, cmd);
		if (n == -1)
		{
			builder->code.size = old_size;
			return -1;
		}

		/* n == 0  no bound character. just continue */
		/* n == 1  bound has been applied by build_occurrences */

		/*CODEAT(builder,pos_na,sse_size_t) += 1;*/
		SET_CODE (builder, pos_na, sse_size_t,
			GET_CODE (builder, pos_na, sse_size_t) + 1);
	}

	/*CODEAT(builder,pos_bl,sse_size_t) = builder->code.size - old_size;*/
	SET_CODE (builder, pos_bl, sse_size_t, builder->code.size - old_size);
	return (builder->code.size == old_size)? 0: 1;
}

static int __build_atom (__builder_t* builder)
{
	int n;
	struct __code_t tmp;

	if (builder->ptn.curc.type == CT_EOF) return 0;

	if (builder->ptn.curc.type == CT_SPECIAL)
	{
		if (builder->ptn.curc.value == SSE_T('('))
		{
			tmp.cmd = CMD_GROUP;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, sse_sizeof(tmp));

			NEXT_CHAR (builder, LEVEL_TOP);

			n = __build_pattern (builder);
			if (n == -1) return -1;

			if (builder->ptn.curc.type != CT_SPECIAL || 
			    builder->ptn.curc.value != SSE_T(')')) 
			{
				builder->errnum = SSE_AWK_EREXRPAREN;
				return -1;
			}
		}
		else if (builder->ptn.curc.value == SSE_T('^'))
		{
			tmp.cmd = CMD_BOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, sse_sizeof(tmp));
		}
		else if (builder->ptn.curc.value == SSE_T('$'))
		{
			tmp.cmd = CMD_EOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, sse_sizeof(tmp));
		}
		else if (builder->ptn.curc.value == SSE_T('.'))
		{
			tmp.cmd = CMD_ANY_CHAR;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, sse_sizeof(tmp));
		}
		else if (builder->ptn.curc.value == SSE_T('['))
		{
			struct __code_t* cmd;

			cmd = (struct __code_t*)&builder->code.buf[builder->code.size];

			tmp.cmd = CMD_CHARSET;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, sse_sizeof(tmp));

			NEXT_CHAR (builder, LEVEL_CHARSET);

			n = __build_charset (builder, cmd);
			if (n == -1) return -1;

			sse_awk_assert (builder->awk, n != 0);

			if (builder->ptn.curc.type != CT_SPECIAL ||
			    builder->ptn.curc.value != SSE_T(']'))
			{
				builder->errnum = SSE_AWK_EREXRBRACKET;
				return -1;
			}

		}
		else return 0;

		NEXT_CHAR (builder, LEVEL_TOP);
		return 1;
	}
	else 
	{
		sse_awk_assert (builder->awk, builder->ptn.curc.type == CT_NORMAL);

		tmp.cmd = CMD_ORD_CHAR;
		tmp.negate = 0;
		tmp.lbound = 1;
		tmp.ubound = 1;
		ADD_CODE (builder, &tmp, sse_sizeof(tmp));

		ADD_CODE (builder, &builder->ptn.curc.value, sse_sizeof(builder->ptn.curc.value));
		NEXT_CHAR (builder, LEVEL_TOP);

		return 1;
	}
}

static int __build_charset (__builder_t* builder, struct __code_t* cmd)
{
	sse_size_t zero = 0;
	sse_size_t old_size;
	sse_size_t pos_csc, pos_csl;

	old_size = builder->code.size;

	pos_csc = builder->code.size;
	ADD_CODE (builder, &zero, sse_sizeof(zero));

	pos_csl = builder->code.size;
	ADD_CODE (builder, &zero, sse_sizeof(zero));

	if (builder->ptn.curc.type == CT_NORMAL &&
	    builder->ptn.curc.value == SSE_T('^')) 
	{
		cmd->negate = 1;
		NEXT_CHAR (builder, LEVEL_CHARSET);
	}

	while (builder->ptn.curc.type == CT_NORMAL)
	{
		sse_char_t c0, c1, c2;
		int cc = 0;

		c1 = builder->ptn.curc.value;
		NEXT_CHAR(builder, LEVEL_CHARSET);

		if (c1 == SSE_T('[') &&
		    builder->ptn.curc.type == CT_NORMAL &&
		    builder->ptn.curc.value == SSE_T(':'))
		{
			if (__build_cclass (builder, &c1) == -1) return -1;
			cc = cc | 1;
		}

		c2 = c1;
		if (builder->ptn.curc.type == CT_NORMAL &&
		    builder->ptn.curc.value == SSE_T('-'))
		{
			NEXT_CHAR (builder, LEVEL_CHARSET);

			if (builder->ptn.curc.type == CT_NORMAL)
			{
				c2 = builder->ptn.curc.value;
				NEXT_CHAR (builder, LEVEL_CHARSET);

				if (c2 == SSE_T('[') &&
				    builder->ptn.curc.type == CT_NORMAL &&
				    builder->ptn.curc.value == SSE_T(':'))
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
				ADD_CODE (builder, &c0, sse_sizeof(c0));
				ADD_CODE (builder, &c1, sse_sizeof(c1));
			}
			else
			{
				c0 = CHARSET_RANGE;
				ADD_CODE (builder, &c0, sse_sizeof(c0));
				ADD_CODE (builder, &c1, sse_sizeof(c1));
				ADD_CODE (builder, &c2, sse_sizeof(c2));
			}
		}
		else if (cc == 1)
		{
			c0 = CHARSET_CLASS;
			ADD_CODE (builder, &c0, sse_sizeof(c0));
			ADD_CODE (builder, &c1, sse_sizeof(c1));
		}
		else
		{
			/* invalid range */
#ifdef DEBUG_REX
sse_printf (SSE_T("__build_charset: invalid character set range\n"));
#endif
			builder->errnum = SSE_AWK_EREXCRANGE;
			return -1;
		}

		/*CODEAT(builder,pos_csc,sse_size_t) += 1;*/
		SET_CODE (builder, pos_csc, sse_size_t,
			GET_CODE (builder, pos_csc, sse_size_t) + 1);
	}

	/*CODEAT(builder,pos_csl,sse_size_t) = builder->code.size - old_size;*/
	SET_CODE (builder, pos_csl, sse_size_t, builder->code.size - old_size);

	return 1;
}

static int __build_cclass (__builder_t* builder, sse_char_t* cc)
{
	const struct __char_class_t* ccp = __char_class;
	sse_size_t len = builder->ptn.end - builder->ptn.curp;

	while (ccp->name != SSE_NULL)
	{
		if (__begin_with (builder->ptn.curp, len, ccp->name)) break;
		ccp++;
	}

	if (ccp->name == SSE_NULL)
	{
		/* wrong class name */
#ifdef DEBUG_REX
sse_printf (SSE_T("__build_cclass: wrong class name\n"));*/
#endif
		builder->errnum = SSE_AWK_EREXCCLASS;
		return -1;
	}

	builder->ptn.curp += ccp->name_len;

	NEXT_CHAR (builder, LEVEL_CHARSET);
	if (builder->ptn.curc.type != CT_NORMAL ||
	    builder->ptn.curc.value != SSE_T(':'))
	{
#ifdef BUILD_REX
sse_printf (SSE_T("__build_cclass: a colon(:) esseected\n"));
#endif
		builder->errnum = SSE_AWK_EREXCOLON;
		return -1;
	}

	NEXT_CHAR (builder, LEVEL_CHARSET); 
	
	/* ] happens to be the charset ender ] */
	if (builder->ptn.curc.type != CT_SPECIAL ||
	    builder->ptn.curc.value != SSE_T(']'))
	{
#ifdef DEBUG_REX
sse_printf (SSE_T("__build_cclass: ] esseected\n"));
#endif
		builder->errnum = SSE_AWK_EREXRBRACKET;	
		return -1;
	}

	NEXT_CHAR (builder, LEVEL_CHARSET);

	*cc = (sse_char_t)(ccp - __char_class);
	return 1;
}

static int __build_occurrences (__builder_t* builder, struct __code_t* cmd)
{
	if (builder->ptn.curc.type != CT_SPECIAL) return 0;

	switch (builder->ptn.curc.value)
	{
		case SSE_T('+'):
		{
			cmd->lbound = 1;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case SSE_T('*'):
		{
			cmd->lbound = 0;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case SSE_T('?'):
		{
			cmd->lbound = 0;
			cmd->ubound = 1;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case SSE_T('{'):
		{
			NEXT_CHAR (builder, LEVEL_RANGE);

			if (__build_range(builder, cmd) == -1) return -1;

			if (builder->ptn.curc.type != CT_SPECIAL || 
			    builder->ptn.curc.value != SSE_T('}')) 
			{
				builder->errnum = SSE_AWK_EREXRBRACE;
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
	sse_size_t bound;

/* TODO: should allow white spaces in the range???
what if it is not in the raight format? convert it to ordinary characters?? */
	bound = 0;
	while (builder->ptn.curc.type == CT_NORMAL &&
	       (builder->ptn.curc.value >= SSE_T('0') && 
	        builder->ptn.curc.value <= SSE_T('9')))
	{
		bound = bound * 10 + builder->ptn.curc.value - SSE_T('0');
		NEXT_CHAR (builder, LEVEL_RANGE);
	}

	cmd->lbound = bound;

	if (builder->ptn.curc.type == CT_SPECIAL &&
	    builder->ptn.curc.value == SSE_T(',')) 
	{
		NEXT_CHAR (builder, LEVEL_RANGE);

		bound = 0;
		while (builder->ptn.curc.type == CT_NORMAL &&
		       (builder->ptn.curc.value >= SSE_T('0') && 
		        builder->ptn.curc.value <= SSE_T('9')))
		{
			bound = bound * 10 + builder->ptn.curc.value - SSE_T('0');
			NEXT_CHAR (builder, LEVEL_RANGE);
		}

		cmd->ubound = bound;
	}
	else cmd->ubound = BOUND_MAX;

	if (cmd->lbound > cmd->ubound)
	{
		/* invalid occurrences range */
		builder->errnum = SSE_AWK_EREXBRANGE;
		return -1;
	}

	return 0;
}

static int __next_char (__builder_t* builder, int level)
{
	if (builder->ptn.curp >= builder->ptn.end)
	{
		builder->ptn.curc.type = CT_EOF;
		builder->ptn.curc.value = SSE_T('\0');
		return 0;
	}

	builder->ptn.curc.type = CT_NORMAL;
	builder->ptn.curc.value = *builder->ptn.curp++;

	if (builder->ptn.curc.value == SSE_T('\\'))
	{	       
		if (builder->ptn.curp >= builder->ptn.end)
		{
			builder->errnum = SSE_AWK_EREXEND;
			return -1;	
		}

		builder->ptn.curc.value = *builder->ptn.curp++;
		return 0;
	}
	else
	{
		if (level == LEVEL_TOP)
		{
			if (builder->ptn.curc.value == SSE_T('[') ||
			    builder->ptn.curc.value == SSE_T('|') ||
			    builder->ptn.curc.value == SSE_T('^') ||
			    builder->ptn.curc.value == SSE_T('$') ||
			    builder->ptn.curc.value == SSE_T('{') ||
			    builder->ptn.curc.value == SSE_T('+') ||
			    builder->ptn.curc.value == SSE_T('?') ||
			    builder->ptn.curc.value == SSE_T('*') ||
			    builder->ptn.curc.value == SSE_T('.') ||
			    builder->ptn.curc.value == SSE_T('(') ||
			    builder->ptn.curc.value == SSE_T(')')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_CHARSET)
		{
			if (builder->ptn.curc.value == SSE_T(']')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_RANGE)
		{
			if (builder->ptn.curc.value == SSE_T(',') ||
			    builder->ptn.curc.value == SSE_T('}')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
	}

	return 0;
}

static int __add_code (__builder_t* builder, void* data, sse_size_t len)
{
	if (len > builder->code.capa - builder->code.size)
	{
		sse_size_t capa = builder->code.capa * 2;
		sse_byte_t* tmp;
		
		if (capa == 0) capa = DEF_CODE_CAPA;
		while (len > capa - builder->code.size) { capa = capa * 2; }

		if (builder->awk->syscas.realloc != SSE_NULL)
		{
			tmp = (sse_byte_t*) SSE_AWK_REALLOC (
				builder->awk, builder->code.buf, capa);
			if (tmp == SSE_NULL)
			{
				builder->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
		}
		else
		{
			tmp = (sse_byte_t*) SSE_AWK_MALLOC (builder->awk, capa);
			if (tmp == SSE_NULL)
			{
				builder->errnum = SSE_AWK_ENOMEM;
				return -1;
			}

			if (builder->code.buf != SSE_NULL)
			{
				SSE_AWK_MEMCPY (builder->awk, tmp, 
					builder->code.buf, builder->code.capa);
				SSE_AWK_FREE (builder->awk, builder->code.buf);
			}
		}

		builder->code.buf = tmp;
		builder->code.capa = capa;
	}

	SSE_AWK_MEMCPY (builder->awk, 
		&builder->code.buf[builder->code.size], data, len);
	builder->code.size += len;

	return 0;
}

static sse_bool_t __begin_with (
	const sse_char_t* str, sse_size_t len, const sse_char_t* what)
{
	const sse_char_t* end = str + len;

	while (str < end)
	{
		if (*what == SSE_T('\0')) return sse_true;
		if (*what != *str) return sse_false;

		str++; what++;
	}

	if (*what == SSE_T('\0')) return sse_true;
	return sse_false;
}

static const sse_byte_t* __match_pattern (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p;
	__match_t mat2;
	sse_size_t nb, el, i;

	p = base;
	nb = *(sse_size_t*)p; p += sse_sizeof(nb);
	el = *(sse_size_t*)p; p += sse_sizeof(el);

#ifdef BUILD_REX
sse_printf (SSE_T("__match_pattern: NB = %u, EL = %u\n"), (unsigned)nb, (unsigned)el);
#endif
	mat->matched = sse_false;
	mat->match_len = 0;

	for (i = 0; i < nb; i++)
	{
		mat2.match_ptr = mat->match_ptr;

		p = __match_branch (matcher, p, &mat2);
		if (p == SSE_NULL) return SSE_NULL;

		if (mat2.matched)
		{
			mat->matched = sse_true;
			mat->match_len = mat2.match_len;
			break;
		}
	}

	return base + el;
}

static const sse_byte_t* __match_branch (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	/*
	 * branch body (base+sizeof(NA)+sizeof(BL)---+
	 * BL=base+sizeof(NA) ---------+             |
	 * base=NA ------+             |             |
	 *               |             |             |
	 *               |NA(sse_size_t)|BL(sse_size_t)|ATOMS.........|
	 */
	mat->branch = base;
	mat->branch_end = base + *((sse_size_t*)(base+sse_sizeof(sse_size_t)));

	return __match_branch_body (
		matcher, base+sse_sizeof(sse_size_t)*2, mat);
}

static const sse_byte_t* __match_branch_body (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* n;

	if (matcher->depth.max > 0 && matcher->depth.cur >= matcher->depth.max)
	{
		matcher->errnum = SSE_AWK_ERECURSION;
		return SSE_NULL;
	}

	matcher->depth.cur++;
	n = __match_branch_body0 (matcher, base, mat);
	matcher->depth.cur--;

	return n;
}

static const sse_byte_t* __match_branch_body0 (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p;
/*	__match_t mat2;*/
	sse_size_t match_len = 0;

	mat->matched = sse_false;
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
		if (p == SSE_NULL) return SSE_NULL;

		if (!mat->matched) break;

		mat->match_ptr = &mat->match_ptr[mat->match_len];
		match_len += mat->match_len;
#if 0
		p = __match_atom (matcher, p, &mat2);
		if (p == SSE_NULL) return SSE_NULL;

		if (!mat2.matched) 
		{
			mat->matched = sse_false;
			break; /* stop matching */
		}

		mat->matched = sse_true;
		mat->match_len += mat2.match_len;

		mat2.match_ptr = &mat2.match_ptr[mat2.match_len];
#endif
	}

	if (mat->matched) mat->match_len = match_len;
	return mat->branch_end;
}

static const sse_byte_t* __match_atom (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
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
       
	sse_awk_assert (matcher->awk, 
		((struct __code_t*)base)->cmd >= 0 && 
		((struct __code_t*)base)->cmd < sse_countof(matchers));

	return matchers[((struct __code_t*)base)->cmd] (matcher, base, mat);
}

static const sse_byte_t* __match_bol (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p = base;
	const struct __code_t* cp;

	cp = (const struct __code_t*)p; p += sse_sizeof(*cp);
	sse_awk_assert (matcher->awk, cp->cmd == CMD_BOL);

	mat->matched = (mat->match_ptr == matcher->match.str.ptr ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const sse_byte_t* __match_eol (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p = base;
	const struct __code_t* cp;

	cp = (const struct __code_t*)p; p += sse_sizeof(*cp);
	sse_awk_assert (matcher->awk, cp->cmd == CMD_EOL);

	mat->matched = (mat->match_ptr == matcher->match.str.end ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const sse_byte_t* __match_any_char (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p = base;
	const struct __code_t* cp;
	sse_size_t si = 0, lbound, ubound;

	cp = (const struct __code_t*)p; p += sse_sizeof(*cp);
	sse_awk_assert (matcher->awk, cp->cmd == CMD_ANY_CHAR);

	lbound = cp->lbound;
	ubound = cp->ubound;

	mat->matched = sse_false;
	mat->match_len = 0;

	/* merge the same consecutive codes */
	while (p < mat->branch_end &&
	       cp->cmd == ((const struct __code_t*)p)->cmd)
	{
		lbound += ((const struct __code_t*)p)->lbound;
		ubound += ((const struct __code_t*)p)->ubound;

		p += sse_sizeof(*cp);
	}

#ifdef BUILD_REX
sse_printf (SSE_T("__match_any_char: lbound = %u, ubound = %u\n"), 
      (unsigned int)lbound, (unsigned int)ubound);
#endif

	/* find the longest match */
	while (si < ubound)
	{
		if (&mat->match_ptr[si] >= matcher->match.str.end) break;
		si++;
	}

#ifdef BUILD_REX
sse_printf (SSE_T("__match_any_char: max si = %d\n"), si);
#endif
	if (si >= lbound && si <= ubound)
	{
		p = __match_occurrences (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const sse_byte_t* __match_ord_char (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p = base;
	const struct __code_t* cp;
	sse_size_t si = 0, lbound, ubound;
	sse_char_t cc;

	cp = (const struct __code_t*)p; p += sse_sizeof(*cp);
	sse_awk_assert (matcher->awk, cp->cmd == CMD_ORD_CHAR);

	lbound = cp->lbound; 
	ubound = cp->ubound;

	cc = *(sse_char_t*)p; p += sse_sizeof(cc);
	if (matcher->ignorecase) cc = SSE_AWK_TOUPPER(matcher->awk, cc);

	/* merge the same consecutive codes 
	 * for example, a{1,10}a{0,10} is shortened to a{1,20} 
	 */
	if (matcher->ignorecase) 
	{
		while (p < mat->branch_end &&
		       cp->cmd == ((const struct __code_t*)p)->cmd)
		{
			if (SSE_AWK_TOUPPER (matcher->awk, *(sse_char_t*)(p+sse_sizeof(*cp))) != cc) break;

			lbound += ((const struct __code_t*)p)->lbound;
			ubound += ((const struct __code_t*)p)->ubound;

			p += sse_sizeof(*cp) + sse_sizeof(cc);
		}
	}
	else
	{
		while (p < mat->branch_end &&
		       cp->cmd == ((const struct __code_t*)p)->cmd)
		{
			if (*(sse_char_t*)(p+sse_sizeof(*cp)) != cc) break;

			lbound += ((const struct __code_t*)p)->lbound;
			ubound += ((const struct __code_t*)p)->ubound;

			p += sse_sizeof(*cp) + sse_sizeof(cc);
		}
	}
	
#ifdef BUILD_REX
sse_printf (SSE_T("__match_ord_char: lbound = %u, ubound = %u\n"), 
  (unsigned int)lbound, (unsigned int)ubound);*/
#endif

	mat->matched = sse_false;
	mat->match_len = 0;

	/* find the longest match */
	if (matcher->ignorecase) 
	{
		while (si < ubound)
		{
			if (&mat->match_ptr[si] >= matcher->match.str.end) break;
			if (cc != SSE_AWK_TOUPPER (matcher->awk, mat->match_ptr[si])) break;
			si++;
		}
	}
	else
	{
		while (si < ubound)
		{
			if (&mat->match_ptr[si] >= matcher->match.str.end) break;
			if (cc != mat->match_ptr[si]) break;
			si++;
		}
	}

#ifdef DEBUG_REX
sse_printf (SSE_T("__match_ord_char: max si = %d, lbound = %u, ubound = %u\n"), si, lbound, ubound);
#endif

	if (si >= lbound && si <= ubound)
	{
		p = __match_occurrences (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const sse_byte_t* __match_charset (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p = base;
	const struct __code_t* cp;
	sse_size_t si = 0, lbound, ubound, csc, csl;
	sse_bool_t n;
	sse_char_t c;

	cp = (const struct __code_t*)p; p += sse_sizeof(*cp);
	sse_awk_assert (matcher->awk, cp->cmd == CMD_CHARSET);

	lbound = cp->lbound;
	ubound = cp->ubound;

	csc = *(sse_size_t*)p; p += sse_sizeof(csc);
	csl = *(sse_size_t*)p; p += sse_sizeof(csl);

	mat->matched = sse_false;
	mat->match_len = 0;

	while (si < ubound)
	{
		if (&mat->match_ptr[si] >= matcher->match.str.end) break;

		c = mat->match_ptr[si];
		if (matcher->ignorecase) c = SSE_AWK_TOUPPER(matcher->awk, c);

		n = __test_charset (matcher, p, csc, c);
		if (cp->negate) n = !n;
		if (!n) break;

		si++;
	}

	p = p + csl - (sse_sizeof(csc) + sse_sizeof(csl));

	if (si >= lbound && si <= ubound)
	{
		p = __match_occurrences (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const sse_byte_t* __match_group (
	__matcher_t* matcher, const sse_byte_t* base, __match_t* mat)
{
	const sse_byte_t* p = base;
	const struct __code_t* cp;
	__match_t mat2;
	sse_size_t si = 0, grp_len_static[16], * grp_len;

	cp = (const struct __code_t*)p; p += sse_sizeof(*cp);
	sse_awk_assert (matcher->awk, cp->cmd == CMD_GROUP);

	mat->matched = sse_false;
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

	if (cp->ubound < sse_countof(grp_len_static))
	{
		grp_len = grp_len_static;
	}
	else 
	{
		grp_len = (sse_size_t*) SSE_AWK_MALLOC (
			matcher->awk, sse_sizeof(sse_size_t) * cp->ubound);
		if (grp_len == SSE_NULL)
		{
			matcher->errnum = SSE_AWK_ENOMEM;
			return SSE_NULL;
		}
	}

	grp_len[si] = 0;

	mat2.match_ptr = mat->match_ptr;
	while (si < cp->ubound)
	{
		if (mat2.match_ptr >= matcher->match.str.end) break;

		if (__match_pattern (matcher, p, &mat2) == SSE_NULL) 
		{
			if (grp_len != grp_len_static) 
				SSE_AWK_FREE (matcher->awk, grp_len);
			return SSE_NULL;
		}
		if (!mat2.matched) break;

		grp_len[si+1] = grp_len[si] + mat2.match_len;

		mat2.match_ptr += mat2.match_len;
		mat2.match_len = 0;
		mat2.matched = sse_false;

		si++;
	}

	/* increment p by the length of the subpattern */
	p += *(sse_size_t*)(p+sse_sizeof(sse_size_t));

	/* check the occurrences */
	if (si >= cp->lbound && si <= cp->ubound)
	{
		if (cp->lbound == cp->ubound || p >= mat->branch_end)
		{
			mat->matched = sse_true;
			mat->match_len = grp_len[si];
		}
		else 
		{
			sse_awk_assert (matcher->awk, cp->ubound > cp->lbound);

			do
			{
				const sse_byte_t* tmp;
	
				mat2.match_ptr = &mat->match_ptr[grp_len[si]];
				mat2.branch = mat->branch;
				mat2.branch_end = mat->branch_end;
	
#ifdef DEBUG_REX
sse_printf (SSE_T("__match_group: GROUP si = %d [%s]\n"), si, mat->match_ptr);
#endif
				tmp = __match_branch_body (matcher, p, &mat2);
				if (tmp == SSE_NULL)
				{
					if (grp_len != grp_len_static) 
						SSE_AWK_FREE (matcher->awk, grp_len);
					return SSE_NULL;
				}

				if (mat2.matched)
				{
					mat->matched = sse_true;
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

	if (grp_len != grp_len_static) SSE_AWK_FREE (matcher->awk, grp_len);
	return p;
}

static const sse_byte_t* __match_occurrences (
	__matcher_t* matcher, sse_size_t si, const sse_byte_t* p,
	sse_size_t lbound, sse_size_t ubound, __match_t* mat)
{
	sse_awk_assert (matcher->awk, si >= lbound && si <= ubound);
	/* the match has been found */

	if (lbound == ubound || p >= mat->branch_end)
	{
		/* if the match for fixed occurrences was 
		 * requested or no atoms remain unchecked in 
		 * the branch, the match is returned. */
		mat->matched = sse_true;
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

		sse_awk_assert (matcher->awk, ubound > lbound);

		do
		{
			__match_t mat2;
			const sse_byte_t* tmp;

			mat2.match_ptr = &mat->match_ptr[si];
			mat2.branch = mat->branch;
			mat2.branch_end = mat->branch_end;

#ifdef DEBUG_REX
sse_printf (SSE_T("__match occurrences: si = %d [%s]\n"), si, mat->match_ptr);
#endif
			tmp = __match_branch_body (matcher, p, &mat2);

			if (mat2.matched)
			{
				mat->matched = sse_true;
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

sse_bool_t __test_charset (
	__matcher_t* matcher, const sse_byte_t* p, sse_size_t csc, sse_char_t c)
{
	sse_size_t i;

	for (i = 0; i < csc; i++)
	{
		sse_char_t c0, c1, c2;

		c0 = *(sse_char_t*)p;
		p += sse_sizeof(c0);
		if (c0 == CHARSET_ONE)
		{
			c1 = *(sse_char_t*)p;
			if (matcher->ignorecase) 
				c1 = SSE_AWK_TOUPPER(matcher->awk, c1);
			if (c == c1) return sse_true;
		}
		else if (c0 == CHARSET_RANGE)
		{
			c1 = *(sse_char_t*)p;
			p += sse_sizeof(c1);
			c2 = *(sse_char_t*)p;

			if (matcher->ignorecase) 
			{
				c1 = SSE_AWK_TOUPPER(matcher->awk, c1);
				c2 = SSE_AWK_TOUPPER(matcher->awk, c2);
			}
			if (c >= c1 && c <= c2) return sse_true;
		}
		else if (c0 == CHARSET_CLASS)
		{
			c1 = *(sse_char_t*)p;
			if (__char_class[c1].func (
				matcher->awk, c)) return sse_true;
		}
		else
		{
			sse_awk_assert (matcher->awk,
				!"should never happen - invalid charset code");
			break;
		}

		p += sse_sizeof(c1);
	}

	return sse_false;
}

static sse_bool_t __cc_isalnum (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISALNUM (awk, c);
}

static sse_bool_t __cc_isalpha (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISALPHA (awk, c);
}

static sse_bool_t __cc_isblank (sse_awk_t* awk, sse_char_t c)
{
	return c == SSE_T(' ') || c == SSE_T('\t');
}

static sse_bool_t __cc_iscntrl (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISCNTRL (awk, c);
}

static sse_bool_t __cc_isdigit (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISDIGIT (awk, c);
}

static sse_bool_t __cc_isgraph (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISGRAPH (awk, c);
}

static sse_bool_t __cc_islower (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISLOWER (awk, c);
}

static sse_bool_t __cc_isprint (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISPRINT (awk, c);
}

static sse_bool_t __cc_ispunct (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISPUNCT (awk, c);
}

static sse_bool_t __cc_isspace (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISSPACE (awk, c);
}

static sse_bool_t __cc_isupper (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISUPPER (awk, c);
}

static sse_bool_t __cc_isxdigit (sse_awk_t* awk, sse_char_t c)
{
	return SSE_AWK_ISXDIGIT (awk, c);
}

void sse_awk_printrex (void* rex)
{
	__print_pattern (rex);
	sse_printf (SSE_T("\n"));
}

static const sse_byte_t* __print_pattern (const sse_byte_t* p)
{
	sse_size_t nb, el, i;

	nb = *(sse_size_t*)p; p += sse_sizeof(nb);
	el = *(sse_size_t*)p; p += sse_sizeof(el);
#ifdef DEBUG_REX
sse_printf (SSE_T("__print_pattern: NB = %u, EL = %u\n"), (unsigned int)nb, (unsigned int)el);
#endif

	for (i = 0; i < nb; i++)
	{
		if (i != 0) sse_printf (SSE_T("|"));
		p = __print_branch (p);
	}

	return p;
}

static const sse_byte_t* __print_branch (const sse_byte_t* p)
{
	sse_size_t na, bl, i;

	na = *(sse_size_t*)p; p += sse_sizeof(na);
	bl = *(sse_size_t*)p; p += sse_sizeof(bl);
#ifdef DEBUG_REX
sse_printf (SSE_T("__print_branch: NA = %u, BL = %u\n"), (unsigned int) na, (unsigned int)bl);
#endif

	for (i = 0; i < na; i++)
	{
		p = __print_atom (p);
	}

	return p;
}

static const sse_byte_t* __print_atom (const sse_byte_t* p)
{
	const struct __code_t* cp = (const struct __code_t*)p;

	if (cp->cmd == CMD_BOL)
	{
		sse_printf (SSE_T("^"));
		p += sse_sizeof(*cp);
	}
	else if (cp->cmd == CMD_EOL)
	{
		sse_printf (SSE_T("$"));
		p += sse_sizeof(*cp);
	}
	else if (cp->cmd == CMD_ANY_CHAR) 
	{
		sse_printf (SSE_T("."));
		p += sse_sizeof(*cp);
	}
	else if (cp->cmd == CMD_ORD_CHAR) 
	{
		p += sse_sizeof(*cp);
		sse_printf (SSE_T("%c"), *(sse_char_t*)p);
		p += sse_sizeof(sse_char_t);
	}
	else if (cp->cmd == CMD_CHARSET)
	{
		sse_size_t csc, csl, i;

		p += sse_sizeof(*cp);
		sse_printf (SSE_T("["));
		if (cp->negate) sse_printf (SSE_T("^"));

		csc = *(sse_size_t*)p; p += sse_sizeof(csc);
		csl = *(sse_size_t*)p; p += sse_sizeof(csl);

		for (i = 0; i < csc; i++)
		{
			sse_char_t c0, c1, c2;

			c0 = *(sse_char_t*)p;
			p += sse_sizeof(c0);

			if (c0 == CHARSET_ONE)
			{
				c1 = *(sse_char_t*)p;
				sse_printf (SSE_T("%c"), c1);
			}
			else if (c0 == CHARSET_RANGE)
			{
				c1 = *(sse_char_t*)p;
				p += sse_sizeof(c1);
				c2 = *(sse_char_t*)p;
				sse_printf (SSE_T("%c-%c"), c1, c2);
			}
			else if (c0 == CHARSET_CLASS)
			{
				c1 = *(sse_char_t*)p;
				sse_printf (SSE_T("[:%s:]"), __char_class[c1].name);
			}
			else
			{
				sse_printf ("should never happen - invalid charset code\n");
			}

			p += sse_sizeof(c1);
		}

		sse_printf (SSE_T("]"));
	}
	else if (cp->cmd == CMD_GROUP)
	{
		p += sse_sizeof(*cp);
		sse_printf (SSE_T("("));
		p = __print_pattern (p);
		sse_printf (SSE_T(")"));
	}
	else 
	{
		sse_printf ("should never happen - invalid atom code\n");
	}

	if (cp->lbound == 0 && cp->ubound == BOUND_MAX)
		sse_printf (SSE_T("*"));
	else if (cp->lbound == 1 && cp->ubound == BOUND_MAX)
		sse_printf (SSE_T("+"));
	else if (cp->lbound == 0 && cp->ubound == 1)
		sse_printf (SSE_T("?"));
	else if (cp->lbound != 1 || cp->ubound != 1)
	{
		sse_printf (SSE_T("{%lu,%lu}"), 
			(unsigned long)cp->lbound, (unsigned long)cp->ubound);
	}

	return p;
}

