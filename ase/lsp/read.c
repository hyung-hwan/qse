/*
 * $Id: read.c,v 1.27 2006-10-29 13:00:39 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

#define IS_IDENT(c) \
	((c) == ASE_T('+') || (c) == ASE_T('-') || \
	 (c) == ASE_T('*') || (c) == ASE_T('/') || \
	 (c) == ASE_T('%') || (c) == ASE_T('&') || \
	 (c) == ASE_T('<') || (c) == ASE_T('>') || \
	 (c) == ASE_T('=') || (c) == ASE_T('_') || \
	 (c) == ASE_T('?'))

#define TOKEN_CLEAR(lsp)  ase_lsp_name_clear (&(lsp)->token.name)
#define TOKEN_TYPE(lsp)  (lsp)->token.type
#define TOKEN_IVAL(lsp)  (lsp)->token.ival
#define TOKEN_RVAL(lsp)  (lsp)->token.rval
#define TOKEN_SVAL(lsp)  (lsp)->token.name.buf
#define TOKEN_SLEN(lsp)  (lsp)->token.name.size

#define TOKEN_ADD_CHAR(lsp,ch) do { \
	if (ase_lsp_name_addc(&(lsp)->token.name, ch) == -1) { \
		lsp->errnum = ASE_LSP_ENOMEM; \
		return -1; \
	} \
} while (0)

#define TOKEN_COMPARE(lsp,str) ase_lsp_name_compare (&(lsp)->token.name, str)
		
#define TOKEN_END            0
#define TOKEN_INT            1
#define TOKEN_REAL           2
#define TOKEN_STRING         3
#define TOKEN_LPAREN         4
#define TOKEN_RPAREN         5
#define TOKEN_IDENT          6
#define TOKEN_QUOTE          7
#define TOKEN_DOT            8
#define TOKEN_INVALID        50
#define TOKEN_UNTERM_STRING  51

#define NEXT_CHAR(lsp) \
	do { if (read_char(lsp) == -1) return -1;} while (0)

#define NEXT_TOKEN(lsp) \
	do { if (read_token(lsp) == -1) return ASE_NULL; } while (0)

static ase_lsp_obj_t* read_obj   (ase_lsp_t* lsp);
static ase_lsp_obj_t* read_list  (ase_lsp_t* lsp);
static ase_lsp_obj_t* read_quote (ase_lsp_t* lsp);

static int read_char   (ase_lsp_t* lsp);
static int read_token  (ase_lsp_t* lsp);
static int read_number (ase_lsp_t* lsp, int negative);
static int read_ident  (ase_lsp_t* lsp);
static int read_string (ase_lsp_t* lsp);

ase_lsp_obj_t* ase_lsp_read (ase_lsp_t* lsp)
{
	if (lsp->curc == ASE_CHAR_EOF && 
	    read_char(lsp) == -1) return ASE_NULL;

	lsp->errnum = ASE_LSP_ENOERR;
	NEXT_TOKEN (lsp);

	if (lsp->mem->locked != ASE_NULL) 
	{
		ase_lsp_unlockallobjs (lsp, lsp->mem->locked);
		lsp->mem->locked = ASE_NULL;
	}
	lsp->mem->locked = read_obj (lsp);
	return lsp->mem->locked;
}

static ase_lsp_obj_t* read_obj (ase_lsp_t* lsp)
{
	ase_lsp_obj_t* obj;

	switch (TOKEN_TYPE(lsp)) 
	{
		case TOKEN_END:
			lsp->errnum = ASE_LSP_ERR_END;
			return ASE_NULL;

		case TOKEN_LPAREN:
			NEXT_TOKEN (lsp);
			return read_list (lsp);

		case TOKEN_QUOTE:
			NEXT_TOKEN (lsp);
			return read_quote (lsp);

		case TOKEN_INT:
			obj = ase_lsp_makeintobj (lsp->mem, TOKEN_IVAL(lsp));
			if (obj == ASE_NULL) lsp->errnum = ASE_LSP_ENOMEM;
			ase_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_REAL:
			obj = ase_lsp_makerealobj (lsp->mem, TOKEN_RVAL(lsp));
			if (obj == ASE_NULL) lsp->errnum = ASE_LSP_ENOMEM;
			ase_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_STRING:
			obj = ase_lsp_makestrobj (
				lsp->mem, TOKEN_SVAL(lsp), TOKEN_SLEN(lsp));
			if (obj == ASE_NULL) lsp->errnum = ASE_LSP_ENOMEM;
			ase_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_IDENT:
			ASE_LSP_ASSERT (lsp,
				lsp->mem->nil != ASE_NULL && lsp->mem->t != ASE_NULL); 
			if (TOKEN_COMPARE(lsp,ASE_T("nil")) == 0) obj = lsp->mem->nil;
			else if (TOKEN_COMPARE(lsp,ASE_T("t")) == 0) obj = lsp->mem->t;
			else 
			{
				obj = ase_lsp_makesymobj (
					lsp->mem, TOKEN_SVAL(lsp), TOKEN_SLEN(lsp));
				if (obj == ASE_NULL) lsp->errnum = ASE_LSP_ENOMEM;
				ase_lsp_lockobj (lsp, obj);
			}
			return obj;
	}

	lsp->errnum = ASE_LSP_ERR_SYNTAX;
	return ASE_NULL;
}

static ase_lsp_obj_t* read_list (ase_lsp_t* lsp)
{
	ase_lsp_obj_t* obj;
	ase_lsp_obj_cons_t* p, * first = ASE_NULL, * prev = ASE_NULL;

	while (TOKEN_TYPE(lsp) != TOKEN_RPAREN) 
	{
		if (TOKEN_TYPE(lsp) == TOKEN_END) 
		{
			lsp->errnum = ASE_LSP_ERR_SYNTAX; // unexpected end of input
			return ASE_NULL;
		}

		if (TOKEN_TYPE(lsp) == TOKEN_DOT) 
		{
			if (prev == ASE_NULL) {
				lsp->errnum = ASE_LSP_ERR_SYNTAX; // unexpected .
				return ASE_NULL;
			}

			NEXT_TOKEN (lsp);
			obj = read_obj (lsp);
			if (obj == ASE_NULL) 
			{
				if (lsp->errnum == ASE_LSP_ERR_END) 
				{
					//unexpected end of input
					lsp->errnum = ASE_LSP_ERR_SYNTAX; 
				}
				return ASE_NULL;
			}
			prev->cdr = obj;

			NEXT_TOKEN (lsp);
			if (TOKEN_TYPE(lsp) != TOKEN_RPAREN) 
			{
				lsp->errnum = ASE_LSP_ERR_SYNTAX; // ) expected
				return ASE_NULL;
			}

			break;
		}

		obj = read_obj (lsp);
		if (obj == ASE_NULL) 
		{
			if (lsp->errnum == ASE_LSP_ERR_END)
			{	
				// unexpected end of input
				lsp->errnum = ASE_LSP_ERR_SYNTAX;
			}
			return ASE_NULL;
		}

		p = (ase_lsp_obj_cons_t*)ase_lsp_makecons (
			lsp->mem, lsp->mem->nil, lsp->mem->nil);
		if (p == ASE_NULL) 
		{
			lsp->errnum = ASE_LSP_ENOMEM;
			return ASE_NULL;
		}
		ase_lsp_lockobj (lsp, (ase_lsp_obj_t*)p);

		if (first == ASE_NULL) first = p;
		if (prev != ASE_NULL) prev->cdr = (ase_lsp_obj_t*)p;

		p->car = obj;
		prev = p;

		NEXT_TOKEN (lsp);
	}	

	return (first == ASE_NULL)? lsp->mem->nil: (ase_lsp_obj_t*)first;
}

static ase_lsp_obj_t* read_quote (ase_lsp_t* lsp)
{
	ase_lsp_obj_t* cons, * tmp;

	tmp = read_obj (lsp);
	if (tmp == ASE_NULL) 
	{
		if (lsp->errnum == ASE_LSP_ERR_END) 
		{
			// unexpected end of input
			lsp->errnum = ASE_LSP_ERR_SYNTAX;
		}
		return ASE_NULL;
	}

	cons = ase_lsp_makecons (lsp->mem, tmp, lsp->mem->nil);
	if (cons == ASE_NULL) 
	{
		lsp->errnum = ASE_LSP_ENOMEM;
		return ASE_NULL;
	}
	ase_lsp_lockobj (lsp, cons);

	cons = ase_lsp_makecons (lsp->mem, lsp->mem->quote, cons);
	if (cons == ASE_NULL) 
	{
		lsp->errnum = ASE_LSP_ENOMEM;
		return ASE_NULL;
	}
	ase_lsp_lockobj (lsp, cons);

	return cons;
}

static int read_char (ase_lsp_t* lsp)
{
	ase_ssize_t n;
	ase_char_t c;

	if (lsp->input_func == ASE_NULL) 
	{
		lsp->errnum = ASE_LSP_ERR_INPUT_NOT_ATTACHED;
		return -1;
	}

	n = lsp->input_func(ASE_LSP_IO_READ, lsp->input_arg, &c, 1);
	if (n == -1) 
	{
		lsp->errnum = ASE_LSP_ERR_INPUT;
		return -1;
	}

	if (n == 0) lsp->curc = ASE_CHAR_EOF;
	else lsp->curc = c;
	return 0;
}

static int read_token (ase_lsp_t* lsp)
{
	ASE_LSP_ASSERT (lsp, lsp->input_func != ASE_NULL);

	TOKEN_CLEAR (lsp);

	while (1)
	{
		// skip white spaces
		while (ASE_LSP_ISSPACE(lsp, lsp->curc)) NEXT_CHAR (lsp);

		// skip the comments here
		if (lsp->curc == ASE_T(';')) 
		{
			do 
			{
				NEXT_CHAR (lsp);
			} 
			while (lsp->curc != ASE_T('\n') && 
			       lsp->curc != ASE_CHAR_EOF);
		}
		else break;
	}

	if (lsp->curc == ASE_CHAR_EOF) 
	{
		TOKEN_TYPE(lsp) = TOKEN_END;
		return 0;
	}
	else if (lsp->curc == ASE_T('(')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_LPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == ASE_T(')')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_RPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == ASE_T('\'')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_QUOTE;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == ASE_T('.')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_DOT;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == ASE_T('-')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
		if (ASE_LSP_ISDIGIT(lsp,lsp->curc)) 
		{
			return read_number (lsp, 1);
		}
		else if (IS_IDENT(lsp->curc)) 
		{
			return read_ident (lsp);
		}
		else 
		{
			TOKEN_TYPE(lsp) = TOKEN_IDENT;
			return 0;
		}
	}
	else if (ASE_LSP_ISDIGIT(lsp,lsp->curc)) 
	{
		return read_number (lsp, 0);
	}
	else if (ASE_LSP_ISALPHA(lsp,lsp->curc) || IS_IDENT(lsp->curc)) 
	{
		return read_ident (lsp);
	}
	else if (lsp->curc == ASE_T('\"')) 
	{
		NEXT_CHAR (lsp);
		return read_string (lsp);
	}

	TOKEN_TYPE(lsp) = TOKEN_INVALID;
	NEXT_CHAR (lsp); // consume
	return 0;
}

static int read_number (ase_lsp_t* lsp, int negative)
{
	ase_long_t ival = 0;
	ase_real_t rval = 0.;

	do 
	{
		ival = ival * 10 + (lsp->curc - ASE_T('0'));
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} 
	while (ASE_LSP_ISDIGIT(lsp,lsp->curc));

/* TODO: extend parsing floating point number  */
	if (lsp->curc == ASE_T('.')) 
	{
		ase_real_t fraction = 0.1;

		NEXT_CHAR (lsp);
		rval = (ase_real_t)ival;

		while (ASE_LSP_ISDIGIT(lsp, lsp->curc)) 
		{
			rval += (ase_real_t)(lsp->curc - ASE_T('0')) * fraction;
			fraction *= 0.1;
			NEXT_CHAR (lsp);
		}

		TOKEN_RVAL(lsp) = rval;
		TOKEN_TYPE(lsp) = TOKEN_REAL;
		if (negative) rval *= -1;
	}
	else {
		TOKEN_IVAL(lsp) = ival;
		TOKEN_TYPE(lsp) = TOKEN_INT;
		if (negative) ival *= -1;
	}

	return 0;
}

