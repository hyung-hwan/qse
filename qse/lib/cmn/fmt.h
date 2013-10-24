/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#ifndef _QSE_LIB_CMN_FMT_H_
#define _QSE_LIB_CMN_FMT_H_

#include <qse/cmn/fmt.h>
#include <stdarg.h>

typedef int (*qse_fmtout_mchar_t) (
	qse_mchar_t c,
	void*       ctx
);

typedef int (*qse_fmtout_wchar_t) (
	qse_wchar_t c,
	void*       ctx
);

struct qse_fmtout_t
{
	qse_size_t         count;     /* out */
	qse_size_t         limit;     /* in */
	void*              ctx;       /* in */
	qse_fmtout_mchar_t put_mchar; /* in */
	qse_fmtout_wchar_t put_wchar; /* in */
};

typedef struct qse_fmtout_t qse_fmtout_t;

#ifdef __cplusplus
extern "C" {
#endif

int qse_mfmtout (
	const qse_mchar_t* fmt,
	qse_fmtout_t*      data,
	va_list            ap
);

int qse_wfmtout (
	const qse_wchar_t* fmt,
	qse_fmtout_t*      data,
	va_list            ap
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_fmtout(fmt,fo,ap) qse_mfmtout(fmt,fo,ap)
#else
#	define qse_fmtout(fmt,fo,ap) qse_wfmtout(fmt,fo,ap)
#endif

#ifdef __cplusplus
}
#endif

#endif
