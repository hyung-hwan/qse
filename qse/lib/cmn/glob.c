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

#include <qse/cmn/glob.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/path.h>
#include <qse/cmn/dir.h>
#include "mem.h"

#if defined(_WIN32) 
#	include <windows.h>
#elif defined(__OS2__) 
#	define INCL_DOSFILEMGR
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__) 
#	include <dos.h>
#	include <errno.h>
#elif defined(macintosh)
#	include <Files.h>
#else
#	include "syscall.h"
#endif

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* i don't support escaping in these systems */
#	define IS_ESC(c) (0)
#else
#	define IS_ESC(c) ((c) == QSE_T('\\'))
#endif

#define IS_SEP(c) QSE_ISPATHSEP(c)

#define IS_NIL(c) ((c) == QSE_T('\0'))
#define IS_SEP_OR_NIL(c) (IS_SEP(c) || IS_NIL(c))

/* only for win32/os2/dos */
#define IS_DRIVE(s) \
     (((s[0] >= QSE_T('A') && s[0] <= QSE_T('Z')) || \
       (s[0] >= QSE_T('a') && s[0] <= QSE_T('z'))) && \
      s[1] == QSE_T(':'))

/* this macro only checks for top-level wild-cards among these.
 *  *, ?, [], !, -  
 * see str-fnmat.c for more wild-card letters
 */
#define IS_WILD(c) ((c) == QSE_T('*') || (c) == QSE_T('?') || (c) == QSE_T('['))

#define NO_RECURSION 1

#if defined(NO_RECURSION)
typedef struct stack_node_t stack_node_t;
#endif

struct glob_t
{
	qse_glob_cbimpl_t cbimpl;
	void* cbctx;

	qse_mmgr_t* mmgr;
	int flags;

	qse_str_t path;
	qse_str_t tbuf; /* temporary buffer */
#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	/* nothing */
#else
	qse_mbs_t mbuf;
#endif

	int expanded;
	int fnmat_flags;

#if defined(NO_RECURSION)
	stack_node_t* stack;
	stack_node_t* free;
#endif
};

typedef struct glob_t glob_t;

static qse_mchar_t* wcs_to_mbuf (glob_t* g, const qse_wchar_t* wcs, qse_mbs_t* mbs)
{
	qse_size_t ml, wl;

	if (qse_wcstombs (wcs, &wl, QSE_NULL, &ml) <= -1 ||
	    qse_mbs_setlen (mbs, ml) == (qse_size_t)-1) return QSE_NULL;

	qse_wcstombs (wcs, &wl, QSE_MBS_PTR(mbs), &ml);
	return QSE_MBS_PTR(mbs);
}

static int path_exists (glob_t* g, const qse_char_t* name)
{
#if defined(_WIN32)

	/* ------------------------------------------------------------------- */
	#if !defined(INVALID_FILE_ATTRIBUTES)
	#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
	#endif
	return (GetFileAttributes(name) != INVALID_FILE_ATTRIBUTES)? 1: 0;
	/* ------------------------------------------------------------------- */

#elif defined(__OS2__)

	/* ------------------------------------------------------------------- */
	FILESTATUS3 fs;
	APIRET rc;
	const qse_mchar_t* mptr;

#if defined(QSE_CHAR_IS_MCHAR)
	mptr = name;
#else
	mptr = wcs_to_mbuf (g, name, &g->mbuf);
	if (mptr == QSE_NULL) return -1;
#endif

	rc = DosQueryPathInfo (mptr, FIL_STANDARD, &fs, QSE_SIZEOF(fs));

	return (rc == NO_ERROR)? 1:
	       (rc == ERROR_PATH_NOT_FOUND)? 0: -1;
	/* ------------------------------------------------------------------- */

#elif defined(__DOS__)

	/* ------------------------------------------------------------------- */
	unsigned int x, attr;
	const qse_mchar_t* mptr;

#if defined(QSE_CHAR_IS_MCHAR)
	mptr = name;
#else
	mptr = wcs_to_mbuf (g, name, &g->mbuf);
	if (mptr == QSE_NULL) return -1;
#endif

	x = _dos_getfileattr (mptr, &attr);
	return (x == 0)? 1: 
	       (errno == ENOENT)? 0: -1;
	/* ------------------------------------------------------------------- */

#elif defined(macintosh)
	HFileInfo fpb;
	const qse_mchar_t* mptr;

#if defined(QSE_CHAR_IS_MCHAR)
	mptr = name;
#else
	mptr = wcs_to_mbuf (g, name, &g->mbuf);
	if (mptr == QSE_NULL) return -1;
#endif

	QSE_MEMSET (&fpb, 0, QSE_SIZEOF(fpb));
	fpb.ioNamePtr = (unsigned char*)mptr;
	
	return (PBGetCatInfoSync ((CInfoPBRec*)&fpb) == noErr)? 1: 0;
#else

	/* ------------------------------------------------------------------- */
	struct stat st;
	const qse_mchar_t* mptr;

#if defined(QSE_CHAR_IS_MCHAR)
	mptr = name;
#else
	mptr = wcs_to_mbuf (g, name, &g->mbuf);
	if (mptr == QSE_NULL) return -1;
#endif

#if defined(HAVE_LSTAT)
	return (QSE_LSTAT (mptr, &st) == 0)? 1: 0;
#else
	/* use stat() if no lstat() is available. */
	return (QSE_STAT (mptr, &st) == 0)? 1: 0;
#endif

	/* ------------------------------------------------------------------- */

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

	qse_char_t sep; /* preceeding separator */
	unsigned int wild: 1;  /* indicate that it contains wildcards */
	unsigned int esc: 1;  /* indicate that it contains escaped letters */
	unsigned int next: 1;  /* indicate that it has the following segment */
};

