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

/* ----------------------------------- */

#undef char_t
#undef xstr_t
#undef T
#undef NOBUF
#undef strlen
#undef scan_dollar
#undef expand_dollar
#undef subst_t
#undef strxsubst
#undef strxnsubst

#define char_t qse_mchar_t
#define xstr_t qse_mxstr_t
#define T(x) QSE_MT(x)
#define NOBUF QSE_MBSSUBST_NOBUF
#define strlen qse_mbslen
#define scan_dollar mbs_scan_dollar
#define expand_dollar mbs_expand_dollar
#define subst_t qse_mbssubst_t
#define strxsubst qse_mbsxsubst
#define strxnsubst qse_mbsxnsubst
#include "str-subst.h"

/* ----------------------------------- */

#undef char_t
#undef xstr_t
#undef T
#undef NOBUF
#undef strlen
#undef scan_dollar
#undef expand_dollar
#undef subst_t
#undef strxsubst
#undef strxnsubst

#define char_t qse_wchar_t
#define xstr_t qse_wxstr_t
#define T(x) QSE_WT(x)
#define NOBUF QSE_WCSSUBST_NOBUF
#define strlen qse_wcslen
#define scan_dollar wcs_scan_dollar
#define expand_dollar wcs_expand_dollar
#define subst_t qse_wcssubst_t
#define strxsubst qse_wcsxsubst
#define strxnsubst qse_wcsxnsubst
#include "str-subst.h"

