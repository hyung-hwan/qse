/*
 * $Id: rex.c 203 2009-06-17 12:43:50Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/rex.h>
#include <qse/cmn/chr.h>
#include "mem.h"

#ifdef DEBUG_REX
#include <qse/bas/sio.h>
#define DPUTS(x) qse_sio_puts(&qse_sio_err,x)
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
	LEVEL_RANGE
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

#define DEF_CODE_CAPA 512
#define BOUND_MIN 0
#define BOUND_MAX (QSE_TYPE_MAX(qse_size_t))

typedef struct builder_t builder_t;
typedef struct matcher_t matcher_t;
typedef struct match_t match_t;

typedef struct code_t code_t;
typedef struct rhdr_t rhdr_t;
typedef struct bhdr_t bhdr_t;
typedef struct cshdr_t cshdr_t;

struct builder_t
{
	qse_mmgr_t* mmgr;

	struct
	{
		const qse_char_t* ptr;
		const qse_char_t* end;
		const qse_char_t* curp;
		struct
		{
			int type;
			qse_char_t value;
			qse_bool_t escaped;
		} curc;
	} ptn;

	struct
	{
		qse_byte_t* buf;
		qse_size_t  size;
		qse_size_t  capa;
	} code;	

	struct
	{
		qse_size_t max;
		qse_size_t cur;
	} depth;

	int option;
	qse_rex_errnum_t errnum;
};

struct matcher_t
{
	qse_mmgr_t* mmgr;

	struct
	{
		struct
		{
			const qse_char_t* ptr;
			const qse_char_t* end;
		} str;

		struct
		{
			const qse_char_t* ptr;
			const qse_char_t* end;
		} realstr;
	} match;

	struct
	{
		qse_size_t max;
		qse_size_t cur;
	} depth;

	int option;
	qse_rex_errnum_t errnum;
};

struct match_t
{
	const qse_char_t* match_ptr;

	qse_bool_t matched;
	qse_size_t match_len;

	const qse_byte_t* branch;
	const qse_byte_t* branch_end;
};

#include <qse/pack1.h>

QSE_BEGIN_PACKED_STRUCT (code_t)
	/*qse_byte_t cmd;*/
	short cmd;
	short negate; /* only for CMD_CHARSET */
	qse_size_t lbound;
	qse_size_t ubound;
QSE_END_PACKED_STRUCT ()

/* compiled regular expression header */
QSE_BEGIN_PACKED_STRUCT (rhdr_t)
	qse_size_t nb;  /* number of branches */
	qse_size_t el;  /* expression length in bytes */
QSE_END_PACKED_STRUCT ()

/* branch header */
QSE_BEGIN_PACKED_STRUCT (bhdr_t)
	qse_size_t na;  /* number of atoms */
	qse_size_t bl;  /* branch length in bytes */
QSE_END_PACKED_STRUCT ()

/* character set header */
QSE_BEGIN_PACKED_STRUCT (cshdr_t)
	qse_size_t csc; /* count */
	qse_size_t csl; /* length */
QSE_END_PACKED_STRUCT ()

#include <qse/unpack.h>

typedef const qse_byte_t* (*atom_matcher_t) (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);

#define NCHARS_REMAINING(rex) ((rex)->ptn.end - (rex)->ptn.curp)
	
#define NEXT_CHAR(rex,level) \
	do { if (next_char(rex,level) == -1) return -1; } while (0)

#define ADD_CODE(rex,data,len) \
	do { if (add_code(rex,data,len) == -1) return -1; } while (0)

static int build_pattern (builder_t* rex);
static int build_pattern0 (builder_t* rex);
static int build_branch (builder_t* rex);
static int build_atom (builder_t* rex);
static int build_charset (builder_t* rex, code_t* cmd);
static int build_occurrences (builder_t* rex, code_t* cmd);
static int build_cclass (builder_t* rex, qse_char_t* cc);
static int build_range (builder_t* rex, code_t* cmd);
static int next_char (builder_t* rex, int level);
static int add_code (builder_t* rex, void* data, qse_size_t len);

static qse_bool_t __begin_with (
	const qse_char_t* str, qse_size_t len, const qse_char_t* what);