typedef struct segment_t segment_t;

static int get_next_segment (glob_t* g, segment_t* seg)
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
			seg->sep = QSE_T('\0');
			seg->wild = 0;
			seg->esc = 0;
		}
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		else if (IS_DRIVE(seg->ptr))
		{
			seg->type = ROOT;
			seg->len = 2;
			if (IS_SEP(seg->ptr[2])) seg->len++;
			seg->next = IS_NIL(seg->ptr[seg->len])? 0: 1;
			seg->sep = QSE_T('\0');
			seg->wild = 0;
			seg->esc = 0;
		}
#endif
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

static int handle_non_wild_segments (glob_t* g, segment_t* seg)
{
	while (get_next_segment(g, seg) != NONE && !seg->wild)
	{
		QSE_ASSERT (seg->type != NONE && !seg->wild);

		if (seg->sep && qse_str_ccat (&g->path, seg->sep) == (qse_size_t)-1) return -1;

		if (seg->esc)
		{
			/* if the segment contains escape sequences,
			 * strip the escape letters off the segment */

			qse_cstr_t tmp;
			qse_size_t i;
			int escaped = 0;

			if (QSE_STR_CAPA(&g->tbuf) < seg->len &&
			    qse_str_setcapa (&g->tbuf, seg->len) == (qse_size_t)-1) return -1;

			tmp.ptr = QSE_STR_PTR(&g->tbuf);
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
			/* if the segmetn doesn't contain escape sequences,
			 * append the segment to the path without special handling */
			if (qse_str_ncat (&g->path, seg->ptr, seg->len) == (qse_size_t)-1) return -1;
		}

		if (!seg->next && path_exists(g, QSE_STR_PTR(&g->path)) > 0)
		{
			/* reached the last segment. match if the path exists */
			if (g->cbimpl (QSE_STR_XSTR(&g->path), g->cbctx) <= -1) return -1;
			g->expanded = 1;
		}
	}

	return 0;
}

#if defined(NO_RECURSION)
struct stack_node_t
{
	qse_size_t tmp;
	qse_size_t tmp2;
	qse_dir_t* dp;
	segment_t seg;

	stack_node_t* next;
};
#endif

