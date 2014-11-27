/*
 * $Id
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

#include <qse/cmn/path.h>

/* TODO: support the \\?\ prefix and the \\.\ prefix on windows 
 *       support \\?\UNC\server\path which is equivalent to \\server\path. 
 * */

/* ------------------------------------------------------------------ */
/*  MBS IMPLEMENTATION                                                */
/* ------------------------------------------------------------------ */

#define IS_MSEP(c) QSE_ISPATHMBSEP(c)

#define IS_MNIL(c) ((c) == QSE_MT('\0'))
#define IS_MSEP_OR_MNIL(c) (IS_MSEP(c) || IS_MNIL(c))

#define IS_MDRIVE(s) \
	(((s[0] >= QSE_MT('A') && s[0] <= QSE_MT('Z')) || \
	  (s[0] >= QSE_MT('a') && s[0] <= QSE_MT('z'))) && \
	 s[1] == QSE_MT(':'))


int qse_ismbsabspath (const qse_mchar_t* path)
{
	if (IS_MSEP(path[0])) return 1;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* a drive like c:tmp is absolute in positioning the drive.
	 * but the path within the drive is kind of relative */
	if (IS_MDRIVE(path)) return 1;
#endif
	return 0;
}

int qse_ismbsdrivepath (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_MDRIVE(path)) return 1;
#endif
	return 0;
}

int qse_ismbsdriveabspath (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_MDRIVE(path) && IS_MSEP(path[2])) return 1;
#endif
	return 0;
}

int qse_ismbsdrivecurpath (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_MDRIVE(path) && path[2] == QSE_MT('\0')) return 1;
#endif
	return 0;
}

qse_mchar_t* qse_getmbspathcore (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_MDRIVE(path)) return path + 2;
	#if defined(_WIN32)
	else if (IS_MSEP(*ptr) && IS_MSEP(*(ptr + 1)) && !IS_MSEP_OR_MNIL(*(ptr + 2)))
	{
		/* UNC Path */
		ptr += 2;
		do { ptr++; } while (!IS_MSEP_OR_MNIL(*ptr));
		if (IS_MSEP(*ptr)) return ptr;
	}
	#endif
/* TOOD: \\server\XXX \\.\XXX \\?\XXX \\?\UNC\server\XXX */
	
#endif
	return path;
}

qse_size_t qse_canonmbspath (const qse_mchar_t* path, qse_mchar_t* canon, int flags)
{
	const qse_mchar_t* ptr;
	qse_mchar_t* dst;
	qse_mchar_t* non_root_start;
	int has_root = 0;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	int is_drive = 0;
#endif
	qse_size_t canon_len;

	if (path[0] == QSE_MT('\0')) 
	{
		/* if the source is empty, no translation is needed */
		canon[0] = QSE_MT('\0');
		return 0;
	}

	ptr = path;
	dst = canon;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_MDRIVE(ptr))
	{
		/* handle drive letter */
		*dst++ = *ptr++; /* drive letter */
		*dst++ = *ptr++; /* colon */

		is_drive = 1;
		if (IS_MSEP(*ptr)) 
		{
			*dst++ = *ptr++; /* root directory */
			has_root = 1;
		}
	}
	else if (IS_MSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;

	#if defined(_WIN32)
		/* handle UNC path for Windows */
		if (IS_MSEP(*ptr)) 
		{
			*dst++ = *ptr++;

			if (IS_MSEP_OR_MNIL(*ptr))
			{
				/* if there is another separator after \\,
				 * it's not an UNC path. */
				dst--;
			}
			else
			{
				/* if it starts with \\, process host name */
				do { *dst++ = *ptr++; } while (!IS_MSEP_OR_MNIL(*ptr));
				if (IS_MSEP(*ptr)) *dst++ = *ptr++;
			}
		}
	#endif
	}
#else
	if (IS_MSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;
	}
