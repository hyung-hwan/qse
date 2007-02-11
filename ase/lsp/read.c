/*
 * $Id: read.c,v 1.32 2007-02-11 07:36:55 bacon Exp $
 *
 * {License}
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
#define TOKEN_SPTR(lsp)  (lsp)->token.name.buf
#define TOKEN_SLEN(lsp)  (lsp)->token.name.size

#define TOKEN_ADD_CHAR(lsp,ch) \
	do { \
		if (ase_lsp_name_addc(&(lsp)->token.name, ch) == -1) { \
			ase_lsp_seterror (lsp, ASE_LSP_ENOMEM, ASE_NULL, 0); \
			return -1; \
		} \
	} while (0)

#define TOKEN_COMPARE(lsp,str) \
	ase_lsp_name_compare (&(lsp)->token.name, str)
		
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

#define NEXT_CHAR_TO(lsp,c) \
	do { \
		if (read_char(lsp) == -1) return -1;\
		c = (lsp)->curc; \
	} while (0)

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

	NEXT_TOKEN (lsp);

	lsp->mem->read = read_obj (lsp);
	if (lsp->mem->read != ASE_NULL) 
		ase_lsp_deepunlockobj (lsp, lsp->mem->read);
	return lsp->mem->read;
}

static ase_lsp_obj_t* read_obj (ase_lsp_t* lsp)
{
	ase_lsp_obj_t* obj;

	switch (TOKEN_TYPE(lsp)) 
	{
		case TOKEN_END:
			ase_lsp_seterror (lsp, ASE_LSP_EEND, ASE_NULL, 0);
			return ASE_NULL;

		case TOKEN_LPAREN:
			NEXT_TOKEN (lsp);
			return read_list (lsp);

		case TOKEN_QUOTE:
			NEXT_TOKEN (lsp);
			return read_quote (lsp);

		case TOKEN_INT:
			obj = ase_lsp_makeintobj (lsp->mem, TOKEN_IVAL(lsp));
			if (obj == ASE_NULL) return ASE_NULL;
			ase_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_REAL:
			obj = ase_lsp_makerealobj (lsp->mem, TOKEN_RVAL(lsp));
			if (obj == ASE_NULL) return ASE_NULL;
			ase_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_STRING:
			obj = ase_lsp_makestr (
				lsp->mem, TOKEN_SPTR(lsp), TOKEN_SLEN(lsp));
			if (obj == ASE_NULL) return ASE_NULL;
			ase_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_IDENT:
			ASE_LSP_ASSERT (lsp,
				lsp->mem->nil != ASE_NULL && 
				lsp->mem->t != ASE_NULL); 

			if (TOKEN_COMPARE(lsp,ASE_T("nil")) == 0) 
			{
				obj = lsp->mem->nil;
			}
			else if (TOKEN_COMPARE(lsp,ASE_T("t")) == 0) 
			{
				obj = lsp->mem->t;
			}
			else 
			{
				obj = ase_lsp_makesym (
					lsp->mem, 
					TOKEN_SPTR(lsp), 
					TOKEN_SLEN(lsp));
				if (obj == ASE_NULL) return ASE_NULL;
				ase_lsp_lockobj (lsp, obj);
			}

			return obj;
	}

	ase_lsp_seterror (lsp, ASE_LSP_ESYNTAX, ASE_NULL, 0);
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
			/* unexpected end of input */
			ase_lsp_seterror (lsp, ASE_LSP_ESYNTAX, ASE_NULL, 0);
			return ASE_NULL;
		}

		if (TOKEN_TYPE(lsp) == TOKEN_DOT) 
		{
			if (prev == ASE_NULL) 
			{
				/* unexpected dot */
				ase_lsp_seterror (lsp, ASE_LSP_ESYNTAX, ASE_NULL, 0);
				return ASE_NULL;
			}

			NEXT_TOKEN (lsp);
			obj = read_obj (lsp);
			if (obj == ASE_NULL) 
			{
				if (lsp->errnum == ASE_LSP_EEND) 
				{
					/* unexpected end of input */
					ase_lsp_seterror (lsp, ASE_LSP_ESYNTAX, ASE_NULL, 0);
				}
				return ASE_NULL;
			}
			prev->cdr = obj;

			NEXT_TOKEN (lsp);
			if (TOKEN_TYPE(lsp) != TOKEN_RPAREN) 
			{
				/* ) expected */
				ase_lsp_seterror (lsp, ASE_LSP_ERPAREN, ASE_NULL, 0);
				return ASE_NULL;
			}

			break;
		}

		obj = read_obj (lsp);
		if (obj == ASE_NULL) 
		{
			if (lsp->errnum == ASE_LSP_EEND)
			{	
				/* unexpected end of input */
				ase_lsp_seterror (lsp, ASE_LSP_ESYNTAX, ASE_NULL, 0);
			}
			return ASE_NULL;
		}

		p = (ase_lsp_obj_cons_t*)ase_lsp_makecons (
			lsp->mem, lsp->mem->nil, lsp->mem->nil);
		if (p == ASE_NULL) return ASE_NULL;
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
		if (lsp->errnum == ASE_LSP_EEND) 
		{
			/* unexpected end of input */
			ase_lsp_seterror (lsp, ASE_LSP_ESYNTAX, ASE_NULL, 0);
		}
		return ASE_NULL;
	}

	cons = ase_lsp_makecons (lsp->mem, tmp, lsp->mem->nil);
	if (cons == ASE_NULL) return ASE_NULL;
	ase_lsp_lockobj (lsp, cons);

	cons = ase_lsp_makecons (lsp->mem, lsp->mem->quote, cons);
	if (cons == ASE_NULL) return ASE_NULL;
	ase_lsp_lockobj (lsp, cons); 

	return cons;
}

