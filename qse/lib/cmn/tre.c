/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#include "tre.h"
#include "tre-compile.h"
#include <qse/cmn/str.h>

qse_tre_t* qse_tre_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_tre_t* tre;

	tre = (qse_tre_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_tre_t) + xtnsize);
	if (tre == QSE_NULL) return QSE_NULL;

	if (qse_tre_init (tre, mmgr) <= -1)
	{
		QSE_MMGR_FREE (mmgr, tre);
		return QSE_NULL;
	}

	QSE_MEMSET (tre + 1, 0, xtnsize);
	return tre;
}

void qse_tre_close (qse_tre_t* tre)
{
	qse_tre_fini (tre);
	QSE_MMGR_FREE (tre->mmgr, tre);
}

int qse_tre_init (qse_tre_t* tre, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (tre, 0, QSE_SIZEOF(*tre));
	tre->mmgr = mmgr;

	return 0;
}

void qse_tre_fini (qse_tre_t* tre)
{
	if (tre->TRE_REGEX_T_FIELD) 
	{
		tre_free (tre);
		tre->TRE_REGEX_T_FIELD = QSE_NULL;
	}
}

qse_mmgr_t* qse_tre_getmmgr (qse_tre_t* tre)
{
	return tre->mmgr;
}

void* qse_tre_getxtn (qse_tre_t* tre)
{
	return QSE_XTN (tre);
}

int qse_tre_compx (
	qse_tre_t* tre, const qse_char_t* regex, qse_size_t n,
	unsigned int* nsubmat, int cflags)
{
	int ret;

	if (tre->TRE_REGEX_T_FIELD) 
	{
		tre_free (tre);
		tre->TRE_REGEX_T_FIELD = QSE_NULL;
	}

	ret = tre_compile (tre, regex, n, cflags);
	if (ret > 0) 
	{
		tre->TRE_REGEX_T_FIELD = QSE_NULL; /* just to make sure */
		tre->errnum = ret;
		return -1;	
	}
	
	if (nsubmat) 
	{
		*nsubmat = ((struct tnfa*)tre->TRE_REGEX_T_FIELD)->num_submatches;
	}
	return 0;
}

int qse_tre_comp (
	qse_tre_t* tre, const qse_char_t* regex,
	unsigned int* nsubmat, int cflags)
{
	return qse_tre_compx (
		tre, regex, (regex? qse_strlen(regex):0), 
		nsubmat, cflags
	);
}

/* Fills the POSIX.2 regmatch_t array according to the TNFA tag and match
   endpoint values. */
void tre_fill_pmatch(size_t nmatch, regmatch_t pmatch[], int cflags,
                const tre_tnfa_t *tnfa, int *tags, int match_eo)
{
	tre_submatch_data_t *submatch_data;
	unsigned int i, j;
	int *parents;

	i = 0;
	if (match_eo >= 0 && !(cflags & REG_NOSUB))
	{
		/* Construct submatch offsets from the tags. */
		DPRINT(("end tag = t%d = %d\n", tnfa->end_tag, match_eo));
		submatch_data = tnfa->submatch_data;
		while (i < tnfa->num_submatches && i < nmatch)
		{
			if (submatch_data[i].so_tag == tnfa->end_tag)
				pmatch[i].rm_so = match_eo;
			else
				pmatch[i].rm_so = tags[submatch_data[i].so_tag];

			if (submatch_data[i].eo_tag == tnfa->end_tag)
				pmatch[i].rm_eo = match_eo;
			else
				pmatch[i].rm_eo = tags[submatch_data[i].eo_tag];

			/* If either of the endpoints were not used, this submatch
			   was not part of the match. */
			if (pmatch[i].rm_so == -1 || pmatch[i].rm_eo == -1)
				pmatch[i].rm_so = pmatch[i].rm_eo = -1;

			DPRINT(("pmatch[%d] = {t%d = %d, t%d = %d}\n", i,
			        submatch_data[i].so_tag, pmatch[i].rm_so,
			        submatch_data[i].eo_tag, pmatch[i].rm_eo));
			i++;
		}
		/* Reset all submatches that are not within all of their parent
			 submatches. */
		i = 0;
		while (i < tnfa->num_submatches && i < nmatch)
		{
			if (pmatch[i].rm_eo == -1)
				assert(pmatch[i].rm_so == -1);
			assert(pmatch[i].rm_so <= pmatch[i].rm_eo);

			parents = submatch_data[i].parents;
			if (parents != QSE_NULL)
				for (j = 0; parents[j] >= 0; j++)
				{
					DPRINT(("pmatch[%d] parent %d\n", i, parents[j]));
					if (pmatch[i].rm_so < pmatch[parents[j]].rm_so
					        || pmatch[i].rm_eo > pmatch[parents[j]].rm_eo)
						pmatch[i].rm_so = pmatch[i].rm_eo = -1;
				}
			i++;
		}
	}

	while (i < nmatch)
	{
		pmatch[i].rm_so = -1;
		pmatch[i].rm_eo = -1;
		i++;
	}
}


