/*
 * $Id: rec.c 372 2008-09-23 09:51:24Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "awk.h"

static int split_record (qse_awk_rtx_t* run);
static int recomp_record_fields (
	qse_awk_rtx_t* run, qse_size_t lv, 
	const qse_char_t* str, qse_size_t len);

int qse_awk_setrec (
	qse_awk_rtx_t* run, qse_size_t idx, 
	const qse_char_t* str, qse_size_t len)
{
	qse_awk_val_t* v;

	if (idx == 0)
	{
		if (str == QSE_STR_PTR(&run->inrec.line) &&
		    len == QSE_STR_LEN(&run->inrec.line))
		{
			if (qse_awk_clrrec (run, QSE_TRUE) == -1) return -1;
		}
		else
		{
			if (qse_awk_clrrec (run, QSE_FALSE) == -1) return -1;

			if (qse_str_ncpy (&run->inrec.line, str, len) == (qse_size_t)-1)
			{
				qse_awk_clrrec (run, QSE_FALSE);
				qse_awk_setrunerror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return -1;
			}
		}

		v = qse_awk_makestrval (run, str, len);
		if (v == QSE_NULL)
		{
			qse_awk_clrrec (run, QSE_FALSE);
			return -1;
		}

		QSE_ASSERT (run->inrec.d0->type == QSE_AWK_VAL_NIL);
		/* d0 should be cleared before the next line is reached
		 * as it doesn't call qse_awk_refdownval on run->inrec.d0 */
		run->inrec.d0 = v;
		qse_awk_refupval (run, v);

		if (split_record (run) == -1) 
		{
			qse_awk_clrrec (run, QSE_FALSE);
			return -1;
		}
	}
	else
	{
		if (recomp_record_fields (run, idx, str, len) == -1)
		{
			qse_awk_clrrec (run, QSE_FALSE);
			return -1;
		}
	
		/* recompose $0 */
		v = qse_awk_makestrval (run,
			QSE_STR_PTR(&run->inrec.line), 
			QSE_STR_LEN(&run->inrec.line));
		if (v == QSE_NULL)
		{
			qse_awk_clrrec (run, QSE_FALSE);
			return -1;
		}

		qse_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = v;
		qse_awk_refupval (run, v);
	}

	return 0;
}

