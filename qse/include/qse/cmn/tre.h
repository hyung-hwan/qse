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

#ifndef _QSE_TRE_TRE_H_
#define _QSE_TRE_TRE_H_

#include <qse/types.h>
#include <qse/macros.h>

enum qse_tre_errnum_t
{
	QSE_TRE_ENOERR,
	QSE_TRE_EOTHER,
	QSE_TRE_ENOIMPL,
	QSE_TRE_ESYSERR,
	QSE_TRE_EINTERN,

	QSE_TRE_ENOMEM,   /* Out of memory */
	QSE_TRE_ENOMATCH, /* No match */
	QSE_TRE_EBADPAT,  /* Invalid regular expression */
	QSE_TRE_ECOLLATE, /* Unknown collating element */
	QSE_TRE_ECTYPE,   /* Unknown character class name */
	QSE_TRE_EESCAPE,  /* Traling backslash */
	QSE_TRE_ESUBREG,  /* Invalid backreference */
	QSE_TRE_EBRACK,   /* "[]" imbalance */
	QSE_TRE_EPAREN,   /* "\(\)" or "()" imbalance */
	QSE_TRE_EBRACE,   /* "\{\}" or "{}" imbalance */
	QSE_TRE_EBADBR,   /* Invalid content of {} */
	QSE_TRE_ERANGE,   /* Invalid use of range operator */
	QSE_TRE_EBADRPT   /* Invalid use of repetition operators */
};
typedef enum qse_tre_errnum_t qse_tre_errnum_t;

typedef struct qse_tre_t qse_tre_t;
struct qse_tre_t
{
	qse_mmgr_t* mmgr;
	qse_tre_errnum_t errnum; 

	qse_size_t       re_nsub;  /* Number of parenthesized subexpressions. */
	void*            value;	 /* For internal use only. */
};

#define QSE_TRE_ERRNUM(tre) ((const qse_tre_errnum_t)((tre)->errnum))

typedef int qse_tre_off_t;

typedef struct qse_tre_match_t qse_tre_match_t;
struct qse_tre_match_t
{
	qse_tre_off_t rm_so;
	qse_tre_off_t rm_eo;
};

enum qse_tre_cflag_t
{
	QSE_TRE_EXTENDED    = (1 << 0),
	QSE_TRE_IGNORECASE  = (1 << 1),
	QSE_TRE_NEWLINE     = (1 << 2),
	QSE_TRE_NOSUBREG    = (1 << 3),
	QSE_TRE_LITERAL     = (1 << 4),
	QSE_TRE_RIGHTASSOC  = (1 << 5),
	QSE_TRE_UNGREEDY    = (1 << 6),

	/* Disable {n,m} occrrence specifier 
	 * in the QSE_TRE_EXTENDED mode.
	 * it doesn't affect the BRE's \{\}. */
	QSE_TRE_NOBOUND     = (1 << 7),

	/* Enable non-standard extensions:
	 *  - Enable (?:text) for no submatch backreference.
	 *  - Enable perl-like (?...) extensions like (?i) 
	 *    if QSE_TRE_EXTENDED is also set.
	 */
	QSE_TRE_NONSTDEXT   = (1 << 8) 
};

enum qse_tre_eflag_t
{
	QSE_TRE_BACKTRACKING = (1 << 0),
	QSE_TRE_NOTBOL       = (1 << 1),
	QSE_TRE_NOTEOL       = (1 << 2)
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT qse_tre_t* qse_tre_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize
);
	
QSE_EXPORT void qse_tre_close (
	qse_tre_t*  tre
);

QSE_EXPORT int qse_tre_init (
	qse_tre_t*  tre,
	qse_mmgr_t* mmgr
);

QSE_EXPORT void qse_tre_fini (
	qse_tre_t* tre
);

QSE_EXPORT qse_mmgr_t* qse_tre_getmmgr (
	qse_tre_t* tre
);

QSE_EXPORT void* qse_tre_getxtn (
	qse_tre_t* tre
);

QSE_EXPORT qse_tre_errnum_t qse_tre_geterrnum (
	qse_tre_t* tre
);

QSE_EXPORT const qse_char_t* qse_tre_geterrmsg (
	qse_tre_t* tre
);

QSE_EXPORT int qse_tre_compx (
	qse_tre_t*        tre,
	const qse_char_t* regex,
	qse_size_t        n,
	unsigned int*     nsubmat,
	int               cflags
);

QSE_EXPORT int qse_tre_comp (
	qse_tre_t*        tre,
	const qse_char_t* regex,
	unsigned int*     nsubmat,
	int               cflags
);

QSE_EXPORT int qse_tre_execx (
	qse_tre_t*        tre,
	const qse_char_t* str,
	qse_size_t        len,
	qse_tre_match_t*  pmatch,
	qse_size_t        nmatch,
	int               eflags
);

QSE_EXPORT int qse_tre_exec (
	qse_tre_t*        tre,
	const qse_char_t* str,
	qse_tre_match_t*  pmatch,
	qse_size_t        nmatch,
	int               eflags
);

#ifdef __cplusplus
}
#endif

#endif
