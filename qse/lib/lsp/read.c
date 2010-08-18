/*
 * $Id: read.c 337 2008-08-20 09:17:25Z baconevi $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "lsp.h"

enum list_flag_t
{
	QUOTED = (1 << 0),
	DOTTED = (1 << 1),
	CLOSED = (1 << 2)
};

enum token_type_t
{
	TOKEN_END     = 0,
	TOKEN_INT     = 1,
	TOKEN_REAL    = 2,
	TOKEN_STRING  = 3,
	TOKEN_LPAREN  = 4,
	TOKEN_RPAREN  = 5,
	TOKEN_IDENT   = 6,
	TOKEN_QUOTE   = 7,
	TOKEN_DOT     = 8,
	TOKEN_INVALID = 50
};

#define IS_SPECIAL_CHAR(c) \
	((c) == QSE_T('(') || (c) == QSE_T(')') || \
	 (c) == QSE_T('.') || (c) == QSE_T('\'') || (c) == QSE_T('\"'))

#define IS_IDENT_CHAR(lsp,c) \
	(c != QSE_T('\0') && !IS_SPECIAL_CHAR(c) && !QSE_LSP_ISSPACE(lsp, c))

#define TOKEN_CLEAR(lsp)  qse_str_clear (&(lsp)->token.name)
#define TOKEN_TYPE(lsp)  (lsp)->token.type
#define TOKEN_IVAL(lsp)  (lsp)->token.ival
#define TOKEN_RVAL(lsp)  (lsp)->token.rval
#define TOKEN_STR(lsp)   (lsp)->token.name
#define TOKEN_SPTR(lsp)  (lsp)->token.name.ptr
#define TOKEN_SLEN(lsp)  (lsp)->token.name.len
#define TOKEN_LOC(lsp)   (lsp)->token.loc

#define TOKEN_ADD_CHAR(lsp,ch) \
	do { \
		if (qse_str_ccat(&(lsp)->token.name, ch) == -1) { \
			qse_lsp_seterror (lsp, QSE_LSP_ENOMEM, QSE_NULL, &lsp->curloc); \
			return -1; \
		} \
	} while (0)

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

static int read_char   (qse_lsp_t* lsp);
static int read_token  (qse_lsp_t* lsp);
static int read_number (qse_lsp_t* lsp, int negative);
static int read_ident  (qse_lsp_t* lsp);
static int read_string (qse_lsp_t* lsp);

static QSE_INLINE_ALWAYS void protect (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	/* remember an object for temporary GC protection */
	QSE_ASSERT (lsp->mem->r.tmp == QSE_NULL);
	lsp->mem->r.tmp = obj;
}

static QSE_INLINE_ALWAYS void unprotect (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	/* release an object from temporary GC protection */
	QSE_ASSERT (lsp->mem->r.tmp == obj);
	lsp->mem->r.tmp = QSE_NULL;
}

qse_lsp_obj_t* qse_lsp_read (qse_lsp_t* lsp)
{
	if (lsp->curc == QSE_CHAR_EOF && 
	    read_char(lsp) <= -1) return QSE_NULL;
	NEXT_TOKEN (lsp);

	lsp->mem->r.obj = read_obj (lsp);

	/* clear the stack.
	 * TODO: better to clear stack elements instead of counting on GC?
	 */      
	lsp->mem->r.stack = lsp->mem->nil; 

	return lsp->mem->r.obj;
}

static QSE_INLINE qse_lsp_obj_t* makesym (qse_lsp_t* lsp, const qse_str_t* name)
{
	QSE_ASSERT (lsp->mem->nil != QSE_NULL && lsp->mem->t != QSE_NULL); 

	if (qse_strxcmp (name->ptr, name->len, QSE_T("t")) == 0)
		return lsp->mem->t;
	if (qse_strxcmp (name->ptr, name->len, QSE_T("nil")) == 0)
		return lsp->mem->nil;

	return qse_lsp_makesym (lsp->mem, name->ptr, name->len);
}

static QSE_INLINE qse_lsp_obj_t* push (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	qse_lsp_obj_t* cell;

	protect (lsp, obj); /* temporary protection */
	cell = qse_lsp_makecons (lsp->mem, obj, lsp->mem->r.stack);
	unprotect (lsp, obj); /* ok to unprotected as it is already chained to the stack... */
	if (cell == QSE_NULL) return QSE_NULL;

	lsp->mem->r.stack = cell;
	return cell; /* return the containing cell */
}