static const qse_byte_t* match_pattern (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_branch (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_branch_body (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_branch_body0 (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_atom (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_bol (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_eol (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_any_char (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_ord_char (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_charset (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);
static const qse_byte_t* match_group (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat);

static const qse_byte_t* match_occurrences (
	matcher_t* matcher, qse_size_t si, const qse_byte_t* p,
	qse_size_t lbound, qse_size_t ubound, match_t* mat);

static qse_bool_t __test_charset (
	matcher_t* matcher, const qse_byte_t* p, qse_size_t csc, qse_char_t c);

static qse_bool_t cc_isalnum (qse_char_t c)
{
	return QSE_ISALNUM (c);
}

static qse_bool_t cc_isalpha (qse_char_t c)
{
	return QSE_ISALPHA (c);
}

static qse_bool_t cc_isblank (qse_char_t c)
{
	return c == QSE_T(' ') || c == QSE_T('\t');
}

static qse_bool_t cc_iscntrl (qse_char_t c)
{
	return QSE_ISCNTRL (c);
}

static qse_bool_t cc_isdigit (qse_char_t c)
{
	return QSE_ISDIGIT (c);
}

static qse_bool_t cc_isgraph (qse_char_t c)
{
	return QSE_ISGRAPH (c);
}

static qse_bool_t cc_islower (qse_char_t c)
{
	return QSE_ISLOWER (c);
}

static qse_bool_t cc_isprint (qse_char_t c)
{
	return QSE_ISPRINT (c);
}

static qse_bool_t cc_ispunct (qse_char_t c)
{
	return QSE_ISPUNCT (c);
}

static qse_bool_t cc_isspace (qse_char_t c)
{
	return QSE_ISSPACE (c);
}

static qse_bool_t cc_isupper (qse_char_t c)
{
	return QSE_ISUPPER (c);
}

static qse_bool_t cc_isxdigit (qse_char_t c)
{
	return QSE_ISXDIGIT (c);
}


#if 0
XXX
static const qse_byte_t* __print_pattern (qse_awk_t* awk, const qse_byte_t* p);
static const qse_byte_t* __print_branch (qse_awk_t* awk, const qse_byte_t* p);
static const qse_byte_t* __print_atom (qse_awk_t* awk, const qse_byte_t* p);
#endif

struct __char_class_t
{
	const qse_char_t* name;
	qse_size_t name_len;
	qse_bool_t (*func) (qse_char_t c);
}; 

static struct __char_class_t __char_class[] =
{
	{ QSE_T("alnum"),  5, cc_isalnum },
	{ QSE_T("alpha"),  5, cc_isalpha },
	{ QSE_T("blank"),  5, cc_isblank },
	{ QSE_T("cntrl"),  5, cc_iscntrl },
	{ QSE_T("digit"),  5, cc_isdigit },
	{ QSE_T("graph"),  5, cc_isgraph },
	{ QSE_T("lower"),  5, cc_islower },
	{ QSE_T("print"),  5, cc_isprint },
	{ QSE_T("punct"),  5, cc_ispunct },
	{ QSE_T("space"),  5, cc_isspace },
	{ QSE_T("upper"),  5, cc_isupper },
	{ QSE_T("xdigit"), 6, cc_isxdigit },

	/*
	{ QSE_T("arabic"),   6, cc_isarabic },
	{ QSE_T("chinese"),  7, cc_ischinese },
	{ QSE_T("english"),  7, cc_isenglish },
	{ QSE_T("japanese"), 8, cc_isjapanese },
	{ QSE_T("korean"),   6, cc_iskorean }, 
	{ QSE_T("thai"),     4, cc_isthai }, 
	*/

	{ QSE_NULL,          0, QSE_NULL }
};

qse_rex_t* qse_rex_open (qse_mmgr_t* mmgr, qse_size_t xtn)
{
	qse_rex_t* rex;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	rex = (qse_rex_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_rex_t) + xtn);
	if (rex == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (rex, 0, QSE_SIZEOF(*rex));
	rex->mmgr = mmgr;

	return rex;
}

void qse_rex_close (qse_rex_t* rex)
{
	if (rex->code != QSE_NULL) qse_freerex (rex->mmgr, rex->code);
	QSE_MMGR_FREE (rex->mmgr, rex);
}

int qse_rex_build (qse_rex_t* rex, const qse_char_t* ptn, qse_size_t len)
{
	void* code;

	code = qse_buildrex (
		rex->mmgr, rex->depth.build, 0,
		ptn, len, &rex->errnum);
	if (code == QSE_NULL) return -1;

	if (rex->code != QSE_NULL) qse_freerex (rex->mmgr, rex->code);
	rex->code = code;

	return 0;
}

int qse_rex_match (
	qse_rex_t* rex,
	const qse_char_t* str, qse_size_t len,
	const qse_char_t* substr, qse_size_t sublen, qse_cstr_t* match)
{
	return qse_matchrex (
		rex->mmgr, rex->depth.match, rex->code, rex->option,
		str, len, substr, sublen, match, &rex->errnum);
}

void* qse_buildrex (
	qse_mmgr_t* mmgr, qse_size_t depth, int option,
	const qse_char_t* ptn, qse_size_t len, qse_rex_errnum_t* errnum)
{
	builder_t builder;

	builder.mmgr = mmgr;
	builder.code.capa = DEF_CODE_CAPA;
	builder.code.size = 0;
	builder.code.buf = (qse_byte_t*) 
		QSE_MALLOC (builder.mmgr, builder.code.capa);
	if (builder.code.buf == QSE_NULL) 
	{
		*errnum = QSE_REX_ENOMEM;
		return QSE_NULL;
	}

	builder.ptn.ptr = ptn;
	builder.ptn.end = builder.ptn.ptr + len;
	builder.ptn.curp = builder.ptn.ptr;

	builder.ptn.curc.type = CT_EOF;
	builder.ptn.curc.value = QSE_T('\0');
	builder.ptn.curc.escaped = QSE_FALSE;

	builder.depth.max = depth;
	builder.depth.cur = 0;
	builder.option = option;

	if (next_char (&builder, LEVEL_TOP) == -1) 
	{
		if (errnum != QSE_NULL) *errnum = builder.errnum;
		QSE_FREE (builder.mmgr, builder.code.buf);
		return QSE_NULL;
	}

	if (build_pattern (&builder) == -1) 
	{
		if (errnum != QSE_NULL) *errnum = builder.errnum;
		QSE_FREE (builder.mmgr, builder.code.buf);
		return QSE_NULL;
	}

	if (builder.ptn.curc.type != CT_EOF)
	{
		if (errnum != QSE_NULL) 
		{
			if (builder.ptn.curc.type ==  CT_SPECIAL &&
			    builder.ptn.curc.value == QSE_T(')'))
			{
				*errnum = QSE_REX_EUNBALPAREN;
			}
			else if (builder.ptn.curc.type ==  CT_SPECIAL &&
			         builder.ptn.curc.value == QSE_T('{'))
			{
				*errnum = QSE_REX_EINVALBRACE;
			}
			else
			{
				*errnum = QSE_REX_EGARBAGE;
			}
		}

		QSE_FREE (builder.mmgr, builder.code.buf);
		return QSE_NULL;
	}

	return builder.code.buf;
}

int qse_matchrex (
	qse_mmgr_t* mmgr, qse_size_t depth,
	void* code, int option,
	const qse_char_t* str, qse_size_t len, 
	const qse_char_t* substr, qse_size_t sublen, 
	qse_cstr_t* match, qse_rex_errnum_t* errnum)
{
	matcher_t matcher;
	match_t mat;
	qse_size_t offset = 0;
	/*const qse_char_t* match_ptr_zero = QSE_NULL;*/

	matcher.mmgr = mmgr;

	/* store the source string */
	matcher.match.str.ptr = substr;
	matcher.match.str.end = substr + sublen;

	matcher.match.realstr.ptr = str;
	matcher.match.realstr.end = str + len;

	matcher.depth.max = depth;
	matcher.depth.cur = 0;
	matcher.option = option;

	mat.matched = QSE_FALSE;
	/* TODO: should it allow an offset here??? */
	mat.match_ptr = substr + offset;

	/*while (mat.match_ptr < matcher.match.str.end)*/
	while (mat.match_ptr <= matcher.match.str.end)
	{
		if (match_pattern (&matcher, code, &mat) == QSE_NULL) 
		{
			if (errnum != QSE_NULL) *errnum = matcher.errnum;
			return -1;
		}

		if (mat.matched)
		{
			/*
			if (mat.match_len == 0)
			{
				if (match_ptr_zero == QSE_NULL)
					match_ptr_zero = mat.match_ptr;
				mat.match_ptr++;
				continue;
			}
			*/

			if (match != QSE_NULL) 
			{
				match->ptr = mat.match_ptr;
				match->len = mat.match_len;
			}

			/*match_ptr_zero = QSE_NULL;*/
			break;
		}

		mat.match_ptr++;
	}

	/*
	if (match_ptr_zero != QSE_NULL) 
	{
		if (match != QSE_NULL) 
		{
			match->ptr = match_ptr_zero;
			match->len = 0;
		}
		return 1;
	}
	*/

	return (mat.matched)? 1: 0;
}

void qse_freerex (qse_mmgr_t* mmgr, void* code)
{
	QSE_ASSERT (code != QSE_NULL);
	QSE_FREE (mmgr, code);
}

qse_bool_t qse_isemptyrex (void* code)
{
	rhdr_t* rhdr = (rhdr_t*) code;
	QSE_ASSERT (rhdr != QSE_NULL);

	/* an empty regular expression look like:
	 *  | expression                     | 
	 *  | header         | branch        |
	 *  |                | branch header |
	 *  | NB(1) | EL(16) | NA(1) | BL(8) | */
	return (rhdr->nb == 1 && 
	        rhdr->el == QSE_SIZEOF(qse_size_t)*4)? QSE_TRUE: QSE_FALSE;
}

static int build_pattern (builder_t* builder)
{
	int n;

	if (builder->depth.max > 0 && builder->depth.cur >= builder->depth.max)
	{
		builder->errnum = QSE_REX_ERECUR;
		return -1;
	}

	builder->depth.cur++;
	n = build_pattern0 (builder);
	builder->depth.cur--;

	return n;
}

static int build_pattern0 (builder_t* builder)
{
	qse_size_t zero = 0;
	qse_size_t old_size;
	qse_size_t pos_nb;
	rhdr_t* rhdr;
	int n;

	old_size = builder->code.size;

	/* secure space for header and set the header fields to zero */
	pos_nb = builder->code.size;
	ADD_CODE (builder, &zero, QSE_SIZEOF(zero));
	ADD_CODE (builder, &zero, QSE_SIZEOF(zero));

	/* handle the first branch */
	n = build_branch (builder);
	if (n == -1) return -1;
	if (n == 0) 
	{
		/* if the pattern is empty, the control reaches here */
		return 0;
	}

	rhdr = (rhdr_t*)&builder->code.buf[pos_nb];
	rhdr->nb++;

	/* handle subsequent branches if any */
	while (builder->ptn.curc.type == CT_SPECIAL && 
	       builder->ptn.curc.value == QSE_T('|'))
	{
		NEXT_CHAR (builder, LEVEL_TOP);

		n = build_branch(builder);
		if (n == -1) return -1;
		if (n == 0) 
		{
			/* if the pattern ends with a vertical bar(|),
			 * this block can be reached. however, such a 
			 * pattern is highly discouraged */
			break;
		}

		rhdr = (rhdr_t*)&builder->code.buf[pos_nb];
		rhdr->nb++;
	}

	rhdr = (rhdr_t*)&builder->code.buf[pos_nb];
	rhdr->el = builder->code.size - old_size;

	return 1;
}

static int build_branch (builder_t* builder)
{
	int n;
	qse_size_t zero = 0;
	qse_size_t old_size;
	qse_size_t pos_na;
	code_t* cmd;
	bhdr_t* bhdr;

	old_size = builder->code.size;

	pos_na = builder->code.size;
	ADD_CODE (builder, &zero, QSE_SIZEOF(zero));
	ADD_CODE (builder, &zero, QSE_SIZEOF(zero));

	while (1)
	{
		cmd = (code_t*)&builder->code.buf[builder->code.size];

		n = build_atom (builder);
		if (n == -1) 
		{
			builder->code.size = old_size;
			return -1;
		}

		if (n == 0) break; /* no atom */

		n = build_occurrences (builder, cmd);
		if (n == -1)
		{
			builder->code.size = old_size;
			return -1;
		}

		/* n == 0  no bound character. just continue */
		/* n == 1  bound has been applied by build_occurrences */

		bhdr = (bhdr_t*)&builder->code.buf[pos_na];
		bhdr->na++;
	}

	bhdr = (bhdr_t*)&builder->code.buf[pos_na];
	bhdr->bl = builder->code.size - old_size;

	return (builder->code.size == old_size)? 0: 1;
}

static int build_atom (builder_t* builder)
{
	int n;
	code_t tmp;

	if (builder->ptn.curc.type == CT_EOF) return 0;

	if (builder->ptn.curc.type == CT_SPECIAL)
	{
		if (builder->ptn.curc.value == QSE_T('('))
		{
			tmp.cmd = CMD_GROUP;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, QSE_SIZEOF(tmp));

			NEXT_CHAR (builder, LEVEL_TOP);

			n = build_pattern (builder);
			if (n == -1) return -1;

			if (builder->ptn.curc.type != CT_SPECIAL || 
			    builder->ptn.curc.value != QSE_T(')')) 
			{
				builder->errnum = QSE_REX_ERPAREN;
				return -1;
			}
		}
		else if (builder->ptn.curc.value == QSE_T('^'))
		{
			tmp.cmd = CMD_BOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, QSE_SIZEOF(tmp));
		}
		else if (builder->ptn.curc.value == QSE_T('$'))
		{
			tmp.cmd = CMD_EOL;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, QSE_SIZEOF(tmp));
		}
		else if (builder->ptn.curc.value == QSE_T('.'))
		{
			tmp.cmd = CMD_ANY_CHAR;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, QSE_SIZEOF(tmp));
		}
		else if (builder->ptn.curc.value == QSE_T('['))
		{
			code_t* cmd;

			cmd = (code_t*)&builder->code.buf[builder->code.size];

			tmp.cmd = CMD_CHARSET;
			tmp.negate = 0;
			tmp.lbound = 1;
			tmp.ubound = 1;
			ADD_CODE (builder, &tmp, QSE_SIZEOF(tmp));

			NEXT_CHAR (builder, LEVEL_CHARSET);

			n = build_charset (builder, cmd);
			if (n == -1) return -1;

			QSE_ASSERT (n != 0);

			if (builder->ptn.curc.type != CT_SPECIAL ||
			    builder->ptn.curc.value != QSE_T(']'))
			{
				builder->errnum = QSE_REX_ERBRACKET;
				return -1;
			}

		}
		else return 0;

		NEXT_CHAR (builder, LEVEL_TOP);
		return 1;
	}
	else 
	{
		QSE_ASSERT (builder->ptn.curc.type == CT_NORMAL);

		tmp.cmd = CMD_ORD_CHAR;
		tmp.negate = 0;
		tmp.lbound = 1;
		tmp.ubound = 1;
		ADD_CODE (builder, &tmp, QSE_SIZEOF(tmp));

		ADD_CODE (builder, 
			&builder->ptn.curc.value, 
			QSE_SIZEOF(builder->ptn.curc.value));
		NEXT_CHAR (builder, LEVEL_TOP);

		return 1;
	}
}

static int build_charset (builder_t* builder, code_t* cmd)
{
	qse_size_t zero = 0;
	qse_size_t old_size;
	qse_size_t pos_csc;
	cshdr_t* cshdr;

	old_size = builder->code.size;

	pos_csc = builder->code.size;
	ADD_CODE (builder, &zero, QSE_SIZEOF(zero));
	ADD_CODE (builder, &zero, QSE_SIZEOF(zero));

	if (builder->ptn.curc.type == CT_NORMAL &&
	    builder->ptn.curc.value == QSE_T('^')) 
	{
		cmd->negate = 1;
		NEXT_CHAR (builder, LEVEL_CHARSET);
	}

	while (builder->ptn.curc.type == CT_NORMAL)
	{
		qse_char_t c0, c1, c2;
		int cc = 0;

		c1 = builder->ptn.curc.value;
		NEXT_CHAR(builder, LEVEL_CHARSET);

		if (c1 == QSE_T('[') &&
		    builder->ptn.curc.type == CT_NORMAL &&
		    builder->ptn.curc.value == QSE_T(':'))
		{
			if (build_cclass (builder, &c1) == -1) return -1;
			cc = cc | 1;
		}

		c2 = c1;
		if (builder->ptn.curc.type == CT_NORMAL &&
		    builder->ptn.curc.value == QSE_T('-') && 
		    builder->ptn.curc.escaped == QSE_FALSE)
		{
			NEXT_CHAR (builder, LEVEL_CHARSET);

			if (builder->ptn.curc.type == CT_NORMAL)
			{
				c2 = builder->ptn.curc.value;
				NEXT_CHAR (builder, LEVEL_CHARSET);

				if (c2 == QSE_T('[') &&
				    builder->ptn.curc.type == CT_NORMAL &&
				    builder->ptn.curc.value == QSE_T(':'))
				{
					if (build_cclass (builder, &c2) == -1)
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
				ADD_CODE (builder, &c0, QSE_SIZEOF(c0));
				ADD_CODE (builder, &c1, QSE_SIZEOF(c1));
			}
			else
			{
				c0 = CHARSET_RANGE;
				ADD_CODE (builder, &c0, QSE_SIZEOF(c0));
				ADD_CODE (builder, &c1, QSE_SIZEOF(c1));
				ADD_CODE (builder, &c2, QSE_SIZEOF(c2));
			}
		}
		else if (cc == 1)
		{
			c0 = CHARSET_CLASS;
			ADD_CODE (builder, &c0, QSE_SIZEOF(c0));
			ADD_CODE (builder, &c1, QSE_SIZEOF(c1));
		}
		else
		{
			/* invalid range */
		#ifdef DEBUG_REX
			DPUTS (QSE_T("build_charset: invalid character set range\n"));
		#endif
			builder->errnum = QSE_REX_ECRANGE;
			return -1;
		}

		cshdr = (cshdr_t*)&builder->code.buf[pos_csc];
		cshdr->csc++;
	}

	cshdr = (cshdr_t*)&builder->code.buf[pos_csc];
	cshdr->csl = builder->code.size - old_size;

	return 1;
}

static int build_cclass (builder_t* builder, qse_char_t* cc)
{
	const struct __char_class_t* ccp = __char_class;
	qse_size_t len = builder->ptn.end - builder->ptn.curp;

	while (ccp->name != QSE_NULL)
	{
		if (__begin_with (builder->ptn.curp, len, ccp->name)) break;
		ccp++;
	}

	if (ccp->name == QSE_NULL)
	{
		/* wrong class name */
	#ifdef DEBUG_REX
		DPUTS (QSE_T("build_cclass: wrong class name\n"));
	#endif
		builder->errnum = QSE_REX_ECCLASS;
		return -1;
	}

	builder->ptn.curp += ccp->name_len;

	NEXT_CHAR (builder, LEVEL_CHARSET);
	if (builder->ptn.curc.type != CT_NORMAL ||
	    builder->ptn.curc.value != QSE_T(':'))
	{
	#ifdef DEBUG_REX
		DPUTS (QSE_T("build_cclass: a colon(:) expected\n"));
	#endif
		builder->errnum = QSE_REX_ECOLON;
		return -1;
	}

	NEXT_CHAR (builder, LEVEL_CHARSET); 
	
	/* ] happens to be the charset ender ] */
	if (builder->ptn.curc.type != CT_SPECIAL ||
	    builder->ptn.curc.value != QSE_T(']'))
	{
	#ifdef DEBUG_REX
		DPUTS (QSE_T("build_cclass: ] expected\n"));
	#endif
		builder->errnum = QSE_REX_ERBRACKET;	
		return -1;
	}

	NEXT_CHAR (builder, LEVEL_CHARSET);

	*cc = (qse_char_t)(ccp - __char_class);
	return 1;
}

static int build_occurrences (builder_t* builder, code_t* cmd)
{
	if (builder->ptn.curc.type != CT_SPECIAL) return 0;

	switch (builder->ptn.curc.value)
	{
		case QSE_T('+'):
		{
			cmd->lbound = 1;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case QSE_T('*'):
		{
			cmd->lbound = 0;
			cmd->ubound = BOUND_MAX;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case QSE_T('?'):
		{
			cmd->lbound = 0;
			cmd->ubound = 1;
			NEXT_CHAR(builder, LEVEL_TOP);
			return 1;
		}

		case QSE_T('{'):
		{
			NEXT_CHAR (builder, LEVEL_RANGE);

			if (build_range(builder, cmd) == -1) return -1;

			if (builder->ptn.curc.type != CT_SPECIAL || 
			    builder->ptn.curc.value != QSE_T('}')) 
			{
				builder->errnum = QSE_REX_ERBRACE;
				return -1;
			}

			NEXT_CHAR (builder, LEVEL_TOP);
			return 1;
		}
	}

	return 0;
}

static int build_range (builder_t* builder, code_t* cmd)
{
	qse_size_t bound;

/* TODO: should allow white spaces in the range???
what if it is not in the raight format? convert it to ordinary characters?? */
	bound = 0;
	while (builder->ptn.curc.type == CT_NORMAL &&
	       (builder->ptn.curc.value >= QSE_T('0') && 
	        builder->ptn.curc.value <= QSE_T('9')))
	{
		bound = bound * 10 + builder->ptn.curc.value - QSE_T('0');
		NEXT_CHAR (builder, LEVEL_RANGE);
	}

	cmd->lbound = bound;

	if (builder->ptn.curc.type == CT_SPECIAL &&
	    builder->ptn.curc.value == QSE_T(',')) 
	{
		NEXT_CHAR (builder, LEVEL_RANGE);

		if (builder->ptn.curc.type == CT_NORMAL &&
		    (builder->ptn.curc.value >= QSE_T('0') && 
		     builder->ptn.curc.value <= QSE_T('9')))
		{
			bound = 0;

			do
			{
				bound = bound * 10 + builder->ptn.curc.value - QSE_T('0');
				NEXT_CHAR (builder, LEVEL_RANGE);
			}
			while (builder->ptn.curc.type == CT_NORMAL &&
			       (builder->ptn.curc.value >= QSE_T('0') && 
			        builder->ptn.curc.value <= QSE_T('9')));

			cmd->ubound = bound;
		}
		else cmd->ubound = BOUND_MAX;
	}
	else cmd->ubound = cmd->lbound;

	if (cmd->lbound > cmd->ubound)
	{
		/* invalid occurrences range */
		builder->errnum = QSE_REX_EBRANGE;
		return -1;
	}

	return 0;
}

#define CHECK_END(builder) \
	do { \
		if (builder->ptn.curp >= builder->ptn.end) \
		{ \
			builder->errnum = QSE_REX_EEND; \
			return -1; \
		} \
	} while(0)

#define IS_HEX(c) \
	((c >= QSE_T('0') && c <= QSE_T('9')) || \
	 (c >= QSE_T('A') && c <= QSE_T('F')) || \
	 (c >= QSE_T('a') && c <= QSE_T('f')))

#define HEX_TO_NUM(c) \
	((c >= QSE_T('0') && c <= QSE_T('9'))? c-QSE_T('0'):  \
	 (c >= QSE_T('A') && c <= QSE_T('F'))? c-QSE_T('A')+10: \
	                                       c-QSE_T('a')+10)

static int next_char (builder_t* builder, int level)
{
	if (builder->ptn.curp >= builder->ptn.end)
	{
		builder->ptn.curc.type = CT_EOF;
		builder->ptn.curc.value = QSE_T('\0');
		builder->ptn.curc.escaped = QSE_FALSE;
		return 0;
	}

	builder->ptn.curc.type = CT_NORMAL;
	builder->ptn.curc.value = *builder->ptn.curp++;
	builder->ptn.curc.escaped = QSE_FALSE;

	if (builder->ptn.curc.value == QSE_T('\\'))
	{	       
		qse_char_t c;

		CHECK_END (builder);
		c = *builder->ptn.curp++;

		if (c == QSE_T('n')) c = QSE_T('\n');
		else if (c == QSE_T('r')) c = QSE_T('\r');
		else if (c == QSE_T('t')) c = QSE_T('\t');
		else if (c == QSE_T('f')) c = QSE_T('\f');
		else if (c == QSE_T('b')) c = QSE_T('\b');
		else if (c == QSE_T('v')) c = QSE_T('\v');
		else if (c == QSE_T('a')) c = QSE_T('\a');
		else if (c >= QSE_T('0') && c <= QSE_T('7')) 
		{
			qse_char_t cx;

			c = c - QSE_T('0');

			CHECK_END (builder);
			cx = *builder->ptn.curp++;
			if (cx >= QSE_T('0') && cx <= QSE_T('7'))
			{
				c = c * 8 + cx - QSE_T('0');

				CHECK_END (builder);
				cx = *builder->ptn.curp++;
				if (cx >= QSE_T('0') && cx <= QSE_T('7'))
				{
					c = c * 8 + cx - QSE_T('0');
				}
			}
		}
		else if (c == QSE_T('x')) 
		{
			qse_char_t cx;

			CHECK_END (builder);
			cx = *builder->ptn.curp++;
			if (IS_HEX(cx))
			{
				c = HEX_TO_NUM(cx);

				CHECK_END (builder);
				cx = *builder->ptn.curp++;
				if (IS_HEX(cx))
				{
					c = c * 16 + HEX_TO_NUM(cx);
				}
			}
		}
	#ifdef QSE_CHAR_IS_WCHAR
		else if (c == QSE_T('u') && QSE_SIZEOF(qse_char_t) >= 2) 
		{
			qse_char_t cx;

			CHECK_END (builder);
			cx = *builder->ptn.curp++;
			if (IS_HEX(cx))
			{
				qse_size_t i;

				c = HEX_TO_NUM(cx);

				for (i = 0; i < 3; i++)
				{
					CHECK_END (builder);
					cx = *builder->ptn.curp++;

					if (!IS_HEX(cx)) break;
					c = c * 16 + HEX_TO_NUM(cx);
				}
			}
		}
		else if (c == QSE_T('U') && QSE_SIZEOF(qse_char_t) >= 4) 
		{
			qse_char_t cx;

			CHECK_END (builder);
			cx = *builder->ptn.curp++;
			if (IS_HEX(cx))
			{
				qse_size_t i;

				c = HEX_TO_NUM(cx);

				for (i = 0; i < 7; i++)
				{
					CHECK_END (builder);
					cx = *builder->ptn.curp++;

					if (!IS_HEX(cx)) break;
					c = c * 16 + HEX_TO_NUM(cx);
				}
			}
		}
	#endif

		builder->ptn.curc.value = c;
		builder->ptn.curc.escaped = QSE_TRUE;

		return 0;
	}
	else
	{
		if (level == LEVEL_TOP)
		{
			if (builder->ptn.curc.value == QSE_T('[') ||
			    builder->ptn.curc.value == QSE_T('|') ||
			    builder->ptn.curc.value == QSE_T('^') ||
			    builder->ptn.curc.value == QSE_T('$') ||
			    (!(builder->option & QSE_REX_BUILD_NOBOUND) &&
			     builder->ptn.curc.value == QSE_T('{')) ||
			    builder->ptn.curc.value == QSE_T('+') ||
			    builder->ptn.curc.value == QSE_T('?') ||
			    builder->ptn.curc.value == QSE_T('*') ||
			    builder->ptn.curc.value == QSE_T('.') ||
			    builder->ptn.curc.value == QSE_T('(') ||
			    builder->ptn.curc.value == QSE_T(')')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_CHARSET)
		{
			if (builder->ptn.curc.value == QSE_T(']')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
		else if (level == LEVEL_RANGE)
		{
			if (builder->ptn.curc.value == QSE_T(',') ||
			    builder->ptn.curc.value == QSE_T('}')) 
			{
				builder->ptn.curc.type = CT_SPECIAL;
			}
		}
	}

	return 0;
}

static int add_code (builder_t* builder, void* data, qse_size_t len)
{
	if (len > builder->code.capa - builder->code.size)
	{
		qse_size_t capa = builder->code.capa * 2;
		qse_byte_t* tmp;
		
		if (capa == 0) capa = DEF_CODE_CAPA;
		while (len > capa - builder->code.size) { capa = capa * 2; }

		if (builder->mmgr->realloc != QSE_NULL)
		{
			tmp = (qse_byte_t*) QSE_REALLOC (
				builder->mmgr, builder->code.buf, capa);
			if (tmp == QSE_NULL)
			{
				builder->errnum = QSE_REX_ENOMEM;
				return -1;
			}
		}
		else
		{
			tmp = (qse_byte_t*) QSE_MALLOC (builder->mmgr, capa);
			if (tmp == QSE_NULL)
			{
				builder->errnum = QSE_REX_ENOMEM;
				return -1;
			}

			if (builder->code.buf != QSE_NULL)
			{
				QSE_MEMCPY (tmp, builder->code.buf, builder->code.capa);
				QSE_FREE (builder->mmgr, builder->code.buf);
			}
		}

		builder->code.buf = tmp;
		builder->code.capa = capa;
	}

	QSE_MEMCPY (&builder->code.buf[builder->code.size], data, len);
	builder->code.size += len;

	return 0;
}

static qse_bool_t __begin_with (
	const qse_char_t* str, qse_size_t len, const qse_char_t* what)
{
	const qse_char_t* end = str + len;

	while (str < end)
	{
		if (*what == QSE_T('\0')) return QSE_TRUE;
		if (*what != *str) return QSE_FALSE;

		str++; what++;
	}

	if (*what == QSE_T('\0')) return QSE_TRUE;
	return QSE_FALSE;
}

static const qse_byte_t* match_pattern (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	match_t mat2;
	qse_size_t i;
	const qse_byte_t* p;
	rhdr_t* rhdr;

	p = base;
	rhdr = (rhdr_t*) p; p += QSE_SIZEOF(*rhdr);

#ifdef DEBUG_REX
	qse_dprintf (
		QSE_T("match_pattern: NB = %u, EL = %u\n"), 
		(unsigned int)rhdr->nb, (unsigned int)rhdr->el);
#endif

	mat->matched = QSE_FALSE;
	mat->match_len = 0;

	for (i = 0; i < rhdr->nb; i++)
	{
		mat2.match_ptr = mat->match_ptr;

		p = match_branch (matcher, p, &mat2);
		if (p == QSE_NULL) return QSE_NULL;

		if (mat2.matched)
		{
			mat->matched = QSE_TRUE;
			mat->match_len = mat2.match_len;
			break;
		}
	}

	return base + rhdr->el;
}

static const qse_byte_t* match_branch (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	/* branch body  base+sizeof(NA)+sizeof(BL)-----+
	 * BL base+sizeof(NA) ----------+              |
	 * base NA ------+              |              |
	 *               |              |              |
	 *               |NA(qse_size_t)|BL(qse_size_t)|ATOMS.........|
	 */
	mat->branch = base;
	mat->branch_end = base + ((bhdr_t*)base)->bl;

	return match_branch_body (
		matcher, (const qse_byte_t*)((bhdr_t*)base+1), mat);
}

static const qse_byte_t* match_branch_body (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* n;

	if (matcher->depth.max > 0 && matcher->depth.cur >= matcher->depth.max)
	{
		matcher->errnum = QSE_REX_ERECUR;
		return QSE_NULL;
	}

	matcher->depth.cur++;
	n = match_branch_body0 (matcher, base, mat);
	matcher->depth.cur--;

	return n;
}

static const qse_byte_t* match_branch_body0 (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* p;
/*	match_t mat2;*/
	qse_size_t match_len = 0;

	mat->matched = QSE_FALSE;
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
		p = match_atom (matcher, p, mat);
		if (p == QSE_NULL) return QSE_NULL;

		if (!mat->matched) break;

		mat->match_ptr = &mat->match_ptr[mat->match_len];
		match_len += mat->match_len;
#if 0
		p = match_atom (matcher, p, &mat2);
		if (p == QSE_NULL) return QSE_NULL;

		if (!mat2.matched) 
		{
			mat->matched = QSE_FALSE;
			break; /* stop matching */
		}

		mat->matched = QSE_TRUE;
		mat->match_len += mat2.match_len;

		mat2.match_ptr = &mat2.match_ptr[mat2.match_len];
#endif
	}

	if (mat->matched) mat->match_len = match_len;
	return mat->branch_end;
}

static const qse_byte_t* match_atom (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	static atom_matcher_t matchers[] =
	{
		match_bol,
		match_eol,
		match_any_char,
		match_ord_char,
		match_charset,
		match_group
	};
       
	QSE_ASSERT (
		((code_t*)base)->cmd >= 0 && 
		((code_t*)base)->cmd < QSE_COUNTOF(matchers));

	return matchers[((code_t*)base)->cmd] (matcher, base, mat);
}

static const qse_byte_t* match_bol (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* p = base;
	const code_t* cp;

	cp = (const code_t*)p; p += QSE_SIZEOF(*cp);
	QSE_ASSERT (cp->cmd == CMD_BOL);

	/*mat->matched = (mat->match_ptr == matcher->match.str.ptr ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));*/
	mat->matched = (mat->match_ptr == matcher->match.realstr.ptr ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const qse_byte_t* match_eol (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* p = base;
	const code_t* cp;

	cp = (const code_t*)p; p += QSE_SIZEOF(*cp);
	QSE_ASSERT (cp->cmd == CMD_EOL);

	/*mat->matched = (mat->match_ptr == matcher->match.str.end ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));*/
	mat->matched = (mat->match_ptr == matcher->match.realstr.end ||
	               (cp->lbound == cp->ubound && cp->lbound == 0));
	mat->match_len = 0;

	return p;
}

static const qse_byte_t* match_any_char (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* p = base;
	const code_t* cp;
	qse_size_t si = 0, lbound, ubound;

	cp = (const code_t*)p; p += QSE_SIZEOF(*cp);
	QSE_ASSERT (cp->cmd == CMD_ANY_CHAR);

	lbound = cp->lbound;
	ubound = cp->ubound;

	mat->matched = QSE_FALSE;
	mat->match_len = 0;

	/* merge the same consecutive codes */
	while (p < mat->branch_end &&
	       cp->cmd == ((const code_t*)p)->cmd)
	{
		lbound += ((const code_t*)p)->lbound;
		ubound += ((const code_t*)p)->ubound;

		p += QSE_SIZEOF(*cp);
	}

#ifdef DEBUG_REX
	qse_dprintf (
		QSE_T("match_any_char: lbound = %u, ubound = %u\n"), 
		(unsigned int)lbound, (unsigned int)ubound);
#endif

	/* find the longest match */
	while (si < ubound)
	{
		if (&mat->match_ptr[si] >= matcher->match.str.end) break;
		si++;
	}

#ifdef DEBUG_REX
	qse_dprintf (
		QSE_T("match_any_char: max si = %u\n"), (unsigned)si);
#endif

	if (si >= lbound && si <= ubound)
	{
		p = match_occurrences (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const qse_byte_t* match_ord_char (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* p = base;
	const code_t* cp;
	qse_size_t si = 0, lbound, ubound;
	qse_char_t cc;

	cp = (const code_t*)p; p += QSE_SIZEOF(*cp);
	QSE_ASSERT (cp->cmd == CMD_ORD_CHAR);

	lbound = cp->lbound; 
	ubound = cp->ubound;

	cc = *(qse_char_t*)p; p += QSE_SIZEOF(cc);
	if (matcher->option & QSE_REX_MATCH_IGNORECASE) cc = QSE_TOUPPER(cc);

	/* merge the same consecutive codes 
	 * for example, a{1,10}a{0,10} is shortened to a{1,20} */
	if (matcher->option & QSE_REX_MATCH_IGNORECASE) 
	{
		while (p < mat->branch_end &&
		       cp->cmd == ((const code_t*)p)->cmd)
		{
			if (QSE_TOUPPER (*(qse_char_t*)(p+QSE_SIZEOF(*cp))) != cc) break;

			lbound += ((const code_t*)p)->lbound;
			ubound += ((const code_t*)p)->ubound;

			p += QSE_SIZEOF(*cp) + QSE_SIZEOF(cc);
		}
	}
	else
	{
		while (p < mat->branch_end &&
		       cp->cmd == ((const code_t*)p)->cmd)
		{
			if (*(qse_char_t*)(p+QSE_SIZEOF(*cp)) != cc) break;

			lbound += ((const code_t*)p)->lbound;
			ubound += ((const code_t*)p)->ubound;

			p += QSE_SIZEOF(*cp) + QSE_SIZEOF(cc);
		}
	}
	
#ifdef DEBUG_REX
	qse_dprintf (
		QSE_T("match_ord_char: cc = %c, lbound = %u, ubound = %u\n"), 
		cc, (unsigned int)lbound, (unsigned int)ubound);
#endif

	mat->matched = QSE_FALSE;
	mat->match_len = 0;

	/* find the longest match */
	if (matcher->option & QSE_REX_MATCH_IGNORECASE) 
	{
		while (si < ubound)
		{
			if (&mat->match_ptr[si] >= matcher->match.str.end) break;
		#ifdef DEBUG_REX
			qse_dprintf (
				QSE_T("match_ord_char: <ignorecase> %c %c\n"),
				cc, mat->match_ptr[si]);
		#endif
			if (cc != QSE_TOUPPER (mat->match_ptr[si])) break;
			si++;
		}
	}
	else
	{
		while (si < ubound)
		{
			if (&mat->match_ptr[si] >= matcher->match.str.end) break;
		#ifdef DEBUG_REX
			qse_dprintf (
				QSE_T("match_ord_char: %c %c\n"), 
				cc, mat->match_ptr[si]);
		#endif
			if (cc != mat->match_ptr[si]) break;
			si++;
		}
	}

#ifdef DEBUG_REX
	qse_dprintf (
		QSE_T("match_ord_char: max occurrences=%u, lbound=%u, ubound=%u\n"), 
		(unsigned)si, (unsigned)lbound, (unsigned)ubound);
#endif

	if (si >= lbound && si <= ubound)
	{
		p = match_occurrences (matcher, si, p, lbound, ubound, mat);
	}

	return p;
}

static const qse_byte_t* match_charset (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* p = base;
	qse_size_t si = 0;
	qse_bool_t n;
	qse_char_t c;

	code_t* cp;
	cshdr_t* cshdr;

	cp = (code_t*)p; p += QSE_SIZEOF(*cp);
	QSE_ASSERT (cp->cmd == CMD_CHARSET);

	cshdr = (cshdr_t*)p; p += QSE_SIZEOF(*cshdr);

#ifdef DEBUG_REX
	qse_dprintf (
		QSE_T("match_charset: lbound = %u, ubound = %u\n"), 
		(unsigned int)cp->lbound, (unsigned int)cp->ubound);
#endif

	mat->matched = QSE_FALSE;
	mat->match_len = 0;

	while (si < cp->ubound)
	{
		if (&mat->match_ptr[si] >= matcher->match.str.end) break;

		c = mat->match_ptr[si];
		if (matcher->option & QSE_REX_MATCH_IGNORECASE) c = QSE_TOUPPER(c);

		n = __test_charset (matcher, p, cshdr->csc, c);
		if (cp->negate) n = !n;
		if (!n) break;

		si++;
	}

	p = p + cshdr->csl - QSE_SIZEOF(*cshdr);

#ifdef DEBUG_REX
	qse_dprintf (
		QSE_T("match_charset: max occurrences=%u, lbound=%u, ubound=%u\n"), 
		(unsigned)si, (unsigned)cp->lbound, (unsigned)cp->ubound);
#endif

	if (si >= cp->lbound && si <= cp->ubound)
	{
		p = match_occurrences (matcher, si, p, cp->lbound, cp->ubound, mat);
	}

	return p;
}

static const qse_byte_t* match_group (
	matcher_t* matcher, const qse_byte_t* base, match_t* mat)
{
	const qse_byte_t* p = base;
	const code_t* cp;
	match_t mat2;
	qse_size_t si = 0, grp_len_static[16], * grp_len;

	cp = (const code_t*)p; p += QSE_SIZEOF(*cp);
	QSE_ASSERT (cp->cmd == CMD_GROUP);

	mat->matched = QSE_FALSE;
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

	if (cp->ubound < QSE_COUNTOF(grp_len_static))
	{
		grp_len = grp_len_static;
	}
	else 
	{
		grp_len = (qse_size_t*) QSE_MALLOC (
			matcher->mmgr, QSE_SIZEOF(qse_size_t) * cp->ubound);
		if (grp_len == QSE_NULL)
		{
			matcher->errnum = QSE_REX_ENOMEM;
			return QSE_NULL;
		}
	}

	grp_len[si] = 0;

	mat2.match_ptr = mat->match_ptr;
	while (si < cp->ubound)
	{
		if (mat2.match_ptr >= matcher->match.str.end) break;

		if (match_pattern (matcher, p, &mat2) == QSE_NULL) 
		{
			if (grp_len != grp_len_static) 
				QSE_FREE (matcher->mmgr, grp_len);
			return QSE_NULL;
		}
		if (!mat2.matched) break;

		grp_len[si+1] = grp_len[si] + mat2.match_len;

		mat2.match_ptr += mat2.match_len;
		mat2.match_len = 0;
		mat2.matched = QSE_FALSE;

		si++;
	}

	/* increment p by the length of the subpattern */
	p += *(qse_size_t*)(p+QSE_SIZEOF(qse_size_t));

	/* check the occurrences */
	if (si >= cp->lbound && si <= cp->ubound)
	{
		if (cp->lbound == cp->ubound || p >= mat->branch_end)
		{
			mat->matched = QSE_TRUE;
			mat->match_len = grp_len[si];
		}
		else 
		{
			QSE_ASSERT (cp->ubound > cp->lbound);

			do
			{
				const qse_byte_t* tmp;
	
				mat2.match_ptr = &mat->match_ptr[grp_len[si]];
				mat2.branch = mat->branch;
				mat2.branch_end = mat->branch_end;
	
			#ifdef DEBUG_REX
				qse_dprintf (
					QSE_T("match_group: GROUP si=%d [%s]\n"),
					(unsigned)si, mat->match_ptr);
			#endif
				tmp = match_branch_body (matcher, p, &mat2);
				if (tmp == QSE_NULL)
				{
					if (grp_len != grp_len_static) 
						QSE_FREE (matcher->mmgr, grp_len);
					return QSE_NULL;
				}

				if (mat2.matched)
				{
					mat->matched = QSE_TRUE;
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

	if (grp_len != grp_len_static) QSE_FREE (matcher->mmgr, grp_len);
	return p;
}

static const qse_byte_t* match_occurrences (
	matcher_t* matcher, qse_size_t si, const qse_byte_t* p,
	qse_size_t lbound, qse_size_t ubound, match_t* mat)
{
	QSE_ASSERT (si >= lbound && si <= ubound);
	/* the match has been found */

	if (lbound == ubound || p >= mat->branch_end)
	{
		/* if the match for fixed occurrences was 
		 * requested or no atoms remain unchecked in 
		 * the branch, the match is returned. */
		mat->matched = QSE_TRUE;
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

		QSE_ASSERT (ubound > lbound);

		do
		{
			match_t mat2;
			const qse_byte_t* tmp;

			mat2.match_ptr = &mat->match_ptr[si];
			mat2.branch = mat->branch;
			mat2.branch_end = mat->branch_end;

		#ifdef DEBUG_REX
			qse_dprintf (
				QSE_T("__match occurrences: si=%u [%s]\n"), 
				(unsigned)si, mat->match_ptr);
		#endif
			tmp = match_branch_body (matcher, p, &mat2);

			if (mat2.matched)
			{
				mat->matched = QSE_TRUE;
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

static qse_bool_t __test_charset (
	matcher_t* matcher, const qse_byte_t* p, qse_size_t csc, qse_char_t c)
{
	qse_size_t i;

	for (i = 0; i < csc; i++)
	{
		qse_char_t c0, c1, c2;

		c0 = *(const qse_char_t*)p;
		p += QSE_SIZEOF(c0);
		if (c0 == CHARSET_ONE)
		{
			c1 = *(const qse_char_t*)p;
			if (matcher->option & QSE_REX_MATCH_IGNORECASE) 
				c1 = QSE_TOUPPER(c1);
		#ifdef DEBUG_REX
			qse_dprintf (
				QSE_T("match_charset: <one> %c %c\n"), c, c1);
		#endif
			if (c == c1) return QSE_TRUE;
		}
		else if (c0 == CHARSET_RANGE)
		{
			c1 = *(const qse_char_t*)p;
			p += QSE_SIZEOF(c1);
			c2 = *(const qse_char_t*)p;

			if (matcher->option & QSE_REX_MATCH_IGNORECASE) 
			{
				c1 = QSE_TOUPPER(c1);
				c2 = QSE_TOUPPER(c2);
			}
		#ifdef DEBUG_REX
			qse_dprintf (
				QSE_T("match_charset: <range> %c %c-%c\n"), c, c1, c2);
		#endif
			if (c >= c1 && c <= c2) return QSE_TRUE;
		}
		else if (c0 == CHARSET_CLASS)
		{
			c1 = *(const qse_char_t*)p;
		#ifdef DEBUG_REX
			qse_dprintf (
				QSE_T("match_charset: <class> %c %s\n"), 
				c, __char_class[c1].name);
		#endif
			if (__char_class[c1].func(c)) return QSE_TRUE;
		}
		else
		{
			QSE_ASSERT (!"should never happen - invalid charset code");
			break;
		}

		p += QSE_SIZEOF(c1);
	}

	return QSE_FALSE;
}

#if 0
#define DPRINTF awk->prmfns.misc.dprintf
#define DCUSTOM awk->prmfns.misc.custom_data

void qse_awk_dprintrex (qse_awk_t* awk, void* rex)
{
	__print_pattern (awk, rex);
	DPRINTF (DCUSTOM, awk->prmfns.misc.custom_data, QSE_T("\n"));
}

static const qse_byte_t* __print_pattern (qse_awk_t* awk, const qse_byte_t* p)
{
	qse_size_t i;
	rhdr_t* rhdr;

	rhdr = (rhdr_t*)p; p += QSE_SIZEOF(*rhdr);

	for (i = 0; i < rhdr->nb; i++)
	{
		if (i != 0) DPRINTF (DCUSTOM, QSE_T("|"));
		p = __print_branch (awk, p);
	}

	return p;
}

static const qse_byte_t* __print_branch (qse_awk_t* awk, const qse_byte_t* p)
{
	qse_size_t i;
	bhdr_t* bhdr;
       
	bhdr = (bhdr_t*)p; p += QSE_SIZEOF(*bhdr);

	for (i = 0; i < bhdr->na; i++)
	{
		p = __print_atom (awk, p);
	}

	return p;
}

static const qse_byte_t* __print_atom (qse_awk_t* awk, const qse_byte_t* p)
{
	const code_t* cp = (const code_t*)p;

	if (cp->cmd == CMD_BOL)
	{
		DPRINTF (DCUSTOM, QSE_T("^"));
		p += QSE_SIZEOF(*cp);
	}
	else if (cp->cmd == CMD_EOL)
	{
		DPRINTF (DCUSTOM, QSE_T("$"));
		p += QSE_SIZEOF(*cp);
	}
	else if (cp->cmd == CMD_ANY_CHAR) 
	{
		DPRINTF (DCUSTOM, QSE_T("."));
		p += QSE_SIZEOF(*cp);
	}
	else if (cp->cmd == CMD_ORD_CHAR) 
	{
		p += QSE_SIZEOF(*cp);
		DPRINTF (DCUSTOM, QSE_T("%c"), *(qse_char_t*)p);
		p += QSE_SIZEOF(qse_char_t);
	}
	else if (cp->cmd == CMD_CHARSET)
	{
		qse_size_t i;
		cshdr_t* cshdr;

		p += QSE_SIZEOF(*cp);
		DPRINTF (DCUSTOM, QSE_T("["));
		if (cp->negate) DPRINTF (DCUSTOM, QSE_T("^"));

		cshdr = (cshdr_t*)p; p += QSE_SIZEOF(*cshdr);

		for (i = 0; i < cshdr->csc; i++)
		{
			qse_char_t c0, c1, c2;

			c0 = *(qse_char_t*)p;
			p += QSE_SIZEOF(c0);

			if (c0 == CHARSET_ONE)
			{
				c1 = *(qse_char_t*)p;
				DPRINTF (DCUSTOM, QSE_T("%c"), c1);
			}
			else if (c0 == CHARSET_RANGE)
			{
				c1 = *(qse_char_t*)p;
				p += QSE_SIZEOF(c1);
				c2 = *(qse_char_t*)p;
				DPRINTF (DCUSTOM, QSE_T("%c-%c"), c1, c2);
			}
			else if (c0 == CHARSET_CLASS)
			{
				c1 = *(qse_char_t*)p;
				DPRINTF (DCUSTOM, QSE_T("[:%s:]"), __char_class[c1].name);
			}
			else
			{
				DPRINTF (DCUSTOM, QSE_T("should never happen - invalid charset code\n"));
			}

			p += QSE_SIZEOF(c1);
		}

		DPRINTF (DCUSTOM, QSE_T("]"));
	}
	else if (cp->cmd == CMD_GROUP)
	{
		p += QSE_SIZEOF(*cp);
		DPRINTF (DCUSTOM, QSE_T("("));
		p = __print_pattern (awk, p);
		DPRINTF (DCUSTOM, QSE_T(")"));
	}
	else 
	{
		DPRINTF (DCUSTOM, QSE_T("should never happen - invalid atom code\n"));
	}

	if (cp->lbound == 0 && cp->ubound == BOUND_MAX)
		DPRINTF (DCUSTOM, QSE_T("*"));
	else if (cp->lbound == 1 && cp->ubound == BOUND_MAX)
		DPRINTF (DCUSTOM, QSE_T("+"));
	else if (cp->lbound == 0 && cp->ubound == 1)
		DPRINTF (DCUSTOM, QSE_T("?"));
	else if (cp->lbound != 1 || cp->ubound != 1)
	{
		DPRINTF (DCUSTOM, QSE_T("{%lu,%lu}"), 
			(unsigned long)cp->lbound, (unsigned long)cp->ubound);
	}

	return p;
}

#endif
