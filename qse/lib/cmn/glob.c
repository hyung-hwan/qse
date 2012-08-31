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
#include <qse/cmn/path.h>
#include "mem.h"

#if defined(_WIN32) 
#	include <windows.h>
#elif defined(__OS2__) 
#	define INCL_DOSFILEMGR
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__) 
	/* what? */
#else
#	include <dirent.h>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

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

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* i don't support escaping in these systems */
#	define IS_ESC(c) (0)
#else
#	define IS_ESC(c) ((c) == QSE_T('\\'))
#endif

/* this macro only checks for top-level wild-cards among these.
 *  *, ?, [], !, -  
 * see str-fnmat.c for more wild-card letters
 */
#define IS_WILD(c) ((c) == QSE_T('*') || (c) == QSE_T('?') || (c) == QSE_T('['))

struct glob_t
{
	qse_glob_cbfun_t cbfun;
	void* cbctx;

	qse_mmgr_t* mmgr;
	qse_cmgr_t* cmgr;

	qse_str_t path;
	qse_str_t segtmp;

	int expanded;
	int fnmat_flags;
	int depth;
};

typedef struct glob_t glob_t;

static int path_exists (glob_t* g, const qse_char_t* name)
{
#if defined(_WIN32)

	return (GetFileAttributes(name) != INVALID_FILE_ATTRIBUTES)? 1: 0;

#elif defined(__OS2__)

	return -1;

#elif defined(__DOS__)

	return -1;

#else

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

#endif
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
	unsigned int wild: 1;  /* indicate that it contains wildcards */
	unsigned int esc: 1;  /* indicate that it contains escaped letters */
	unsigned int next: 1;  /* indicate that it has the following segment */
};

typedef struct segment_t segment_t;

static int get_next_segment (glob_t* g, segment_t* seg)
{
/* TODO: WIN32 X: drive letter segment... */

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
			seg->sep = QSE_T('\0');
			seg->wild = 0;
			seg->esc = 0;
		}
		else
		{
			int escaped = 0;
			seg->type = NORMAL;
			seg->sep = QSE_T('\0');
			seg->wild = 0;
			seg->esc = 0;
			do
			{
				if (escaped) escaped = 0;
				else
				{
					if (IS_ESC(seg->ptr[seg->len])) 
					{
						escaped = 1;
						seg->esc = 1;
					}
					else if (IS_WILD(seg->ptr[seg->len])) seg->wild = 1;
				}

				seg->len++;
			}
			while (!IS_SEP_OR_NIL(seg->ptr[seg->len]));
			seg->next = IS_NIL(seg->ptr[seg->len])? 0: 1;
		}
	}
	else if (seg->type == ROOT)
	{
		int escaped = 0;
		seg->type = NORMAL;
		seg->ptr = &seg->ptr[seg->len];
		seg->len = 0;
		seg->sep = QSE_T('\0');
		seg->wild = 0;
		seg->esc = 0;

		while (!IS_SEP_OR_NIL(seg->ptr[seg->len])) 
		{
			if (escaped) escaped = 0;
			else
			{
				if (IS_ESC(seg->ptr[seg->len])) 
				{
					escaped = 1;
					seg->esc = 1;
				}
				else if (IS_WILD(seg->ptr[seg->len])) seg->wild = 1;
			}
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
		seg->esc = 0;
		if (IS_NIL(seg->ptr[-1])) 
		{
			seg->type = NONE;
			seg->next = 0;
			seg->sep = QSE_T('\0');
		}
		else
		{
			int escaped = 0;
			seg->sep = seg->ptr[-1];
			while (!IS_SEP_OR_NIL(seg->ptr[seg->len])) 
			{
				if (escaped) escaped = 0;
				else
				{
					if (IS_ESC(seg->ptr[seg->len])) 
					{
						escaped = 1;
						seg->esc = 1;
					}
					else if (IS_WILD(seg->ptr[seg->len])) seg->wild = 1;
				}
				seg->len++;
			}
			seg->next = IS_NIL(seg->ptr[seg->len])? 0: 1;
		}
	}

	return seg->type;
}

#if defined(_WIN32)

struct DIR
{
	HANDLE h;
	WIN32_FIND_DATA wfd;	
};
typedef struct DIR DIR;

#elif defined(__OS2__)