static int search (glob_t* g, segment_t* seg)
{
	qse_dir_t* dp;
	qse_size_t tmp, tmp2;
	qse_dir_ent_t ent;
	int x;

#if defined(NO_RECURSION)
	stack_node_t* r;

entry:
#endif

	dp = QSE_NULL;

	if (handle_non_wild_segments (g, seg) <= -1) goto oops;

	if (seg->wild)
	{
		dp = qse_dir_open (g->mmgr, 0, QSE_STR_PTR(&g->path), 0, QSE_NULL);
		if (dp)
		{
			tmp = QSE_STR_LEN(&g->path);

			if (seg->sep && qse_str_ccat (&g->path, seg->sep) == (qse_size_t)-1) goto oops;
			tmp2 = QSE_STR_LEN(&g->path);

			while (1)
			{
				qse_str_setlen (&g->path, tmp2);

				x = qse_dir_read (dp, &ent);
				if (x <= -1) 
				{
					if (g->flags & QSE_GLOB_TOLERANT) break;
					else goto oops;
				}
				if (x == 0) break;

				if (qse_str_cat (&g->path, ent.name) == (qse_size_t)-1) goto oops;

				if (qse_strnfnmat (QSE_STR_CPTR(&g->path,tmp2), seg->ptr, seg->len, g->fnmat_flags) > 0)
				{
					if (seg->next)
					{
#if defined(NO_RECURSION)
						if (g->free) 
						{
							r = g->free;
							g->free = r->next;
						}
						else
						{
							r = QSE_MMGR_ALLOC (g->mmgr, QSE_SIZEOF(*r));
							if (r == QSE_NULL) goto oops;
						}
						
						/* push key variables that must be restored 
						 * into the stack. */
						r->tmp = tmp;
						r->tmp2 = tmp2;
						r->dp = dp;
						r->seg = *seg;

						r->next = g->stack;
						g->stack = r;
		
						/* move to the function entry point as if
						 * a recursive call has been made */
						goto entry;

					resume:
						;

#else
						segment_t save;
						int x;

						save = *seg;
						x = search (g, seg);
						*seg = save;
						if (x <= -1) goto oops;
#endif
					}
					else
					{
						if (g->cbimpl (QSE_STR_XSTR(&g->path), g->cbctx) <= -1) goto oops;
						g->expanded = 1;
					}
				}
			}

			qse_str_setlen (&g->path, tmp);
			qse_dir_close (dp); dp = QSE_NULL;
		}
	}

	QSE_ASSERT (dp == QSE_NULL);

#if defined(NO_RECURSION)
	if (g->stack)
	{
		/* the stack is not empty. the emulated recusive call
		 * must have been made. restore the variables pushed
		 * and jump to the resumption point */
		r = g->stack;
		g->stack = r->next;

		tmp = r->tmp;
		tmp2 = r->tmp2;
		dp = r->dp;
		*seg = r->seg;

		/* link the stack node to the free list 
		 * instead of freeing it here */
		r->next = g->free;
		g->free = r;

		goto resume;
	}

	while (g->free)
	{
		/* destory the free list */
		r = g->free;
		g->free = r->next;
		QSE_MMGR_FREE (g->mmgr, r);
	}
#endif

	return 0;

oops:
	if (dp) qse_dir_close (dp);	

#if defined(NO_RECURSION)
	while (g->stack)
	{
		r = g->stack;
		g->stack = r->next;
		qse_dir_close (r->dp);
		QSE_MMGR_FREE (g->mmgr, r);
	}

	while (g->free)
	{
		r = g->stack;
		g->free = r->next;
		QSE_MMGR_FREE (g->mmgr, r);
	}
#endif
	return -1;
}

int qse_glob (const qse_char_t* pattern, qse_glob_cbimpl_t cbimpl, void* cbctx, int flags, qse_mmgr_t* mmgr)
{
	segment_t seg;
	glob_t g;
	int x;

	QSE_MEMSET (&g, 0, QSE_SIZEOF(g));
	g.cbimpl = cbimpl;
	g.cbctx = cbctx;
	g.mmgr = mmgr;
	g.flags = flags;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	g.fnmat_flags |= QSE_STRFNMAT_IGNORECASE;
	g.fnmat_flags |= QSE_STRFNMAT_NOESCAPE;
#else
	if (flags & QSE_GLOB_IGNORECASE) g.fnmat_flags |= QSE_STRFNMAT_IGNORECASE;
	if (flags & QSE_GLOB_NOESCAPE) g.fnmat_flags |= QSE_STRFNMAT_NOESCAPE;
#endif
	if (flags & QSE_GLOB_PERIOD) g.fnmat_flags |= QSE_STRFNMAT_PERIOD;

	if (qse_str_init (&g.path, mmgr, 512) <= -1) return -1;
	if (qse_str_init (&g.tbuf, mmgr, 256) <= -1) 
	{
		qse_str_fini (&g.path);
		return -1;
	}
#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	/* nothing */
#else
	if (qse_mbs_init (&g.mbuf, mmgr, 512) <= -1) 
	{
		qse_str_fini (&g.path);
		qse_str_fini (&g.path);
		return -1;
	}
#endif

	QSE_MEMSET (&seg, 0, QSE_SIZEOF(seg));
	seg.type = NONE;
	seg.ptr = pattern;
	seg.len = 0;

	x = search (&g, &seg);

#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
	/* nothing */
#else
	qse_mbs_fini (&g.mbuf);
#endif
	qse_str_fini (&g.tbuf);
	qse_str_fini (&g.path);

	if (x <= -1) return -1;
	return g.expanded;
}

