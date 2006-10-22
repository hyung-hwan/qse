/*
 * $Id: rec.c,v 1.4 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

static int __split_record (sse_awk_run_t* run);
static int __recomp_record_fields (
	sse_awk_run_t* run, sse_size_t lv, 
	const sse_char_t* str, sse_size_t len);

int sse_awk_setrec (
	sse_awk_run_t* run, sse_size_t idx, const sse_char_t* str, sse_size_t len)
{
	sse_awk_val_t* v;
	int errnum;

	if (idx == 0)
	{
		if (str == SSE_AWK_STR_BUF(&run->inrec.line) &&
		    len == SSE_AWK_STR_LEN(&run->inrec.line))
		{
			if (sse_awk_clrrec (run, sse_true) == -1) return -1;
		}
		else
		{
			if (sse_awk_clrrec (run, sse_false) == -1) return -1;

			if (sse_awk_str_ncpy (&run->inrec.line, str, len) == (sse_size_t)-1)
			{
				sse_awk_clrrec (run, sse_false);
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
		}

		v = sse_awk_makestrval (run, str, len);
		if (v == SSE_NULL)
		{
			sse_awk_clrrec (run, sse_false);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		sse_awk_assert (run->awk, run->inrec.d0->type == SSE_AWK_VAL_NIL);
		/* d0 should be cleared before the next line is reached
		 * as it doesn't call sse_awk_refdownval on run->inrec.d0 */
		run->inrec.d0 = v;
		sse_awk_refupval (v);

		if (__split_record (run) == -1) 
		{
			errnum = run->errnum;
			sse_awk_clrrec (run, sse_false);
			run->errnum = errnum;
			return -1;
		}
	}
	else
	{
		if (__recomp_record_fields (run, idx, str, len) == -1)
		{
			errnum = run->errnum;
			sse_awk_clrrec (run, sse_false);
			run->errnum = errnum;
			return -1;
		}
	
		/* recompose $0 */
		v = sse_awk_makestrval (run,
			SSE_AWK_STR_BUF(&run->inrec.line), 
			SSE_AWK_STR_LEN(&run->inrec.line));
		if (v == SSE_NULL)
		{
			sse_awk_clrrec (run, sse_false);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		sse_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = v;
		sse_awk_refupval (v);
	}

	return 0;
}

