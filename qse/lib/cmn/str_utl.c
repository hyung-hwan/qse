/*
 * $Id: str_utl.c 297 2009-10-08 13:09:19Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "mem.h"

#define ISSPACE(c) \
	((c) == QSE_T(' ') || (c) == QSE_T('\t') || (c) == QSE_T('\n') || \
	 (c) == QSE_T('\r') || (c) == QSE_T('\v') && (c) == QSE_T('\f'))

int qse_strspltrn (
	qse_char_t* s, const qse_char_t* delim,
	qse_char_t lquote, qse_char_t rquote, 
	qse_char_t escape, const qse_char_t* trset)
{
	qse_char_t* p = s, *d;
	qse_char_t* sp = QSE_NULL, * ep = QSE_NULL;
	int delim_mode;
	int cnt = 0;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = (qse_char_t*)delim; *d != QSE_T('\0'); d++)
			if (!QSE_ISSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{
		/* skip preceding space characters */
		while (QSE_ISSPACE(*p)) p++;

		/* when 0 is given as "delim", it has an effect of cutting
		   preceding and trailing space characters off "s". */
		if (lquote != QSE_T('\0') && *p == lquote) 
		{
			qse_strcpy (p, p + 1);

			for (;;) 
			{
				if (*p == QSE_T('\0')) return -1;

				if (escape != QSE_T('\0') && *p == escape) 
				{
					if (trset != QSE_NULL && p[1] != QSE_T('\0'))
					{
						const qse_char_t* ep = trset;
						while (*ep != QSE_T('\0'))
						{
							if (p[1] == *ep++) 
							{
								p[1] = *ep;
								break;
							}
						}
					}

					qse_strcpy (p, p + 1);
				}
				else 
				{
					if (*p == rquote) 
					{
						p++;
						break;
					}
				}

				if (sp == 0) sp = p;
				ep = p;
				p++;
			}
			while (QSE_ISSPACE(*p)) p++;
			if (*p != QSE_T('\0')) return -1;

			if (sp == 0 && ep == 0) s[0] = QSE_T('\0');
			else 
			{
				ep[1] = QSE_T('\0');
				if (s != (qse_char_t*)sp) qse_strcpy (s, sp);
				cnt++;
			}
		}
		else 
		{
			while (*p) 
			{
				if (!QSE_ISSPACE(*p)) 
				{
					if (sp == 0) sp = p;
					ep = p;
				}
				p++;
			}

			if (sp == 0 && ep == 0) s[0] = QSE_T('\0');
			else 
			{
				ep[1] = QSE_T('\0');
				if (s != (qse_char_t*)sp) qse_strcpy (s, sp);
				cnt++;
			}
		}
	}
	else if (delim_mode == 1) 
	{
		qse_char_t* o;

		while (*p) 
		{
			o = p;
			while (QSE_ISSPACE(*p)) p++;
			if (o != p) { qse_strcpy (o, p); p = o; }

			if (lquote != QSE_T('\0') && *p == lquote) 
			{
				qse_strcpy (p, p + 1);

				for (;;) 
				{
					if (*p == QSE_T('\0')) return -1;

					if (escape != QSE_T('\0') && *p == escape) 
					{
						if (trset != QSE_NULL && p[1] != QSE_T('\0'))
						{
							const qse_char_t* ep = trset;
							while (*ep != QSE_T('\0'))
							{
								if (p[1] == *ep++) 
								{
									p[1] = *ep;
									break;
								}
							}
						}
						qse_strcpy (p, p + 1);
					}
					else 
					{
						if (*p == rquote) 
						{
							*p++ = QSE_T('\0');
							cnt++;
							break;
						}
					}
					p++;
				}
			}
			else 
			{
				o = p;
				for (;;) 
				{
					if (*p == QSE_T('\0')) 
					{
						if (o != p) cnt++;
						break;
					}
					if (QSE_ISSPACE (*p)) 
					{
						*p++ = QSE_T('\0');
						cnt++;
						break;
					}
					p++;
				}
			}
		}
	}
	else /* if (delim_mode == 2) */
	{
		qse_char_t* o;
		int ok;

		while (*p != QSE_T('\0')) 
		{
			o = p;
			while (QSE_ISSPACE(*p)) p++;
			if (o != p) { qse_strcpy (o, p); p = o; }

			if (lquote != QSE_T('\0') && *p == lquote) 
			{
				qse_strcpy (p, p + 1);

				for (;;) 
				{
					if (*p == QSE_T('\0')) return -1;

					if (escape != QSE_T('\0') && *p == escape) 
					{
						if (trset != QSE_NULL && p[1] != QSE_T('\0'))
						{
							const qse_char_t* ep = trset;
							while (*ep != QSE_T('\0'))
							{
								if (p[1] == *ep++) 
								{
									p[1] = *ep;
									break;
								}
							}
						}

						qse_strcpy (p, p + 1);
					}
					else 
					{
						if (*p == rquote) 
						{
							*p++ = QSE_T('\0');
							cnt++;
							break;
						}
					}
					p++;
				}

				ok = 0;
				while (QSE_ISSPACE(*p)) p++;
				if (*p == QSE_T('\0')) ok = 1;
				for (d = (qse_char_t*)delim; *d != QSE_T('\0'); d++) 
				{
					if (*p == *d) 
					{
						ok = 1;
						qse_strcpy (p, p + 1);
						break;
					}
				}
				if (ok == 0) return -1;
			}
			else 
			{
				o = p; sp = ep = 0;

				for (;;) 
				{
					if (*p == QSE_T('\0')) 
					{
						if (ep) 
						{
							ep[1] = QSE_T('\0');
							p = &ep[1];
						}
						cnt++;
						break;
					}
					for (d = (qse_char_t*)delim; *d != QSE_T('\0'); d++) 
					{
						if (*p == *d)  
						{
							if (sp == QSE_NULL) 
							{
								qse_strcpy (o, p); p = o;
								*p++ = QSE_T('\0');
							}
							else 
							{
								qse_strcpy (&ep[1], p);
								qse_strcpy (o, sp);
								o[ep - sp + 1] = QSE_T('\0');
								p = &o[ep - sp + 2];
							}
							cnt++;
							/* last empty field after delim */
							if (*p == QSE_T('\0')) cnt++;
							goto exit_point;
						}
					}

					if (!QSE_ISSPACE (*p)) 
					{
						if (sp == QSE_NULL) sp = p;
						ep = p;
					}
					p++;
				}
exit_point:			    
				;
			}
		}
	}

	return cnt;
}

