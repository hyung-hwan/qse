/*
 * $Id: read.c,v 1.4 2005-02-05 05:18:20 bacon Exp $
 */

#include <xp/lisp/lisp.h>
#include <xp/lisp/token.h>

#define IS_SPACE(x) xp_isspace(x)
#define IS_DIGIT(x) xp_isdigit(x)
#define IS_ALPHA(x) xp_isalpha(x)
#define IS_ALNUM(x) xp_isalnum(x)

#define IS_IDENT(c) \
	((c) == XP_CHAR('+') || (c) == XP_CHAR('-') || \
	 (c) == XP_CHAR('*') || (c) == XP_CHAR('/') || \
	 (c) == XP_CHAR('%') || (c) == XP_CHAR('&') || \
	 (c) == XP_CHAR('<') || (c) == XP_CHAR('>') || \
	 (c) == XP_CHAR('=') || (c) == XP_CHAR('_') || \
	 (c) == XP_CHAR('?'))

#define TOKEN_CLEAR(lsp)   xp_lisp_token_clear (lsp->token)
#define TOKEN_TYPE(lsp)    lsp->token->type
#define TOKEN_IVALUE(lsp)  lsp->token->ivalue
#define TOKEN_FVALUE(lsp)  lsp->token->fvalue
#define TOKEN_SVALUE(lsp)  lsp->token->buffer
#define TOKEN_SLENGTH(lsp) lsp->token->size
#define TOKEN_ADD_CHAR(lsp,ch) \
	do { \
		if (xp_lisp_token_addc (lsp->token, ch) == -1) { \
			lsp->error = XP_LISP_ERR_MEM; \
			return -1; \
		} \
	} while (0)
#define TOKEN_COMPARE(lsp,str) xp_lisp_token_compare (lsp->token, str)
		

#define TOKEN_END            0
#define TOKEN_INT            1
#define TOKEN_FLOAT          2
#define TOKEN_STRING         3
#define TOKEN_LPAREN         4
#define TOKEN_RPAREN         5
#define TOKEN_IDENT          6
#define TOKEN_QUOTE          7
#define TOKEN_DOT            8
#define TOKEN_INVALID        50
#define TOKEN_UNTERM_STRING  51

#ifdef __cplusplus
extern "C" {
#endif

static xp_lisp_obj_t* read_obj   (xp_lisp_t* lsp);
static xp_lisp_obj_t* read_list  (xp_lisp_t* lsp);
static xp_lisp_obj_t* read_quote (xp_lisp_t* lsp);

static int read_token  (xp_lisp_t* lsp);
static int read_number (xp_lisp_t* lsp, int negative);
static int read_ident  (xp_lisp_t* lsp);
static int read_string (xp_lisp_t* lsp);

#ifdef __cplusplus
}
#endif

#define NEXT_CHAR(lsp) \
	do { \
		if (lsp->creader (&lsp->curc, lsp->creader_extra) == -1) { \
			lsp->error = XP_LISP_ERR_READ; \
			return -1; \
		} \
	} while (0) 

#define NEXT_TOKEN(lsp) \
	do { \
		if (read_token(lsp) == -1) return XP_NULL; \
	} while (0)


void xp_lisp_set_creader (xp_lisp_t* lsp, xp_lisp_creader_t func, void* extra)
{
	xp_assert (lsp != XP_NULL);

	lsp->creader          = func;
	lsp->creader_extra    = extra;
	lsp->creader_just_set = 1;
}

xp_lisp_obj_t* xp_lisp_read (xp_lisp_t* lsp)
{
	xp_assert (lsp != XP_NULL && lsp->creader != XP_NULL);

	if (lsp->creader_just_set) {
		// NEXT_CHAR (lsp);
		if (lsp->creader (&lsp->curc, lsp->creader_extra) == -1) {
			lsp->error = XP_LISP_ERR_READ;
			return XP_NULL;
		}
		lsp->creader_just_set = 0;
	}

	lsp->error = XP_LISP_ERR_NONE;
	NEXT_TOKEN (lsp);

	if (lsp->mem->locked != XP_NULL) {
		xp_lisp_unlock_all (lsp->mem->locked);
		lsp->mem->locked = XP_NULL;
	}
	lsp->mem->locked = read_obj (lsp);
	return lsp->mem->locked;
}

