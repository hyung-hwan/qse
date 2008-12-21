/*
 * $Id: read.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include "lsp.h"

#define IS_IDENT(c) \
	((c) == QSE_T('+') || (c) == QSE_T('-') || \
	 (c) == QSE_T('*') || (c) == QSE_T('/') || \
	 (c) == QSE_T('%') || (c) == QSE_T('&') || \
	 (c) == QSE_T('<') || (c) == QSE_T('>') || \
	 (c) == QSE_T('=') || (c) == QSE_T('_') || \
	 (c) == QSE_T('?'))

#define TOKEN_CLEAR(lsp)  qse_lsp_name_clear (&(lsp)->token.name)
#define TOKEN_TYPE(lsp)  (lsp)->token.type
#define TOKEN_IVAL(lsp)  (lsp)->token.ival
#define TOKEN_RVAL(lsp)  (lsp)->token.rval
#define TOKEN_SPTR(lsp)  (lsp)->token.name.buf
#define TOKEN_SLEN(lsp)  (lsp)->token.name.size

#define TOKEN_ADD_CHAR(lsp,ch) \
	do { \
		if (qse_lsp_name_addc(&(lsp)->token.name, ch) == -1) { \
			qse_lsp_seterror (lsp, QSE_LSP_ENOMEM, QSE_NULL, 0); \
			return -1; \
		} \
	} while (0)

#define TOKEN_COMPARE(lsp,str) \
	qse_lsp_name_compare (&(lsp)->token.name, str)
		
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
	do { if (read_token(lsp) == -1) return QSE_NULL; } while (0)

static qse_lsp_obj_t* read_obj   (qse_lsp_t* lsp);
static qse_lsp_obj_t* read_list  (qse_lsp_t* lsp);
static qse_lsp_obj_t* read_quote (qse_lsp_t* lsp);

static int read_char   (qse_lsp_t* lsp);
static int read_token  (qse_lsp_t* lsp);
static int read_number (qse_lsp_t* lsp, int negative);
static int read_ident  (qse_lsp_t* lsp);
static int read_string (qse_lsp_t* lsp);

qse_lsp_obj_t* qse_lsp_read (qse_lsp_t* lsp)
{
	if (lsp->curc == QSE_CHAR_EOF && 
	    read_char(lsp) == -1) return QSE_NULL;

	NEXT_TOKEN (lsp);

	lsp->mem->read = read_obj (lsp);
	if (lsp->mem->read != QSE_NULL) 
		qse_lsp_deepunlockobj (lsp, lsp->mem->read);
	return lsp->mem->read;
}

static qse_lsp_obj_t* read_obj (qse_lsp_t* lsp)
{
	qse_lsp_obj_t* obj;

	switch (TOKEN_TYPE(lsp)) 
	{
		case TOKEN_END:
			qse_lsp_seterror (lsp, QSE_LSP_EEND, QSE_NULL, 0);
			return QSE_NULL;

		case TOKEN_LPAREN:
			NEXT_TOKEN (lsp);
			return read_list (lsp);

		case TOKEN_QUOTE:
			NEXT_TOKEN (lsp);
			return read_quote (lsp);

		case TOKEN_INT:
			obj = qse_lsp_makeintobj (lsp->mem, TOKEN_IVAL(lsp));
			if (obj == QSE_NULL) return QSE_NULL;
			qse_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_REAL:
			obj = qse_lsp_makerealobj (lsp->mem, TOKEN_RVAL(lsp));
			if (obj == QSE_NULL) return QSE_NULL;
			qse_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_STRING:
			obj = qse_lsp_makestr (
				lsp->mem, TOKEN_SPTR(lsp), TOKEN_SLEN(lsp));
			if (obj == QSE_NULL) return QSE_NULL;
			qse_lsp_lockobj (lsp, obj);
			return obj;

		case TOKEN_IDENT:
			QSE_ASSERT (
				lsp->mem->nil != QSE_NULL && 
				lsp->mem->t != QSE_NULL); 

			if (TOKEN_COMPARE(lsp,QSE_T("nil")) == 0) 
			{
				obj = lsp->mem->nil;
			}
			else if (TOKEN_COMPARE(lsp,QSE_T("t")) == 0) 
			{
				obj = lsp->mem->t;
			}
			else 
			{
				obj = qse_lsp_makesym (
					lsp->mem, 
					TOKEN_SPTR(lsp), 
					TOKEN_SLEN(lsp));
				if (obj == QSE_NULL) return QSE_NULL;
				qse_lsp_lockobj (lsp, obj);
			}

			return obj;
	}

	qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, 0);
	return QSE_NULL;
}

static qse_lsp_obj_t* read_list (qse_lsp_t* lsp)
{
	qse_lsp_obj_t* obj;
	qse_lsp_obj_cons_t* p, * first = QSE_NULL, * prev = QSE_NULL;

	while (TOKEN_TYPE(lsp) != TOKEN_RPAREN) 
	{
		if (TOKEN_TYPE(lsp) == TOKEN_END) 
		{
			/* unexpected end of input */
			qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, 0);
			return QSE_NULL;
		}

		if (TOKEN_TYPE(lsp) == TOKEN_DOT) 
		{
			if (prev == QSE_NULL) 
			{
				/* unexpected dot */
				qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, 0);
				return QSE_NULL;
			}

			NEXT_TOKEN (lsp);
			obj = read_obj (lsp);
			if (obj == QSE_NULL) 
			{
				if (lsp->errnum == QSE_LSP_EEND) 
				{
					/* unexpected end of input */
					qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, 0);
				}
				return QSE_NULL;
			}
			prev->cdr = obj;

			NEXT_TOKEN (lsp);
			if (TOKEN_TYPE(lsp) != TOKEN_RPAREN) 
			{
				/* ) expected */
				qse_lsp_seterror (lsp, QSE_LSP_ERPAREN, QSE_NULL, 0);
				return QSE_NULL;
			}

			break;
		}

		obj = read_obj (lsp);
		if (obj == QSE_NULL) 
		{
			if (lsp->errnum == QSE_LSP_EEND)
			{	
				/* unexpected end of input */
				qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, 0);
			}
			return QSE_NULL;
		}

		p = (qse_lsp_obj_cons_t*)qse_lsp_makecons (
			lsp->mem, lsp->mem->nil, lsp->mem->nil);
		if (p == QSE_NULL) return QSE_NULL;
		qse_lsp_lockobj (lsp, (qse_lsp_obj_t*)p);

		if (first == QSE_NULL) first = p;
		if (prev != QSE_NULL) prev->cdr = (qse_lsp_obj_t*)p;

		p->car = obj;
		prev = p;

		NEXT_TOKEN (lsp);
	}	

	return (first == QSE_NULL)? lsp->mem->nil: (qse_lsp_obj_t*)first;
}

