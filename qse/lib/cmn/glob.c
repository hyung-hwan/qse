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

#include <qse/cmn/glob.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include "mem.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define SEPC QSE_T('\\')
#else
#	define SEPC QSE_T('/')
#endif

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define IS_SEP(c) ((c) == QSE_T('/') || (c) == QSE_T('\\'))
#else
#	define IS_SEP(c) ((c) == QSE_T('/'))
#endif

#define IS_NIL(c) ((c) == QSE_T('\0'))
#define IS_SEP_OR_NIL(c) (IS_SEP(c) || IS_NIL(c))

#define IS_WILD(c) ((c) == QSE_T('*') || (c) == QSE_T('?') || (c) == QSE_T('['))

#define GLOB_MULTI  QSE_T('*')
#define GLOB_SINGLE QSE_T('?')
#define GLOB_RANGE  QSE_T("[]")
#define GLOB_NEGATE QSE_T("^!")

#define string_has_globs(ptr) \
	(qse_strpbrk (ptr, QSE_T("*?[")) != QSE_NULL)
#define string_has_globs2(ptr,len) \
	(qse_strxpbrk (ptr, len, QSE_T("*?[")) != QSE_NULL)


struct glob_t
{
	qse_mmgr_t* mmgr;
	qse_cmgr_t* cmgr;
	qse_str_t path;
	int depth;
};

typedef struct glob_t glob_t;

static int record_a_match (const qse_char_t* x)
{
wprintf (L"MATCH => [%.*S]\n", (int)qse_strlen(x), x);
return 0;
}

static int path_exists (glob_t* g, const qse_char_t* name)
{
	struct stat st;
	int x;

#if defined(QSE_CHAR_IS_MCHAR)

	x =  lstat (name, &st);

#else
	qse_mchar_t* ptr;

	ptr = qse_wcstombsdup (name, g->mmgr);
	if (ptr == QSE_NULL) return -1;

	x = lstat (ptr, &st);
	QSE_MMGR_FREE (g->mmgr, ptr);

#endif
	return (x == 0)? 1: 0;
}

static int path_is_directory (glob_t* g, const qse_char_t* name)
{
	struct stat st;
	int x;

#if defined(QSE_CHAR_IS_MCHAR)

	x =  lstat (name, &st);

#else
	qse_mchar_t* ptr;

	ptr = qse_wcstombsdup (name, g->mmgr);
	if (ptr == QSE_NULL) return -1;

	x = lstat (ptr, &st);
	QSE_MMGR_FREE (g->mmgr, ptr);

#endif
	return (x == 0 && S_ISDIR(st.st_mode))? 1: 0;
}

struct segment_t
{
	enum
	{
		NONE,
		ROOT,
		NORMAL
	} type;

	const qse_char_t* ptr;
	qse_size_t        len;
	qse_char_t        sep; /* preceeding separator */
	unsigned int wild: 1;
	unsigned int next: 1;
};

typedef struct segment_t segment_t;

static int get_next_segment (segment_t* seg)
{
	if (seg->type == NONE)
	{
		/* seg->ptr must point to the beginning of the pattern
		 * and seg->len must be zero when seg->type is NONE. */
		if (IS_NIL(seg->ptr[0]))
		{
			/* nothing to do */
		}
		else if (IS_SEP(seg->ptr[0]))
		{
			seg->type = ROOT;
			seg->len = 1;
			seg->next = IS_NIL(seg->ptr[1])? 0: 1;
		}
		else
		{
			seg->type = NORMAL;
			seg->sep = QSE_T('\0');
			seg->wild = 0;
			do
			{
				if (IS_WILD(seg->ptr[seg->len])) seg->wild = 1;
				seg->len++;
			}
			while (!IS_SEP_OR_NIL(seg->ptr[seg->len]));
			seg->next = IS_NIL(seg->ptr[seg->len])? 0: 1;
		}
	}
	else if (seg->type == ROOT)
	{
		seg->type = NORMAL;
		seg->ptr = &seg->ptr[seg->len];
		seg->len = 0;
		seg->sep = QSE_T('\0');
		seg->wild = 0;
		while (!IS_SEP_OR_NIL(seg->ptr[seg->len])) 
		{
			if (IS_WILD(seg->ptr[seg->len])) seg->wild = 1;
			seg->len++;
		}
		seg->next = IS_NIL(seg->ptr[seg->len])? 0: 1;
	}
	else
	{
		QSE_ASSERT (seg->type == NORMAL);

		seg->ptr = &seg->ptr[seg->len + 1];
		seg->len = 0;
		seg->wild = 0;
		if (IS_NIL(seg->ptr[-1])) 
		{
			seg->type = NONE;
			seg->next = 0;
		}
		else
		{
			seg->sep = seg->ptr[-1];
			while (!IS_SEP_OR_NIL(seg->ptr[seg->len])) 
			{
				if (IS_WILD(seg->ptr[seg->len])) seg->wild = 1;
				seg->len++;
			}
			seg->next = IS_NIL(seg->ptr[seg->len])? 0: 1;
		}
	}

	return seg->type;
}

