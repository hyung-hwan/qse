/*
 * $Id: rec.c,v 1.8 2006-11-29 02:54:16 bacon Exp $
 */

#include <ase/awk/awk_i.h>

static int __split_record (ase_awk_run_t* run);
static int __recomp_record_fields (
	ase_awk_run_t* run, ase_size_t lv, 
	const ase_char_t* str, ase_size_t len);

int ase_awk_setrec (
	ase_awk_run_t* run, ase_size_t idx, const ase_char_t* str, ase_size_t len)
{
	ase_awk_val_t* v;
	int errnum;

	if (idx == 0)
	{
		if (str == ASE_AWK_STR_BUF(&run->inrec.line) &&
		    len == ASE_AWK_STR_LEN(&run->inrec.line))
		{
			if (ase_awk_clrrec (run, ase_true) == -1) return -1;
		}
		else
		{
			if (ase_awk_clrrec (run, ase_false) == -1) return -1;

			if (ase_awk_str_ncpy (&run->inrec.line, str, len) == (ase_size_t)-1)
			{
				ase_awk_clrrec (run, ase_false);
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}
		}

		v = ase_awk_makestrval (run, str, len);
		if (v == ASE_NULL)
		{
			ase_awk_clrrec (run, ase_false);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		ASE_AWK_ASSERT (run->awk, run->inrec.d0->type == ASE_AWK_VAL_NIL);
		/* d0 should be cleared before the next line is reached
		 * as it doesn't call ase_awk_refdownval on run->inrec.d0 */
		run->inrec.d0 = v;
		ase_awk_refupval (run, v);

		if (__split_record (run) == -1) 
		{
			errnum = run->errnum;
			ase_awk_clrrec (run, ase_false);
			run->errnum = errnum;
			return -1;
		}
	}
	else
	{
		if (__recomp_record_fields (run, idx, str, len) == -1)
		{
			errnum = run->errnum;
			ase_awk_clrrec (run, ase_false);
			run->errnum = errnum;
			return -1;
		}
	
		/* recompose $0 */
		v = ase_awk_makestrval (run,
			ASE_AWK_STR_BUF(&run->inrec.line), 
			ASE_AWK_STR_LEN(&run->inrec.line));
		if (v == ASE_NULL)
		{
			ase_awk_clrrec (run, ase_false);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		ase_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = v;
		ase_awk_refupval (run, v);
	}

	return 0;
}

static int __split_record (ase_awk_run_t* run)
{
	ase_char_t* p, * tok;
	ase_size_t len, tok_len, nflds;
	ase_awk_val_t* v, * fs;
	ase_char_t* fs_ptr, * fs_free;
	ase_size_t fs_len;
	int errnum;
       
	/* inrec should be cleared before __split_record is called */
	ASE_AWK_ASSERT (run->awk, run->inrec.nflds == 0);

	/* get FS */
	fs = ase_awk_getglobal (run, ASE_AWK_GLOBAL_FS);
	if (fs->type == ASE_AWK_VAL_NIL)
	{
		fs_ptr = ASE_T(" ");
		fs_len = 1;
		fs_free = ASE_NULL;
	}
	else if (fs->type == ASE_AWK_VAL_STR)
	{
		fs_ptr = ((ase_awk_val_str_t*)fs)->buf;
		fs_len = ((ase_awk_val_str_t*)fs)->len;
		fs_free = ASE_NULL;
	}
	else 
	{
		fs_ptr = ase_awk_valtostr (
			run, fs, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &fs_len);
		if (fs_ptr == ASE_NULL) return -1;
		fs_free = fs_ptr;
	}

	/* scan the input record to count the fields */
	p = ASE_AWK_STR_BUF(&run->inrec.line);
	len = ASE_AWK_STR_LEN(&run->inrec.line);

	nflds = 0;
	while (p != ASE_NULL)
	{
		if (fs_len <= 1)
		{
			p = ase_awk_strxntok (run,
				p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = ase_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == ASE_NULL && errnum != ASE_AWK_ENOERR)
			{
				if (fs_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, fs_free);
				run->errnum = errnum;
				return -1;
			}
		}

		if (nflds == 0 && p == ASE_NULL && tok_len == 0)
		{
			/* there are no fields. it can just return here
			 * as ase_awk_clrrec has been called before this */
			if (fs_free != ASE_NULL) ASE_AWK_FREE (run->awk, fs_free);
			return 0;
		}

		ASE_AWK_ASSERT (run->awk,
			(tok != ASE_NULL && tok_len > 0) || tok_len == 0);

		nflds++;
		len = ASE_AWK_STR_LEN(&run->inrec.line) - 
			(p - ASE_AWK_STR_BUF(&run->inrec.line));
	}

	/* allocate space */
	if (nflds > run->inrec.maxflds)
	{
		void* tmp = ASE_AWK_MALLOC (
			run->awk, ASE_SIZEOF(*run->inrec.flds) * nflds);
		if (tmp == ASE_NULL) 
		{
			if (fs_free != ASE_NULL) ASE_AWK_FREE (run->awk, fs_free);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		if (run->inrec.flds != ASE_NULL) 
			ASE_AWK_FREE (run->awk, run->inrec.flds);
		run->inrec.flds = tmp;
		run->inrec.maxflds = nflds;
	}

	/* scan again and split it */
	p = ASE_AWK_STR_BUF(&run->inrec.line);
	len = ASE_AWK_STR_LEN(&run->inrec.line);

	while (p != ASE_NULL)
	{
		if (fs_len <= 1)
		{
			p = ase_awk_strxntok (
				run, p, len, fs_ptr, fs_len, &tok, &tok_len);
		}
		else
		{
			p = ase_awk_strxntokbyrex (run, p, len, 
				run->global.fs, &tok, &tok_len, &errnum); 
			if (p == ASE_NULL && errnum != ASE_AWK_ENOERR)
			{
				if (fs_free != ASE_NULL) 
					ASE_AWK_FREE (run->awk, fs_free);
				run->errnum = errnum;
				return -1;
			}
		}

		ASE_AWK_ASSERT (run->awk,
			(tok != ASE_NULL && tok_len > 0) || tok_len == 0);

		run->inrec.flds[run->inrec.nflds].ptr = tok;
		run->inrec.flds[run->inrec.nflds].len = tok_len;
		run->inrec.flds[run->inrec.nflds].val = 
			ase_awk_makestrval (run, tok, tok_len);

		if (run->inrec.flds[run->inrec.nflds].val == ASE_NULL)
		{
			if (fs_free != ASE_NULL) ASE_AWK_FREE (run->awk, fs_free);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		ase_awk_refupval (run, run->inrec.flds[run->inrec.nflds].val);
		run->inrec.nflds++;

		len = ASE_AWK_STR_LEN(&run->inrec.line) - 
			(p - ASE_AWK_STR_BUF(&run->inrec.line));
	}

	if (fs_free != ASE_NULL) ASE_AWK_FREE (run->awk, fs_free);

	/* set the number of fields */
	v = ase_awk_makeintval (run, (ase_long_t)nflds);
	if (v == ASE_NULL) 
	{
		run->errnum = ASE_AWK_ENOMEM;
		return -1;
	}

	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_NF, v) == -1) return -1;

	ASE_AWK_ASSERT (run->awk, nflds == run->inrec.nflds);
	return 0;
}

int ase_awk_clrrec (ase_awk_run_t* run, ase_bool_t skip_inrec_line)
{
	ase_size_t i;
	int n = 0;

	if (run->inrec.d0 != ase_awk_val_nil)
	{
		ase_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = ase_awk_val_nil;
	}

	if (run->inrec.nflds > 0)
	{
		ASE_AWK_ASSERT (run->awk, run->inrec.flds != ASE_NULL);

		for (i = 0; i < run->inrec.nflds; i++) 
		{
			ASE_AWK_ASSERT (run->awk,
				run->inrec.flds[i].val != ASE_NULL);
			ase_awk_refdownval (run, run->inrec.flds[i].val);
		}
		run->inrec.nflds = 0;

		if (ase_awk_setglobal (
			run, ASE_AWK_GLOBAL_NF, ase_awk_val_zero) == -1)
		{
			/* first of all, this should never happen. 
			 * if it happened, it would return an error
			 * after all the clearance tasks */
			n = -1;
		}
	}

	ASE_AWK_ASSERT (run->awk, run->inrec.nflds == 0);
	if (!skip_inrec_line) ase_awk_str_clear (&run->inrec.line);

	return n;
}

static int __recomp_record_fields (
	ase_awk_run_t* run, ase_size_t lv, 
	const ase_char_t* str, ase_size_t len)
{
	ase_awk_val_t* v;
	ase_size_t max, i, nflds;

	/* recomposes the record and the fields when $N has been assigned 
	 * a new value and recomputes NF accordingly */

	ASE_AWK_ASSERT (run->awk, lv > 0);
	max = (lv > run->inrec.nflds)? lv: run->inrec.nflds;

	nflds = run->inrec.nflds;
	if (max > run->inrec.maxflds)
	{
		void* tmp;

		/* if the given field number is greater than the maximum
		 * number of fields that the current record can hold,
		 * the field spaces are resized */

		if (run->awk->syscas.realloc != ASE_NULL)
		{
			tmp = ASE_AWK_REALLOC (
				run->awk, run->inrec.flds, 
				ASE_SIZEOF(*run->inrec.flds) * max);
			if (tmp == ASE_NULL) 
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}
		}
		else
		{
			tmp = ASE_AWK_MALLOC (
				run->awk, ASE_SIZEOF(*run->inrec.flds) * max);
			if (tmp == ASE_NULL)
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}
			if (run->inrec.flds != ASE_NULL)
			{
				ASE_AWK_MEMCPY (run->awk, tmp, run->inrec.flds, 
					ASE_SIZEOF(*run->inrec.flds) * run->inrec.maxflds);
				ASE_AWK_FREE (run->awk, run->inrec.flds);
			}
		}

		run->inrec.flds = tmp;
		run->inrec.maxflds = max;
	}

	lv = lv - 1; /* adjust the value to 0-based index */

	ase_awk_str_clear (&run->inrec.line);

	for (i = 0; i < max; i++)
	{
		if (i > 0)
		{
			if (ase_awk_str_ncat (
				&run->inrec.line, 
				run->global.ofs.ptr, 
				run->global.ofs.len) == (ase_size_t)-1) 
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}
		}

		if (i == lv)
		{
			ase_awk_val_t* tmp;

			run->inrec.flds[i].ptr = 
				ASE_AWK_STR_BUF(&run->inrec.line) +
				ASE_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = len;

			if (ase_awk_str_ncat (
				&run->inrec.line, str, len) == (ase_size_t)-1)
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}

			tmp = ase_awk_makestrval (run, str,len);
			if (tmp == ASE_NULL) 
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}

			if (i < nflds)
				ase_awk_refdownval (run, run->inrec.flds[i].val);
			else run->inrec.nflds++;

			run->inrec.flds[i].val = tmp;
			ase_awk_refupval (run, tmp);
		}
		else if (i >= nflds)
		{
			run->inrec.flds[i].ptr = 
				ASE_AWK_STR_BUF(&run->inrec.line) +
				ASE_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = 0;

			if (ase_awk_str_cat (
				&run->inrec.line, ASE_T("")) == (ase_size_t)-1)
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}

			/* ase_awk_refdownval should not be called over 
			 * run->inrec.flds[i].val as it is not initialized
			 * to any valid values */
			/*ase_awk_refdownval (run, run->inrec.flds[i].val);*/
			run->inrec.flds[i].val = ase_awk_val_zls;
			ase_awk_refupval (run, ase_awk_val_zls);
			run->inrec.nflds++;
		}
		else
		{
			ase_awk_val_str_t* tmp;

			tmp = (ase_awk_val_str_t*)run->inrec.flds[i].val;

			run->inrec.flds[i].ptr = 
				ASE_AWK_STR_BUF(&run->inrec.line) +
				ASE_AWK_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = tmp->len;

			if (ase_awk_str_ncat (&run->inrec.line, 
				tmp->buf, tmp->len) == (ase_size_t)-1)
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}
		}
	}

	v = ase_awk_getglobal (run, ASE_AWK_GLOBAL_NF);
	ASE_AWK_ASSERT (run->awk, v->type == ASE_AWK_VAL_INT);
	if (((ase_awk_val_int_t*)v)->val != max)
	{
		v = ase_awk_makeintval (run, (ase_long_t)max);
		if (v == ASE_NULL) 
		{
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		if (ase_awk_setglobal (
			run, ASE_AWK_GLOBAL_NF, v) == -1) return -1;
	}

	return 0;
}

