/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#define OUTPUT_STR(scm,str) QSE_BLOCK (\
	if (scm->io.fns.out(scm, QSE_SCM_IO_WRITE, &scm->io.arg.out, (qse_char_t*)str, qse_strlen(str)) == -1) \
	{ \
		qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, 0); \
		return -1; \
	} \
)

#define OUTPUT_STRX(scm,str,len) QSE_BLOCK ( \
	if (scm->io.fns.out(scm, QSE_SCM_IO_WRITE, &scm->io.arg.out, (qse_char_t*)str, qse_strlen(str)) == -1) \
	{ \
		qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, 0); \
		return -1; \
	} \
)

static qse_size_t long_to_str (
	qse_long_t value, int radix, 
	const qse_char_t* prefix, qse_char_t* buf, qse_size_t size)
{
	qse_long_t t, rem;
	qse_size_t len, ret, i;
	qse_size_t prefix_len;

	prefix_len = (prefix != QSE_NULL)? qse_strlen(prefix): 0;

	t = value;
	if (t == 0)
	{
		/* zero */
		if (buf == QSE_NULL) 
		{
			/* if buf is not given, 
			 * return the number of bytes required */
			return prefix_len + 1;
		}

		if (size < prefix_len+1) 
		{
			/* buffer too small */
			return (qse_size_t)-1;
		}

		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
		buf[prefix_len] = QSE_T('0');
		if (size > prefix_len+1) buf[prefix_len+1] = QSE_T('\0');
		return prefix_len+1;
	}

	/* non-zero values */
	len = prefix_len;
	if (t < 0) { t = -t; len++; }
	while (t > 0) { len++; t /= radix; }

	if (buf == QSE_NULL)
	{
		/* if buf is not given, return the number of bytes required */
		return len;
	}

	if (size < len) return (qse_size_t)-1; /* buffer too small */
	if (size > len) buf[len] = QSE_T('\0');
	ret = len;

	t = value;
	if (t < 0) t = -t;

	while (t > 0) 
	{
		rem = t % radix;
		if (rem >= 10)
			buf[--len] = (qse_char_t)rem + QSE_T('a') - 10;
		else
			buf[--len] = (qse_char_t)rem + QSE_T('0');
		t /= radix;
	}

	if (value < 0) 
	{
		for (i = 1; i <= prefix_len; i++) 
		{
			buf[i] = prefix[i-1];
			len--;
		}
		buf[--len] = QSE_T('-');
	}
	else
	{
		for (i = 0; i < prefix_len; i++) buf[i] = prefix[i];
	}

	return ret;
}

static QSE_INLINE int push (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	qse_scm_ent_t* top;

	top = qse_scm_makepairent (scm, obj, scm->p.s);
	if (top == QSE_NULL) return -1;	

	scm->p.s = top;
	return 0;
}

static QSE_INLINE qse_scm_ent_t* pop (qse_scm_t* scm)
{
	qse_scm_ent_t* top = scm->p.s;
	scm->p.s = PAIR_CDR(scm->p.s);
	return PAIR_CAR(top);
}

static QSE_INLINE int print_num (qse_scm_t* scm, qse_long_t nval)
{
	qse_char_t tmp[QSE_SIZEOF(qse_long_t)*8+2];
	qse_size_t len;

	len = long_to_str (nval, 10, QSE_NULL, tmp, QSE_COUNTOF(tmp));
	OUTPUT_STRX (scm, tmp, len);
	return 0;
}

