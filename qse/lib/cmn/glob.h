/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#if defined(NO_RECURSION)
typedef struct stack_node_t stack_node_t;
#endif

struct glob_t
{
	cbimpl_t cbimpl;
	void* cbctx;

	qse_mmgr_t* mmgr;
	qse_cmgr_t* cmgr;
	int flags;

	str_t path;
	str_t tbuf; /* temporary buffer */

#if defined(DECLARE_MBUF)
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

struct segment_t
{
	segment_type_t type;

	const char_t* ptr;
	qse_size_t    len;

	char_t sep; /* preceeding separator */
	unsigned int wild: 1;  /* indicate that it contains wildcards */
	unsigned int esc: 1;  /* indicate that it contains escaped letters */
	unsigned int next: 1;  /* indicate that it has the following segment */
};

typedef struct segment_t segment_t;

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

#if defined(DECLARE_MBUF)
static qse_mchar_t* wcs_to_mbuf (glob_t* g, const qse_wchar_t* wcs, qse_mbs_t* mbs)
{
	qse_size_t ml, wl;

	if (qse_wcstombswithcmgr (wcs, &wl, QSE_NULL, &ml, g->cmgr) <= -1 ||
	    qse_mbs_setlen (mbs, ml) == (qse_size_t)-1) return QSE_NULL;

	qse_wcstombswithcmgr (wcs, &wl, QSE_MBS_PTR(mbs), &ml, g->cmgr);
	return QSE_MBS_PTR(mbs);
}
#endif

static int path_exists (glob_t* g, const char_t* name)
{
#if defined(_WIN32)

	/* ------------------------------------------------------------------- */
	#if !defined(INVALID_FILE_ATTRIBUTES)
	#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
	#endif
	#if defined(CHAR_IS_MCHAR)
	return (GetFileAttributesA(name) != INVALID_FILE_ATTRIBUTES)? 1: 0;
	#else
	return (GetFileAttributesW(name) != INVALID_FILE_ATTRIBUTES)? 1: 0;
	#endif
	/* ------------------------------------------------------------------- */

#elif defined(__OS2__)

	/* ------------------------------------------------------------------- */
	FILESTATUS3 fs;
	APIRET rc;
	const qse_mchar_t* mptr;

#if defined(CHAR_IS_MCHAR)
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

#if defined(CHAR_IS_MCHAR)
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

#if defined(CHAR_IS_MCHAR)
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
#if defined(HAVE_LSTAT)
	qse_lstat_t st;
#else
	qse_stat_t st;
#endif
	const qse_mchar_t* mptr;

#if defined(CHAR_IS_MCHAR)
	mptr = name;
#else
	mptr = wcs_to_mbuf (g, name, &g->mbuf);
	if (mptr == QSE_NULL) return -1;
#endif

#if defined(HAVE_LSTAT)
	return (QSE_LSTAT (mptr, &st) <= -1)? 0: 1;
#else
	/* use stat() if no lstat() is available. */
	return (QSE_STAT (mptr, &st) <= -1)? 0: 1;
#endif

	/* ------------------------------------------------------------------- */
#endif
}


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
			seg->sep = T('\0');
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
			seg->sep = T('\0');
			seg->wild = 0;
			seg->esc = 0;
		}
	#endif
		else
		{
			int escaped = 0;
			seg->type = NORMAL;
			seg->sep = T('\0');
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
		seg->sep = T('\0');
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
			seg->sep = T('\0');
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

		if (seg->sep && str_ccat (&g->path, seg->sep) == (qse_size_t)-1) return -1;
		if (seg->esc)
		{
			/* if the segment contains escape sequences,
			 * strip the escape letters off the segment */

			cstr_t tmp;
			qse_size_t i;
			int escaped = 0;

			if (STR_CAPA(&g->tbuf) < seg->len &&
			    str_setcapa (&g->tbuf, seg->len) == (qse_size_t)-1) return -1;

			tmp.ptr = STR_PTR(&g->tbuf);
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

			if (str_ncat (&g->path, tmp.ptr, tmp.len) == (qse_size_t)-1) return -1;
		}
		else
		{
			/* if the segmetn doesn't contain escape sequences,
			 * append the segment to the path without special handling */
			if (str_ncat (&g->path, seg->ptr, seg->len) == (qse_size_t)-1) return -1;
		}

		if (!seg->next && path_exists (g, STR_PTR(&g->path)) > 0)
		{
			/* reached the last segment. match if the path exists */
			if (g->cbimpl (STR_XSTR(&g->path), g->cbctx) <= -1) return -1;
			g->expanded = 1;
		}
	}

	return 0;
}

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
		int dir_flags = DIR_CHAR_FLAGS;
		if (g->flags & QSE_GLOB_SKIPSPCDIR) dir_flags |= QSE_DIR_SKIPSPCDIR;

		dp = qse_dir_open (
			g->mmgr, 0, (const qse_char_t*)STR_PTR(&g->path),
			dir_flags, QSE_NULL);
		if (dp)
		{
			tmp = STR_LEN(&g->path);

			if (seg->sep && str_ccat (&g->path, seg->sep) == (qse_size_t)-1) goto oops;
			tmp2 = STR_LEN(&g->path);

			while (1)
			{
				str_setlen (&g->path, tmp2);

				x = qse_dir_read (dp, &ent);
				if (x <= -1) 
				{
					if (g->flags & QSE_GLOB_TOLERANT) break;
					else goto oops;
				}
				if (x == 0) break;

				if (str_cat (&g->path, (const char_t*)ent.name) == (qse_size_t)-1) goto oops;

				if (strnfnmat (STR_CPTR(&g->path,tmp2), seg->ptr, seg->len, g->fnmat_flags) > 0)
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
						if (g->cbimpl (STR_XSTR(&g->path), g->cbctx) <= -1) goto oops;
						g->expanded = 1;
					}
				}
			}

			str_setlen (&g->path, tmp);
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

