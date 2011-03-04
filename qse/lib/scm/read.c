/*
 * $Id$
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

#include "scm.h"

enum list_flag_t
{
	QUOTED = (1 << 0),
	DOTTED = (1 << 1),
	CLOSED = (1 << 2)
};

enum tok_type_t
{
	TOK_END     = 0,
	TOK_T       = 1,
	TOK_F       = 2,
	TOK_INT     = 3,
	TOK_REAL    = 4,
	TOK_SYMBOL  = 5,
	TOK_STRING  = 6,
	TOK_LPAREN  = 7,
	TOK_RPAREN  = 8,
	TOK_DOT     = 9,
	TOK_QUOTE   = 10,
	TOK_QQUOTE  = 11, /* quasiquote */
	TOK_COMMA   = 12,
	TOK_COMMAAT = 13,
#if 0
	TOK_INVALID = 50
#endif
};

#define TOK_CLR(scm)      qse_str_clear(&(scm)->r.t.name)
#define TOK_TYPE(scm)     (scm)->r.t.type
#define TOK_IVAL(scm)     (scm)->r.t.ival
#define TOK_RVAL(scm)     (scm)->r.t.rval
#define TOK_NAME(scm)     (&(scm)->r.t.name)
#define TOK_NAME_PTR(scm) TOK_NAME(scm)->ptr
#define TOK_NAME_LEN(scm) TOK_NAME(scm)->len
#define TOK_LOC(scm)      (scm)->r.t.loc

#define TOK_ADD_CHAR(scm,ch) QSE_BLOCK (\
	if (qse_str_ccat(TOK_NAME(scm), ch) == -1) \
	{ \
		qse_scm_seterror (scm, QSE_SCM_ENOMEM, QSE_NULL, &scm->r.curloc); \
		return -1; \
	} \
)

#define IS_DIGIT(ch) ((ch) >= QSE_T('0') && (ch) <= QSE_T('9'))
#define IS_SPACE(ch) ((ch) == QSE_T(' ') || (ch) == QSE_T('\t'))
#define IS_NEWLINE(ch) ((ch) == QSE_T('\n') || (ch) == QSE_T('\r'))
#define IS_WHSPACE(ch) IS_SPACE(ch) || IS_NEWLINE(ch)
#define IS_DELIM(ch) \
	(IS_WHSPACE(ch) || (ch) == QSE_T('(') || (ch) == QSE_T(')') || \
	 (ch) == QSE_T('\"') || (ch) == QSE_T(';') || (ch) == QSE_CHAR_EOF)

#define READ_CHAR(scm) QSE_BLOCK(if (read_char(scm) <= -1) return -1;)
#define READ_TOKEN(scm) QSE_BLOCK(if (read_token(scm) <= -1) return -1;)

static int read_char (qse_scm_t* scm)
{
	qse_ssize_t n;
	qse_char_t c;

/* TODO: do bufferring */
	scm->err.num = QSE_SCM_ENOERR;
	n = scm->io.fns.in (scm, QSE_SCM_IO_READ, &scm->io.arg.in, &c, 1);
	if (n <= -1)
	{
		if (scm->err.num == QSE_SCM_ENOERR)
			qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		return -1;
	}

/* TODO: handle the case when a new file is included or loaded ... 
 *       stacking of curloc is needed??? see qseawk for reference
 */
	if (n == 0) scm->r.curc = QSE_CHAR_EOF;
	else
	{
		scm->r.curc = c;

		if (c == QSE_T('\n'))
		{
			scm->r.curloc.colm = 0;
			scm->r.curloc.line++;
		}
		else scm->r.curloc.colm++;
	}

/*qse_printf (QSE_T("[%c]\n"), scm->r.curc);*/
	return 0;
}