static int __split_record (sse_awk_run_t* run)
{
	sse_char_t* p, * tok;
	sse_size_t len, tok_len, nflds;
	sse_awk_val_t* v, * fs;
	sse_char_t* fs_ptr, * fs_free;
	sse_size_t fs_len;
	int errnum;
       
	/* inrec should be cleared before __split_record is called */
	sse_awk_assert (run->awk, run->inrec.nflds == 0);

	/* get FS */
	fs = sse_awk_getglobal (run, SSE_AWK_GLOBAL_FS);
	if (fs->type == SSE_AWK_VAL_NIL)
	{
		fs_ptr = SSE_T(" ");
		fs_len = 1;
		fs_free = SSE_NULL;
	}
	else if (fs->type == SSE_AWK_VAL_STR)
	{
		fs_ptr = ((sse_awk_val_str_t*)fs)->buf;
		fs_len = ((sse_awk_val_str_t*)fs)->len;
		fs_free = SSE_NULL;
	}
	else 
	{
		fs_ptr = sse_awk_valtostr (
			run, fs, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &fs_len);
		if (fs_ptr == SSE_NULL) return -1;
		fs_free = fs_ptr;
	}

	/* scan the input record to count the fields */
	p = SSE_AWK_STR_BUF(&run->inrec.line);
	len = SSE_AWK_STR_LEN(&run->inrec.line);

	nflds = 0;
	while (p != SSE_NULL)
	{
		if (fs_len <= 1)
		{
			p = sse_awk_strxntok (run,
				p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = sse_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == SSE_NULL && errnum != SSE_AWK_ENOERR)
			{
				if (fs_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, fs_free);
				run->errnum = errnum;
				return -1;
			}
		}

		if (nflds == 0 && p == SSE_NULL && tok_len == 0)
		{
			/* there are no fields. it can just return here
			 * as sse_awk_clrrec has been called before this */
			if (fs_free != SSE_NULL) SSE_AWK_FREE (run->awk, fs_free);
			return 0;
		}

		sse_awk_assert (run->awk,
			(tok != SSE_NULL && tok_len > 0) || tok_len == 0);

		nflds++;
		len = SSE_AWK_STR_LEN(&run->inrec.line) - 
			(p - SSE_AWK_STR_BUF(&run->inrec.line));
	}

	/* allocate space */
	if (nflds > run->inrec.maxflds)
	{
		void* tmp = SSE_AWK_MALLOC (
			run->awk, sse_sizeof(*run->inrec.flds) * nflds);
		if (tmp == SSE_NULL) 
		{
			if (fs_free != SSE_NULL) SSE_AWK_FREE (run->awk, fs_free);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		if (run->inrec.flds != SSE_NULL) 
			SSE_AWK_FREE (run->awk, run->inrec.flds);
		run->inrec.flds = tmp;
		run->inrec.maxflds = nflds;
	}

	/* scan again and split it */
	p = SSE_AWK_STR_BUF(&run->inrec.line);
	len = SSE_AWK_STR_LEN(&run->inrec.line);

	while (p != SSE_NULL)
	{
		if (fs_len <= 1)
		{
			p = sse_awk_strxntok (
				run, p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = sse_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == SSE_NULL && errnum != SSE_AWK_ENOERR)
			{
				if (fs_free != SSE_NULL) 
					SSE_AWK_FREE (run->awk, fs_free);
				run->errnum = errnum;
				return -1;
			}
		}

		sse_awk_assert (run->awk,
			(tok != SSE_NULL && tok_len > 0) || tok_len == 0);

		run->inrec.flds[run->inrec.nflds].ptr = tok;
		run->inrec.flds[run->inrec.nflds].len = tok_len;
		run->inrec.flds[run->inrec.nflds].val = 
			sse_awk_makestrval (run, tok, tok_len);

		if (run->inrec.flds[run->inrec.nflds].val == SSE_NULL)
		{
			if (fs_free != SSE_NULL) SSE_AWK_FREE (run->awk, fs_free);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		sse_awk_refupval (run->inrec.flds[run->inrec.nflds].val);
		run->inrec.nflds++;

		len = SSE_AWK_STR_LEN(&run->inrec.line) - 
			(p - SSE_AWK_STR_BUF(&run->inrec.line));
	}

	if (fs_free != SSE_NULL) SSE_AWK_FREE (run->awk, fs_free);

	/* set the number of fields */
	v = sse_awk_makeintval (run, (sse_long_t)nflds);
	if (v == SSE_NULL) 
	{
		run->errnum = SSE_AWK_ENOMEM;
		return -1;
	}

	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_NF, v) == -1) return -1;

	sse_awk_assert (run->awk, nflds == run->inrec.nflds);
	return 0;
}

int sse_awk_clrrec (sse_awk_run_t* run, sse_bool_t skip_inrec_line)
{
	sse_size_t i;
	int n = 0;

	if (run->inrec.d0 != sse_awk_val_nil)
	{
		sse_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = sse_awk_val_nil;
	}

	if (run->inrec.nflds > 0)
	{
		sse_awk_assert (run->awk, run->inrec.flds != SSE_NULL);

		for (i = 0; i < run->inrec.nflds; i++) 
		{
			sse_awk_assert (run->awk,
				run->inrec.flds[i].val != SSE_NULL);
			sse_awk_refdownval (run, run->inrec.flds[i].val);
		}
		run->inrec.nflds = 0;

		if (sse_awk_setglobal (
			run, SSE_AWK_GLOBAL_NF, sse_awk_val_zero) == -1)
		{
			/* first of all, this should never happen. 
			 * if it happened, it would return an error
			 * after all the clearance tasks */
			n = -1;
		}
	}

	sse_awk_assert (run->awk, run->inrec.nflds == 0);
	if (!skip_inrec_line) sse_awk_str_clear (&run->inrec.line);

	return n;
}