#endif

	/* non_root_start points to the beginning of the canonicalized 
	 * path excluding the root directory part. */
	non_root_start = dst;

	do
	{
		const qse_mchar_t* seg;
		qse_size_t seglen;

		/* skip duplicate separators */
		while (IS_MSEP(*ptr)) ptr++;

		/* end of path reached */
		if (*ptr == QSE_MT('\0')) break;

		/* find the next segment */
		seg = ptr;
		while (!IS_MSEP_OR_MNIL(*ptr)) ptr++;
		seglen = ptr - seg;

		/* handle the segment */
		if (seglen == 1 && seg[0] == QSE_MT('.'))
		{
			/* eat up . */
		}
		else if (!(flags & QSE_CANONPATH_KEEPDOUBLEDOTS) &&
		         seglen == 2 && seg[0] == QSE_MT('.') && seg[1] == QSE_MT('.'))
		{
			/* eat up the previous segment */
			qse_mchar_t* tmp;

			tmp = dst;
			if (tmp > non_root_start) 
			{
				/* there is a previous segment. */

				tmp--; /* skip the separator just before .. */

				/* find the beginning of the previous segment */
				while (tmp > non_root_start)
				{
					tmp--;
					if (IS_MSEP(*tmp)) 
					{
						tmp++; /* position it next to the separator */
						break; 
					}
				}
			}

			if (has_root)
			{
				/*
				 * Eat up the previous segment if it exists.
				 *
				 * If it doesn't exist, tmp == dst so dst = tmp
				 * keeps dst unchanged. If it exists, 
				 * tmp != dst. so dst = tmp changes dst.
				 *
				 * path  /abc/def/..
				 *                ^ ^
				 *              seg ptr
				 *
				 * canon /abc/def/
				 *            ^   ^   
				 *           tmp dst
				 */
				dst = tmp;
			}
			else
			{
				qse_size_t prevlen;

				prevlen = dst - tmp;

				if (/*tmp == non_root_start &&*/ prevlen == 0)
				{
					/* there is no previous segment */
					goto normal;		
				}

				if (prevlen == 3 && tmp[0] == QSE_MT('.') && tmp[1] == QSE_MT('.')) 
				{
					/* nothing to eat away because the previous segment is ../
					 *
					 * path  ../../
					 *          ^ ^
					 *        seg ptr
					 *
					 * canon ../
					 *       ^  ^
					 *      tmp dst
					 */
					goto normal;
				}

				dst = tmp;
			}
		}
		else
		{
		normal:
			while (seg < ptr) *dst++ = *seg++;
			if (IS_MSEP(*ptr)) 
			{
				/* this segment ended with a separator */
				*dst++ = *seg++; /* copy the separator */
				ptr++; /* move forward the pointer */
			}
		}
	}
	while (1);

	if (dst > non_root_start && IS_MSEP(dst[-1]) && 
	    ((flags & QSE_CANONPATH_DROPTRAILINGSEP) || !IS_MSEP(ptr[-1]))) 
	{
		/* if the canoncal path composed so far ends with a separator
		 * and the original path didn't end with the separator, delete
		 * the ending separator. 
		 * also delete it if QSE_CANONPATH_DROPTRAILINGSEP is set.
		 *
		 *   dst > non_root_start:
		 *     there is at least 1 character after the root directory 
		 *     part.
		 *   IS_MSEP(dst[-1]):
		 *     the canonical path ends with a separator.
		 *   IS_MSEP(ptr[-1]):
		 *     the origial path ends with a separator.
		 */
		dst[-1] = QSE_MT('\0');
		canon_len = dst - canon - 1;
	}
	else 
	{
		/* just null-terminate the canonical path normally */
		dst[0] = QSE_MT('\0');	
		canon_len = dst - canon;
	}

	if (canon_len <= 0) 
	{
		if (!(flags & QSE_CANONPATH_EMPTYSINGLEDOT))
		{
			/* when resolving to a single dot, a trailing separator is not
			 * retained though the orignal path name contains it. */
			canon[0] = QSE_MT('.');
			canon[1] = QSE_MT('\0');
			canon_len = 1;
		}
	}
	else 
	{
		/* drop a traling separator if the last segment is 
		 * double slashes */

		int adj_base_len = 3;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		if (is_drive && !has_root) 
		{
			/* A path like A:..\\\ need some adjustment for
			 * finalization below. */
			adj_base_len += 2;
		}
#endif

		if (canon_len == adj_base_len)
		{
			/* i don't have to retain a trailing separator
			 * if the last segment is double slashes because 
			 * the double slahses indicate a directory obviously */
			if (canon[canon_len-3] == QSE_MT('.') &&
			    canon[canon_len-2] == QSE_MT('.') &&
			    IS_MSEP(canon[canon_len-1]))
			{
				canon[--canon_len] = QSE_MT('\0');
			}
		}
		else if (canon_len > adj_base_len)
		{
			if (IS_MSEP(canon[canon_len-4]) &&
			    canon[canon_len-3] == QSE_MT('.') &&
			    canon[canon_len-2] == QSE_MT('.') &&
			    IS_MSEP(canon[canon_len-1]))
			{
				canon[--canon_len] = QSE_MT('\0');
			}
		}
	}

	return canon_len;
}