static int split_record (qse_awk_rtx_t* run)
{
	qse_char_t* p, * tok;
	qse_size_t len, tok_len, nflds;
	qse_awk_val_t* v, * fs;
	qse_char_t* fs_ptr, * fs_free;
	qse_size_t fs_len;
	int errnum;
       
	/* inrec should be cleared before split_record is called */
	QSE_ASSERT (run->inrec.nflds == 0);

	/* get FS */
	fs = qse_awk_getglobal (run, QSE_AWK_GLOBAL_FS);
	if (fs->type == QSE_AWK_VAL_NIL)
	{
		fs_ptr = QSE_T(" ");
		fs_len = 1;
		fs_free = QSE_NULL;
	}
	else if (fs->type == QSE_AWK_VAL_STR)
	{
		fs_ptr = ((qse_awk_val_str_t*)fs)->ptr;
		fs_len = ((qse_awk_val_str_t*)fs)->len;
		fs_free = QSE_NULL;
	}
	else 
	{
		fs_ptr = qse_awk_valtostr (
			run, fs, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &fs_len);
		if (fs_ptr == QSE_NULL) return -1;
		fs_free = fs_ptr;
	}

	/* scan the input record to count the fields */
	p = QSE_STR_PTR(&run->inrec.line);
	len = QSE_STR_LEN(&run->inrec.line);

	nflds = 0;
	while (p != QSE_NULL)
	{
		if (fs_len <= 1)
		{
			p = qse_awk_strxntok (run,
				p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = qse_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == QSE_NULL && errnum != QSE_AWK_ENOERR)
			{
				if (fs_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, fs_free);
				qse_awk_setrunerror (run, errnum, 0, QSE_NULL, 0);
				return -1;
			}
		}

		if (nflds == 0 && p == QSE_NULL && tok_len == 0)
		{
			/* there are no fields. it can just return here
			 * as qse_awk_clrrec has been called before this */
			if (fs_free != QSE_NULL) QSE_AWK_FREE (run->awk, fs_free);
			return 0;
		}

		QSE_ASSERT ((tok != QSE_NULL && tok_len > 0) || tok_len == 0);

		nflds++;
		len = QSE_STR_LEN(&run->inrec.line) - 
			(p - QSE_STR_PTR(&run->inrec.line));
	}

	/* allocate space */
	if (nflds > run->inrec.maxflds)
	{
		void* tmp = QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(*run->inrec.flds) * nflds);
		if (tmp == QSE_NULL) 
		{
			if (fs_free != QSE_NULL) QSE_AWK_FREE (run->awk, fs_free);
			qse_awk_setrunerror (run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return -1;
		}

		if (run->inrec.flds != QSE_NULL) 
			QSE_AWK_FREE (run->awk, run->inrec.flds);
		run->inrec.flds = tmp;
		run->inrec.maxflds = nflds;
	}

	/* scan again and split it */
	p = QSE_STR_PTR(&run->inrec.line);
	len = QSE_STR_LEN(&run->inrec.line);

	while (p != QSE_NULL)
	{
		if (fs_len <= 1)
		{
			p = qse_awk_strxntok (
				run, p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = qse_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == QSE_NULL && errnum != QSE_AWK_ENOERR)
			{
				if (fs_free != QSE_NULL) 
					QSE_AWK_FREE (run->awk, fs_free);
				qse_awk_setrunerror (run, errnum, 0, QSE_NULL, 0);
				return -1;
			}
		}

		QSE_ASSERT ((tok != QSE_NULL && tok_len > 0) || tok_len == 0);

		run->inrec.flds[run->inrec.nflds].ptr = tok;
		run->inrec.flds[run->inrec.nflds].len = tok_len;
		run->inrec.flds[run->inrec.nflds].val = 
			qse_awk_makestrval (run, tok, tok_len);

		if (run->inrec.flds[run->inrec.nflds].val == QSE_NULL)
		{
			if (fs_free != QSE_NULL) QSE_AWK_FREE (run->awk, fs_free);
			return -1;
		}

		qse_awk_refupval (run, run->inrec.flds[run->inrec.nflds].val);
		run->inrec.nflds++;

		len = QSE_STR_LEN(&run->inrec.line) - 
			(p - QSE_STR_PTR(&run->inrec.line));
	}

	if (fs_free != QSE_NULL) QSE_AWK_FREE (run->awk, fs_free);

	/* set the number of fields */
	v = qse_awk_makeintval (run, (qse_long_t)nflds);
	if (v == QSE_NULL) return -1;

	qse_awk_refupval (run, v);
	if (qse_awk_setglobal (run, QSE_AWK_GLOBAL_NF, v) == -1) 
	{
		qse_awk_refdownval (run, v);
		return -1;
	}

	qse_awk_refdownval (run, v);
	QSE_ASSERT (nflds == run->inrec.nflds);
	return 0;
}