struct DIR
{
	HDIR h;
	FILEFINDBUF3L ffb;	
	ULONG count;
};
typedef struct DIR DIR;

#elif defined(__DOS__)
struct DIR
{
	int xxx;
};
typedef struct DIR DIR;

#endif

static DIR* xopendir (glob_t* g, const qse_cstr_t* path)
{
#if defined(_WIN32)

	/* ------------------------------------------------------------------- */
	DIR* dp;

	dp = QSE_MMGR_ALLOC (g->mmgr, QSE_SIZEOF(*dp));
	if (dp == QSE_NULL) return QSE_NULL;

	if (path->len <= 0)
	{
		if (qse_str_cpy (&g->segtmp, QSE_T("*")) == (qse_size_t)-1)
		{
			QSE_MMGR_FREE (g->mmgr, dp);
			return QSE_NULL;
		}
	}
	else
	{
		if (qse_str_cpy (&g->segtmp, path->ptr) == (qse_size_t)-1 ||
		    (!IS_SEP(path->ptr[path->len-1]) && 
		     !qse_isdrivecurpath(path->ptr) &&
		     qse_str_ccat (&g->segtmp, QSE_T('\\')) == (qse_size_t)-1) ||
		    qse_str_ccat (&g->segtmp, QSE_T('*')) == (qse_size_t)-1)
		{
			QSE_MMGR_FREE (g->mmgr, dp);
			return QSE_NULL;
		}
	}

	dp->h = FindFirstFile (QSE_STR_PTR(&g->segtmp), &dp->wfd);
	if (dp->h == INVALID_HANDLE_VALUE) 
	{
		QSE_MMGR_FREE (g->mmgr, dp);
		return QSE_NULL;
	}

	return dp;
	/* ------------------------------------------------------------------- */

#elif defined(__OS2__)

	/* ------------------------------------------------------------------- */
	DIR* dp;
	APIRET rc;
	qse_mchar_t* mptr;

	dp = QSE_MMGR_ALLOC (g->mmgr, QSE_SIZEOF(*dp));
	if (dp == QSE_NULL) return QSE_NULL;

	if (path->len <= 0)
	{
		if (qse_str_cpy (&g->segtmp, QSE_T("*.*")) == (qse_size_t)-1)
		{
			QSE_MMGR_FREE (g->mmgr, dp);
			return QSE_NULL;
		}
	}
	else
	{
		if (qse_str_cpy (&g->segtmp, path->ptr) == (qse_size_t)-1 ||
		    (!IS_SEP(path->ptr[path->len-1]) && 
		     !qse_isdrivecurpath(path->ptr) &&
		     qse_str_ccat (&g->segtmp, QSE_T('\\')) == (qse_size_t)-1) ||
		    qse_str_cat (&g->segtmp, QSE_T("*.*")) == (qse_size_t)-1)
		{
			QSE_MMGR_FREE (g->mmgr, dp);
			return QSE_NULL;
		}
	}

	dp->h = HDIR_CREATE;
	dp->count = 1;

#if defined(QSE_CHAR_IS_MCHAR)
	mptr = QSE_STR_PTR(&g->segtmp);
#else
	mptr = qse_wcstombsdup (QSE_STR_PTR(&g->segtmp), g->mmgr);
	if (mptr == QSE_NULL)
	{
		QSE_MMGR_FREE (g->mmgr, dp);
		return QSE_NULL;
	}
#endif

	rc = DosFindFirst (
		mptr,
		&dp->h, 
		FILE_DIRECTORY | FILE_READONLY,
		&dp->ffb,
		QSE_SIZEOF(dp->ffb),
		&dp->count,
		FIL_STANDARDL);
#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing to do */
#else
	QSE_MMGR_FREE (g->mmgr, mptr);
#endif
	if (rc != NO_ERROR)
	{
		QSE_MMGR_FREE (g->mmgr, dp);
		return QSE_NULL;
	}

	return dp;
	/* ------------------------------------------------------------------- */

#elif defined(__DOS__)

	/* ------------------------------------------------------------------- */
	return QSE_NULL;
	/* ------------------------------------------------------------------- */

#else

	/* ------------------------------------------------------------------- */

#if defined(QSE_CHAR_IS_MCHAR)
	return opendir ((path->len <= 0)? p = QSE_T("."): path->ptr);
#else
	if (path->len <= 0)
	{
		return opendir (QSE_MT("."));
	}
	else
	{
		DIR* dp;
		qse_mchar_t* mptr;
	
		mptr = qse_wcstombsdup (path->ptr, g->mmgr);
		if (mptr == QSE_NULL) return QSE_NULL;

		dp = opendir (mptr);

		QSE_MMGR_FREE (g->mmgr, mptr);

		return dp;
	}
#endif 
	/* ------------------------------------------------------------------- */

#endif
}