/* ------------------------------------------------------------------ */
/*  WCS IMPLEMENTATION                                                */
/* ------------------------------------------------------------------ */
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define IS_WSEP(c) ((c) == QSE_WT('/') || (c) == QSE_WT('\\'))
#else
#	define IS_WSEP(c) ((c) == QSE_WT('/'))
#endif

#define IS_WNIL(c) ((c) == QSE_WT('\0'))
#define IS_WSEP_OR_WNIL(c) (IS_WSEP(c) || IS_WNIL(c))

#define IS_WDRIVE(s) \
	(((s[0] >= QSE_WT('A') && s[0] <= QSE_WT('Z')) || \
	  (s[0] >= QSE_WT('a') && s[0] <= QSE_WT('z'))) && \
	 s[1] == QSE_WT(':'))

int qse_iswcsabspath (const qse_wchar_t* path)
{
	if (IS_WSEP(path[0])) return 1;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* a drive like c:tmp is absolute in positioning the drive.
	 * but the path within the drive is kind of relative */
	if (IS_WDRIVE(path)) return 1;
#endif
     return 0;
}

int qse_iswcsdrivepath (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_WDRIVE(path)) return 1;
#endif
	return 0;
}

int qse_iswcsdriveabspath (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_WDRIVE(path) && IS_WSEP(path[2])) return 1;
#endif
	return 0;
}

int qse_iswcsdrivecurpath (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_WDRIVE(path) && path[2] == QSE_WT('\0')) return 1;
#endif
	return 0;
}

qse_wchar_t* qse_getwcspathcore (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_WDRIVE(path)) return path + 2;
	#if defined(_WIN32)
	else if (IS_WSEP(*ptr) && IS_WSEP(*(ptr + 1)) && !IS_WSEP_OR_WNIL(*(ptr + 2)))
	{
		/* UNC Path */
		ptr += 2;
		do { ptr++; } while (!IS_WSEP_OR_WNIL(*ptr));
		if (IS_WSEP(*ptr)) return ptr;
	}
	#endif
#endif
	return path;
}