static QSE_INLINE_ALWAYS void pop (qse_lsp_t* lsp)
{
	QSE_ASSERT (lsp->mem->r.stack != lsp->mem->nil);
	lsp->mem->r.stack = QSE_LSP_CDR(lsp->mem->r.stack);
}

static QSE_INLINE qse_lsp_obj_t* enter_list (qse_lsp_t* lsp, int flagv)
{
	/* upon entering a list, it pushes three cells into a stack.
	 *
      *  r.stack -------+
      *                 V
	 *             +---cons--+    
	 *         +------  |  -------+
	 *      car|   +---------+    |cdr
	 *         V                  |
	 *        nil#1               V
	 *                          +---cons--+
	 *                      +------  |  --------+
	 *                   car|   +---------+     |cdr
	 *                      v                   |
	 *                     nil#2                V
	 *                                       +---cons--+
	 *                                   +------  | --------+
	 *                                car|   +---------+    |cdr
	 *                                   V                  |
	 *                                flag number           V
	 *                                                  previous stack top
	 *
	 * nil#1 to store the first element in the list.
	 * nil#2 to store the last element in the list.
	 * both to be updated in chain_to_list() as items are added.
	 */
	return (push (lsp, lsp->mem->num[flagv]) == QSE_NULL ||
	        push (lsp, lsp->mem->nil) == QSE_NULL ||
	        push (lsp, lsp->mem->nil) == QSE_NULL)? QSE_NULL: lsp->mem->r.stack;
}

static QSE_INLINE_ALWAYS qse_lsp_obj_t* leave_list (qse_lsp_t* lsp, int* flagv)
{
	qse_lsp_obj_t* head;

	/* the stack must not be empty */
	QSE_ASSERT (lsp->mem->r.stack != lsp->mem->nil);

	/* remember the current list head */
	head = QSE_LSP_CAR(QSE_LSP_CDR(lsp->mem->r.stack));

	/* upon leaving a list, it pops the three cells off the stack */
	pop (lsp);
	pop (lsp);
	pop (lsp);

	if (lsp->mem->r.stack == lsp->mem->nil)
	{
		/* the stack is empty after popping. 
		 * it is back to the top level. 
		 * the top level can never be quoted. */
		*flagv = 0;
	}
	else
	{
		/* restore the flag for the outer returning level */
		qse_lsp_obj_t* flag = QSE_LSP_CDR(QSE_LSP_CDR(lsp->mem->r.stack));
		QSE_ASSERT (QSE_LSP_TYPE(QSE_LSP_CAR(flag)) == QSE_LSP_OBJ_INT);
		*flagv = QSE_LSP_IVAL(QSE_LSP_CAR(flag));
	}

	/* return the head of the list being left */
	return head;
}

static QSE_INLINE_ALWAYS void dot_list (qse_lsp_t* lsp)
{
	qse_lsp_obj_t* cell;

	/* mark the state that a dot has appeared in the list */
	QSE_ASSERT (lsp->mem->r.stack != lsp->mem->nil);
	cell = QSE_LSP_CDR(QSE_LSP_CDR(lsp->mem->r.stack));
	QSE_LSP_CAR(cell) = lsp->mem->num[QSE_LSP_IVAL(QSE_LSP_CAR(cell)) | DOTTED];
}

