/*
 * $Id: rec.c,v 1.1 2006-10-03 14:57:01 bacon Exp $
 */

#include <xp/awk/awk_i.h>

static int __split_record (xp_awk_run_t* run);
static int __recomp_record_fields (
	xp_awk_run_t* run, xp_size_t lv, 
	const xp_char_t* str, xp_size_t len);

int xp_awk_setrec (
	xp_awk_run_t* run, xp_size_t idx, const xp_char_t* str, xp_size_t len)
{
	xp_awk_val_t* v;
	int errnum;

	if (idx == 0)
	{
		if (str == XP_AWK_STR_BUF(&run->inrec.line) &&
		    len == XP_AWK_STR_LEN(&run->inrec.line))
		{
			if (xp_awk_clrrec (run, xp_true) == -1) return -1;
		}
		else
		{
			if (xp_awk_clrrec (run, xp_false) == -1) return -1;

			if (xp_awk_str_ncpy (&run->inrec.line, str, len) == (xp_size_t)-1)
			{
				xp_awk_clrrec (run, xp_false);
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}
		}

		v = xp_awk_makestrval (run, str, len);
		if (v == XP_NULL)
		{
			xp_awk_clrrec (run, xp_false);
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		xp_assert (run->inrec.d0->type == XP_AWK_VAL_NIL);
		/* d0 should be cleared before the next line is reached
		 * as it doesn't call xp_awk_refdownval on run->inrec.d0 */
		run->inrec.d0 = v;
		xp_awk_refupval (v);

		if (__split_record (run) == -1) 
		{
			errnum = run->errnum;
			xp_awk_clrrec (run, xp_false);
			run->errnum = errnum;
			return -1;
		}
	}
	else
	{
		if (__recomp_record_fields (run, idx, str, len) == -1)
		{
			errnum = run->errnum;
			xp_awk_clrrec (run, xp_false);
			run->errnum = errnum;
			return -1;
		}
	
		/* recompose $0 */
		v = xp_awk_makestrval (run,
			XP_AWK_STR_BUF(&run->inrec.line), 
			XP_AWK_STR_LEN(&run->inrec.line));
		if (v == XP_NULL)
		{
			xp_awk_clrrec (run, xp_false);
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		xp_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = v;
		xp_awk_refupval (v);
	}

	return 0;
}

static int __split_record (xp_awk_run_t* run)
{
	xp_char_t* p, * tok;
	xp_size_t len, tok_len, nflds;
	xp_awk_val_t* v, * fs;
	xp_char_t* fs_ptr, * fs_free;
	xp_size_t fs_len;
	int errnum;
       
	/* inrec should be cleared before __split_record is called */
	xp_assert (run->inrec.nflds == 0);

	/* get FS */
	fs = xp_awk_getglobal (run, XP_AWK_GLOBAL_FS);
	if (fs->type == XP_AWK_VAL_NIL)
	{
		fs_ptr = XP_T(" ");
		fs_len = 1;
		fs_free = XP_NULL;
	}
	else if (fs->type == XP_AWK_VAL_STR)
	{
		fs_ptr = ((xp_awk_val_str_t*)fs)->buf;
		fs_len = ((xp_awk_val_str_t*)fs)->len;
		fs_free = XP_NULL;
	}
	else 
	{
		fs_ptr = xp_awk_valtostr (
			run, fs, xp_true, XP_NULL, &fs_len);
		if (fs_ptr == XP_NULL) return -1;
		fs_free = fs_ptr;
	}

	/* scan the input record to count the fields */
	p = XP_AWK_STR_BUF(&run->inrec.line);
	len = XP_AWK_STR_LEN(&run->inrec.line);

	nflds = 0;
	while (p != XP_NULL)
	{
		if (fs_len <= 1)
		{
			p = xp_awk_strxntok (run,
				p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = xp_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == XP_NULL && errnum != XP_AWK_ENOERR)
			{
				if (fs_free != XP_NULL) 
					XP_AWK_FREE (run->awk, fs_free);
				run->errnum = errnum;
				return -1;
			}
		}

		if (nflds == 0 && p == XP_NULL && tok_len == 0)
		{
			/* there are no fields. it can just return here
			 * as xp_awk_clrrec has been called before this */
			if (fs_free != XP_NULL) XP_AWK_FREE (run->awk, fs_free);
			return 0;
		}

		xp_assert ((tok != XP_NULL && tok_len > 0) || tok_len == 0);

		nflds++;
		len = XP_AWK_STR_LEN(&run->inrec.line) - 
			(p - XP_AWK_STR_BUF(&run->inrec.line));
	}

	/* allocate space */
	if (nflds > run->inrec.maxflds)
	{
		void* tmp = XP_AWK_MALLOC (
			run->awk, xp_sizeof(*run->inrec.flds) * nflds);
		if (tmp == XP_NULL) 
		{
			if (fs_free != XP_NULL) XP_AWK_FREE (run->awk, fs_free);
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		if (run->inrec.flds != XP_NULL) 
			XP_AWK_FREE (run->awk, run->inrec.flds);
		run->inrec.flds = tmp;
		run->inrec.maxflds = nflds;
	}

	/* scan again and split it */
	p = XP_AWK_STR_BUF(&run->inrec.line);
	len = XP_AWK_STR_LEN(&run->inrec.line);

	while (p != XP_NULL)
	{
		if (fs_len <= 1)
		{
			p = xp_awk_strxntok (
				run, p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = xp_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == XP_NULL && errnum != XP_AWK_ENOERR)
			{
				if (fs_free != XP_NULL) 
					XP_AWK_FREE (run->awk, fs_free);
				run->errnum = errnum;
				return -1;
			}
		}

		xp_assert ((tok != XP_NULL && tok_len > 0) || tok_len == 0);

		run->inrec.flds[run->inrec.nflds].ptr = tok;
		run->inrec.flds[run->inrec.nflds].len = tok_len;
		run->inrec.flds[run->inrec.nflds].val = 
			xp_awk_makestrval (run, tok, tok_len);

		if (run->inrec.flds[run->inrec.nflds].val == XP_NULL)
		{
			if (fs_free != XP_NULL) XP_AWK_FREE (run->awk, fs_free);
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		xp_awk_refupval (run->inrec.flds[run->inrec.nflds].val);
		run->inrec.nflds++;

		len = XP_AWK_STR_LEN(&run->inrec.line) - 
			(p - XP_AWK_STR_BUF(&run->inrec.line));
	}

	if (fs_free != XP_NULL) XP_AWK_FREE (run->awk, fs_free);

	/* set the number of fields */
	v = xp_awk_makeintval (run, (xp_long_t)nflds);
	if (v == XP_NULL) 
	{
		run->errnum = XP_AWK_ENOMEM;
		return -1;
	}

	if (xp_awk_setglobal (run, XP_AWK_GLOBAL_NF, v) == -1) return -1;

	xp_assert (nflds == run->inrec.nflds);
	return 0;
}

int xp_awk_clrrec (xp_awk_run_t* run, xp_bool_t skip_inrec_line)
{
	xp_size_t i;
	int n = 0;

	if (run->inrec.d0 != xp_awk_val_nil)
	{
		xp_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = xp_awk_val_nil;
	}

	if (run->inrec.nflds > 0)
	{
		xp_assert (run->inrec.flds != XP_NULL);

		for (i = 0; i < run->inrec.nflds; i++) 
		{
			xp_assert (run->inrec.flds[i].val != XP_NULL);
			xp_awk_refdownval (run, run->inrec.flds[i].val);
		}
		run->inrec.nflds = 0;

		if (xp_awk_setglobal (
			run, XP_AWK_GLOBAL_NF, xp_awk_val_zero) == -1)
		{
			/* first of all, this should never happen. 
			 * if it happened, it would return an error
			 * after all the clearance tasks */
			n = -1;
		}
	}

	xp_assert (run->inrec.nflds == 0);
	if (!skip_inrec_line) xp_awk_str_clear (&run->inrec.line);

	return n;
}

static int __recomp_record_fields (
	xp_awk_run_t* run, xp_size_t lv, 
	const xp_char_t* str, xp_size_t len)
{
	xp_awk_val_t* v;
	xp_size_t max, i, nflds;

	/* recomposes the record and the fields when $N has been assigned 
	 * a new value and recomputes NF accordingly */

	xp_assert (lv > 0);
	max = (lv > run->inrec.nflds)? lv: run->inrec.nflds;

	nflds = run->inrec.nflds;
	if (max > run->inrec.maxflds)
	{
		void* tmp;

		/* if the given field number is greater than the maximum
		 * number of fields that the current record can hold,
		 * the field spaces are resized */

		if (run->awk->syscas->realloc != XP_NULL)
		{
			tmp = XP_AWK_REALLOC (
				run->awk, run->inrec.flds, 
				xp_sizeof(*run->inrec.flds) * max);
			if (tmp == XP_NULL) 
			{
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}
		}
		else
		{
			tmp = XP_AWK_MALLOC (
				run->awk, xp_sizeof(*run->inrec.flds) * max);
			if (tmp == XP_NULL)
			{
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}
			if (run->inrec.flds != XP_NULL)
			{
				XP_AWK_MEMCPY (run->awk, tmp, run->inrec.flds, 
					xp_sizeof(*run->inrec.flds) * run->inrec.maxflds);
				XP_AWK_FREE (run->awk, run->inrec.flds);
			}
		}

		run->inrec.flds = tmp;
		run->inrec.maxflds = max;
	}

	lv = lv - 1; /* adjust the value to 0-based index */

	xp_awk_str_clear (&run->inrec.line);

	for (i = 0; i < max; i++)
	{
		if (i > 0)
		{
			if (xp_awk_str_ncat (
				&run->inrec.line, 
				run->global.ofs.ptr, 
				run->global.ofs.len) == (xp_size_t)-1) 
			{
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}
		}

		if (i == lv)
		{
			xp_awk_val_t* tmp;

			run->inrec.flds[i].ptr = 
				XP_AWK_STR_BUF(&run->inrec.line) +
				XP_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = len;

			if (xp_awk_str_ncat (
				&run->inrec.line, str, len) == (xp_size_t)-1)
			{
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}

			tmp = xp_awk_makestrval (run, str,len);
			if (tmp == XP_NULL) 
			{
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}

			if (i < nflds)
				xp_awk_refdownval (run, run->inrec.flds[i].val);
			else run->inrec.nflds++;

			run->inrec.flds[i].val = tmp;
			xp_awk_refupval (tmp);
		}
		else if (i >= nflds)
		{
			run->inrec.flds[i].ptr = 
				XP_AWK_STR_BUF(&run->inrec.line) +
				XP_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = 0;

			if (xp_awk_str_cat (
				&run->inrec.line, XP_T("")) == (xp_size_t)-1)
			{
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}

			/* xp_awk_refdownval should not be called over 
			 * run->inrec.flds[i].val as it is not initialized
			 * to any valid values */
			/*xp_awk_refdownval (run, run->inrec.flds[i].val);*/
			run->inrec.flds[i].val = xp_awk_val_zls;
			xp_awk_refupval (xp_awk_val_zls);
			run->inrec.nflds++;
		}
		else
		{
			xp_awk_val_str_t* tmp;

			tmp = (xp_awk_val_str_t*)run->inrec.flds[i].val;

			run->inrec.flds[i].ptr = 
				XP_AWK_STR_BUF(&run->inrec.line) +
				XP_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = tmp->len;

			if (xp_awk_str_ncat (&run->inrec.line, 
				tmp->buf, tmp->len) == (xp_size_t)-1)
			{
				run->errnum = XP_AWK_ENOMEM;
				return -1;
			}
		}
	}

	v = xp_awk_getglobal (run, XP_AWK_GLOBAL_NF);
	xp_assert (v->type == XP_AWK_VAL_INT);
	if (((xp_awk_val_int_t*)v)->val != max)
	{
		v = xp_awk_makeintval (run, (xp_long_t)max);
		if (v == XP_NULL) 
		{
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		if (xp_awk_setglobal (
			run, XP_AWK_GLOBAL_NF, v) == -1) return -1;
	}

	return 0;
}

