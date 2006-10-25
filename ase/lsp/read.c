/*
 * $Id: read.c,v 1.22 2006-10-25 13:42:31 bacon Exp $
 */

#include <ase/lsp/lsp_i.h>

#define IS_SPACE(x) ase_isspace(x)
#define IS_DIGIT(x) ase_isdigit(x)
#define IS_ALPHA(x) ase_isalpha(x)
#define IS_ALNUM(x) ase_isalnum(x)

#define IS_IDENT(c) \
	((c) == ASE_T('+') || (c) == ASE_T('-') || \
	 (c) == ASE_T('*') || (c) == ASE_T('/') || \
	 (c) == ASE_T('%') || (c) == ASE_T('&') || \
	 (c) == ASE_T('<') || (c) == ASE_T('>') || \
	 (c) == ASE_T('=') || (c) == ASE_T('_') || \
	 (c) == ASE_T('?'))

#define TOKEN_CLEAR(lsp)   ase_lsp_token_clear (&(lsp)->token)
#define TOKEN_TYPE(lsp)    (lsp)->token.type
#define TOKEN_IVALUE(lsp)  (lsp)->token.ivalue
#define TOKEN_RVALUE(lsp)  (lsp)->token.rvalue
#define TOKEN_SVALUE(lsp)  (lsp)->token.name.buffer
#define TOKEN_SLENGTH(lsp) (lsp)->token.name.size

#define TOKEN_ADD_CHAR(lsp,ch) do { \
	if (ase_lsp_token_addc(&(lsp)->token, ch) == -1) { \
		lsp->errnum = ASE_LSP_ENOMEM; \
		return -1; \
	} \
} while (0)