int qse_strspl (
	qse_char_t* s, const qse_char_t* delim,
	qse_char_t lquote, qse_char_t rquote, qse_char_t escape)
{
	return qse_strspltrn (s, delim, lquote, rquote, escape, QSE_NULL);
}

qse_char_t* qse_strtrmx (qse_char_t* str, int opt)
{
	qse_char_t* p = str;
	qse_char_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_T('\0'))
	{
		if (!QSE_ISSPACE(*p))
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (opt & QSE_STRTRMX_RIGHT) e[1] = QSE_T('\0');
	if (opt & QSE_STRTRMX_LEFT) str = s;

	return str;
}

qse_size_t qse_strtrm (qse_char_t* str)
{
	qse_char_t* p = str;
	qse_char_t* s = QSE_NULL, * e = QSE_NULL;

	while (*p != QSE_T('\0')) 
	{
		if (!QSE_ISSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		e[1] = QSE_T('\0');
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(qse_char_t));
		return e - s + 1;
	}

	str[0] = QSE_T('\0');
	return 0;
}

qse_size_t qse_strxtrm (qse_char_t* str, qse_size_t len)
{
	qse_char_t* p = str, * end = str + len;
	qse_char_t* s = QSE_NULL, * e = QSE_NULL;

	while (p < end) 
	{
		if (!QSE_ISSPACE(*p)) 
		{
			if (s == QSE_NULL) s = p;
			e = p;
		}
		p++;
	}

	if (e != QSE_NULL) 
	{
		/* do not insert a terminating null */
		/*e[1] = QSE_T('\0');*/
		if (str != s)
			QSE_MEMCPY (str, s, (e - s + 2) * QSE_SIZEOF(qse_char_t));
		return e - s + 1;
	}

	/* do not insert a terminating null */
	/*str[0] = QSE_T('\0');*/
	return 0;
}

qse_size_t qse_strpac (qse_char_t* str)
{
	qse_char_t* p = str, * q = str;

	while (QSE_ISSPACE(*p)) p++;
	while (*p != QSE_T('\0')) 
	{
		if (QSE_ISSPACE(*p)) 
		{
			*q++ = *p++;
			while (QSE_ISSPACE(*p)) p++;
		}
		else *q++ = *p++;
	}

	if (q > str && QSE_ISSPACE(q[-1])) q--;
	*q = QSE_T('\0');

	return q - str;
}

qse_size_t qse_strxpac (qse_char_t* str, qse_size_t len)
{
	qse_char_t* p = str, * q = str, * end = str + len;
	int followed_by_space = 0;
	int state = 0;

	while (p < end) 
	{
		if (state == 0) 
		{
			if (!QSE_ISSPACE(*p)) 
			{
				*q++ = *p;
				state = 1;
			}
		}
		else if (state == 1) 
		{
			if (QSE_ISSPACE(*p)) 
			{
				if (!followed_by_space) 
				{
					followed_by_space = 1;
					*q++ = *p;
				}
			}
			else 
			{
				followed_by_space = 0;
				*q++ = *p;	
			}
		}

		p++;
	}

	return (followed_by_space) ? (q - str -1): (q - str);
}