static int read_string_token (qse_scm_t* scm)
{
	qse_cint_t c;
	int escaped = 0;
	int digit_count = 0;
	qse_cint_t c_acc = 0;

	while (1)
	{
		READ_CHAR (scm);
		c = scm->r.curc;

		if (c == QSE_CHAR_EOF)
		{
			qse_scm_seterror (
				scm, QSE_SCM_EENDSTR,
				QSE_NULL, &scm->r.curloc);
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
					TOK_ADD_CHAR (scm, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				TOK_ADD_CHAR (scm, c_acc);
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
					TOK_ADD_CHAR (scm, c_acc);
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
					TOK_ADD_CHAR (scm, c_acc);
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
					TOK_ADD_CHAR (scm, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				qse_char_t rc;

				rc = (escaped == 2)? QSE_T('x'):
				     (escaped == 4)? QSE_T('u'): QSE_T('U');

				if (digit_count == 0) TOK_ADD_CHAR (scm, rc);
				else TOK_ADD_CHAR (scm, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == QSE_T('\"'))
		{
			/* terminating quote */
			/*NEXT_CHAR_TO (scm, c);*/
			READ_CHAR (scm);
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

		TOK_ADD_CHAR (scm, c);
	}

	TOK_TYPE(scm) = TOK_STRING;
	return 0;
}


enum read_number_token_flag_t
{
	RNT_NEGATIVE         = (1 << 0),
	RNT_SKIP_TO_FRACTION = (1 << 1)
};

static int read_number_token (qse_scm_t* scm, int flags)
{
	qse_long_t ival = 0;
	qse_real_t rval = .0;
	qse_real_t fraction;

	if (flags & RNT_SKIP_TO_FRACTION) goto fraction_part;

	do
	{
		ival = ival * 10 + (scm->r.curc - QSE_T('0'));
		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);
	}
	while (IS_DIGIT(scm->r.curc));

/* TODO: extend parsing floating point number  */
	if (scm->r.curc == QSE_T('.'))
	{
	fraction_part:
		fraction = 0.1;

		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);
		rval = (qse_real_t)ival;

		while (IS_DIGIT(scm->r.curc))
		{
			rval += (qse_real_t)(scm->r.curc - QSE_T('0')) * fraction;
			fraction *= 0.1;
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);
		}

		TOK_RVAL(scm) = rval;
		TOK_TYPE(scm) = TOK_REAL;
		if (flags & RNT_NEGATIVE) rval *= -1;
	}
	else
	{
		TOK_IVAL(scm) = ival;
		TOK_TYPE(scm) = TOK_INT;
		if (flags & RNT_NEGATIVE) ival *= -1;
	}

	return 0;
}

static int read_sharp_token (qse_scm_t* scm)
{
/* TODO: read a token beginning with #.*/

	TOK_ADD_CHAR (scm, scm->r.curc); /* add # to the token name */

	READ_CHAR (scm);
	switch (scm->r.curc)
	{
		case QSE_T('t'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);
			if (!IS_DELIM(scm->r.curc)) goto charname;
			TOK_TYPE(scm) = TOK_T;
			break;

		case QSE_T('f'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);
			if (!IS_DELIM(scm->r.curc)) goto charname;
			TOK_TYPE(scm) = TOK_F;
			break;

		case QSE_T('\\'):
			break;

		case QSE_T('b'):
			break;

		case QSE_T('o'):
			break;

		case QSE_T('d'):
			break;

		case QSE_T('x'):
			break;
	}

	return 0;


charname:
	do
	{
		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);	
	}
	while (!IS_DELIM(scm->r.curc));

/* TODO: character name comparison... */
	qse_scm_seterror (scm, QSE_SCM_ESHARP, QSE_NULL, &scm->r.curloc);
	return -1;
}

