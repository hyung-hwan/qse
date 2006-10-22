/*
 * $Id: read.c,v 1.18 2006-10-22 13:10:46 bacon Exp $
 */

#include <sse/lsp/lsp.h>
#include <sse/lsp/token.h>
#include <sse/bas/assert.h>
#include <sse/bas/ctype.h>

#define IS_SPACE(x) sse_isspace(x)
#define IS_DIGIT(x) sse_isdigit(x)
#define IS_ALPHA(x) sse_isalpha(x)
#define IS_ALNUM(x) sse_isalnum(x)

#define IS_IDENT(c) \
	((c) == SSE_CHAR('+') || (c) == SSE_CHAR('-') || \
	 (c) == SSE_CHAR('*') || (c) == SSE_CHAR('/') || \
	 (c) == SSE_CHAR('%') || (c) == SSE_CHAR('&') || \
	 (c) == SSE_CHAR('<') || (c) == SSE_CHAR('>') || \
	 (c) == SSE_CHAR('=') || (c) == SSE_CHAR('_') || \
	 (c) == SSE_CHAR('?'))

#define TOKEN_CLEAR(lsp)   sse_lsp_token_clear (&(lsp)->token)
#define TOKEN_TYPE(lsp)    (lsp)->token.type
#define TOKEN_IVALUE(lsp)  (lsp)->token.ivalue
#define TOKEN_RVALUE(lsp)  (lsp)->token.rvalue
#define TOKEN_SVALUE(lsp)  (lsp)->token.name.buffer
#define TOKEN_SLENGTH(lsp) (lsp)->token.name.size

#define TOKEN_ADD_CHAR(lsp,ch) do { \
	if (sse_lsp_token_addc(&(lsp)->token, ch) == -1) { \
		lsp->errnum = SSE_LSP_ERR_MEMORY; \
		return -1; \
	} \
} while (0)

#define TOKEN_COMPARE(lsp,str) sse_lsp_token_compare_name (&(lsp)->token, str)
		
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
	do { if (read_token(lsp) == -1) return SSE_NULL; } while (0)

static sse_lsp_obj_t* read_obj   (sse_lsp_t* lsp);
static sse_lsp_obj_t* read_list  (sse_lsp_t* lsp);
static sse_lsp_obj_t* read_quote (sse_lsp_t* lsp);

static int read_char   (sse_lsp_t* lsp);
static int read_token  (sse_lsp_t* lsp);
static int read_number (sse_lsp_t* lsp, int negative);
static int read_ident  (sse_lsp_t* lsp);
static int read_string (sse_lsp_t* lsp);

sse_lsp_obj_t* sse_lsp_read (sse_lsp_t* lsp)
{
	if (lsp->curc == SSE_CHAR_EOF && 
	    read_char(lsp) == -1) return SSE_NULL;

	lsp->errnum = SSE_LSP_ERR_NONE;
	NEXT_TOKEN (lsp);

	if (lsp->mem->locked != SSE_NULL) {
		sse_lsp_unlock_all (lsp->mem->locked);
		lsp->mem->locked = SSE_NULL;
	}
	lsp->mem->locked = read_obj (lsp);
	return lsp->mem->locked;
}

static sse_lsp_obj_t* read_obj (sse_lsp_t* lsp)
{
	sse_lsp_obj_t* obj;

	switch (TOKEN_TYPE(lsp)) {
	case TOKEN_END:
		lsp->errnum = SSE_LSP_ERR_END;
		return SSE_NULL;
	case TOKEN_LPAREN:
		NEXT_TOKEN (lsp);
		return read_list (lsp);
	case TOKEN_QUOTE:
		NEXT_TOKEN (lsp);
		return read_quote (lsp);
	case TOKEN_INT:
		obj = sse_lsp_make_int (lsp->mem, TOKEN_IVALUE(lsp));
		if (obj == SSE_NULL) lsp->errnum = SSE_LSP_ERR_MEMORY;
		sse_lsp_lock (obj);
		return obj;
	case TOKEN_REAL:
		obj = sse_lsp_make_real (lsp->mem, TOKEN_RVALUE(lsp));
		if (obj == SSE_NULL) lsp->errnum = SSE_LSP_ERR_MEMORY;
		sse_lsp_lock (obj);
		return obj;
	case TOKEN_STRING:
		obj = sse_lsp_make_stringx (
			lsp->mem, TOKEN_SVALUE(lsp), TOKEN_SLENGTH(lsp));
		if (obj == SSE_NULL) lsp->errnum = SSE_LSP_ERR_MEMORY;
		sse_lsp_lock (obj);
		return obj;
	case TOKEN_IDENT:
		sse_assert (lsp->mem->nil != SSE_NULL && lsp->mem->t != SSE_NULL); 
		if (TOKEN_COMPARE(lsp,SSE_TEXT("nil")) == 0) obj = lsp->mem->nil;
		else if (TOKEN_COMPARE(lsp,SSE_TEXT("t")) == 0) obj = lsp->mem->t;
		else {
			obj = sse_lsp_make_symbolx (
				lsp->mem, TOKEN_SVALUE(lsp), TOKEN_SLENGTH(lsp));
			if (obj == SSE_NULL) lsp->errnum = SSE_LSP_ERR_MEMORY;
			sse_lsp_lock (obj);
		}
		return obj;
	}

	lsp->errnum = SSE_LSP_ERR_SYNTAX;
	return SSE_NULL;
}