static int __recomp_record_fields (
	sse_awk_run_t* run, sse_size_t lv, 
	const sse_char_t* str, sse_size_t len)
{
	sse_awk_val_t* v;
	sse_size_t max, i, nflds;

	/* recomposes the record and the fields when $N has been assigned 
	 * a new value and recomputes NF accordingly */

	sse_awk_assert (run->awk, lv > 0);
	max = (lv > run->inrec.nflds)? lv: run->inrec.nflds;

	nflds = run->inrec.nflds;
	if (max > run->inrec.maxflds)
	{
		void* tmp;

		/* if the given field number is greater than the maximum
		 * number of fields that the current record can hold,
		 * the field spaces are resized */

		if (run->awk->syscas.realloc != SSE_NULL)
		{
			tmp = SSE_AWK_REALLOC (
				run->awk, run->inrec.flds, 
				sse_sizeof(*run->inrec.flds) * max);
			if (tmp == SSE_NULL) 
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
		}
		else
		{
			tmp = SSE_AWK_MALLOC (
				run->awk, sse_sizeof(*run->inrec.flds) * max);
			if (tmp == SSE_NULL)
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
			if (run->inrec.flds != SSE_NULL)
			{
				SSE_AWK_MEMCPY (run->awk, tmp, run->inrec.flds, 
					sse_sizeof(*run->inrec.flds) * run->inrec.maxflds);
				SSE_AWK_FREE (run->awk, run->inrec.flds);
			}
		}

		run->inrec.flds = tmp;
		run->inrec.maxflds = max;
	}

	lv = lv - 1; /* adjust the value to 0-based index */

	sse_awk_str_clear (&run->inrec.line);

	for (i = 0; i < max; i++)
	{
		if (i > 0)
		{
			if (sse_awk_str_ncat (
				&run->inrec.line, 
				run->global.ofs.ptr, 
				run->global.ofs.len) == (sse_size_t)-1) 
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
		}

		if (i == lv)
		{
			sse_awk_val_t* tmp;

			run->inrec.flds[i].ptr = 
				SSE_AWK_STR_BUF(&run->inrec.line) +
				SSE_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = len;

			if (sse_awk_str_ncat (
				&run->inrec.line, str, len) == (sse_size_t)-1)
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}

			tmp = sse_awk_makestrval (run, str,len);
			if (tmp == SSE_NULL) 
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}

			if (i < nflds)
				sse_awk_refdownval (run, run->inrec.flds[i].val);
			else run->inrec.nflds++;

			run->inrec.flds[i].val = tmp;
			sse_awk_refupval (tmp);
		}
		else if (i >= nflds)
		{
			run->inrec.flds[i].ptr = 
				SSE_AWK_STR_BUF(&run->inrec.line) +
				SSE_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = 0;

			if (sse_awk_str_cat (
				&run->inrec.line, SSE_T("")) == (sse_size_t)-1)
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}

			/* sse_awk_refdownval should not be called over 
			 * run->inrec.flds[i].val as it is not initialized
			 * to any valid values */
			/*sse_awk_refdownval (run, run->inrec.flds[i].val);*/
			run->inrec.flds[i].val = sse_awk_val_zls;
			sse_awk_refupval (sse_awk_val_zls);
			run->inrec.nflds++;
		}
		else
		{
			sse_awk_val_str_t* tmp;

			tmp = (sse_awk_val_str_t*)run->inrec.flds[i].val;

			run->inrec.flds[i].ptr = 
				SSE_AWK_STR_BUF(&run->inrec.line) +
				SSE_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = tmp->len;

			if (sse_awk_str_ncat (&run->inrec.line, 
				tmp->buf, tmp->len) == (sse_size_t)-1)
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
		}
	}

	v = sse_awk_getglobal (run, SSE_AWK_GLOBAL_NF);
	sse_awk_assert (run->awk, v->type == SSE_AWK_VAL_INT);
	if (((sse_awk_val_int_t*)v)->val != max)
	{
		v = sse_awk_makeintval (run, (sse_long_t)max);
		if (v == SSE_NULL) 
		{
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		if (sse_awk_setglobal (
			run, SSE_AWK_GLOBAL_NF, v) == -1) return -1;
	}

	return 0;
}