static qse_lsp_obj_t* chain_to_list (qse_lsp_t* lsp, qse_lsp_obj_t* obj)
{
	qse_lsp_obj_t* cell, * head, * tail, *flag;
	int flagv;

	/* the stack top is the cons cell pointing to the list tail */
	tail = lsp->mem->r.stack;
	QSE_ASSERT (tail != lsp->mem->nil);

	/* the cons cell pointing to the list head is below the tail cell
	 * connected via cdr. */
	head = QSE_LSP_CDR(tail);
	QSE_ASSERT (head != lsp->mem->nil);

	/* the cons cell pointing to the flag is below the head cell
	 * connected via cdr */
	flag = QSE_LSP_CDR(head);

	/* retrieve the numeric flag value */
	QSE_ASSERT(QSE_LSP_TYPE(QSE_LSP_CAR(flag)) == QSE_LSP_OBJ_INT);
	flagv = (int)QSE_LSP_IVAL(QSE_LSP_CAR(flag));

	if (flagv & CLOSED)
	{
		/* the list has already been closed. cannot add more items.  */
		qse_lsp_seterror (lsp, QSE_LSP_ERPAREN, QSE_NULL, &TOKEN_LOC(lsp));
		return QSE_NULL;
	}
	else if (flagv & DOTTED)
	{
		/* the list must not be empty to have reached the dotted state */
		QSE_ASSERT (QSE_LSP_CAR(tail) != lsp->mem->nil);

		/* chain the object via 'cdr' of the tail cell */
		QSE_LSP_CDR(QSE_LSP_CAR(tail)) = obj;

		/* update the flag to CLOSED */
		QSE_LSP_CAR(flag) = lsp->mem->num[flagv | CLOSED];
	}
	else
	{
		protect (lsp, obj); /* in case makecons() fails */
		cell = qse_lsp_makecons (lsp->mem, obj, lsp->mem->nil);
		unprotect (lsp, obj);

		if (cell == QSE_NULL) return QSE_NULL;

		if (QSE_LSP_CAR(head) == lsp->mem->nil)
		{
			/* the list head is not set yet. it is the first
			 * element added to the list. let both head and tail
			 * point to the new cons cell */
			QSE_ASSERT (QSE_LSP_CAR(tail) == lsp->mem->nil);
			QSE_LSP_CAR(head) = cell; 
			QSE_LSP_CAR(tail) = cell;
		}
		else
		{
			/* the new cons cell is not the first element.
			 * append it to the list */
			QSE_LSP_CDR(QSE_LSP_CAR(tail)) = cell;
			QSE_LSP_CAR(tail) = cell;
		}
	}

	return obj;
}

static QSE_INLINE_ALWAYS int is_list_empty (qse_lsp_t* lsp)
{
	/* the stack must not be empty */
	QSE_ASSERT (lsp->mem->r.stack != lsp->mem->nil);

	/* if the tail pointer is pointing to nil, the list is empty */
	return QSE_LSP_CAR(lsp->mem->r.stack) == lsp->mem->nil;
}