static xp_lisp_obj_t* read_obj (xp_lisp_t* lsp)
{
	xp_lisp_obj_t* obj;

	switch (TOKEN_TYPE(lsp)) {
	case TOKEN_END:
		lsp->error = XP_LISP_ERR_END;
		return XP_NULL;
	case TOKEN_LPAREN:
		NEXT_TOKEN (lsp);
		return read_list (lsp);
	case TOKEN_QUOTE:
		NEXT_TOKEN (lsp);
		return read_quote (lsp);
	case TOKEN_INT:
		obj = xp_lisp_make_int (lsp->mem, TOKEN_IVALUE(lsp));
		if (obj == XP_NULL) lsp->error = XP_LISP_ERR_MEM;
		xp_lisp_lock (obj);
		return obj;
	case TOKEN_FLOAT:
		obj = xp_lisp_make_float (lsp->mem, TOKEN_FVALUE(lsp));
		if (obj == XP_NULL) lsp->error = XP_LISP_ERR_MEM;
		xp_lisp_lock (obj);
		return obj;
	case TOKEN_STRING:
		obj = xp_lisp_make_string (
			lsp->mem, TOKEN_SVALUE(lsp), TOKEN_SLENGTH(lsp));
		if (obj == XP_NULL) lsp->error = XP_LISP_ERR_MEM;
		xp_lisp_lock (obj);
		return obj;
	case TOKEN_IDENT:
		xp_assert (lsp->mem->nil != XP_NULL && lsp->mem->t != XP_NULL); 
		if (TOKEN_COMPARE(lsp,XP_TEXT("nil")) == 0) obj = lsp->mem->nil;
		else if (TOKEN_COMPARE(lsp,XP_TEXT("t")) == 0) obj = lsp->mem->t;
		else {
			obj = xp_lisp_make_symbol (
				lsp->mem, TOKEN_SVALUE(lsp), TOKEN_SLENGTH(lsp));
			if (obj == XP_NULL) lsp->error = XP_LISP_ERR_MEM;
			xp_lisp_lock (obj);
		}
		return obj;
	}

	lsp->error = XP_LISP_ERR_SYNTAX;
	return XP_NULL;
}

static xp_lisp_obj_t* read_list (xp_lisp_t* lsp)
{
	xp_lisp_obj_t* obj;
	xp_lisp_obj_cons_t* p, * first = XP_NULL, * prev = XP_NULL;

	while (TOKEN_TYPE(lsp) != TOKEN_RPAREN) {
		if (TOKEN_TYPE(lsp) == TOKEN_END) {
			lsp->error = XP_LISP_ERR_SYNTAX; // unexpected end of input
			return XP_NULL;
		}

		if (TOKEN_TYPE(lsp) == TOKEN_DOT) {
			if (prev == XP_NULL) {
				lsp->error = XP_LISP_ERR_SYNTAX; // unexpected .
				return XP_NULL;
			}

			NEXT_TOKEN (lsp);
			obj = read_obj (lsp);
			if (obj == XP_NULL) {
				if (lsp->error == XP_LISP_ERR_END) {
					//unexpected end of input
					lsp->error = XP_LISP_ERR_SYNTAX; 
				}
				return XP_NULL;
			}
			prev->cdr = obj;

			NEXT_TOKEN (lsp);
			if (TOKEN_TYPE(lsp) != TOKEN_RPAREN) {
				lsp->error = XP_LISP_ERR_SYNTAX; // ) expected
				return XP_NULL;
			}

			break;
		}

		obj = read_obj (lsp);
		if (obj == XP_NULL) {
			if (lsp->error == XP_LISP_ERR_END) { 
				// unexpected end of input
				lsp->error = XP_LISP_ERR_SYNTAX;
			}
			return XP_NULL;
		}

		p = (xp_lisp_obj_cons_t*)xp_lisp_make_cons (
			lsp->mem, lsp->mem->nil, lsp->mem->nil);
		if (p == XP_NULL) {
			lsp->error = XP_LISP_ERR_MEM;
			return XP_NULL;
		}
		xp_lisp_lock ((xp_lisp_obj_t*)p);

		if (first == XP_NULL) first = p;
		if (prev != XP_NULL) prev->cdr = (xp_lisp_obj_t*)p;

		p->car = obj;
		prev = p;

		NEXT_TOKEN (lsp);
	}	

	return (first == XP_NULL)? lsp->mem->nil: (xp_lisp_obj_t*)first;
}