qse_size_t qse_canonwcspath (const qse_wchar_t* path, qse_wchar_t* canon, int flags)
{
	const qse_wchar_t* ptr;
	qse_wchar_t* dst;
	qse_wchar_t* non_root_start;
	int has_root = 0;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	int is_drive = 0;
#endif
	qse_size_t canon_len;

	if (path[0] == QSE_WT('\0')) 
	{
		/* if the source is empty, no translation is needed */
		canon[0] = QSE_WT('\0');
		return 0;
	}

	ptr = path;
	dst = canon;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (IS_WDRIVE(ptr))
	{
		/* handle drive letter */
		*dst++ = *ptr++; /* drive letter */
		*dst++ = *ptr++; /* colon */

		is_drive = 1;
		if (IS_WSEP(*ptr)) 
		{
			*dst++ = *ptr++; /* root directory */
			has_root = 1;
		}
	}
	else if (IS_WSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;

	#if defined(_WIN32)
		/* handle UNC path for Windows */
		if (IS_WSEP(*ptr)) 
		{
			*dst++ = *ptr++;

			if (IS_WSEP_OR_WNIL(*ptr))
			{
				/* if there is another separator after \\,
				 * it's not an UNC path. */
				dst--;
			}
			else
			{
				/* if it starts with \\, process host name */
				do { *dst++ = *ptr++; } while (!IS_WSEP_OR_WNIL(*ptr));
				if (IS_WSEP(*ptr)) *dst++ = *ptr++;
			}
		}
	#endif
	}
#else
	if (IS_WSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;
	}
#endif

	/* non_root_start points to the beginning of the canonicalized 
	 * path excluding the root directory part. */
	non_root_start = dst;

	do
	{
		const qse_wchar_t* seg;
		qse_size_t seglen;

		/* skip duplicate separators */
		while (IS_WSEP(*ptr)) ptr++;

		/* end of path reached */
		if (*ptr == QSE_WT('\0')) break;

		/* find the next segment */
		seg = ptr;
		while (!IS_WSEP_OR_WNIL(*ptr)) ptr++;
		seglen = ptr - seg;

		/* handle the segment */
		if (seglen == 1 && seg[0] == QSE_WT('.'))
		{
			/* eat up . */
		}
		else if (!(flags & QSE_CANONPATH_KEEPDOUBLEDOTS) &&
		         seglen == 2 && seg[0] == QSE_WT('.') && seg[1] == QSE_WT('.'))
		{
			/* eat up the previous segment */
			qse_wchar_t* tmp;

			tmp = dst;
			if (tmp > non_root_start) 
			{
				/* there is a previous segment. */

				tmp--; /* skip the separator just before .. */

				/* find the beginning of the previous segment */
				while (tmp > non_root_start)
				{
					tmp--;
					if (IS_WSEP(*tmp)) 
					{
						tmp++; /* position it next to the separator */
						break; 
					}
				}
			}

			if (has_root)
			{
				/*
				 * Eat up the previous segment if it exists.
				 *
				 * If it doesn't exist, tmp == dst so dst = tmp
				 * keeps dst unchanged. If it exists, 
				 * tmp != dst. so dst = tmp changes dst.
				 *
				 * path  /abc/def/..
				 *                ^ ^
				 *              seg ptr
				 *
				 * canon /abc/def/
				 *            ^   ^   
				 *           tmp dst
				 */
				dst = tmp;
			}
			else
			{
				qse_size_t prevlen;

				prevlen = dst - tmp;

				if (/*tmp == non_root_start &&*/ prevlen == 0)
				{
					/* there is no previous segment */
					goto normal;		
				}

				if (prevlen == 3 && tmp[0] == QSE_WT('.') && tmp[1] == QSE_WT('.')) 
				{
					/* nothing to eat away because the previous segment is ../
					 *
					 * path  ../../
					 *          ^ ^
					 *        seg ptr
					 *
					 * canon ../
					 *       ^  ^
					 *      tmp dst
					 */
					goto normal;
				}

				dst = tmp;
			}
		}
		else
		{
		normal:
			while (seg < ptr) *dst++ = *seg++;
			if (IS_WSEP(*ptr)) 
			{
				/* this segment ended with a separator */
				*dst++ = *seg++; /* copy the separator */
				ptr++; /* move forward the pointer */
			}
		}
	}
	while (1);

	if (dst > non_root_start && IS_WSEP(dst[-1]) && 
	    ((flags & QSE_CANONPATH_DROPTRAILINGSEP) || !IS_WSEP(ptr[-1]))) 
	{
		/* if the canoncal path composed so far ends with a separator
		 * and the original path didn't end with the separator, delete
		 * the ending separator. 
		 * also delete it if QSE_CANONPATH_DROPTRAILINGSEP is set.
		 *
		 *   dst > non_root_start:
		 *     there is at least 1 character after the root directory 
		 *     part.
		 *   IS_WSEP(dst[-1]):
		 *     the canonical path ends with a separator.
		 *   IS_WSEP(ptr[-1]):
		 *     the origial path ends with a separator.
		 */
		dst[-1] = QSE_WT('\0');
		canon_len = dst - canon - 1;
	}
	else 
	{
		/* just null-terminate the canonical path normally */
		dst[0] = QSE_WT('\0');	
		canon_len = dst - canon;
	}

	if (canon_len <= 0) 
	{
		if (!(flags & QSE_CANONPATH_EMPTYSINGLEDOT))
		{
			/* when resolving to a single dot, a trailing separator is not
			 * retained though the orignal path name contains it. */
			canon[0] = QSE_WT('.');
			canon[1] = QSE_WT('\0');
			canon_len = 1;
		}
	}
	else 
	{
		/* drop a traling separator if the last segment is 
		 * double slashes */

		int adj_base_len = 3;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		if (is_drive && !has_root) 
		{
			/* A path like A:..\\\ need some adjustment for
			 * finalization below. */
			adj_base_len += 2;
		}
#endif

		if (canon_len == adj_base_len)
		{
			/* i don't have to retain a trailing separator
			 * if the last segment is double slashes because 
			 * the double slahses indicate a directory obviously */
			if (canon[canon_len-3] == QSE_WT('.') &&
			    canon[canon_len-2] == QSE_WT('.') &&
			    IS_WSEP(canon[canon_len-1]))
			{
				canon[--canon_len] = QSE_WT('\0');
			}
		}
		else if (canon_len > adj_base_len)
		{
			if (IS_WSEP(canon[canon_len-4]) &&
			    canon[canon_len-3] == QSE_WT('.') &&
			    canon[canon_len-2] == QSE_WT('.') &&
			    IS_WSEP(canon[canon_len-1]))
			{
				canon[--canon_len] = QSE_WT('\0');
			}
		}
	}

	return canon_len;
}

