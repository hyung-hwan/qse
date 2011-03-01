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

static int print_entity (
	qse_scm_t* scm, const qse_scm_ent_t* obj, int prt_cons_par)
{
	qse_char_t buf[256];
	qse_long_t nval;

	if (IS_SMALLINT(scm,obj))
	{
		nval = FROM_SMALLINT(scm,obj);
		goto printnum;
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
			qse_char_t tmp[QSE_SIZEOF(qse_long_t)*8+2];
			qse_size_t len;

			nval = NUM_VALUE(obj);

		printnum:
			len = long_to_str (nval, 10, QSE_NULL, tmp, QSE_COUNTOF(tmp));
               OUTPUT_STRX (scm, tmp, len);
			break;
		}

#if 0
		case QSE_SCM_ENT_REAL:
			scm->prm.sprintf (
				scm->prm.udd,
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
#endif

		case QSE_SCM_ENT_SYM:
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
			const qse_scm_ent_t* p = obj;
			if (prt_cons_par) OUTPUT_STR (scm, QSE_T("("));
			do 
			{
				qse_scm_print (scm, PAIR_CAR(p));
				p = PAIR_CDR(p);
				if (!IS_NIL(scm,p))
				{
					OUTPUT_STR (scm, QSE_T(" "));
					if (TYPE(p) != QSE_SCM_ENT_PAIR) 
					{
						OUTPUT_STR (scm, QSE_T(". "));
						qse_scm_print (scm, p);
					}
				}
			} 
			while (p != scm->nil && TYPE(p) == QSE_SCM_ENT_PAIR);
			if (prt_cons_par) OUTPUT_STR (scm, QSE_T(")"));

			break;
		}

#if 0
		case QSE_SCM_ENT_FUNC:
			/*OUTPUT_STR (scm, QSE_T("func"));*/
			OUTPUT_STR (scm, QSE_T("(lambda "));
			if (print_entity (scm, QSE_SCM_FFORMAL(obj), 1) == -1) return -1;
			OUTPUT_STR (scm, QSE_T(" "));
			if (print_entity (scm, QSE_SCM_FBODY(obj), 0) == -1) return -1;
			OUTPUT_STR (scm, QSE_T(")"));
			break;

		case QSE_SCM_ENT_MACRO:
			OUTPUT_STR (scm, QSE_T("(macro "));
			if (print_entity (scm, QSE_SCM_FFORMAL(obj), 1) == -1) return -1;
			OUTPUT_STR (scm, QSE_T(" "));
			if (print_entity (scm, QSE_SCM_FBODY(obj), 0) == -1) return -1;
			OUTPUT_STR (scm, QSE_T(")"));
			break;
		case QSE_SCM_ENT_PRIM:
			OUTPUT_STR (scm, QSE_T("prim"));
			break;
#endif

		default:
			QSE_ASSERT (!"should never happen - unknown entity type");
			qse_scm_seterror (scm, QSE_SCM_EINTERN, QSE_NULL, QSE_NULL);
			return -1;
	}

	return 0;
}

int qse_scm_print (qse_scm_t* scm, const qse_scm_ent_t* obj)
{
	QSE_ASSERTX (
		scm->io.fns.out != QSE_NULL, 
		"Specify output function before calling qse_scm_print()"
	);	

	return print_entity (scm, obj, 1);
}