static int read_char (ase_lsp_t* lsp)
{
	ase_ssize_t n;
	ase_char_t c;

	if (lsp->input_func == ASE_NULL) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_ENOINP, ASE_NULL, 0);
		return -1;
	}

	n = lsp->input_func(ASE_LSP_IO_READ, lsp->input_arg, &c, 1);
	if (n == -1) 
	{
		ase_lsp_seterror (lsp, ASE_LSP_EINPUT, ASE_NULL, 0);
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
		/* skip white spaces */
		while (ASE_LSP_ISSPACE(lsp, lsp->curc)) NEXT_CHAR (lsp);

		/* skip the comments here */
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
		return read_string (lsp);
	}

	TOKEN_TYPE(lsp) = TOKEN_INVALID;
	NEXT_CHAR (lsp); /* consume */
	return 0;
}

static int read_number (ase_lsp_t* lsp, int negative)
{
	ase_long_t ival = 0;
	ase_real_t rval = .0;

	do 
	{
		ival = ival * 10 + (lsp->curc - ASE_T('0'));
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} 
	while (ASE_LSP_ISDIGIT(lsp, lsp->curc));

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
	else 
	{
		TOKEN_IVAL(lsp) = ival;
		TOKEN_TYPE(lsp) = TOKEN_INT;
		if (negative) ival *= -1;
	}

	return 0;
}

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
	ase_cint_t c;
	int escaped = 0;
	int digit_count = 0;
	ase_cint_t c_acc = 0;

	while (1)
	{
		NEXT_CHAR_TO (lsp, c);

		if (c == ASE_CHAR_EOF)
		{
			ase_lsp_seterror (lsp, ASE_LSP_EENDSTR, ASE_NULL, 0);
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= ASE_T('0') && c <= ASE_T('7'))
			{
				c_acc = c_acc * 8 + c - ASE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOKEN_ADD_CHAR (lsp, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				TOKEN_ADD_CHAR (lsp, c_acc);
				escaped = 0;
			}
		}
		else if (escaped == 2 || escaped == 4 || escaped == 8)
		{
			if (c >= ASE_T('0') && c <= ASE_T('9'))
			{
				c_acc = c_acc * 16 + c - ASE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOKEN_ADD_CHAR (lsp, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= ASE_T('A') && c <= ASE_T('F'))
			{
				c_acc = c_acc * 16 + c - ASE_T('A') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOKEN_ADD_CHAR (lsp, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= ASE_T('a') && c <= ASE_T('f'))
			{
				c_acc = c_acc * 16 + c - ASE_T('a') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOKEN_ADD_CHAR (lsp, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				ase_char_t rc;

				rc = (escaped == 2)? ASE_T('x'):
				     (escaped == 4)? ASE_T('u'): ASE_T('U');

				if (digit_count == 0) TOKEN_ADD_CHAR (lsp, rc);
				else TOKEN_ADD_CHAR (lsp, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == ASE_T('\"'))
		{
			/* terminating quote */
			/*NEXT_CHAR_TO (lsp, c);*/
			NEXT_CHAR (lsp);
			break;
		}

		if (escaped == 0 && c == ASE_T('\\'))
		{
			escaped = 1;
			continue;
		}

		if (escaped == 1)
		{
			if (c == ASE_T('n')) c = ASE_T('\n');
			else if (c == ASE_T('r')) c = ASE_T('\r');
			else if (c == ASE_T('t')) c = ASE_T('\t');
			else if (c == ASE_T('f')) c = ASE_T('\f');
			else if (c == ASE_T('b')) c = ASE_T('\b');
			else if (c == ASE_T('v')) c = ASE_T('\v');
			else if (c == ASE_T('a')) c = ASE_T('\a');
			else if (c >= ASE_T('0') && c <= ASE_T('7')) 
			{
				escaped = 3;
				digit_count = 1;
				c_acc = c - ASE_T('0');
				continue;
			}
			else if (c == ASE_T('x')) 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#ifdef ASE_CHAR_IS_WCHAR
			else if (c == ASE_T('u') && ASE_SIZEOF(ase_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == ASE_T('U') && ASE_SIZEOF(ase_char_t) >= 4) 
			{
				escaped = 8;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#endif

			escaped = 0;
		}

		TOKEN_ADD_CHAR (lsp, c);
	}

	TOKEN_TYPE(lsp) = TOKEN_STRING;
	return 0;
}
