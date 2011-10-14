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

#include <qse/types.h>
#include <qse/macros.h>
/*#include <qse/fs/path.h>*/

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define ISSEP(c) ((c) == QSE_T('/') || (c) == QSE_T('\\'))
#else
#	define ISSEP(c) ((c) == QSE_T('/'))
#endif

#define ISSEPNIL(c) (ISSEP(c) || ((c) == QSE_T('\0')))

int qse_canonpath (const qse_char_t* path)
{
	const qse_char_t* ptr;
	qse_char_t* dst;
	qse_char_t* root = QSE_NULL;

/* TODO: delete this. */
	qse_char_t* buf = malloc (10000);

	ptr = path;
	dst = buf;

	if (ISSEP(*ptr))
	{
		*dst++ = *ptr++;

#if defined(_WIN32)
		/* handle UNC path */
		if (ISSEP(*ptr)) 
		{
			*dst++ = *ptr++;

			/* if it starts with \\, process host name */
			while (!ISSEPNIL(*ptr)) *dst++ = *ptr++;

			/* \ following the host name */
			if (ISSEP(*ptr)) *dst++ = *ptr++;
		}
#endif
	}	

	root = dst;

	do
	{
		const qse_char_t* seg;
		qse_size_t len;

		/* skip duplicate separators */
		while (ISSEP(*ptr)) ptr++;

		/* end of path reached */
		if (*ptr == QSE_T('\0')) break;

		seg = ptr;
		while (!ISSEPNIL(*ptr)) ptr++;

		len = ptr - seg;
		if (len == 1 && seg[0] == QSE_T('.'))
		{
			/* eat up . */
		}
		else if (len == 2 && seg[0] == QSE_T('.') && seg[1] == QSE_T('.'))
		{
			/* eat up the previous segment */
			/*if (!root) goto normal;*/
			const qse_char_t* tmp;

			tmp = dst;
			if (tmp > root) tmp--;
			while (tmp > root)
			{
				tmp--;
				if (ISSEP(*tmp)) break;
			}

			if (root > buf)
			{
				/* mean that the path contains the root directory */
				dst = tmp;
				if (tmp == root) tmp++;
			}
			else
			{
				if (tmp == root) goto normal;		

				if (dst - tmp == 3 && 
				    tmp[0] == QSE_T('.') && tmp[1] == QSE_T('.')) 
				{
					/* the previous segment ../ */
					goto normal;
				}
				if (dst - tmp == 4 && 
				    ISSEP(tmp[0]) &&
				    tmp[1] == QSE_T('.') && 
				    tmp[2] == QSE_T('.')) 
				{
					/* the previous segment is /../ */
					goto normal;
				}

				dst = tmp + 1;
			}
		}
		else
		{
		normal:
			while (seg <= ptr) *dst++ = *seg++;
			if (ISSEP(*ptr)) ptr++;
		}
	}
	while (1);

	*dst++ = QSE_T('\0');	
	return buf;
}