DIR* xopendir (glob_t* g, const qse_char_t* path)
{
	if (path[0] == QSE_T('\0')) path = QSE_T(".");

#if defined(QSE_CHAR_IS_MCHAR)
	return opendir (path)
#else
	DIR* dp;
	qse_mchar_t* mptr;

	mptr = qse_wcstombsdup (path, g->mmgr);
	if (mptr == QSE_NULL) return QSE_NULL;

	dp = opendir (mptr);

	QSE_MMGR_FREE (g->mmgr, mptr);

	return dp;
#endif
}

int xreaddir (glob_t* g, DIR* dp, qse_str_t* path)
{
	struct dirent* de;
#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing */
#else
	qse_size_t ml, wl, tmp;
#endif

read_more:
	de = readdir (dp);
	if (de == NULL) return 0;

	if (qse_mbscmp (de->d_name, QSE_MT(".")) == 0 ||
	    qse_mbscmp (de->d_name, QSE_MT("..")) == 0) goto read_more;

#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_str_cat (path, de->d_name) == (qse_size_t)-1) return -1;
#else
	tmp = QSE_STR_LEN(path);
	if (qse_mbstowcswithcmgr (de->d_name, &ml, QSE_NULL, &wl, g->cmgr) <= -1 ||
	    qse_str_setlen (path, tmp + wl) == (qse_size_t)-1) return -1;
	qse_mbstowcswithcmgr (de->d_name, &ml, QSE_STR_CPTR(&g->path,tmp), &wl, g->cmgr);
#endif	

	return 1;
}

static int search (glob_t* g, segment_t* seg)
{
	segment_t save = *seg;
	g->depth++;
//wprintf (L"CALL DEPTH = %d\n", (int)g->depth);

	while (get_next_segment(seg) != NONE)
	{
		QSE_ASSERT (seg->type != NONE);

		if (seg->wild)
		{
			DIR* dp;

//wprintf (L"OPENDING %.*S\n", (int)QSE_STR_LEN(&g->path), QSE_STR_PTR(&g->path));
			dp = xopendir (g, QSE_STR_PTR(&g->path));
			if (dp)
			{
				qse_size_t tmp, tmp2;

				tmp = QSE_STR_LEN(&g->path);

				if (seg->sep && qse_str_ccat (&g->path, seg->sep) == (qse_size_t)-1) return -1;
				tmp2 = QSE_STR_LEN(&g->path);

				while (1)
				{
					qse_str_setlen (&g->path, tmp2);

					if (xreaddir (g, dp, &g->path) <= 0) break;

					if (seg->next)
					{
						if (qse_strnfnmat (QSE_STR_CPTR(&g->path,tmp2), seg->ptr, seg->len, 0) > 0 &&
						    search (g, seg) <= -1) return -1;
					}
					else
					{
//wprintf (L"CHECKING %S [%.*S]\n", QSE_STR_CPTR(&g->path,tmp2), (int)seg->len, seg->ptr);
						if (qse_strnfnmat (QSE_STR_CPTR(&g->path,tmp2), seg->ptr, seg->len, 0) > 0)
						{
							record_a_match (QSE_STR_PTR(&g->path));
						}
					}
				}

				qse_str_setlen (&g->path, tmp); /* TODO: error check */
				closedir (dp);
//wprintf (L"CLOSED %S\n", QSE_STR_PTR(&g->path));
			}

			break;
		}
		else
		{
			if ((seg->sep && qse_str_ccat (&g->path, seg->sep) == (qse_size_t)-1) ||
			    qse_str_ncat (&g->path, seg->ptr, seg->len) == (qse_size_t)-1) return -1;

//wprintf (L">> [%.*S]\n", (int)QSE_STR_LEN(&g->path), QSE_STR_PTR(&g->path));
			if (!seg->next && path_exists(g, QSE_STR_PTR(&g->path)))
			{
				record_a_match (QSE_STR_PTR(&g->path));
			}
		}
	}

	*seg = save;
	g->depth--;
	return 0;
}

int qse_globwithcmgr (const qse_char_t* pattern, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	segment_t seg;
	glob_t g;
	int x;

	QSE_MEMSET (&g, 0, QSE_SIZEOF(g));
	g.mmgr = mmgr;
	g.cmgr = cmgr;

	if (qse_str_init (&g.path, mmgr, 512) <= -1) return -1;

	QSE_MEMSET (&seg, 0, QSE_SIZEOF(seg));
	seg.type = NONE;
	seg.ptr = pattern;
	seg.len = 0;

	x = search (&g, &seg);

	qse_str_fini (&g.path);
	return x;
}

int qse_glob (const qse_char_t* pattern, qse_mmgr_t* mmgr)
{
	return qse_globwithcmgr (pattern, mmgr, qse_getdflcmgr());
}