int glob (const char_t* pattern, cbimpl_t cbimpl, void* cbctx, int flags, qse_mmgr_t* mmgr, qse_cmgr_t* cmgr)
{
	segment_t seg;
	glob_t g;
	int x;

	QSE_MEMSET (&g, 0, QSE_SIZEOF(g));
	g.cbimpl = cbimpl;
	g.cbctx = cbctx;
	g.mmgr = mmgr;
	g.cmgr = cmgr;
	g.flags = flags;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	g.fnmat_flags |= QSE_STRFNMAT_IGNORECASE;
	g.fnmat_flags |= QSE_STRFNMAT_NOESCAPE;
#else
	if (flags & QSE_GLOB_IGNORECASE) g.fnmat_flags |= QSE_STRFNMAT_IGNORECASE;
	if (flags & QSE_GLOB_NOESCAPE) g.fnmat_flags |= QSE_STRFNMAT_NOESCAPE;
#endif
	if (flags & QSE_GLOB_PERIOD) g.fnmat_flags |= QSE_STRFNMAT_PERIOD;

	if (str_init (&g.path, mmgr, 512) <= -1) return -1;
	if (str_init (&g.tbuf, mmgr, 256) <= -1) 
	{
		str_fini (&g.path);
		return -1;
	}

#if defined(DECLARE_MBUF)
	if (qse_mbs_init (&g.mbuf, mmgr, 512) <= -1) 
	{
		str_fini (&g.path);
		str_fini (&g.path);
		return -1;
	}
#endif

	QSE_MEMSET (&seg, 0, QSE_SIZEOF(seg));
	seg.type = NONE;
	seg.ptr = pattern;
	seg.len = 0;

	x = search (&g, &seg);

#if defined(DECLARE_MBUF)
	qse_mbs_fini (&g.mbuf);
#endif
	str_fini (&g.tbuf);
	str_fini (&g.path);

	if (x <= -1) return -1;
	return g.expanded;
}