static int print_entity (qse_scm_t* scm, const qse_scm_ent_t* obj)
{
	const qse_scm_ent_t* cur;

next:
	if (IS_SMALLINT(scm,obj))
	{
		if (print_num (scm, FROM_SMALLINT(scm,obj)) <= -1) return -1;
		goto done;
	}

	switch (TYPE(obj)) 
	{
		case QSE_SCM_ENT_NIL:
			OUTPUT_STR (scm, QSE_T("()"));
			break;

		case QSE_SCM_ENT_T:
			OUTPUT_STR (scm, QSE_T("#t"));
			break;

		case QSE_SCM_ENT_F:
			OUTPUT_STR (scm, QSE_T("#f"));
			break;

		case QSE_SCM_ENT_NUM:
		{
			if (print_num (scm, NUM_VALUE(obj)) <= -1) return -1;
			break;
		}

#if 0
		case QSE_SCM_ENT_REAL:
		{
			qse_char_t buf[256];
			scm->prm.sprintf (
				scm->prm.ctx,
				buf, QSE_COUNTOF(buf), 
				QSE_T("%Lf"), 
			#ifdef __MINGW32__
				(double)QSE_SCM_RVAL(obj)
			#else
				(long double)QSE_SCM_RVAL(obj)
			#endif
			);

			OUTPUT_STR (scm, buf);
			break;
		}
#endif

		case QSE_SCM_ENT_SYM:
			/* Any needs for special action if SYNT(obj) is true?
			 * I simply treat the syntax symbol as a normal symbol
			 * for printing currently. */
			OUTPUT_STR (scm, LAB_PTR(SYM_NAME(obj)));
			break;

		case QSE_SCM_ENT_STR:
			OUTPUT_STR (scm, QSE_T("\""));
			/* TODO: deescaping */
			OUTPUT_STRX (scm, STR_PTR(obj), STR_LEN(obj));
			OUTPUT_STR (scm, QSE_T("\""));
			break;

		case QSE_SCM_ENT_PAIR:
		{
			
			OUTPUT_STR (scm, QSE_T("("));
			cur = obj;

			do
			{
				/* Push what to print next on to the stack 
				 * the variable p is */
				if (push (scm, PAIR_CDR(cur)) <= -1) return -1;

				obj = PAIR_CAR(cur);
				/* Jump to the 'next' label so that the entity 
				 * pointed to by 'obj' is printed. Once it 
				 * ends, a jump back to the 'resume' label
				 * is made at the at of this function. */
				goto next; 

			resume:
				cur = pop (scm); /* Get back the CDR pushed */
				if (IS_NIL(scm,cur)) 
				{
					/* The CDR part points to a NIL entity, which
					 * indicates the end of a list. break the loop */
					break;
				}
				if (IS_SMALLINT(scm,cur) || TYPE(cur) != QSE_SCM_ENT_PAIR) 
				{
					/* The CDR part does not point to a pair. */
					OUTPUT_STR (scm, QSE_T(" . "));
						
					/* Push NIL so that the IS_NIL(scm,p) test in 
					 * the 'if' statement above breaks the loop
					 * after the jump is maded back to the 'resume' 
					 * label. */
					if (push (scm, scm->nil) <= -1) return -1;

					/* Make a jump to 'next' to print the CDR part */
					obj = cur;
					goto next;
				}

				/* The CDR part points to a pair. proceed to it */
				OUTPUT_STR (scm, QSE_T(" "));
			}
			while (1);
			OUTPUT_STR (scm, QSE_T(")"));
			break;
		}

		case QSE_SCM_ENT_PROC:
			OUTPUT_STR (scm, QSE_T("#<PROC>"));
			break;

		case QSE_SCM_ENT_CLOS:
			OUTPUT_STR (scm, QSE_T("#<CLOSURE>"));
			break;

		default:
			QSE_ASSERTX (
				0,
				"Unknown entity type - buggy!!"
			);
			qse_scm_seterror (scm, QSE_SCM_EINTERN, QSE_NULL, QSE_NULL);
			return -1;
	}

done:
	/* if the printing stack is not empty, we still got more to print */
	if (!IS_NIL(scm,scm->p.s)) goto resume; 

	return 0;
}

int qse_scm_print (qse_scm_t* scm, qse_scm_ent_t* obj)
{
	int n;

	QSE_ASSERTX (
		scm->io.fns.out != QSE_NULL, 
		"Specify output function before calling qse_scm_print()"
	);	

	QSE_ASSERTX (
		IS_NIL(scm,scm->p.s),
		"The printing stack is not empty before printing - buggy!!"
	);

	scm->p.e = obj; /* remember the head of the entity to print */
	n = print_entity (scm, obj); /* call the actual printing routine */
	scm->p.e = scm->nil; /* reset what's remembered */

	/* clear the printing stack if an error has occurred for GC not to keep
	 * the entities in the stack */
	if (n <= -1) scm->p.s = scm->nil;

	QSE_ASSERTX (
		IS_NIL(scm,scm->p.s),
		"The printing stack is not empty after printing - buggy!!"
	);
		
	return n;
}