static sse_lsp_obj_t* read_list (sse_lsp_t* lsp)
{
	sse_lsp_obj_t* obj;
	sse_lsp_obj_cons_t* p, * first = SSE_NULL, * prev = SSE_NULL;

	while (TOKEN_TYPE(lsp) != TOKEN_RPAREN) {
		if (TOKEN_TYPE(lsp) == TOKEN_END) {
			lsp->errnum = SSE_LSP_ERR_SYNTAX; // unexpected end of input
			return SSE_NULL;
		}

		if (TOKEN_TYPE(lsp) == TOKEN_DOT) {
			if (prev == SSE_NULL) {
				lsp->errnum = SSE_LSP_ERR_SYNTAX; // unexpected .
				return SSE_NULL;
			}

			NEXT_TOKEN (lsp);
			obj = read_obj (lsp);
			if (obj == SSE_NULL) {
				if (lsp->errnum == SSE_LSP_ERR_END) {
					//unexpected end of input
					lsp->errnum = SSE_LSP_ERR_SYNTAX; 
				}
				return SSE_NULL;
			}
			prev->cdr = obj;

			NEXT_TOKEN (lsp);
			if (TOKEN_TYPE(lsp) != TOKEN_RPAREN) {
				lsp->errnum = SSE_LSP_ERR_SYNTAX; // ) expected
				return SSE_NULL;
			}

			break;
		}

		obj = read_obj (lsp);
		if (obj == SSE_NULL) {
			if (lsp->errnum == SSE_LSP_ERR_END) { 
				// unexpected end of input
				lsp->errnum = SSE_LSP_ERR_SYNTAX;
			}
			return SSE_NULL;
		}

		p = (sse_lsp_obj_cons_t*)sse_lsp_make_cons (
			lsp->mem, lsp->mem->nil, lsp->mem->nil);
		if (p == SSE_NULL) {
			lsp->errnum = SSE_LSP_ERR_MEMORY;
			return SSE_NULL;
		}
		sse_lsp_lock ((sse_lsp_obj_t*)p);

		if (first == SSE_NULL) first = p;
		if (prev != SSE_NULL) prev->cdr = (sse_lsp_obj_t*)p;

		p->car = obj;
		prev = p;

		NEXT_TOKEN (lsp);
	}	

	return (first == SSE_NULL)? lsp->mem->nil: (sse_lsp_obj_t*)first;
}

static sse_lsp_obj_t* read_quote (sse_lsp_t* lsp)
{
	sse_lsp_obj_t* cons, * tmp;

	tmp = read_obj (lsp);
	if (tmp == SSE_NULL) {
		if (lsp->errnum == SSE_LSP_ERR_END) {
			// unexpected end of input
			lsp->errnum = SSE_LSP_ERR_SYNTAX;
		}
		return SSE_NULL;
	}

	cons = sse_lsp_make_cons (lsp->mem, tmp, lsp->mem->nil);
	if (cons == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}
	sse_lsp_lock (cons);

	cons = sse_lsp_make_cons (lsp->mem, lsp->mem->quote, cons);
	if (cons == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_MEMORY;
		return SSE_NULL;
	}
	sse_lsp_lock (cons);

	return cons;
}

static int read_char (sse_lsp_t* lsp)
{
	sse_ssize_t n;

	if (lsp->input_func == SSE_NULL) {
		lsp->errnum = SSE_LSP_ERR_INPUT_NOT_ATTACHED;
		return -1;
	}

	n = lsp->input_func(SSE_LSP_IO_DATA, lsp->input_arg, &lsp->curc, 1);
	if (n == -1) {
		lsp->errnum = SSE_LSP_ERR_INPUT;
		return -1;
	}

	if (n == 0) lsp->curc = SSE_CHAR_EOF;
	return 0;
}