#if 0
static int __read_number (ase_lsp_t* lsp, int negative)
{
	ase_cint_t c;

	ASE_LSP_ASSERT (lsp, ASE_LSP_STR_LEN(&lsp->token.name) == 0);
	SET_TOKEN_TYPE (lsp, TOKEN_INT);

	c = lsp->src.lex.curc;

	if (c == ASE_T('0'))
	{
		ADD_TOKEN_CHAR (lsp, c);
		GET_CHAR_TO (lsp, c);

		if (c == ASE_T('x') || c == ASE_T('X'))
		{
			/* hexadecimal number */
			do 
			{
				ADD_TOKEN_CHAR (lsp, c);
				GET_CHAR_TO (lsp, c);
			} 
			while (ASE_LSP_ISXDIGIT (lsp, c));

			return 0;
		}
		else if (c == ASE_T('b') || c == ASE_T('B'))
		{
			/* binary number */
			do
			{
				ADD_TOKEN_CHAR (lsp, c);
				GET_CHAR_TO (lsp, c);
			} 
			while (c == ASE_T('0') || c == ASE_T('1'));

			return 0;
		}
		else if (c != '.')
		{
			/* octal number */
			while (c >= ASE_T('0') && c <= ASE_T('7'))
			{
				ADD_TOKEN_CHAR (lsp, c);
				GET_CHAR_TO (lsp, c);
			}

			return 0;
		}
	}

	while (ASE_LSP_ISDIGIT (lsp, c)) 
	{
		ADD_TOKEN_CHAR (lsp, c);
		GET_CHAR_TO (lsp, c);
	} 

	if (c == ASE_T('.'))
	{
		/* floating-point number */
		SET_TOKEN_TYPE (lsp, TOKEN_REAL);

		ADD_TOKEN_CHAR (lsp, c);
		GET_CHAR_TO (lsp, c);

		while (ASE_LSP_ISDIGIT (lsp, c))
		{
			ADD_TOKEN_CHAR (lsp, c);
			GET_CHAR_TO (lsp, c);
		}
	}

	if (c == ASE_T('E') || c == ASE_T('e'))
	{
		SET_TOKEN_TYPE (lsp, TOKEN_REAL);

		ADD_TOKEN_CHAR (lsp, c);
		GET_CHAR_TO (lsp, c);

		if (c == ASE_T('+') || c == ASE_T('-'))
		{
			ADD_TOKEN_CHAR (lsp, c);
			GET_CHAR_TO (lsp, c);
		}

		while (ASE_LSP_ISDIGIT (lsp, c))
		{
			ADD_TOKEN_CHAR (lsp, c);
			GET_CHAR_TO (lsp, c);
		}
	}

	return 0;
}
#endif