static qse_lsp_obj_t* read_obj (qse_lsp_t* lsp)
{
	/* this function read an s-expression non-recursively
	 * by manipulating its own stack. */

	int level = 0, flag = 0; 
	qse_lsp_obj_t* obj;

	while (1)
	{
	redo:
		switch (TOKEN_TYPE(lsp)) 
		{
			default:
				QSE_ASSERT (!"should never happen - invalid token type");
				qse_lsp_seterror (lsp, QSE_LSP_EINTERN, QSE_NULL, QSE_NULL);
				return QSE_NULL;

			case TOKEN_INVALID:
				qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, &TOKEN_LOC(lsp));
				return QSE_NULL;
			
			case TOKEN_END:
				qse_lsp_seterror (lsp, QSE_LSP_EEND, QSE_NULL, &TOKEN_LOC(lsp));
				return QSE_NULL;

			case TOKEN_QUOTE:
				if (level >= QSE_TYPE_MAX(int))
				{
					/* the nesting level has become too deep */
					qse_lsp_seterror (lsp, QSE_LSP_ELSTDEEP, QSE_NULL, &TOKEN_LOC(lsp));
					return QSE_NULL;
				}

				/* enter a quoted string */
				flag |= QUOTED;
				if (enter_list (lsp, flag) == QSE_NULL) return QSE_NULL;
				level++;

				/* force-chain the quote symbol to the new list entered */
				if (chain_to_list (lsp, lsp->mem->quote) == QSE_NULL) return QSE_NULL;

				/* read the next token */
				NEXT_TOKEN (lsp);
				goto redo;
	
			case TOKEN_LPAREN:
				if (level >= QSE_TYPE_MAX(int))
				{
					/* the nesting level has become too deep */
					qse_lsp_seterror (lsp, QSE_LSP_ELSTDEEP, QSE_NULL, &TOKEN_LOC(lsp));
					return QSE_NULL;
				}

				/* enter a normal string */
				flag = 0;
				if (enter_list (lsp, flag) == QSE_NULL) return QSE_NULL;
				level++;

				/* read the next token */
				NEXT_TOKEN (lsp);
				goto redo;

			case TOKEN_DOT:
				if (level <= 0 || is_list_empty (lsp))
				{
					qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, &TOKEN_LOC(lsp));
					return QSE_NULL;
				}

				dot_list (lsp);
				NEXT_TOKEN (lsp);
				goto redo;
		
			case TOKEN_RPAREN:
				if ((flag & QUOTED) || level <= 0)
				{
					/* the right parenthesis can never appear while 
					 * 'quoted' is true. 'quoted' is set to false when 
					 * entering a normal list. 'quoted' is set to true 
					 * when entering a quoted list. a quoted list does
					 * not have an explicit right parenthesis.
					 * so the right parenthesis can only pair up with 
					 * the left parenthesis for the normal list.
					 *
					 * For example, '(1 2 3 ') 5 6)
					 *
					 * this condition is triggerred when the first ) is 
					 * met after the second quote.
					 *
					 * also it is illegal to have the right parenthesis 
					 * with no opening(left) parenthesis, which is 
					 * indicated by level<=0.
					 */
					qse_lsp_seterror (lsp, QSE_LSP_ESYNTAX, QSE_NULL, &TOKEN_LOC(lsp));
					return QSE_NULL;
				}

				obj = leave_list (lsp, &flag);

				level--;
				break;

			case TOKEN_INT:
				obj = qse_lsp_makeint (lsp->mem, TOKEN_IVAL(lsp));
				break;

			case TOKEN_REAL:
				obj = qse_lsp_makereal (lsp->mem, TOKEN_RVAL(lsp));
				break;
	
			case TOKEN_STRING:
				obj = qse_lsp_makestr (
					lsp->mem, TOKEN_SPTR(lsp), TOKEN_SLEN(lsp));
				break;

			case TOKEN_IDENT:
				obj = makesym (lsp, &TOKEN_STR(lsp));
				break;
		}

		/* check if the element is read for a quoted list */
		while (flag & QUOTED)
		{
			QSE_ASSERT (level > 0);

			/* if so, append the element read into the quote list */
			if (chain_to_list (lsp, obj) == QSE_NULL) return QSE_NULL;

			/* exit out of the quoted list. the quoted list can have 
			 * one element only. */
			obj = leave_list (lsp, &flag);

			/* one level up toward the top */
			level--;
		}

		/* check if we are at the top level */
		if (level <= 0) break; /* yes */

		/* if not, append the element read into the current list.
		 * if we are not at the top level, we must be in a list */
		if (chain_to_list (lsp, obj) == QSE_NULL) return QSE_NULL;

		/* read the next token */
		NEXT_TOKEN (lsp);
	}

	/* upon exit, we must be at the top level */
	QSE_ASSERT (level == 0);

	return obj;
}	

static int read_char (qse_lsp_t* lsp)
{
	qse_ssize_t n;
	qse_char_t c;

	if (lsp->io.fns.in == QSE_NULL) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_ENOINP, QSE_NULL, QSE_NULL);
		return -1;
	}

/* TODO: do some bufferring.... */
	n = lsp->io.fns.in (lsp, QSE_LSP_IO_READ, &lsp->io.arg.in, &c, 1);
	if (n == -1) 
	{
		qse_lsp_seterror (lsp, QSE_LSP_EINPUT, QSE_NULL, QSE_NULL);
		return -1;
	}

	if (n == 0) lsp->curc = QSE_CHAR_EOF;
	else 
	{
		lsp->curc = c;

		if (c == QSE_T('\n')) 
		{
			lsp->curloc.colm = 0;
			lsp->curloc.line++;
		}
		else lsp->curloc.colm++;
	}
	return 0;
}

static int read_token (qse_lsp_t* lsp)
{
	QSE_ASSERT (lsp->io.fns.in != QSE_NULL);

	TOKEN_CLEAR (lsp);

	while (1)
	{
		/* skip white spaces */
		while (QSE_LSP_ISSPACE(lsp, lsp->curc)) NEXT_CHAR (lsp);

		if (lsp->curc != QSE_T(';'))  break;

		/* skip a comment - ignore all the following text */
		do { NEXT_CHAR (lsp); } 
		while (lsp->curc != QSE_T('\n') && 
		       lsp->curc != QSE_CHAR_EOF);
	}

	TOKEN_LOC(lsp) = lsp->curloc;
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
		else if (IS_IDENT_CHAR(lsp,lsp->curc)) 
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
	else if (IS_IDENT_CHAR(lsp,lsp->curc)) 
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
	while (IS_IDENT_CHAR(lsp,lsp->curc));
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
			qse_lsp_seterror (lsp, QSE_LSP_EENDSTR, QSE_NULL, &lsp->curloc);
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

