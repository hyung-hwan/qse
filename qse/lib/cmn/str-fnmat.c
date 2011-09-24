/*
 * $Id$
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

#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>

/* ------------------------------------------------------------------------- */
/* | MBS VERSION                                                           | */
/* ------------------------------------------------------------------------- */

/* separator to use in a pattern. 
 * this also matches a backslash in non-unix OS where a blackslash 
 * is used as a path separator. MBS_SEPCHS defines OS path separators. */
#define MBS_SEP QSE_MT('/') 
#define MBS_ESC QSE_MT('\\')
#define MBS_DOT QSE_MT('.')

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__) 
#	define MBS_ISSEPCH(c) ((c) == QSE_MT('/') || (c) == QSE_MT('\\'))
#	define MBS_SEPCHS QSE_MT("/\\")
#else
#	define MBS_ISSEPCH(c) ((c) == QSE_MT('/'))
#	define MBS_SEPCHS QSE_MT("/")
#endif

static int __mbsxnfnmat (
	const qse_mchar_t* str, qse_size_t slen,
	const qse_mchar_t* ptn, qse_size_t plen, int flags, int no_first_period)
{
	const qse_mchar_t* sp = str;
	const qse_mchar_t* pp = ptn;
	const qse_mchar_t* se = str + slen;
	const qse_mchar_t* pe = ptn + plen;
	qse_mchar_t sc, pc, pc2;

	while (1) 
	{
		if (pp < pe && *pp == MBS_ESC && 
		    !(flags & QSE_MBSFNMAT_NOESCAPE)) 
		{
			/* pattern is escaped and escaping is allowed. */

			if ((++pp) >= pe) 
			{
				/* 
				 * the last character of the pattern is an MBS_ESC. 
				 * matching is performed as if the end of the pattern is
				 * reached just without an MBS_ESC.
				 */
				if (sp < se) return 0;
				return 1;
			}

			if (sp >= se) return 0; /* premature string termination */

			sc = *sp; pc = *pp; /* pc is just a normal character */
			if ((flags & QSE_MBSFNMAT_IGNORECASE) != 0) 
			{
				/* make characters to lower-case */
				sc = QSE_TOMLOWER(sc);
				pc = QSE_TOMLOWER(pc); 
			}

			if (sc != pc) return 0;
			sp++; pp++; 
			continue;
		}
		if (pp >= pe) 
		{
			/* 
			 * the end of the pattern has been reached. 
			 * the string must terminate too.
			 */
			return sp >= se;
		}

		if (sp >= se) 
		{
			/* the string terminats prematurely */
			while (pp < pe && *pp == QSE_MT('*')) pp++;
			return pp >= pe;
		}

		sc = *sp; pc = *pp;

		if (sc == MBS_DOT && (flags & QSE_MBSFNMAT_PERIOD)) 
		{
			/* 
			 * a leading period in the staring must match 
			 * a period in the pattern explicitly 
			 */
			if ((!no_first_period && sp == str) || 
			    (MBS_ISSEPCH(sp[-1]) && (flags & QSE_MBSFNMAT_PATHNAME))) 
			{
				if (pc != MBS_DOT) return 0;
				sp++; pp++;
				continue;
			}
		}
		else if (MBS_ISSEPCH(sc) && (flags & QSE_MBSFNMAT_PATHNAME)) 
		{
			while (pc == QSE_MT('*')) 
			{
				if ((++pp) >= pe) return 0;
				pc = *pp;
			}

			/* a path separator must be matched explicitly */
			if (pc != MBS_SEP) return 0;
			sp++; pp++;
			continue;
		}

		/* the handling of special pattern characters begins here */
		if (pc == QSE_MT('?')) 
		{
			/* match any single character */
			sp++; pp++; 
		} 
		else if (pc == QSE_MT('*')) 
		{ 
			/* match zero or more characters */

			/* compact asterisks */
			do { pp++; } while (pp < pe && *pp == QSE_MT('*'));

			if (pp >= pe) 
			{
				/* 
				 * if the last character in the pattern is an asterisk,
				 * the string should not have any directory separators
				 * when QSE_MBSFNMAT_PATHNAME is set.
				 */
				if (flags & QSE_MBSFNMAT_PATHNAME)
				{
					if (qse_mbsxpbrk(sp, se-sp, MBS_SEPCHS) != QSE_NULL) return 0;
				}
				return 1;
			}
			else 
			{
				do 
				{
					if (__mbsxnfnmat(sp, se - sp, pp, pe - pp, flags, 1)) 
					{ 
						return 1;
					}

					if (MBS_ISSEPCH(*sp) && 
					    (flags & QSE_MBSFNMAT_PATHNAME)) break;

					sp++;
				} 
				while (sp < se);

				return 0;
			}
		}
		else if (pc == QSE_MT('[')) 
		{
			/* match range */
			int negate = 0;
			int matched = 0;

			if ((++pp) >= pe) return 0;
			if (*pp == QSE_MT('!')) { negate = 1; pp++; } 

			while (pp < pe && *pp != QSE_MT(']')) 
			{
				if (*pp == QSE_MT('[')) 
				{
					qse_size_t pl = pe - pp;

					if (pl >= 10) 
					{
						if (qse_mbszcmp(pp, QSE_MT("[:xdigit:]"), 10) == 0) 
						{
							matched = QSE_ISMXDIGIT(sc);
							pp += 10; continue;
						}
					}

					if (pl >= 9) 
					{
						if (qse_mbszcmp(pp, QSE_MT("[:upper:]"), 9) == 0) 
						{
							matched = QSE_ISMUPPER(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:lower:]"), 9) == 0) 
						{
							matched = QSE_ISMLOWER(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:alpha:]"), 9) == 0) 
						{
							matched = QSE_ISMALPHA(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:digit:]"), 9) == 0) 
						{
							matched = QSE_ISMDIGIT(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:alnum:]"), 9) == 0) 
						{
							matched = QSE_ISMALNUM(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:space:]"), 9) == 0) 
						{
							matched = QSE_ISMSPACE(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:print:]"), 9) == 0) 
						{
							matched = QSE_ISMPRINT(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:graph:]"), 9) == 0) 
						{
							matched = QSE_ISMGRAPH(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:cntrl:]"), 9) == 0) 
						{
							matched = QSE_ISMCNTRL(sc);
							pp += 9; continue;
						}
						else if (qse_mbszcmp(pp, QSE_MT("[:punct:]"), 9) == 0) 
						{
							matched = QSE_ISMPUNCT(sc);
							pp += 9; continue;
						}
					}

					/* 
					 * characters in an invalid class name are 
					 * just treated as normal characters 
					 */
				}

				if (*pp == MBS_ESC && 
				    !(flags & QSE_MBSFNMAT_NOESCAPE)) pp++;
				else if (*pp == QSE_MT(']')) break;

				if (pp >= pe) break;

				pc = *pp;
				if ((flags & QSE_MBSFNMAT_IGNORECASE) != 0) 
				{
					sc = QSE_TOMLOWER(sc); 
					pc = QSE_TOMLOWER(pc); 
				}

				if (pp + 1 < pe && pp[1] == QSE_MT('-')) 
				{
					pp += 2; /* move the a character next to a dash */

					if (pp >= pe) 
					{
						if (sc >= pc) matched = 1;
						break;
					}

					if (*pp == MBS_ESC && 
					    !(flags & QSE_MBSFNMAT_NOESCAPE)) 
					{
						if ((++pp) >= pe) 
						{
							if (sc >= pc) matched = 1;
							break;
						}
					}
					else if (*pp == QSE_MT(']')) 
					{
						if (sc >= pc) matched = 1;
						break;
					}

					pc2 = *pp;
					if ((flags & QSE_MBSFNMAT_IGNORECASE) != 0) 
						pc2 = QSE_TOMLOWER(pc2); 

					if (sc >= pc && sc <= pc2) matched = 1;
					pp++;
				}
				else 
				{
					if (sc == pc) matched = 1;
					pp++;
				}
			}

			if (negate) matched = !matched;
			if (!matched) return 0;
			sp++; if (pp < pe) pp++;
		}
		else 
		{
			/* a normal character */
			if ((flags & QSE_MBSFNMAT_IGNORECASE) != 0) 
			{
				sc = QSE_TOMLOWER(sc); 
				pc = QSE_TOMLOWER(pc); 
			}

			if (sc != pc) return 0;
			sp++; pp++;
		}
	}

	/* will never reach here. but make some immature compilers happy... */
	return 0;
}

int qse_mbsfnmat (const qse_mchar_t* str, const qse_mchar_t* ptn, int flags)
{
	return __mbsxnfnmat (
		str, qse_mbslen(str), ptn, qse_mbslen(ptn), flags, 0);
}

int qse_mbsxfnmat  (
	const qse_mchar_t* str, qse_size_t slen, const qse_mchar_t* ptn, int flags)
{
	return __mbsxnfnmat (str, slen, ptn, qse_mbslen(ptn), flags, 0);
}

int qse_mbsnfnmat  (
	const qse_mchar_t* str, const qse_mchar_t* ptn, qse_size_t plen, int flags)
{
	return __mbsxnfnmat (str, qse_mbslen(str), ptn, plen, flags, 0);
}

int qse_mbsxnfnmat (
	const qse_mchar_t* str, qse_size_t slen,
	const qse_mchar_t* ptn, qse_size_t plen, int flags)
{
	return __mbsxnfnmat (str, slen, ptn, plen, flags, 0);
}

/* ------------------------------------------------------------------------- */
/* | WCS VERSION                                                           | */
/* ------------------------------------------------------------------------- */

/* separator to use in a pattern. 
 * this also matches a backslash in non-unix OS where a blackslash 
 * is used as a path separator. WCS_SEPCHS defines OS path separators. */
#define WCS_SEP QSE_WT('/') 
#define WCS_ESC QSE_WT('\\')
#define WCS_DOT QSE_WT('.')

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__) 
#	define WCS_ISSEPCH(c) ((c) == QSE_WT('/') || (c) == QSE_WT('\\'))
#	define WCS_SEPCHS QSE_WT("/\\")
#else
#	define WCS_ISSEPCH(c) ((c) == QSE_WT('/'))
#	define WCS_SEPCHS QSE_WT("/")
#endif

static int __wcsxnfnmat (
	const qse_wchar_t* str, qse_size_t slen,
	const qse_wchar_t* ptn, qse_size_t plen, int flags, int no_first_period)
{
	const qse_wchar_t* sp = str;
	const qse_wchar_t* pp = ptn;
	const qse_wchar_t* se = str + slen;
	const qse_wchar_t* pe = ptn + plen;
	qse_wchar_t sc, pc, pc2;

	while (1) 
	{
		if (pp < pe && *pp == WCS_ESC && 
		    !(flags & QSE_WCSFNMAT_NOESCAPE)) 
		{
			/* pattern is escaped and escaping is allowed. */

			if ((++pp) >= pe) 
			{
				/* 
				 * the last character of the pattern is an WCS_ESC. 
				 * matching is performed as if the end of the pattern is
				 * reached just without an WCS_ESC.
				 */
				if (sp < se) return 0;
				return 1;
			}

			if (sp >= se) return 0; /* premature string termination */

			sc = *sp; pc = *pp; /* pc is just a normal character */
			if ((flags & QSE_WCSFNMAT_IGNORECASE) != 0) 
			{
				/* make characters to lower-case */
				sc = QSE_TOWLOWER(sc);
				pc = QSE_TOWLOWER(pc); 
			}

			if (sc != pc) return 0;
			sp++; pp++; 
			continue;
		}
		if (pp >= pe) 
		{
			/* 
			 * the end of the pattern has been reached. 
			 * the string must terminate too.
			 */
			return sp >= se;
		}

		if (sp >= se) 
		{
			/* the string terminats prematurely */
			while (pp < pe && *pp == QSE_WT('*')) pp++;
			return pp >= pe;
		}

		sc = *sp; pc = *pp;

		if (sc == WCS_DOT && (flags & QSE_WCSFNMAT_PERIOD)) 
		{
			/* 
			 * a leading period in the staring must match 
			 * a period in the pattern explicitly 
			 */
			if ((!no_first_period && sp == str) || 
			    (WCS_ISSEPCH(sp[-1]) && (flags & QSE_WCSFNMAT_PATHNAME))) 
			{
				if (pc != WCS_DOT) return 0;
				sp++; pp++;
				continue;
			}
		}
		else if (WCS_ISSEPCH(sc) && (flags & QSE_WCSFNMAT_PATHNAME)) 
		{
			while (pc == QSE_WT('*')) 
			{
				if ((++pp) >= pe) return 0;
				pc = *pp;
			}

			/* a path separator must be matched explicitly */
			if (pc != WCS_SEP) return 0;
			sp++; pp++;
			continue;
		}

		/* the handling of special pattern characters begins here */
		if (pc == QSE_WT('?')) 
		{
			/* match any single character */
			sp++; pp++; 
		} 
		else if (pc == QSE_WT('*')) 
		{ 
			/* match zero or more characters */

			/* compact asterisks */
			do { pp++; } while (pp < pe && *pp == QSE_WT('*'));

			if (pp >= pe) 
			{
				/* 
				 * if the last character in the pattern is an asterisk,
				 * the string should not have any directory separators
				 * when QSE_WCSFNMAT_PATHNAME is set.
				 */
				if (flags & QSE_WCSFNMAT_PATHNAME)
				{
					if (qse_wcsxpbrk(sp, se-sp, WCS_SEPCHS) != QSE_NULL) return 0;
				}
				return 1;
			}
			else 
			{
				do 
				{
					if (__wcsxnfnmat(sp, se - sp, pp, pe - pp, flags, 1)) 
					{ 
						return 1;
					}

					if (WCS_ISSEPCH(*sp) && 
					    (flags & QSE_WCSFNMAT_PATHNAME)) break;

					sp++;
				} 
				while (sp < se);

				return 0;
			}
		}
		else if (pc == QSE_WT('[')) 
		{
			/* match range */
			int negate = 0;
			int matched = 0;

			if ((++pp) >= pe) return 0;
			if (*pp == QSE_WT('!')) { negate = 1; pp++; } 

			while (pp < pe && *pp != QSE_WT(']')) 
			{
				if (*pp == QSE_WT('[')) 
				{
					qse_size_t pl = pe - pp;

					if (pl >= 10) 
					{
						if (qse_wcszcmp(pp, QSE_WT("[:xdigit:]"), 10) == 0) 
						{
							matched = QSE_ISWXDIGIT(sc);
							pp += 10; continue;
						}
					}

					if (pl >= 9) 
					{
						if (qse_wcszcmp(pp, QSE_WT("[:upper:]"), 9) == 0) 
						{
							matched = QSE_ISWUPPER(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:lower:]"), 9) == 0) 
						{
							matched = QSE_ISWLOWER(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:alpha:]"), 9) == 0) 
						{
							matched = QSE_ISWALPHA(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:digit:]"), 9) == 0) 
						{
							matched = QSE_ISWDIGIT(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:alnum:]"), 9) == 0) 
						{
							matched = QSE_ISWALNUM(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:space:]"), 9) == 0) 
						{
							matched = QSE_ISWSPACE(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:print:]"), 9) == 0) 
						{
							matched = QSE_ISWPRINT(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:graph:]"), 9) == 0) 
						{
							matched = QSE_ISWGRAPH(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:cntrl:]"), 9) == 0) 
						{
							matched = QSE_ISWCNTRL(sc);
							pp += 9; continue;
						}
						else if (qse_wcszcmp(pp, QSE_WT("[:punct:]"), 9) == 0) 
						{
							matched = QSE_ISWPUNCT(sc);
							pp += 9; continue;
						}
					}

					/* 
					 * characters in an invalid class name are 
					 * just treated as normal characters 
					 */
				}

				if (*pp == WCS_ESC && 
				    !(flags & QSE_WCSFNMAT_NOESCAPE)) pp++;
				else if (*pp == QSE_WT(']')) break;

				if (pp >= pe) break;

				pc = *pp;
				if ((flags & QSE_WCSFNMAT_IGNORECASE) != 0) 
				{
					sc = QSE_TOWLOWER(sc); 
					pc = QSE_TOWLOWER(pc); 
				}

				if (pp + 1 < pe && pp[1] == QSE_WT('-')) 
				{
					pp += 2; /* move the a character next to a dash */

					if (pp >= pe) 
					{
						if (sc >= pc) matched = 1;
						break;
					}

					if (*pp == WCS_ESC && 
					    !(flags & QSE_WCSFNMAT_NOESCAPE)) 
					{
						if ((++pp) >= pe) 
						{
							if (sc >= pc) matched = 1;
							break;
						}
					}
					else if (*pp == QSE_WT(']')) 
					{
						if (sc >= pc) matched = 1;
						break;
					}

					pc2 = *pp;
					if ((flags & QSE_WCSFNMAT_IGNORECASE) != 0) 
						pc2 = QSE_TOWLOWER(pc2); 

					if (sc >= pc && sc <= pc2) matched = 1;
					pp++;
				}
				else 
				{
					if (sc == pc) matched = 1;
					pp++;
				}
			}

			if (negate) matched = !matched;
			if (!matched) return 0;
			sp++; if (pp < pe) pp++;
		}
		else 
		{
			/* a normal character */
			if ((flags & QSE_WCSFNMAT_IGNORECASE) != 0) 
			{
				sc = QSE_TOWLOWER(sc); 
				pc = QSE_TOWLOWER(pc); 
			}

			if (sc != pc) return 0;
			sp++; pp++;
		}
	}

	/* will never reach here. but make some immature compilers happy... */
	return 0;
}

int qse_wcsfnmat (const qse_wchar_t* str, const qse_wchar_t* ptn, int flags)
{
	return __wcsxnfnmat (
		str, qse_wcslen(str), ptn, qse_wcslen(ptn), flags, 0);
}

int qse_wcsxfnmat  (
	const qse_wchar_t* str, qse_size_t slen, const qse_wchar_t* ptn, int flags)
{
	return __wcsxnfnmat (str, slen, ptn, qse_wcslen(ptn), flags, 0);
}

int qse_wcsnfnmat  (
	const qse_wchar_t* str, const qse_wchar_t* ptn, qse_size_t plen, int flags)
{
	return __wcsxnfnmat (str, qse_wcslen(str), ptn, plen, flags, 0);
}

int qse_wcsxnfnmat (
	const qse_wchar_t* str, qse_size_t slen,
	const qse_wchar_t* ptn, qse_size_t plen, int flags)
{
	return __wcsxnfnmat (str, slen, ptn, plen, flags, 0);
}