/*
  Wrapper functions for POSIX compatible regexp matching.
*/

int tre_have_backrefs(const regex_t *preg)
{
	tre_tnfa_t *tnfa = (void *)preg->TRE_REGEX_T_FIELD;
	return tnfa->have_backrefs;
}

static int tre_match(
	const regex_t* preg, const void *string, qse_size_t len,
	tre_str_type_t type, qse_size_t nmatch, regmatch_t pmatch[],
	int eflags)
{
	tre_tnfa_t *tnfa = (void *)preg->TRE_REGEX_T_FIELD;
	reg_errcode_t status;
	int *tags = QSE_NULL, eo;
	if (tnfa->num_tags > 0 && nmatch > 0)
	{
		tags = xmalloc (preg->mmgr, sizeof(*tags) * tnfa->num_tags);
		if (tags == QSE_NULL) return REG_ESPACE;
	}

	/* Dispatch to the appropriate matcher. */
	if (tnfa->have_backrefs || (eflags & REG_BACKTRACKING_MATCHER))
	{
		/* The regex has back references, use the backtracking matcher. */
		status = tre_tnfa_run_backtrack (
			preg->mmgr, tnfa, string, (int)len, type,
			tags, eflags, &eo);
	}
	else
	{
		/* Exact matching, no back references, use the parallel matcher. */
		status = tre_tnfa_run_parallel (
			preg->mmgr, tnfa, string, (int)len, type,
			tags, eflags, &eo);
	}

	if (status == REG_OK)
		/* A match was found, so fill the submatch registers. */
		tre_fill_pmatch(nmatch, pmatch, tnfa->cflags, tnfa, tags, eo);
	if (tags) xfree (preg->mmgr, tags);
	return status;
}

int qse_tre_execx (
	qse_tre_t* tre, const qse_char_t *str, qse_size_t len,
	regmatch_t* pmatch, qse_size_t nmatch, int eflags)
{
	int ret;

	if (tre->TRE_REGEX_T_FIELD == QSE_NULL)
	{
		/* regular expression is bad as none is compiled yet */
		tre->errnum = QSE_TRE_EBADPAT; 
		return -1;
	}
#ifdef QSE_CHAR_IS_WCHAR
	ret = tre_match (tre, str, len, STR_WIDE, nmatch, pmatch, eflags);
#else
	ret = tre_match (tre, str, len, STR_BYTE, nmatch, pmatch, eflags);
#endif
	if (ret > 0) 
	{
		tre->errnum = ret;
		return -1;	
	}
	
	return 0;
}

int qse_tre_exec (
	qse_tre_t* tre, const qse_char_t* str,
	regmatch_t* pmatch, qse_size_t nmatch, int eflags)
{
	return qse_tre_execx (tre, str, (qse_size_t)-1, pmatch, nmatch, eflags);
}

qse_tre_errnum_t qse_tre_geterrnum (qse_tre_t* tre)
{
	return tre->errnum;
}

const qse_char_t* qse_tre_geterrmsg (qse_tre_t* tre)
{
	static const qse_char_t* errstr[] = 
	{
		QSE_T("no error"),
		QSE_T("no sufficient memory available"),
		QSE_T("no match"),
		QSE_T("invalid regular expression"),
		QSE_T("unknown collating element"),
		QSE_T("unknown character class name"),
		QSE_T("trailing backslash"),
		QSE_T("invalid backreference"),
		QSE_T("bracket imbalance"),
		QSE_T("parenthesis imbalance"),
		QSE_T("brace imbalance"),
		QSE_T("invalid bracket content"),
		QSE_T("invalid use of range operator"),
		QSE_T("invalid use of repetition operators")
	};
	
	return (tre->errnum >= 0 && tre->errnum < QSE_COUNTOF(errstr))?
		errstr[tre->errnum]: QSE_T("unknown error");
}