static qse_lsp_obj_t* read_quote (qse_lsp_t* lsp)
{
	qse_lsp_obj_t* cons, * tmp;

	tmp = read_obj (lsp);
	if (tmp == QSE_NULL) 
	{
		if (lsp->errnum == QSE_LSP_EEND) 
		{
			/* unexpected end of input */
			qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, 0);
		}
		return QSE_NULL;
	}

	cons = qse_lsp_makecons (lsp->mem, tmp, lsp->mem->nil);
	if (cons == QSE_NULL) return QSE_NULL;
	qse_lsp_lockobj (lsp, cons);

	cons = qse_lsp_makecons (lsp->mem, lsp->mem->quote, cons);
	if (cons == QSE_NULL) return QSE_NULL;
	qse_lsp_lockobj (lsp, cons); 

	return cons;
}

static int read_char (qse_lsp_t* lsp)
{
	qse_ssize_t n;
	qse_char_t c;

	if (lsp->input_func == QSE_NULL) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_ENOINP, QSE_NULL, 0);
		return -1;
	}

	n = lsp->input_func(QSE_LSP_IO_READ, lsp->input_arg, &c, 1);
	if (n == -1) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EINPUT, QSE_NULL, 0);
		return -1;
	}

	if (n == 0) lsp->curc = QSE_CHAR_EOF;
	else lsp->curc = c;
	return 0;
}

static int read_token (qse_lsp_t* lsp)
{
	QSE_ASSERT (lsp->input_func != QSE_NULL);

	TOKEN_CLEAR (lsp);

	while (1)
	{
		/* skip white spaces */
		while (QSE_LSP_ISSPACE(lsp, lsp->curc)) NEXT_CHAR (lsp);

		/* skip the comments here */
		if (lsp->curc == QSE_T(';')) 
		{
			do 
			{
				NEXT_CHAR (lsp);
			} 
			while (lsp->curc != QSE_T('\n') && 
			       lsp->curc != QSE_CHAR_EOF);
		}
		else break;
	}

	if (lsp->curc == QSE_CHAR_EOF) 
	{
		TOKEN_TYPE(lsp) = TOKEN_END;
		return 0;
	}
	else if (lsp->curc == QSE_T('(')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_LPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == QSE_T(')')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_RPAREN;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == QSE_T('\'')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_QUOTE;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == QSE_T('.')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		TOKEN_TYPE(lsp) = TOKEN_DOT;
		NEXT_CHAR (lsp);
		return 0;
	}
	else if (lsp->curc == QSE_T('-')) 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
		if (QSE_LSP_ISDIGIT(lsp,lsp->curc)) 
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
	else if (QSE_LSP_ISDIGIT(lsp,lsp->curc)) 
	{
		return read_number (lsp, 0);
	}
	else if (QSE_LSP_ISALPHA(lsp,lsp->curc) || IS_IDENT(lsp->curc)) 
	{
		return read_ident (lsp);
	}
	else if (lsp->curc == QSE_T('\"')) 
	{
		return read_string (lsp);
	}

	TOKEN_TYPE(lsp) = TOKEN_INVALID;
	NEXT_CHAR (lsp); /* consume */
	return 0;
}