static int xreaddir (glob_t* g, DIR* dp, qse_str_t* path)
{
#if defined(_WIN32)

	/* ------------------------------------------------------------------- */
	if (qse_str_cat (path, dp->wfd.cFileName) == (qse_size_t)-1) return -1;

	if (FindNextFile (dp->h, &dp->wfd) == FALSE) 
		return (GetLastError() == ERROR_NO_MORE_FILES)? 0: -1;

	return 1;
	/* ------------------------------------------------------------------- */

#elif defined(__OS2__)

	/* ------------------------------------------------------------------- */
	APIRET rc;
#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing */
#else
	qse_size_t ml, wl, tmp;
#endif

	if (dp->count <= 0) return 0;

#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_str_cat (path, dp->ffb.achName) == (qse_size_t)-1) return -1;
#else
	tmp = QSE_STR_LEN(path);
	if (qse_mbstowcswithcmgr (dp->ffb.achName, &ml, QSE_NULL, &wl, g->cmgr) <= -1 ||
	    qse_str_setlen (path, tmp + wl) == (qse_size_t)-1) return -1;
	qse_mbstowcswithcmgr (dp->ffb.achName, &ml, QSE_STR_CPTR(&g->path,tmp), &wl, g->cmgr);
#endif

	rc = DosFindNext (dp->h, &dp->ffb, QSE_SIZEOF(dp->ffb), &dp->count);
	if (rc != NO_ERROR) return -1;

	return 1;
	/* ------------------------------------------------------------------- */

#elif defined(__DOS__)

	/* ------------------------------------------------------------------- */
	return -1;
	/* ------------------------------------------------------------------- */

#else

	/* ------------------------------------------------------------------- */
	struct dirent* de;
#if defined(QSE_CHAR_IS_MCHAR)
	/* nothing */
#else
	qse_size_t ml, wl, tmp;
#endif

read_more:
	de = readdir (dp);
	if (de == NULL) return 0;

	/*
	if (qse_mbscmp (de->d_name, QSE_MT(".")) == 0 ||
	    qse_mbscmp (de->d_name, QSE_MT("..")) == 0) goto read_more;
	*/

#if defined(QSE_CHAR_IS_MCHAR)
	if (qse_str_cat (path, de->d_name) == (qse_size_t)-1) return -1;
#else
	tmp = QSE_STR_LEN(path);
	if (qse_mbstowcswithcmgr (de->d_name, &ml, QSE_NULL, &wl, g->cmgr) <= -1 ||
	    qse_str_setlen (path, tmp + wl) == (qse_size_t)-1) return -1;
	qse_mbstowcswithcmgr (de->d_name, &ml, QSE_STR_CPTR(&g->path,tmp), &wl, g->cmgr);
#endif	

	return 1;
	/* ------------------------------------------------------------------- */

#endif
}

static void xclosedir (glob_t* g, DIR* dp)
{
#if defined(_WIN32)
	FindClose (dp->h);
	QSE_MMGR_FREE (g->mmgr, dp);
#elif defined(__OS2__)
	DosFindClose (dp->h);
	QSE_MMGR_FREE (g->mmgr, dp);
#elif defined(__DOS__)
	QSE_MMGR_FREE (g->mmgr, dp);
#else
	closedir (dp);
#endif
}

