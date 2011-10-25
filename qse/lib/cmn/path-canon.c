/*
 * $Id
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/cmn/path.h>

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define ISSEP(c) ((c) == QSE_T('/') || (c) == QSE_T('\\'))
#else
#	define ISSEP(c) ((c) == QSE_T('/'))
#endif

#define ISSEPNIL(c) (ISSEP(c) || ((c) == QSE_T('\0')))

#define ISDRIVE(s) \
	(((s[0] >= QSE_T('A') && s[0] <= QSE_T('Z')) || \
	  (s[0] >= QSE_T('a') && s[0] <= QSE_T('z'))) && \
	 s[1] == QSE_T(':'))

int qse_isabspath (const qse_char_t* path)
{
	if (ISSEP(path[0])) return 1;
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* a drive like c:tmp is absolute in positioning the drive.
	 * but the path within the drive is kind of relative */
	if (ISDRIVE(path)) return 1;
#endif
     return 0;

}

int qse_isdrivepath (const qse_char_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (ISDRIVE(path)) return 1;
#endif
	return 0;
}

int qse_isdrivecurpath (const qse_char_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (ISDRIVE(path) && path[2] == QSE_T('\0')) return 1;
#endif
	return 0;
}

qse_size_t qse_canonpath (const qse_char_t* path, qse_char_t* canon)
{
	const qse_char_t* ptr;
	qse_char_t* dst;
	qse_char_t* non_root_start;
	int has_root = 0;
	int begins_with_curdir = 0;
	qse_size_t canon_len;

	ptr = path;
	dst = canon;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (ISDRIVE(ptr))
	{
		/* handle drive letter */
		*dst++ = *ptr++; /* drive letter */
		*dst++ = *ptr++; /* colon */

		if (ISSEP(*ptr)) 
		{
			*dst++ = *ptr++; /* root directory */
			has_root = 1;
		}
	}
	else if (ISSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;

	#if defined(_WIN32)
		/* handle UNC path for Windows */
		if (ISSEP(*ptr)) 
		{
			*dst++ = *ptr++;

			if (ISSEPNIL(*ptr))
			{
				/* if there is another separator after \\,
				 * it's not an UNC path. */
				dst--;
			}
			else
			{
				/* if it starts with \\, process host name */
				do { *dst++ = *ptr++; } while (!ISSEPNIL(*ptr));
				if (ISSEP(*ptr)) *dst++ = *ptr++;
			}
		}
	#endif
	}
	else if (ptr[0] == QSE_T('.') && ISSEPNIL(ptr[1])) 
	{
		begins_with_curdir = 1;
	}
#else
	if (ISSEP(*ptr)) 
	{
		*dst++ = *ptr++; /* root directory */
		has_root = 1;
	}
	else if (ptr[0] == QSE_T('.') && ISSEPNIL(ptr[1])) 
	{
		begins_with_curdir = 1;
	}
#endif

	/* non_root_start points to the beginning of the canonicalized 
	 * path excluding the root directory part. */
	non_root_start = dst;

	do
	{
		const qse_char_t* seg;
		qse_size_t seglen;

		/* skip duplicate separators */
		while (ISSEP(*ptr)) ptr++;

		/* end of path reached */
		if (*ptr == QSE_T('\0')) break;

		/* find the next segment */
		seg = ptr;
		while (!ISSEPNIL(*ptr)) ptr++;
		seglen = ptr - seg;

		/* handle the segment */
		if (seglen == 1 && seg[0] == QSE_T('.'))
		{
			/* eat up . */
		}
		else if (seglen == 2 && seg[0] == QSE_T('.') && seg[1] == QSE_T('.'))
		{
			/* eat up the previous segment */
			qse_char_t* tmp;

			tmp = dst;
			if (tmp > non_root_start) 
			{
				/* there is a previous segment. */

				tmp--; /* skip the separator just before .. */

				/* find the beginning of the previous segment */
				while (tmp > non_root_start)
				{
					tmp--;
					if (ISSEP(*tmp)) 
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
				 * If it doesn't exist, tmp == dst so dst = tmp keeps dst
				 * unchanged. If it exists, tmp != dst. so dst = tmp 
				 * changes dst.
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

				if (prevlen == 3 && tmp[0] == QSE_T('.') && tmp[1] == QSE_T('.')) 
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
			if (ISSEP(*ptr)) 
			{
				/* this segment ended with a separator */
				*dst++ = *seg++; /* copy the separator */
				ptr++; /* move forward the pointer */
			}
		}
	}
	while (1);

	if (dst > non_root_start && ISSEP(dst[-1]) && !ISSEP(ptr[-1])) 
	{
		/* if the canoncal path composed so far ends with a separator
		 * and the original path didn't end with the separator, delete
		 * the ending separator.
		 *
		 *   dst > non_root_start:
		 *     there is at least 1 character after the root directory part.
		 *   ISSEP(dst[-1]):
		 *     the canonical path ends with a separator.
		 *   ISSEP(ptr[-1]):
		 *     the origial path ends with a separator
		 */
		dst[-1] = QSE_T('\0');
		canon_len = dst - canon - 1;
	}
	else 
	{
		/* just null-terminate the canonical path normally */
		dst[0] = QSE_T('\0');	
		canon_len = dst - canon;
	}

	if (canon_len <= 0 && begins_with_curdir)
	{
		/* when resolving to a single dot, a trailing separator is not
		 * retained though the orignal path name contains it */
		dst[0] = QSE_T('.');
		dst[1] = QSE_T('\0');
		canon_len = 1;
	}

	return canon_len;
}

qse_size_t qse_realpath (const qse_char_t* path, qse_char_t* real)
{
/* TODO: canonicalize path with symbolic links resolved */
	return 0;
}