static int read_number (qse_lsp_t* lsp, int negative)
{
	qse_long_t ival = 0;
	qse_real_t rval = .0;

	do 
	{
		ival = ival * 10 + (lsp->curc - QSE_T('0'));
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} 
	while (QSE_LSP_ISDIGIT(lsp, lsp->curc));

/* TODO: extend parsing floating point number  */
	if (lsp->curc == QSE_T('.')) 
	{
		qse_real_t fraction = 0.1;

		NEXT_CHAR (lsp);
		rval = (qse_real_t)ival;

		while (QSE_LSP_ISDIGIT(lsp, lsp->curc)) 
		{
			rval += (qse_real_t)(lsp->curc - QSE_T('0')) * fraction;
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

static int read_ident (qse_lsp_t* lsp)
{
	do 
	{
		TOKEN_ADD_CHAR (lsp, lsp->curc);
		NEXT_CHAR (lsp);
	} 
	while (QSE_LSP_ISALNUM(lsp,lsp->curc) || IS_IDENT(lsp->curc));
	TOKEN_TYPE(lsp) = TOKEN_IDENT;
	return 0;
}

static int read_string (qse_lsp_t* lsp)
{
	qse_cint_t c;
	int escaped = 0;
	int digit_count = 0;
	qse_cint_t c_acc = 0;

	while (1)
	{
		NEXT_CHAR_TO (lsp, c);

		if (c == QSE_CHAR_EOF)
		{
			qse_lsp_seterror (lsp, QSE_LSP_EENDSTR, QSE_NULL, 0);
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= QSE_T('0') && c <= QSE_T('7'))
			{
				c_acc = c_acc * 8 + c - QSE_T('0');
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
			if (c >= QSE_T('0') && c <= QSE_T('9'))
			{
				c_acc = c_acc * 16 + c - QSE_T('0');
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOKEN_ADD_CHAR (lsp, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= QSE_T('A') && c <= QSE_T('F'))
			{
				c_acc = c_acc * 16 + c - QSE_T('A') + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					TOKEN_ADD_CHAR (lsp, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= QSE_T('a') && c <= QSE_T('f'))
			{
				c_acc = c_acc * 16 + c - QSE_T('a') + 10;
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
				qse_char_t rc;

				rc = (escaped == 2)? QSE_T('x'):
				     (escaped == 4)? QSE_T('u'): QSE_T('U');

				if (digit_count == 0) TOKEN_ADD_CHAR (lsp, rc);
				else TOKEN_ADD_CHAR (lsp, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == QSE_T('\"'))
		{
			/* terminating quote */
			/*NEXT_CHAR_TO (lsp, c);*/
			NEXT_CHAR (lsp);
			break;
		}

		if (escaped == 0 && c == QSE_T('\\'))
		{
			escaped = 1;
			continue;
		}

		if (escaped == 1)
		{
			if (c == QSE_T('n')) c = QSE_T('\n');
			else if (c == QSE_T('r')) c = QSE_T('\r');
			else if (c == QSE_T('t')) c = QSE_T('\t');
			else if (c == QSE_T('f')) c = QSE_T('\f');
			else if (c == QSE_T('b')) c = QSE_T('\b');
			else if (c == QSE_T('v')) c = QSE_T('\v');
			else if (c == QSE_T('a')) c = QSE_T('\a');
			else if (c >= QSE_T('0') && c <= QSE_T('7')) 
			{
				escaped = 3;
				digit_count = 1;
				c_acc = c - QSE_T('0');
				continue;
			}
			else if (c == QSE_T('x')) 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
		#ifdef QSE_CHAR_IS_WCHAR
			else if (c == QSE_T('u') && QSE_SIZEOF(qse_char_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == QSE_T('U') && QSE_SIZEOF(qse_char_t) >= 4) 
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
