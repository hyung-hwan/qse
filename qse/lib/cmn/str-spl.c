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

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>

int qse_mbsspltrn (
	qse_mchar_t* s, const qse_mchar_t* delim,
	qse_mchar_t lquote, qse_mchar_t rquote, 
	qse_mchar_t escape, const qse_mchar_t* trset)
{
	qse_mchar_t* p = s, *d;
	qse_mchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	int delim_mode;
	int cnt = 0;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = (qse_mchar_t*)delim; *d != QSE_MT('\0'); d++)
			if (!QSE_ISMSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{
		/* skip preceding space characters */
		while (QSE_ISMSPACE(*p)) p++;

		/* when 0 is given as "delim", it has an effect of cutting
		   preceding and trailing space characters off "s". */
		if (lquote != QSE_MT('\0') && *p == lquote) 
		{
			qse_mbscpy (p, p + 1);

			for (;;) 
			{
				if (*p == QSE_MT('\0')) return -1;

				if (escape != QSE_MT('\0') && *p == escape) 
				{
					if (trset != QSE_NULL && p[1] != QSE_MT('\0'))
					{
						const qse_mchar_t* ep = trset;
						while (*ep != QSE_MT('\0'))
						{
							if (p[1] == *ep++) 
							{
								p[1] = *ep;
								break;
							}
						}
					}

					qse_mbscpy (p, p + 1);
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
			while (QSE_ISMSPACE(*p)) p++;
			if (*p != QSE_MT('\0')) return -1;

			if (sp == 0 && ep == 0) s[0] = QSE_MT('\0');
			else 
			{
				ep[1] = QSE_MT('\0');
				if (s != (qse_mchar_t*)sp) qse_mbscpy (s, sp);
				cnt++;
			}
		}
		else 
		{
			while (*p) 
			{
				if (!QSE_ISMSPACE(*p)) 
				{
					if (sp == 0) sp = p;
					ep = p;
				}
				p++;
			}

			if (sp == 0 && ep == 0) s[0] = QSE_MT('\0');
			else 
			{
				ep[1] = QSE_MT('\0');
				if (s != (qse_mchar_t*)sp) qse_mbscpy (s, sp);
				cnt++;
			}
		}
	}
	else if (delim_mode == 1) 
	{
		qse_mchar_t* o;

		while (*p) 
		{
			o = p;
			while (QSE_ISMSPACE(*p)) p++;
			if (o != p) { qse_mbscpy (o, p); p = o; }

			if (lquote != QSE_MT('\0') && *p == lquote) 
			{
				qse_mbscpy (p, p + 1);

				for (;;) 
				{
					if (*p == QSE_MT('\0')) return -1;

					if (escape != QSE_MT('\0') && *p == escape) 
					{
						if (trset != QSE_NULL && p[1] != QSE_MT('\0'))
						{
							const qse_mchar_t* ep = trset;
							while (*ep != QSE_MT('\0'))
							{
								if (p[1] == *ep++) 
								{
									p[1] = *ep;
									break;
								}
							}
						}
						qse_mbscpy (p, p + 1);
					}
					else 
					{
						if (*p == rquote) 
						{
							*p++ = QSE_MT('\0');
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
					if (*p == QSE_MT('\0')) 
					{
						if (o != p) cnt++;
						break;
					}
					if (QSE_ISMSPACE (*p)) 
					{
						*p++ = QSE_MT('\0');
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
		qse_mchar_t* o;
		int ok;

		while (*p != QSE_MT('\0')) 
		{
			o = p;
			while (QSE_ISMSPACE(*p)) p++;
			if (o != p) { qse_mbscpy (o, p); p = o; }

			if (lquote != QSE_MT('\0') && *p == lquote) 
			{
				qse_mbscpy (p, p + 1);

				for (;;) 
				{
					if (*p == QSE_MT('\0')) return -1;

					if (escape != QSE_MT('\0') && *p == escape) 
					{
						if (trset != QSE_NULL && p[1] != QSE_MT('\0'))
						{
							const qse_mchar_t* ep = trset;
							while (*ep != QSE_MT('\0'))
							{
								if (p[1] == *ep++) 
								{
									p[1] = *ep;
									break;
								}
							}
						}

						qse_mbscpy (p, p + 1);
					}
					else 
					{
						if (*p == rquote) 
						{
							*p++ = QSE_MT('\0');
							cnt++;
							break;
						}
					}
					p++;
				}

				ok = 0;
				while (QSE_ISMSPACE(*p)) p++;
				if (*p == QSE_MT('\0')) ok = 1;
				for (d = (qse_mchar_t*)delim; *d != QSE_MT('\0'); d++) 
				{
					if (*p == *d) 
					{
						ok = 1;
						qse_mbscpy (p, p + 1);
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
					if (*p == QSE_MT('\0')) 
					{
						if (ep) 
						{
							ep[1] = QSE_MT('\0');
							p = &ep[1];
						}
						cnt++;
						break;
					}
					for (d = (qse_mchar_t*)delim; *d != QSE_MT('\0'); d++) 
					{
						if (*p == *d)  
						{
							if (sp == QSE_NULL) 
							{
								qse_mbscpy (o, p); p = o;
								*p++ = QSE_MT('\0');
							}
							else 
							{
								qse_mbscpy (&ep[1], p);
								qse_mbscpy (o, sp);
								o[ep - sp + 1] = QSE_MT('\0');
								p = &o[ep - sp + 2];
							}
							cnt++;
							/* last empty field after delim */
							if (*p == QSE_MT('\0')) cnt++;
							goto exit_point;
						}
					}

					if (!QSE_ISMSPACE (*p)) 
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

int qse_mbsspl (
	qse_mchar_t* s, const qse_mchar_t* delim,
	qse_mchar_t lquote, qse_mchar_t rquote, qse_mchar_t escape)
{
	return qse_mbsspltrn (s, delim, lquote, rquote, escape, QSE_NULL);
}

int qse_wcsspltrn (
	qse_wchar_t* s, const qse_wchar_t* delim,
	qse_wchar_t lquote, qse_wchar_t rquote, 
	qse_wchar_t escape, const qse_wchar_t* trset)
{
	qse_wchar_t* p = s, *d;
	qse_wchar_t* sp = QSE_NULL, * ep = QSE_NULL;
	int delim_mode;
	int cnt = 0;

	if (delim == QSE_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = (qse_wchar_t*)delim; *d != QSE_WT('\0'); d++)
			if (!QSE_ISWSPACE(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{
		/* skip preceding space characters */
		while (QSE_ISWSPACE(*p)) p++;

		/* when 0 is given as "delim", it has an effect of cutting
		   preceding and trailing space characters off "s". */
		if (lquote != QSE_WT('\0') && *p == lquote) 
		{
			qse_wcscpy (p, p + 1);

			for (;;) 
			{
				if (*p == QSE_WT('\0')) return -1;

				if (escape != QSE_WT('\0') && *p == escape) 
				{
					if (trset != QSE_NULL && p[1] != QSE_WT('\0'))
					{
						const qse_wchar_t* ep = trset;
						while (*ep != QSE_WT('\0'))
						{
							if (p[1] == *ep++) 
							{
								p[1] = *ep;
								break;
							}
						}
					}

					qse_wcscpy (p, p + 1);
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
			while (QSE_ISWSPACE(*p)) p++;
			if (*p != QSE_WT('\0')) return -1;

			if (sp == 0 && ep == 0) s[0] = QSE_WT('\0');
			else 
			{
				ep[1] = QSE_WT('\0');
				if (s != (qse_wchar_t*)sp) qse_wcscpy (s, sp);
				cnt++;
			}
		}
		else 
		{
			while (*p) 
			{
				if (!QSE_ISWSPACE(*p)) 
				{
					if (sp == 0) sp = p;
					ep = p;
				}
				p++;
			}

			if (sp == 0 && ep == 0) s[0] = QSE_WT('\0');
			else 
			{
				ep[1] = QSE_WT('\0');
				if (s != (qse_wchar_t*)sp) qse_wcscpy (s, sp);
				cnt++;
			}
		}
	}
	else if (delim_mode == 1) 
	{
		qse_wchar_t* o;

		while (*p) 
		{
			o = p;
			while (QSE_ISWSPACE(*p)) p++;
			if (o != p) { qse_wcscpy (o, p); p = o; }

			if (lquote != QSE_WT('\0') && *p == lquote) 
			{
				qse_wcscpy (p, p + 1);

				for (;;) 
				{
					if (*p == QSE_WT('\0')) return -1;

					if (escape != QSE_WT('\0') && *p == escape) 
					{
						if (trset != QSE_NULL && p[1] != QSE_WT('\0'))
						{
							const qse_wchar_t* ep = trset;
							while (*ep != QSE_WT('\0'))
							{
								if (p[1] == *ep++) 
								{
									p[1] = *ep;
									break;
								}
							}
						}
						qse_wcscpy (p, p + 1);
					}
					else 
					{
						if (*p == rquote) 
						{
							*p++ = QSE_WT('\0');
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
					if (*p == QSE_WT('\0')) 
					{
						if (o != p) cnt++;
						break;
					}
					if (QSE_ISWSPACE (*p)) 
					{
						*p++ = QSE_WT('\0');
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
		qse_wchar_t* o;
		int ok;

		while (*p != QSE_WT('\0')) 
		{
			o = p;
			while (QSE_ISWSPACE(*p)) p++;
			if (o != p) { qse_wcscpy (o, p); p = o; }

			if (lquote != QSE_WT('\0') && *p == lquote) 
			{
				qse_wcscpy (p, p + 1);

				for (;;) 
				{
					if (*p == QSE_WT('\0')) return -1;

					if (escape != QSE_WT('\0') && *p == escape) 
					{
						if (trset != QSE_NULL && p[1] != QSE_WT('\0'))
						{
							const qse_wchar_t* ep = trset;
							while (*ep != QSE_WT('\0'))
							{
								if (p[1] == *ep++) 
								{
									p[1] = *ep;
									break;
								}
							}
						}

						qse_wcscpy (p, p + 1);
					}
					else 
					{
						if (*p == rquote) 
						{
							*p++ = QSE_WT('\0');
							cnt++;
							break;
						}
					}
					p++;
				}

				ok = 0;
				while (QSE_ISWSPACE(*p)) p++;
				if (*p == QSE_WT('\0')) ok = 1;
				for (d = (qse_wchar_t*)delim; *d != QSE_WT('\0'); d++) 
				{
					if (*p == *d) 
					{
						ok = 1;
						qse_wcscpy (p, p + 1);
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
					if (*p == QSE_WT('\0')) 
					{
						if (ep) 
						{
							ep[1] = QSE_WT('\0');
							p = &ep[1];
						}
						cnt++;
						break;
					}
					for (d = (qse_wchar_t*)delim; *d != QSE_WT('\0'); d++) 
					{
						if (*p == *d)  
						{
							if (sp == QSE_NULL) 
							{
								qse_wcscpy (o, p); p = o;
								*p++ = QSE_WT('\0');
							}
							else 
							{
								qse_wcscpy (&ep[1], p);
								qse_wcscpy (o, sp);
								o[ep - sp + 1] = QSE_WT('\0');
								p = &o[ep - sp + 2];
							}
							cnt++;
							/* last empty field after delim */
							if (*p == QSE_WT('\0')) cnt++;
							goto exit_point;
						}
					}

					if (!QSE_ISWSPACE (*p)) 
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

int qse_wcsspl (
	qse_wchar_t* s, const qse_wchar_t* delim,
	qse_wchar_t lquote, qse_wchar_t rquote, qse_wchar_t escape)
{
	return qse_wcsspltrn (s, delim, lquote, rquote, escape, QSE_NULL);
}
