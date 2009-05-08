/*
 * $Id: str_utl.c 127 2009-05-07 13:15:04Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>

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

qse_char_t* qse_strtrm (qse_char_t* str, int opt)
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

	if (opt & QSE_STRTRM_RIGHT) e[1] = QSE_T('\0');
	if (opt & QSE_STRTRM_LEFT) str = s;

	return str;
}
