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

int qse_ismbsabspath (const qse_mchar_t* path)
{
	if (QSE_ISPATHMBSEP(path[0])) return 1;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* a drive like c:tmp is absolute in positioning the drive.
	 * but the path within the drive is kind of relative */
	if (QSE_ISPATHMBDRIVE(path)) return 1;
#endif
	return 0;
}

int qse_ismbsdrivepath (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHMBDRIVE(path)) return 1;
#endif
	return 0;
}

int qse_ismbsdriveabspath (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHMBDRIVE(path) && QSE_ISPATHMBSEP(path[2])) return 1;
#endif
	return 0;
}

int qse_ismbsdrivecurpath (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHMBDRIVE(path) && path[2] == QSE_MT('\0')) return 1;
#endif
	return 0;
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
	if (QSE_ISPATHMBDRIVE(ptr))
	{
		/* handle drive letter */
		*dst++ = *ptr++; /* drive letter */
		*dst++ = *ptr++; /* colon */

		is_drive = 1;
		if (QSE_ISPATHMBSEP(*ptr)) 
		{
			*dst++ = *ptr++; /* root directory */
			has_root = 1;
		}
	}
	else if (QSE_ISPATHMBSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;

	#if defined(_WIN32)
		/* handle UNC path for Windows */
		if (QSE_ISPATHMBSEP(*ptr)) 
		{
			*dst++ = *ptr++;

			if (QSE_ISPATHMBSEPORNIL(*ptr))
			{
				/* if there is another separator after \\,
				 * it's not an UNC path. */
				dst--;
			}
			else
			{
				/* if it starts with \\, process host name */
				do { *dst++ = *ptr++; } while (!QSE_ISPATHMBSEPORNIL(*ptr));
				if (QSE_ISPATHMBSEP(*ptr)) *dst++ = *ptr++;
			}
		}
	#endif
	}
#else
	if (QSE_ISPATHMBSEP(*ptr)) 
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
		while (QSE_ISPATHMBSEP(*ptr)) ptr++;

		/* end of path reached */
		if (*ptr == QSE_MT('\0')) break;

		/* find the next segment */
		seg = ptr;
		while (!QSE_ISPATHMBSEPORNIL(*ptr)) ptr++;
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
					if (QSE_ISPATHMBSEP(*tmp)) 
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
			if (QSE_ISPATHMBSEP(*ptr)) 
			{
				/* this segment ended with a separator */
				*dst++ = *seg++; /* copy the separator */
				ptr++; /* move forward the pointer */
			}
		}
	}
	while (1);

	if (dst > non_root_start && QSE_ISPATHMBSEP(dst[-1]) && 
	    ((flags & QSE_CANONPATH_DROPTRAILINGSEP) || !QSE_ISPATHMBSEP(ptr[-1]))) 
	{
		/* if the canoncal path composed so far ends with a separator
		 * and the original path didn't end with the separator, delete
		 * the ending separator. 
		 * also delete it if QSE_CANONPATH_DROPTRAILINGSEP is set.
		 *
		 *   dst > non_root_start:
		 *     there is at least 1 character after the root directory 
		 *     part.
		 *   QSE_ISPATHMBSEP(dst[-1]):
		 *     the canonical path ends with a separator.
		 *   QSE_ISPATHMBSEP(ptr[-1]):
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
			    QSE_ISPATHMBSEP(canon[canon_len-1]))
			{
				canon[--canon_len] = QSE_MT('\0');
			}
		}
		else if (canon_len > adj_base_len)
		{
			if (QSE_ISPATHMBSEP(canon[canon_len-4]) &&
			    canon[canon_len-3] == QSE_MT('.') &&
			    canon[canon_len-2] == QSE_MT('.') &&
			    QSE_ISPATHMBSEP(canon[canon_len-1]))
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

int qse_iswcsabspath (const qse_wchar_t* path)
{
	if (QSE_ISPATHWCSEP(path[0])) return 1;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* a drive like c:tmp is absolute in positioning the drive.
	 * but the path within the drive is kind of relative */
	if (QSE_ISPATHWCDRIVE(path)) return 1;
#endif
     return 0;
}

int qse_iswcsdrivepath (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHWCDRIVE(path)) return 1;
#endif
	return 0;
}

int qse_iswcsdriveabspath (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHWCDRIVE(path) && QSE_ISPATHWCSEP(path[2])) return 1;
#endif
	return 0;
}

int qse_iswcsdrivecurpath (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHWCDRIVE(path) && path[2] == QSE_WT('\0')) return 1;
#endif
	return 0;
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
	if (QSE_ISPATHWCDRIVE(ptr))
	{
		/* handle drive letter */
		*dst++ = *ptr++; /* drive letter */
		*dst++ = *ptr++; /* colon */

		is_drive = 1;
		if (QSE_ISPATHWCSEP(*ptr)) 
		{
			*dst++ = *ptr++; /* root directory */
			has_root = 1;
		}
	}
	else if (QSE_ISPATHWCSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;

	#if defined(_WIN32)
		/* handle UNC path for Windows */
		if (QSE_ISPATHWCSEP(*ptr)) 
		{
			*dst++ = *ptr++;

			if (QSE_ISPATHWCSEPORNIL(*ptr))
			{
				/* if there is another separator after \\,
				 * it's not an UNC path. */
				dst--;
			}
			else
			{
				/* if it starts with \\, process host name */
				do { *dst++ = *ptr++; } while (!QSE_ISPATHWCSEPORNIL(*ptr));
				if (QSE_ISPATHWCSEP(*ptr)) *dst++ = *ptr++;
			}
		}
	#endif
	}
#else
	if (QSE_ISPATHWCSEP(*ptr)) 
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
		while (QSE_ISPATHWCSEP(*ptr)) ptr++;

		/* end of path reached */
		if (*ptr == QSE_WT('\0')) break;

		/* find the next segment */
		seg = ptr;
		while (!QSE_ISPATHWCSEPORNIL(*ptr)) ptr++;
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
					if (QSE_ISPATHWCSEP(*tmp)) 
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
			if (QSE_ISPATHWCSEP(*ptr)) 
			{
				/* this segment ended with a separator */
				*dst++ = *seg++; /* copy the separator */
				ptr++; /* move forward the pointer */
			}
		}
	}
	while (1);

	if (dst > non_root_start && QSE_ISPATHWCSEP(dst[-1]) && 
	    ((flags & QSE_CANONPATH_DROPTRAILINGSEP) || !QSE_ISPATHWCSEP(ptr[-1]))) 
	{
		/* if the canoncal path composed so far ends with a separator
		 * and the original path didn't end with the separator, delete
		 * the ending separator. 
		 * also delete it if QSE_CANONPATH_DROPTRAILINGSEP is set.
		 *
		 *   dst > non_root_start:
		 *     there is at least 1 character after the root directory 
		 *     part.
		 *   QSE_ISPATHWCSEP(dst[-1]):
		 *     the canonical path ends with a separator.
		 *   QSE_ISPATHWCSEP(ptr[-1]):
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
			    QSE_ISPATHWCSEP(canon[canon_len-1]))
			{
				canon[--canon_len] = QSE_WT('\0');
			}
		}
		else if (canon_len > adj_base_len)
		{
			if (QSE_ISPATHWCSEP(canon[canon_len-4]) &&
			    canon[canon_len-3] == QSE_WT('.') &&
			    canon[canon_len-2] == QSE_WT('.') &&
			    QSE_ISPATHWCSEP(canon[canon_len-1]))
			{
				canon[--canon_len] = QSE_WT('\0');
			}
		}
	}

	return canon_len;
}