static int read_token (qse_scm_t* scm)
{
	int flags = 0;

	TOK_CLR (scm);

	/* skip a series of white spaces and comment lines */
	do
	{
		/* skip white spaces */
		while (IS_WHSPACE(scm->r.curc)) READ_CHAR (scm);

		if (scm->r.curc != QSE_T(';')) break;

		/* skip a comment line */
		do { READ_CHAR (scm); }
		while (scm->r.curc != QSE_T('\n') &&
		       scm->r.curc != QSE_CHAR_EOF);
	} 
	while (1);

	TOK_LOC(scm) = scm->r.curloc;	
	if (scm->r.curc == QSE_CHAR_EOF)
	{
		TOK_TYPE(scm) = TOK_END;
		return 0;
	}

	switch (scm->r.curc)
	{
		case QSE_T('('):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_LPAREN;
			READ_CHAR (scm);	
			return 0;

		case QSE_T(')'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_RPAREN;
			READ_CHAR (scm);	
			return 0;

		case QSE_T('.'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);	
			if (!IS_DELIM(scm->r.curc)) 
			{
				flags |= RNT_SKIP_TO_FRACTION;
				goto try_number;
			}
			TOK_TYPE(scm) = TOK_DOT;
			return 0;

		case QSE_T('\''):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_QUOTE;
			READ_CHAR (scm);	
			return 0;

		case QSE_T('`'):
			TOK_ADD_CHAR (scm, scm->r.curc);
			TOK_TYPE(scm) = TOK_QQUOTE;
			READ_CHAR (scm);	
			return 0;

		case QSE_T(','):
			TOK_ADD_CHAR (scm, scm->r.curc);
			READ_CHAR (scm);

			if (scm->r.curc == QSE_T('@'))
			{
				TOK_TYPE(scm) = TOK_COMMAAT;
				READ_CHAR (scm);	
			}
			else TOK_TYPE(scm) = TOK_COMMA;
			return 0;

		case QSE_T('#'):
			return read_sharp_token (scm);

		case QSE_T('\"'):
			return read_string_token (scm);
	}

	if (scm->r.curc == QSE_T('+') || scm->r.curc == QSE_T('-')) 
	{
		/* a number can begin with + or -. we don't know
		 * if it is the part of a number or not yet. 
		 * let's set the NEGATIVE bit in 'flags' if the sign is 
		 * negative for later use in case it is followed by a digit.
		 * we also add the sign character to the token name 
		 * so that we can form a complete symbol if the word turns
		 * out to be a symbol eventually.
		 */
		if (scm->r.curc == QSE_T('-')) flags |= RNT_NEGATIVE;
		TOK_ADD_CHAR (scm, scm->r.curc);
		READ_CHAR (scm);
	}

	if (IS_DIGIT(scm->r.curc))
	{
	try_number:
		/* we got a digit, maybe or maybe not following a sign.
		 * call read_number_token() to read the current token 
		 * as a number. */
		if (read_number_token (scm, flags) <= -1) return -1;

		/* the read_number() function exits once it sees a character
		 * that can not compose a number. if it is a delimiter,
		 * the token is numeric. */
		if (IS_DELIM(scm->r.curc)) return 0;

		/* otherwise, we carry on reading trailing characters to
		 * compose a symbol token */
	}

	/* we got here as the current token does not begin with special
	 * token characters. treat it as a symbol token. */
	do 
	{
		TOK_ADD_CHAR (scm, scm->r.curc);	
		READ_CHAR (scm);
	} 
	while (!IS_DELIM(scm->r.curc));
	TOK_TYPE(scm) = TOK_SYMBOL; 

	return 0;
	

#if 0
	TOK_TYPE(scm) = TOK_INVALID;
	READ_CHAR (scm); /* consume */
	return 0;
#endif
}

static QSE_INLINE qse_scm_ent_t* push (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	qse_scm_ent_t* pair;

	pair = qse_scm_makepairent (scm, obj, scm->r.s);
	if (pair == QSE_NULL) return QSE_NULL;

	scm->r.s = pair;

	/* return the top of the stack which is the containing pair */
	return pair;
}

static QSE_INLINE_ALWAYS void pop (qse_scm_t* scm)
{
	QSE_ASSERTX (
		!IS_NIL(scm,scm->r.s),
		"You've called pop() more times than push()"
	);
	scm->r.s = PAIR_CDR(scm->r.s);
}

static QSE_INLINE qse_scm_ent_t* enter_list (qse_scm_t* scm, int flagv)
{
	/* upon entering a list, it pushes three cells into a stack.
	 *
      *  rstack -------+
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
	return (push (scm, TO_SMALLINT(scm,flagv)) == QSE_NULL ||
	        push (scm, scm->nil) == QSE_NULL ||
	        push (scm, scm->nil) == QSE_NULL)? QSE_NULL: scm->r.s;
}

static QSE_INLINE_ALWAYS qse_scm_ent_t* leave_list (qse_scm_t* scm, int* flagv)
{
	qse_scm_ent_t* head;

	/* the stack must not be empty */
	QSE_ASSERTX (
		!IS_NIL(scm,scm->r.s), 
		"You cannot leave a list without entering it"
	);

	/* remember the current list head */
	head = PAIR_CAR(PAIR_CDR(scm->r.s));

	/* upon leaving a list, it pops the three cells off the stack */
	pop (scm);
	pop (scm);
	pop (scm);

	if (IS_NIL(scm,scm->r.s))
	{
		/* the stack is empty after popping. 
		 * it is back to the top level. 
		 * the top level can never be quoted. */
		*flagv = 0;
	}
	else
	{
		/* restore the flag for the outer returning level */
		qse_scm_ent_t* flag = PAIR_CDR(PAIR_CDR(scm->r.s));
		QSE_ASSERT (IS_SMALLINT(scm,PAIR_CAR(flag)));
		*flagv = FROM_SMALLINT(scm,PAIR_CAR(flag));
	}

	/* return the head of the list being left */
	return head;
}

static QSE_INLINE_ALWAYS void dot_list (qse_scm_t* scm)
{
	qse_scm_ent_t* pair;
	int flagv;

	QSE_ASSERT (!IS_NIL(scm,scm->r.s));

	/* mark the state that a dot has appeared in the list */
	pair = PAIR_CDR(PAIR_CDR(scm->r.s));
	flagv = FROM_SMALLINT(scm,PAIR_CAR(pair));
	PAIR_CAR(pair) = TO_SMALLINT(scm,flagv|DOTTED);
}

static qse_scm_ent_t* chain_to_list (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	qse_scm_ent_t* cell, * head, * tail, *flag;
	int flagv;

	/* the stack top is the pair pointing to the list tail */
	tail = scm->r.s;
	QSE_ASSERT (!IS_NIL(scm,tail));

	/* the pair pointing to the list head is below the tail cell
	 * connected via cdr. */
	head = PAIR_CDR(tail);
	QSE_ASSERT (!IS_NIL(scm,head));

	/* the pair pointing to the flag is below the head cell
	 * connected via cdr */
	flag = PAIR_CDR(head);

	/* retrieve the numeric flag value */
	QSE_ASSERT(IS_SMALLINT(scm,PAIR_CAR(flag)));
	flagv = (int)FROM_SMALLINT(scm,PAIR_CAR(flag));

	if (flagv & CLOSED)
	{
		/* the list has already been closed. cannot add more items.  */
		qse_scm_seterror (scm, QSE_SCM_ERPAREN, QSE_NULL, &TOK_LOC(scm));
		return QSE_NULL;
	}
	else if (flagv & DOTTED)
	{
		/* the list must not be empty to have reached the dotted state */
		QSE_ASSERT (!IS_NIL(scm,PAIR_CAR(tail)));

		/* chain the object via 'cdr' of the tail cell */
		PAIR_CDR(PAIR_CAR(tail)) = obj;

		/* update the flag to CLOSED so that you can have more than
		 * one item after the dot. */
		PAIR_CAR(flag) = TO_SMALLINT(scm,flagv|CLOSED);
	}
	else
	{
		cell = qse_scm_makepairent (scm, obj, scm->nil);
		if (cell == QSE_NULL) return QSE_NULL;

		if (PAIR_CAR(head) == scm->nil)
		{
			/* the list head is not set yet. it is the first
			 * element added to the list. let both head and tail
			 * point to the new cons cell */
			QSE_ASSERT (PAIR_CAR(tail) == scm->nil);
			PAIR_CAR(head) = cell; 
			PAIR_CAR(tail) = cell;
		}
		else
		{
			/* the new cons cell is not the first element.
			 * append it to the list */
			PAIR_CDR(PAIR_CAR(tail)) = cell;
			PAIR_CAR(tail) = cell;
		}
	}

	return obj;
}

static QSE_INLINE_ALWAYS int is_list_empty (qse_scm_t* scm)
{
	/* the stack must not be empty */
	QSE_ASSERTX (
		!IS_NIL(scm,scm->r.s), 
		"You can not call this function while the stack is empty"		
	);

	/* if the tail pointer is pointing to nil, the list is empty */
	return IS_NIL(scm,PAIR_CAR(scm->r.s));
}

static int read_entity (qse_scm_t* scm)
{
	/* this function read an s-expression non-recursively
	 * by manipulating its own stack. */

	int level = 0, flagv = 0; 
	qse_scm_ent_t* obj;

	while (1)
	{
	redo:
		switch (TOK_TYPE(scm)) 
		{
			default:
				QSE_ASSERT (!"should never happen - invalid token type");
				qse_scm_seterror (scm, QSE_SCM_EINTERN, QSE_NULL, QSE_NULL);
				return -1;

#if 0
			case TOK_INVALID:
				qse_scm_seterror (
					scm, QSE_SCM_ESYNTAX,
					QSE_NULL, &TOK_LOC(scm));
				return -1;
#endif
			
			case TOK_END:
				qse_scm_seterror (
					scm, QSE_SCM_EEND,
					QSE_NULL, &TOK_LOC(scm));
				return -1;

			case TOK_QUOTE:
				if (level >= QSE_TYPE_MAX(int))
				{
					/* the nesting level has become too deep */
					qse_scm_seterror (
						scm, QSE_SCM_ELSTDEEP,
						QSE_NULL, &TOK_LOC(scm));
					return -1;
				}

				/* enter a quoted string */
				flagv |= QUOTED;
				if (enter_list (scm, flagv) == QSE_NULL) return -1;
				level++;

				/* force-chain the quote symbol to the new list entered */
				if (chain_to_list (scm, scm->quote) == QSE_NULL) return -1;

				/* read the next token */
				READ_TOKEN (scm);
				goto redo;
	
			case TOK_LPAREN:
				if (level >= QSE_TYPE_MAX(int))
				{
					/* the nesting level has become too deep */
					qse_scm_seterror (
						scm, QSE_SCM_ELSTDEEP,
						QSE_NULL, &TOK_LOC(scm));
					return -1;
				}

				/* enter a normal string */
				flagv = 0;
				if (enter_list (scm, flagv) == QSE_NULL) return -1;
				level++;

				/* read the next token */
				READ_TOKEN (scm);
				goto redo;

			case TOK_DOT:
				if (level <= 0 || is_list_empty (scm))
				{
					qse_scm_seterror (
						scm, QSE_SCM_EDOT, 
						QSE_NULL, &TOK_LOC(scm));
					return -1;
				}

				dot_list (scm);
				READ_TOKEN (scm);
				goto redo;
		
			case TOK_RPAREN:
				if ((flagv & QUOTED) || level <= 0)
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
					qse_scm_seterror (
						scm, QSE_SCM_ELPAREN, 
						QSE_NULL, &TOK_LOC(scm));
					return -1;
				}

				obj = leave_list (scm, &flagv);

				level--;
				break;

			case TOK_T:
				obj = scm->t;
				break;

			case TOK_F:
				obj = scm->f;
				break;

			case TOK_INT:
				obj = qse_scm_makenument (scm, TOK_IVAL(scm));
				break;

			case TOK_REAL:
				obj = qse_scm_makerealent (scm, TOK_RVAL(scm));
				break;
	
			case TOK_STRING:
				obj = qse_scm_makestrent (
					scm, TOK_NAME_PTR(scm), TOK_NAME_LEN(scm));
				break;

			case TOK_SYMBOL:
				obj = qse_scm_makesyment (scm, TOK_NAME_PTR(scm));
				break;
		}

		/* check if the element is read for a quoted list */
		while (flagv & QUOTED)
		{
			QSE_ASSERT (level > 0);

			/* if so, append the element read into the quote list */
			if (chain_to_list (scm, obj) == QSE_NULL) return -1;

			/* exit out of the quoted list. the quoted list can have 
			 * one element only. */
			obj = leave_list (scm, &flagv);

			/* one level up toward the top */
			level--;
		}

		/* check if we are at the top level */
		if (level <= 0) break; /* yes */

		/* if not, append the element read into the current list.
		 * if we are not at the top level, we must be in a list */
		if (chain_to_list (scm, obj) == QSE_NULL) return -1;

		/* read the next token */
		READ_TOKEN (scm);
	}

	/* upon exit, we must be at the top level */
	QSE_ASSERT (level == 0);

	scm->r.e = obj; 
	return 0;
}	

qse_scm_ent_t* qse_scm_read (qse_scm_t* scm)
{
	QSE_ASSERTX (
		scm->io.fns.in != QSE_NULL, 
		"Specify input function before calling qse_scm_read()"
	);

	if (read_char(scm) <= -1) return QSE_NULL;
	if (read_token(scm) <= -1) return QSE_NULL;

#if 0
	scm.r.state = READ_NORMAL;
	do 
	{
		if (func[scm.r.state] (scm) <= -1) return QSE_NULL;
	}
	while (scm.r.state != READ_DONE)
#endif

#if 0
	do
	{
		qse_printf (QSE_T("TOKEN: [%s]\n"), TOK_NAME_PTR(scm));
		if (read_token(scm) <= -1) return QSE_NULL;
	}
	while (TOK_TYPE(scm) != TOK_END);
#endif

	if (read_entity (scm) <= -1) return QSE_NULL;

#if 0
{
	int i;
	for (i = 0; i < 100; i++)
	{
		qse_printf (QSE_T("%p\n"), alloc_entity(scm, QSE_NULL, QSE_NULL));
	}
}
#endif
	return scm->r.e;
}