int qse_awk_clrrec (qse_awk_rtx_t* run, qse_bool_t skip_inrec_line)
{
	qse_size_t i;
	int n = 0;

	if (run->inrec.d0 != qse_awk_val_nil)
	{
		qse_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = qse_awk_val_nil;
	}

	if (run->inrec.nflds > 0)
	{
		QSE_ASSERT (run->inrec.flds != QSE_NULL);

		for (i = 0; i < run->inrec.nflds; i++) 
		{
			QSE_ASSERT (run->inrec.flds[i].val != QSE_NULL);
			qse_awk_refdownval (run, run->inrec.flds[i].val);
		}
		run->inrec.nflds = 0;

		if (qse_awk_setglobal (
			run, QSE_AWK_GLOBAL_NF, qse_awk_val_zero) == -1)
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
	qse_awk_rtx_t* run, qse_size_t lv, 
	const qse_char_t* str, qse_size_t len)
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

		if (run->awk->mmgr->realloc != QSE_NULL)
		{
			tmp = QSE_AWK_REALLOC (
				run->awk, run->inrec.flds, 
				QSE_SIZEOF(*run->inrec.flds) * max);
			if (tmp == QSE_NULL) 
			{
				qse_awk_setrunerror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return -1;
			}
		}
		else
		{
			tmp = QSE_AWK_ALLOC (
				run->awk, QSE_SIZEOF(*run->inrec.flds) * max);
			if (tmp == QSE_NULL)
			{
				qse_awk_setrunerror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return -1;
			}
			if (run->inrec.flds != QSE_NULL)
			{
				QSE_MEMCPY (tmp, run->inrec.flds, 
					QSE_SIZEOF(*run->inrec.flds)*run->inrec.maxflds);
				QSE_AWK_FREE (run->awk, run->inrec.flds);
			}
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
			if (qse_str_ncat (
				&run->inrec.line, 
				run->global.ofs.ptr, 
				run->global.ofs.len) == (qse_size_t)-1) 
			{
				qse_awk_setrunerror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return -1;
			}
		}

		if (i == lv)
		{
			qse_awk_val_t* tmp;

			run->inrec.flds[i].ptr = 
				QSE_STR_PTR(&run->inrec.line) +
				QSE_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = len;

			if (qse_str_ncat (
				&run->inrec.line, str, len) == (qse_size_t)-1)
			{
				qse_awk_setrunerror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return -1;
			}

			tmp = qse_awk_makestrval (run, str,len);
			if (tmp == QSE_NULL) return -1;

			if (i < nflds)
				qse_awk_refdownval (run, run->inrec.flds[i].val);
			else run->inrec.nflds++;

			run->inrec.flds[i].val = tmp;
			qse_awk_refupval (run, tmp);
		}
		else if (i >= nflds)
		{
			run->inrec.flds[i].ptr = 
				QSE_STR_PTR(&run->inrec.line) +
				QSE_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = 0;

			if (qse_str_cat (
				&run->inrec.line, QSE_T("")) == (qse_size_t)-1)
			{
				qse_awk_setrunerror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return -1;
			}

			/* qse_awk_refdownval should not be called over 
			 * run->inrec.flds[i].val as it is not initialized
			 * to any valid values */
			/*qse_awk_refdownval (run, run->inrec.flds[i].val);*/
			run->inrec.flds[i].val = qse_awk_val_zls;
			qse_awk_refupval (run, qse_awk_val_zls);
			run->inrec.nflds++;
		}
		else
		{
			qse_awk_val_str_t* tmp;

			tmp = (qse_awk_val_str_t*)run->inrec.flds[i].val;

			run->inrec.flds[i].ptr = 
				QSE_STR_PTR(&run->inrec.line) +
				QSE_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = tmp->len;

			if (qse_str_ncat (&run->inrec.line, 
				tmp->ptr, tmp->len) == (qse_size_t)-1)
			{
				qse_awk_setrunerror (
					run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
				return -1;
			}
		}
	}

	v = qse_awk_getglobal (run, QSE_AWK_GLOBAL_NF);
	QSE_ASSERT (v->type == QSE_AWK_VAL_INT);

	if (((qse_awk_val_int_t*)v)->val != max)
	{
		v = qse_awk_makeintval (run, (qse_long_t)max);
		if (v == QSE_NULL) return -1;

		qse_awk_refupval (run, v);
		if (qse_awk_setglobal (run, QSE_AWK_GLOBAL_NF, v) == -1) 
		{
			qse_awk_refdownval (run, v);
			return -1;
		}
		qse_awk_refdownval (run, v);
	}

	return 0;
}