#define TOKEN_COMPARE(lsp,str) ase_lsp_token_compare_name (&(lsp)->token, str)
		
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

	switch (TOKEN_TYPE(lsp)) {
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
		obj = ase_lsp_makeintobj (lsp->mem, TOKEN_IVALUE(lsp));
		if (obj == ASE_NULL) lsp->errnum = ASE_LSP_ENOMEM;
		ase_lsp_lockobj (lsp, obj);
		return obj;
	case TOKEN_REAL:
		obj = ase_lsp_makerealobj (lsp->mem, TOKEN_RVALUE(lsp));
		if (obj == ASE_NULL) lsp->errnum = ASE_LSP_ENOMEM;
		ase_lsp_lockobj (lsp, obj);
		return obj;
	case TOKEN_STRING:
		obj = ase_lsp_makestrobj (
			lsp->mem, TOKEN_SVALUE(lsp), TOKEN_SLENGTH(lsp));
		if (obj == ASE_NULL) lsp->errnum = ASE_LSP_ENOMEM;
		ase_lsp_lockobj (lsp, obj);
		return obj;
	case TOKEN_IDENT:
		ase_assert (lsp->mem->nil != ASE_NULL && lsp->mem->t != ASE_NULL); 
		if (TOKEN_COMPARE(lsp,ASE_T("nil")) == 0) obj = lsp->mem->nil;
		else if (TOKEN_COMPARE(lsp,ASE_T("t")) == 0) obj = lsp->mem->t;
		else 
		{
			obj = ase_lsp_makesymobj (
				lsp->mem, TOKEN_SVALUE(lsp), TOKEN_SLENGTH(lsp));
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

	while (TOKEN_TYPE(lsp) != TOKEN_RPAREN) {
		if (TOKEN_TYPE(lsp) == TOKEN_END) {
			lsp->errnum = ASE_LSP_ERR_SYNTAX; // unexpected end of input
			return ASE_NULL;
		}

		if (TOKEN_TYPE(lsp) == TOKEN_DOT) {
			if (prev == ASE_NULL) {
				lsp->errnum = ASE_LSP_ERR_SYNTAX; // unexpected .
				return ASE_NULL;
			}

			NEXT_TOKEN (lsp);
			obj = read_obj (lsp);
			if (obj == ASE_NULL) {
				if (lsp->errnum == ASE_LSP_ERR_END) {
					//unexpected end of input
					lsp->errnum = ASE_LSP_ERR_SYNTAX; 
				}
				return ASE_NULL;
			}
			prev->cdr = obj;

			NEXT_TOKEN (lsp);
			if (TOKEN_TYPE(lsp) != TOKEN_RPAREN) {
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

	if (lsp->input_func == ASE_NULL) 
	{
		lsp->errnum = ASE_LSP_ERR_INPUT_NOT_ATTACHED;
		return -1;
	}

	n = lsp->input_func(ASE_LSP_IO_DATA, lsp->input_arg, &lsp->curc, 1);
	if (n == -1) 
	{
		lsp->errnum = ASE_LSP_ERR_INPUT;
		return -1;
	}

	if (n == 0) lsp->curc = ASE_CHAR_EOF;
	return 0;
}

static int read_token (ase_lsp_t* lsp)
{
	ase_assert (lsp->input_func != ASE_NULL);

	TOKEN_CLEAR (lsp);

	while (1)
	{
		// skip white spaces
		while (IS_SPACE(lsp->curc)) NEXT_CHAR (lsp);

		// skip the comments here
		if (lsp->curc == ASE_T(';')) 
		{
			do 
			{
				NEXT_CHAR (lsp);
			} 
			while (lsp->curc != ASE_T('\n') && lsp->curc != ASE_CHAR_EOF);
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
		if (IS_DIGIT(lsp->curc)) 
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
	else if (IS_DIGIT(lsp->curc)) 
	{
		return read_number (lsp, 0);
	}
	else if (IS_ALPHA(lsp->curc) || IS_IDENT(lsp->curc)) 
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
	ase_long_t ivalue = 0;
	ase_real_t rvalue = 0.;

	do 
	{
		ivalue = ivalue * 10 + (lsp->curc - ASE_T('0'));
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} 
	while (IS_DIGIT(lsp->curc));

/* TODO: extend parsing floating point number  */
	if (lsp->curc == ASE_T('.')) 
	{
		ase_real_t fraction = 0.1;

		NEXT_CHAR (lsp);
		rvalue = (ase_real_t)ivalue;

		while (IS_DIGIT(lsp->curc)) 
		{
			rvalue += (ase_real_t)(lsp->curc - ASE_T('0')) * fraction;
			fraction *= 0.1;
			NEXT_CHAR (lsp);
		}

		TOKEN_RVALUE(lsp) = rvalue;
		TOKEN_TYPE(lsp) = TOKEN_REAL;
		if (negative) rvalue *= -1;
	}
	else {
		TOKEN_IVALUE(lsp) = ivalue;
		TOKEN_TYPE(lsp) = TOKEN_INT;
		if (negative) ivalue *= -1;
	}

	return 0;
}

static int read_ident (ase_lsp_t* lsp)
{
	do {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (IS_ALNUM(lsp->curc) || IS_IDENT(lsp->curc));
	TOKEN_TYPE(lsp) = TOKEN_IDENT;
	return 0;
}

static int read_string (ase_lsp_t* lsp)
{
	int escaped = 0;
	ase_cint_t code = 0;

	do {
		if (lsp->curc == ASE_CHAR_EOF) {
			TOKEN_TYPE(lsp) = TOKEN_UNTERM_STRING;
			return 0;
		}

		// TODO: 
		if (escaped == 3) {
			/* \xNN */
		}
		else if (escaped == 2) {
			/* \000 */
		}
		else if (escaped == 1) {
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
			else if (lsp->curc == ASE_T('0')) {
				escaped = 2;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
			else if (lsp->curc == ASE_T('x')) {
				escaped = 3;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
		}
		else if (lsp->curc == ASE_T('\\')) {
			escaped = 1;
			NEXT_CHAR (lsp);
			continue;
		}

		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (lsp->curc != ASE_T('\"'));

	TOKEN_TYPE(lsp) = TOKEN_STRING;
	NEXT_CHAR (lsp);

	return 0;
}
