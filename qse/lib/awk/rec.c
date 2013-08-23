/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include "awk.h"

static int split_record (qse_awk_rtx_t* run);
static int recomp_record_fields (
	qse_awk_rtx_t* run, qse_size_t lv, const qse_cstr_t* str);

int qse_awk_rtx_setrec (
	qse_awk_rtx_t* run, qse_size_t idx, const qse_cstr_t* str)
{
	qse_awk_val_t* v;

	if (idx == 0)
	{
		if (str->ptr == QSE_STR_PTR(&run->inrec.line) &&
		    str->len == QSE_STR_LEN(&run->inrec.line))
		{
			if (qse_awk_rtx_clrrec (run, 1) == -1) return -1;
		}
		else
		{
			if (qse_awk_rtx_clrrec (run, 0) == -1) return -1;

			if (qse_str_ncpy (&run->inrec.line, str->ptr, str->len) == (qse_size_t)-1)
			{
				qse_awk_rtx_clrrec (run, 0);
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
		}

		v = qse_awk_rtx_makenstrvalwithcstr (run, str);

		if (v == QSE_NULL)
		{
			qse_awk_rtx_clrrec (run, 0);
			return -1;
		}

		QSE_ASSERT (run->inrec.d0->type == QSE_AWK_VAL_NIL);
		/* d0 should be cleared before the next line is reached
		 * as it doesn't call qse_awk_rtx_refdownval on run->inrec.d0 */
		run->inrec.d0 = v;
		qse_awk_rtx_refupval (run, v);

		if (split_record (run) == -1) 
		{
			qse_awk_rtx_clrrec (run, 0);
			return -1;
		}
	}
	else
	{
		if (recomp_record_fields (run, idx, str) <= -1)
		{
			qse_awk_rtx_clrrec (run, 0);
			return -1;
		}
	
		/* recompose $0 */
		v = qse_awk_rtx_makestrvalwithcstr (run, QSE_STR_CSTR(&run->inrec.line));
		if (v == QSE_NULL)
		{
			qse_awk_rtx_clrrec (run, 0);
			return -1;
		}

		qse_awk_rtx_refdownval (run, run->inrec.d0);
		run->inrec.d0 = v;
		qse_awk_rtx_refupval (run, v);
	}

	return 0;
}

static int split_record (qse_awk_rtx_t* rtx)
{
	qse_cstr_t tok;
	qse_char_t* p, * px;
	qse_size_t len, nflds;
	qse_awk_val_t* v, * fs;
	qse_char_t* fs_ptr, * fs_free;
	qse_size_t fs_len;
	qse_awk_errnum_t errnum;
	int how;
       
	/* inrec should be cleared before split_record is called */
	QSE_ASSERT (rtx->inrec.nflds == 0);

	/* get FS */
	fs = qse_awk_rtx_getgbl (rtx, QSE_AWK_GBL_FS);
	if (fs->type == QSE_AWK_VAL_NIL)
	{
		fs_ptr = QSE_T(" ");
		fs_len = 1;
		fs_free = QSE_NULL;
	}
	else if (fs->type == QSE_AWK_VAL_STR)
	{
		fs_ptr = ((qse_awk_val_str_t*)fs)->val.ptr;
		fs_len = ((qse_awk_val_str_t*)fs)->val.len;
		fs_free = QSE_NULL;
	}
	else 
	{
		fs_ptr = qse_awk_rtx_valtostrdup (rtx, fs, &fs_len);
		if (fs_ptr == QSE_NULL) return -1;
		fs_free = fs_ptr;
	}

	/* scan the input record to count the fields */
	if (fs_len == 5 && fs_ptr[0] ==  QSE_T('?'))
	{
		if (qse_str_ncpy (
			&rtx->inrec.linew, 
			QSE_STR_PTR(&rtx->inrec.line),
			QSE_STR_LEN(&rtx->inrec.line)) == (qse_size_t)-1)
		{
			if (fs_free != QSE_NULL) 
				QSE_AWK_FREE (rtx->awk, fs_free);
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		px = QSE_STR_PTR(&rtx->inrec.linew);
		how = 1;
	}
	else
	{
		px = QSE_STR_PTR(&rtx->inrec.line);
		how = (fs_len <= 1)? 0: 2;
	}

	p = px; 
	len = QSE_STR_LEN(&rtx->inrec.line);

#if 0
	nflds = 0;
	while (p != QSE_NULL)
	{
		switch (how)
		{
			case 0:
				p = qse_awk_rtx_strxntok (rtx,
					p, len, fs_ptr, fs_len, &tok);
				break;

			case 1:
				break;

			default:
				p = qse_awk_rtx_strxntokbyrex (
					rtx, 
					QSE_STR_PTR(&rtx->inrec.line),
					QSE_STR_LEN(&rtx->inrec.line),
					p, len, 
					rtx->gbl.fs[rtx->gbl.ignorecase], &tok, &errnum
				); 
				if (p == QSE_NULL && errnum != QSE_AWK_ENOERR)
				{
					if (fs_free != QSE_NULL) 
						QSE_AWK_FREE (rtx->awk, fs_free);
					qse_awk_rtx_seterrnum (rtx, errnum, QSE_NULL);
					return -1;
				}
		}

		if (nflds == 0 && p == QSE_NULL && tok.len == 0)
		{
			/* there are no fields. it can just return here
			 * as qse_awk_rtx_clrrec has been called before this */
			if (fs_free != QSE_NULL) QSE_AWK_FREE (rtx->awk, fs_free);
			return 0;
		}

		QSE_ASSERT ((tok.ptr != QSE_NULL && tok.len > 0) || tok.len == 0);

		nflds++;
		len = QSE_STR_LEN(&rtx->inrec.line) - 
			(p - QSE_STR_PTR(&rtx->inrec.line));
	}

	/* allocate space */
	if (nflds > rtx->inrec.maxflds)
	{
		void* tmp = QSE_AWK_ALLOC (
			rtx->awk, QSE_SIZEOF(*rtx->inrec.flds) * nflds);
		if (tmp == QSE_NULL) 
		{
			if (fs_free != QSE_NULL) QSE_AWK_FREE (rtx->awk, fs_free);
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		if (rtx->inrec.flds != QSE_NULL) 
			QSE_AWK_FREE (rtx->awk, rtx->inrec.flds);
		rtx->inrec.flds = tmp;
		rtx->inrec.maxflds = nflds;
	}

	/* scan again and split it */
	if (how == 1)
	{
		if (qse_str_ncpy (
			&rtx->inrec.linew, 
			QSE_STR_PTR(&rtx->inrec.line),
			QSE_STR_LEN(&rtx->inrec.line)) == (qse_size_t)-1)
		{
			if (fs_free != QSE_NULL) 
				QSE_AWK_FREE (rtx->awk, fs_free);
			qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}
		px = QSE_STR_PTR(&rtx->inrec.linew):
	}
	else
	{
		px = QSE_STR_PTR(&rtx->inrec.line);
	}

	p = px; 
	len = QSE_STR_LEN(&rtx->inrec.line);
#endif

	while (p != QSE_NULL)
	{
		switch (how)
		{
			case 0:
				p = qse_awk_rtx_strxntok (
					rtx, p, len, fs_ptr, fs_len, &tok);
				break;

			case 1:
				p = qse_awk_rtx_strxnfld (
					rtx, p, len, 
					fs_ptr[1], fs_ptr[2],
					fs_ptr[3], fs_ptr[4], &tok);
				break;

			default:
				p = qse_awk_rtx_strxntokbyrex (
					rtx, 
					QSE_STR_PTR(&rtx->inrec.line),
					QSE_STR_LEN(&rtx->inrec.line),
					p, len,
					rtx->gbl.fs[rtx->gbl.ignorecase], &tok, &errnum
				); 
				if (p == QSE_NULL && errnum != QSE_AWK_ENOERR)
				{
					if (fs_free != QSE_NULL) 
						QSE_AWK_FREE (rtx->awk, fs_free);
					qse_awk_rtx_seterrnum (rtx, errnum, QSE_NULL);
					return -1;
				}
		}

#if 1
		if (rtx->inrec.nflds == 0 && p == QSE_NULL && tok.len == 0)
		{
			/* there are no fields. it can just return here
			 * as qse_awk_rtx_clrrec has been called before this */
			if (fs_free != QSE_NULL) QSE_AWK_FREE (rtx->awk, fs_free);
			return 0;
		}
#endif

		QSE_ASSERT ((tok.ptr != QSE_NULL && tok.len > 0) || tok.len == 0);

#if 1
		if (rtx->inrec.nflds >= rtx->inrec.maxflds)
		{
			void* tmp;

			if (rtx->inrec.nflds < 16) nflds = 32;
			else nflds = rtx->inrec.nflds * 2;

			tmp = QSE_AWK_ALLOC (
				rtx->awk, 
				QSE_SIZEOF(*rtx->inrec.flds) * nflds
			);
			if (tmp == QSE_NULL) 
			{
				if (fs_free != QSE_NULL) QSE_AWK_FREE (rtx->awk, fs_free);
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}

			if (rtx->inrec.flds != QSE_NULL) 
			{
				QSE_MEMCPY (tmp, rtx->inrec.flds, 
					QSE_SIZEOF(*rtx->inrec.flds) * rtx->inrec.nflds);
				QSE_AWK_FREE (rtx->awk, rtx->inrec.flds);
			}

			rtx->inrec.flds = tmp;
			rtx->inrec.maxflds = nflds;
		}
#endif

		rtx->inrec.flds[rtx->inrec.nflds].ptr = tok.ptr;
		rtx->inrec.flds[rtx->inrec.nflds].len = tok.len;
		rtx->inrec.flds[rtx->inrec.nflds].val = qse_awk_rtx_makenstrvalwithcstr (rtx, &tok);

		if (rtx->inrec.flds[rtx->inrec.nflds].val == QSE_NULL)
		{
			if (fs_free) QSE_AWK_FREE (rtx->awk, fs_free);
			return -1;
		}

		qse_awk_rtx_refupval (
			rtx, rtx->inrec.flds[rtx->inrec.nflds].val);
		rtx->inrec.nflds++;

		len = QSE_STR_LEN(&rtx->inrec.line) - (p - px);
	}

	if (fs_free != QSE_NULL) QSE_AWK_FREE (rtx->awk, fs_free);

	/* set the number of fields */
	v = qse_awk_rtx_makeintval (rtx, (qse_long_t)rtx->inrec.nflds);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, v);
	if (qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_NF, v) == -1) 
	{
		qse_awk_rtx_refdownval (rtx, v);
		return -1;
	}

	qse_awk_rtx_refdownval (rtx, v);
	return 0;
}

int qse_awk_rtx_clrrec (qse_awk_rtx_t* run, int skip_inrec_line)
{
	qse_size_t i;
	int n = 0;

	if (run->inrec.d0 != qse_awk_val_nil)
	{
		qse_awk_rtx_refdownval (run, run->inrec.d0);
		run->inrec.d0 = qse_awk_val_nil;
	}

	if (run->inrec.nflds > 0)
	{
		QSE_ASSERT (run->inrec.flds != QSE_NULL);

		for (i = 0; i < run->inrec.nflds; i++) 
		{
			QSE_ASSERT (run->inrec.flds[i].val != QSE_NULL);
			qse_awk_rtx_refdownval (run, run->inrec.flds[i].val);
		}
		run->inrec.nflds = 0;

		if (qse_awk_rtx_setgbl (
			run, QSE_AWK_GBL_NF, qse_awk_val_zero) == -1)
		{
			/* first of all, this should never happen. 
			 * if it happened, it would return an error
			 * after all the clearance tasks */
			n = -1;
		}
	}

	QSE_ASSERT (run->inrec.nflds == 0);
	if (!skip_inrec_line) qse_str_clear (&run->inrec.line);

	return n;
}

static int recomp_record_fields (
	qse_awk_rtx_t* run, qse_size_t lv, const qse_cstr_t* str)
{
	qse_awk_val_t* v;
	qse_size_t max, i, nflds;

	/* recomposes the record and the fields when $N has been assigned 
	 * a new value and recomputes NF accordingly */

	QSE_ASSERT (lv > 0);
	max = (lv > run->inrec.nflds)? lv: run->inrec.nflds;

	nflds = run->inrec.nflds;
	if (max > run->inrec.maxflds)
	{
		void* tmp;

		/* if the given field number is greater than the maximum
		 * number of fields that the current record can hold,
		 * the field spaces are resized */

		tmp = QSE_AWK_REALLOC (
			run->awk, run->inrec.flds, 
			QSE_SIZEOF(*run->inrec.flds) * max);
		if (tmp == QSE_NULL) 
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		run->inrec.flds = tmp;
		run->inrec.maxflds = max;
	}

	lv = lv - 1; /* adjust the value to 0-based index */

	qse_str_clear (&run->inrec.line);

	for (i = 0; i < max; i++)
	{
		if (i > 0)
		{
			if (qse_str_ncat (&run->inrec.line, run->gbl.ofs.ptr, run->gbl.ofs.len) == (qse_size_t)-1) 
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
		}

		if (i == lv)
		{
			qse_awk_val_t* tmp;

			run->inrec.flds[i].ptr = 
				QSE_STR_PTR(&run->inrec.line) +
				QSE_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = str->len;

			if (qse_str_ncat (&run->inrec.line, str->ptr, str->len) == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}

			tmp = qse_awk_rtx_makestrvalwithcstr (run, str);
			if (tmp == QSE_NULL) return -1;

			if (i < nflds)
				qse_awk_rtx_refdownval (run, run->inrec.flds[i].val);
			else run->inrec.nflds++;

			run->inrec.flds[i].val = tmp;
			qse_awk_rtx_refupval (run, tmp);
		}
		else if (i >= nflds)
		{
			run->inrec.flds[i].ptr = 
				QSE_STR_PTR(&run->inrec.line) +
				QSE_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = 0;

			if (qse_str_cat (&run->inrec.line, QSE_T("")) == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}

			/* qse_awk_rtx_refdownval should not be called over 
			 * run->inrec.flds[i].val as it is not initialized
			 * to any valid values */
			/*qse_awk_rtx_refdownval (run, run->inrec.flds[i].val);*/
			run->inrec.flds[i].val = qse_awk_val_zls;
			qse_awk_rtx_refupval (run, qse_awk_val_zls);
			run->inrec.nflds++;
		}
		else
		{
			qse_awk_val_str_t* tmp;

			tmp = (qse_awk_val_str_t*)run->inrec.flds[i].val;

			run->inrec.flds[i].ptr = 
				QSE_STR_PTR(&run->inrec.line) +
				QSE_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = tmp->val.len;

			if (qse_str_ncat (
				&run->inrec.line, 
				tmp->val.ptr, tmp->val.len) == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				return -1;
			}
		}
	}

	v = qse_awk_rtx_getgbl (run, QSE_AWK_GBL_NF);
	QSE_ASSERT (v->type == QSE_AWK_VAL_INT);

	if (((qse_awk_val_int_t*)v)->val != max)
	{
		v = qse_awk_rtx_makeintval (run, (qse_long_t)max);
		if (v == QSE_NULL) return -1;

		qse_awk_rtx_refupval (run, v);
		if (qse_awk_rtx_setgbl (run, QSE_AWK_GBL_NF, v) == -1) 
		{
			qse_awk_rtx_refdownval (run, v);
			return -1;
		}
		qse_awk_rtx_refdownval (run, v);
	}

	return 0;
}