static int search (glob_t* g, segment_t* seg)
{
	segment_t save = *seg;
	g->depth++;

	while (get_next_segment(g, seg) != NONE)
	{
		QSE_ASSERT (seg->type != NONE);

		if (seg->wild)
		{
			DIR* dp;

			dp = xopendir (g, QSE_STR_CSTR(&g->path));
			if (dp)
			{
				qse_size_t tmp, tmp2;

				tmp = QSE_STR_LEN(&g->path);

				if (seg->sep && qse_str_ccat (&g->path, seg->sep) == (qse_size_t)-1) 
				{
					xclosedir (g, dp);
					return -1;
				}
				tmp2 = QSE_STR_LEN(&g->path);

				while (1)
				{
					qse_str_setlen (&g->path, tmp2);

					if (xreaddir (g, dp, &g->path) <= 0) break;

					if (seg->next)
					{
						if (qse_strnfnmat (QSE_STR_CPTR(&g->path,tmp2), seg->ptr, seg->len, g->fnmat_flags) > 0 &&
						    search (g, seg) <= -1) 
						{
							xclosedir (g, dp);
							return -1;
						}
					}
					else
					{
						if (qse_strnfnmat (QSE_STR_CPTR(&g->path,tmp2), seg->ptr, seg->len, g->fnmat_flags) > 0)
						{
							if (g->cbfun (QSE_STR_CSTR(&g->path), g->cbctx) <= -1) 
							{
								xclosedir (g, dp);
								return -1;
							}
							g->expanded = 1;
						}
					}
				}

				qse_str_setlen (&g->path, tmp);
				xclosedir (g, dp);
			}

			break;
		}
		else
		{
			if (seg->sep && qse_str_ccat (&g->path, seg->sep) == (qse_size_t)-1) return -1;

			if (seg->esc)
			{
				/* if the segment contains escape sequences,
				 * strip the escape letters off the segment */

				qse_xstr_t tmp;
				qse_size_t i;
				int escaped = 0;

				if (QSE_STR_CAPA(&g->segtmp) < seg->len &&
				    qse_str_setcapa (&g->segtmp, seg->len) == (qse_size_t)-1) return -1;

				tmp.ptr = QSE_STR_PTR(&g->segtmp);
				tmp.len = 0;

				/* the following loop drops the last character 
				 * if it is the escape character */
				for (i = 0; i < seg->len; i++)
				{
					if (escaped)
					{
						escaped = 0;
						tmp.ptr[tmp.len++] = seg->ptr[i];
					}
					else
					{
						if (IS_ESC(seg->ptr[i])) 
							escaped = 1;
						else
							tmp.ptr[tmp.len++] = seg->ptr[i];
					}
				}

				if (qse_str_ncat (&g->path, tmp.ptr, tmp.len) == (qse_size_t)-1) return -1;
			}
			else
			{
				if (qse_str_ncat (&g->path, seg->ptr, seg->len) == (qse_size_t)-1) return -1;
			}

			if (!seg->next && path_exists(g, QSE_STR_PTR(&g->path)))
			{
				if (g->cbfun (QSE_STR_CSTR(&g->path), g->cbctx) <= -1) return -1;
				g->expanded = 1;
			}
		}
	}

	*seg = save;
	g->depth--;
	return 0;
}

int qse_globwithcmgr (const qse_char_t* pattern, qse_glob_cbfun_t cbfun, void* cbctx, int flags, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	segment_t seg;
	glob_t g;
	int x;

	QSE_MEMSET (&g, 0, QSE_SIZEOF(g));
	g.cbfun = cbfun;
	g.cbctx = cbctx;
	g.mmgr = mmgr;
	g.cmgr = cmgr;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	g.fnmat_flags |= QSE_STRFNMAT_NOESCAPE | QSE_STRFNMAT_IGNORECASE;
#else
	if (flags & QSE_GLOB_NOESCAPE) g.fnmat_flags |= QSE_STRFNMAT_NOESCAPE;
#endif
	if (flags & QSE_GLOB_PERIOD) g.fnmat_flags |= QSE_STRFNMAT_PERIOD;

	if (qse_str_init (&g.path, mmgr, 512) <= -1) return -1;
	if (qse_str_init (&g.segtmp, mmgr, 256) <= -1) 
	{
		qse_str_fini (&g.path);
		return -1;
	}

	QSE_MEMSET (&seg, 0, QSE_SIZEOF(seg));
	seg.type = NONE;
	seg.ptr = pattern;
	seg.len = 0;

	x = search (&g, &seg);

	qse_str_fini (&g.segtmp);
	qse_str_fini (&g.path);

	if (x <= -1) return -1;
	return g.expanded;
}

int qse_glob (const qse_char_t* pattern, qse_glob_cbfun_t cbfun, void* cbctx, int flags, qse_mmgr_t* mmgr)
{
	return qse_globwithcmgr (pattern, cbfun, cbctx, flags, mmgr, qse_getdflcmgr());
}