static xp_lisp_obj_t* read_quote (xp_lisp_t* lsp)
{
	xp_lisp_obj_t* cons, * tmp;

	tmp = read_obj (lsp);
	if (tmp == XP_NULL) {
		if (lsp->error == XP_LISP_ERR_END) {
			// unexpected end of input
			lsp->error = XP_LISP_ERR_SYNTAX;
		}
		return XP_NULL;
	}

	cons = xp_lisp_make_cons (lsp->mem, tmp, lsp->mem->nil);
	if (cons == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}
	xp_lisp_lock (cons);

	cons = xp_lisp_make_cons (lsp->mem, lsp->mem->quote, cons);
	if (cons == XP_NULL) {
		lsp->error = XP_LISP_ERR_MEM;
		return XP_NULL;
	}
	xp_lisp_lock (cons);

	return cons;
}

static int read_token (xp_lisp_t* lsp)
{
	xp_assert (lsp->creader != XP_NULL);

	TOKEN_CLEAR (lsp);

	for (;;) {
		// skip white spaces
		while (IS_SPACE(lsp->curc)) NEXT_CHAR (lsp);

		// skip the comments here
		if (lsp->curc == XP_CHAR(';')) {
			do {
				NEXT_CHAR (lsp);
			} while (lsp->curc != XP_CHAR('\n') && lsp->curc != XP_EOF);
		}
		else break;
	}

	if (lsp->curc == XP_EOF) {
		TOKEN_TYPE(lsp) = TOKEN_END;
		return 0;
	}
	else if (lsp->curc == XP_CHAR('(')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_LPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == XP_CHAR(')')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_RPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == XP_CHAR('\'')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_QUOTE;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == XP_CHAR('.')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_DOT;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == XP_CHAR('-')) {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
		return (IS_DIGIT(lsp->curc))? 
			read_number (lsp, 1): read_ident (lsp);
	}
	else if (IS_DIGIT(lsp->curc)) {
		return read_number (lsp, 0);
	}
	else if (IS_ALPHA(lsp->curc) || IS_IDENT(lsp->curc)) {
		return read_ident (lsp);
	}
	else if (lsp->curc == XP_CHAR('\"')) {
		NEXT_CHAR (lsp);
		return read_string (lsp);
	}

	TOKEN_TYPE(lsp) = TOKEN_INVALID;
	NEXT_CHAR (lsp); // consume
	return 0;
}

static int read_number (xp_lisp_t* lsp, int negative)
{
	do {
		TOKEN_IVALUE(lsp) = 
			TOKEN_IVALUE(lsp) * 10 + lsp->curc - XP_CHAR('0');
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (IS_DIGIT(lsp->curc));

	if (negative) TOKEN_IVALUE(lsp) *= -1; 
	TOKEN_TYPE(lsp) = TOKEN_INT;

	// TODO: read floating point numbers

	return 0;
}

static int read_ident (xp_lisp_t* lsp)
{
	do {
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (IS_ALNUM(lsp->curc) || IS_IDENT(lsp->curc));
	TOKEN_TYPE(lsp) = TOKEN_IDENT;
	return 0;
}

static int read_string (xp_lisp_t* lsp)
{
	int escaped = 0;
	xp_lisp_cint code = 0;

	do {
		if (lsp->curc == XP_EOF) {
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
			if (lsp->curc == XP_CHAR('a')) 
				lsp->curc = XP_CHAR('\a');
			else if (lsp->curc == XP_CHAR('b')) 
				lsp->curc = XP_CHAR('\b');
			else if (lsp->curc == XP_CHAR('f')) 
				lsp->curc = XP_CHAR('\f');
			else if (lsp->curc == XP_CHAR('n')) 
				lsp->curc = XP_CHAR('\n');
			else if (lsp->curc == XP_CHAR('r')) 
				lsp->curc = XP_CHAR('\r');
			else if (lsp->curc == XP_CHAR('t')) 
				lsp->curc = XP_CHAR('\t');
			else if (lsp->curc == XP_CHAR('v')) 
				lsp->curc = XP_CHAR('\v');
			else if (lsp->curc == XP_CHAR('0')) {
				escaped = 2;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
			else if (lsp->curc == XP_CHAR('x')) {
				escaped = 3;
				code = 0;
				NEXT_CHAR (lsp);
				continue;
			}
		}
		else if (lsp->curc == XP_CHAR('\\')) {
			escaped = 1;
			NEXT_CHAR (lsp);
			continue;
		}

		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} while (lsp->curc != XP_CHAR('\"'));

	TOKEN_TYPE(lsp) = TOKEN_STRING;
	NEXT_CHAR (lsp);

	return 0;
}