static int read_ident (ase_lsp_t* lsp)
{
	do 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} 
	while (ASE_LSP_ISALNUM(lsp,lsp->curc) || IS_IDENT(lsp->curc));
	TOKEN_TYPE(lsp) = TOKEN_IDENT;
	return 0;
}

static int read_string (ase_lsp_t* lsp)
{
	int escaped = 0;
	ase_cint_t code = 0;

	do 
	{
		if (lsp->curc == ASE_CHAR_EOF) 
		{
			TOKEN_TYPE(lsp) = TOKEN_UNTERM_STRING;
			return 0;
		}

		// TODO: 
		if (escaped == 3) 
		{
			/* \xNN */
		}
		else if (escaped == 2) 
		{
			/* \000 */
		}
		else if (escaped == 1) 
		{
			/* backslash + character */
			if (lsp->curc == ASE_T('a')) 
				lsp->curc = ASE_T('\a');
			else if (lsp->curc == ASE_T('b')) 
				lsp->curc = ASE_T('\b');
			else if (lsp->curc == ASE_T('f')) 
				lsp->curc = ASE_T('\f');
			else if (lsp->curc == ASE_T('n')) 
				lsp->curc = ASE_T('\n');
			else if (lsp->curc == ASE_T('r')) 
				lsp->curc = ASE_T('\r');
			else if (lsp->curc == ASE_T('t')) 
				lsp->curc = ASE_T('\t');
			else if (lsp->curc == ASE_T('v')) 
				lsp->curc = ASE_T('\v');
			else if (lsp->curc == ASE_T('0')) 
			{
				escaped = 2;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
			else if (lsp->curc == ASE_T('x')) 
			{
				escaped = 3;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
		}
		else if (lsp->curc == ASE_T('\\')) 
		{
			escaped = 1;
			NEXT_CHAR (lsp);
			continue;
		}

		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} 
	while (lsp->curc != ASE_T('\"'));

	TOKEN_TYPE(lsp) = TOKEN_STRING;
	NEXT_CHAR (lsp);

	return 0;
}