static int read_token (sse_lsp_t* lsp)
{
	sse_assert (lsp->input_func != SSE_NULL);

	TOKEN_CLEAR (lsp);

	for (;;) {
		// skip white spaces
		while (IS_SPACE(lsp->curc)) NEXT_CHAR (lsp);

		// skip the comments here
		if (lsp->curc == SSE_CHAR(';')) {
			do {
				NEXT_CHAR (lsp);
			} while (lsp->curc != SSE_CHAR('\n') && lsp->curc != SSE_CHAR_EOF);
		}
		else break;
	}

	if (lsp->curc == SSE_CHAR_EOF) {
		TOKEN_TYPE(lsp) = TOKEN_END;
		return 0;
	}
	else if (lsp->curc == SSE_CHAR('(')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_LPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == SSE_CHAR(')')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_RPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == SSE_CHAR('\'')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_QUOTE;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == SSE_CHAR('.')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_DOT;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == SSE_CHAR('-')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
		if (IS_DIGIT(lsp->curc)) {
			return read_number (lsp, 1);
		}
		else if (IS_IDENT(lsp->curc)) {
			return read_ident (lsp);
		}
		else {
			TOKEN_TYPE(lsp) = TOKEN_IDENT;
			return 0;
		}
	}
	else if (IS_DIGIT(lsp->curc)) {
		return read_number (lsp, 0);
	}
	else if (IS_ALPHA(lsp->curc) || IS_IDENT(lsp->curc)) {
		return read_ident (lsp);
	}
	else if (lsp->curc == SSE_CHAR('\"')) {
		NEXT_CHAR (lsp);
		return read_string (lsp);
	}

	TOKEN_TYPE(lsp) = TOKEN_INVALID;
	NEXT_CHAR (lsp); // consume
	return 0;
}

static int read_number (sse_lsp_t* lsp, int negative)
{
	sse_lsp_int_t ivalue = 0;
	sse_lsp_real_t rvalue = 0.;

	do {
		ivalue = ivalue * 10 + (lsp->curc - SSE_CHAR('0'));
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (IS_DIGIT(lsp->curc));

/* TODO: extend parsing floating point number  */
	if (lsp->curc == SSE_CHAR('.')) {
		sse_lsp_real_t fraction = 0.1;

		NEXT_CHAR (lsp);
		rvalue = (sse_lsp_real_t)ivalue;

		while (IS_DIGIT(lsp->curc)) {
			rvalue += (sse_lsp_real_t)(lsp->curc - SSE_CHAR('0')) * fraction;
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

static int read_ident (sse_lsp_t* lsp)
{
	do {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (IS_ALNUM(lsp->curc) || IS_IDENT(lsp->curc));
	TOKEN_TYPE(lsp) = TOKEN_IDENT;
	return 0;
}

static int read_string (sse_lsp_t* lsp)
{
	int escaped = 0;
	sse_cint_t code = 0;

	do {
		if (lsp->curc == SSE_CHAR_EOF) {
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
			if (lsp->curc == SSE_CHAR('a')) 
				lsp->curc = SSE_CHAR('\a');
			else if (lsp->curc == SSE_CHAR('b')) 
				lsp->curc = SSE_CHAR('\b');
			else if (lsp->curc == SSE_CHAR('f')) 
				lsp->curc = SSE_CHAR('\f');
			else if (lsp->curc == SSE_CHAR('n')) 
				lsp->curc = SSE_CHAR('\n');
			else if (lsp->curc == SSE_CHAR('r')) 
				lsp->curc = SSE_CHAR('\r');
			else if (lsp->curc == SSE_CHAR('t')) 
				lsp->curc = SSE_CHAR('\t');
			else if (lsp->curc == SSE_CHAR('v')) 
				lsp->curc = SSE_CHAR('\v');
			else if (lsp->curc == SSE_CHAR('0')) {
				escaped = 2;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
			else if (lsp->curc == SSE_CHAR('x')) {
				escaped = 3;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
		}
		else if (lsp->curc == SSE_CHAR('\\')) {
			escaped = 1;
			NEXT_CHAR (lsp);
			continue;
		}

		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (lsp->curc != SSE_CHAR('\"'));

	TOKEN_TYPE(lsp) = TOKEN_STRING;
	NEXT_CHAR (lsp);

	return 0;
}
