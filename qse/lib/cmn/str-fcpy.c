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

#undef T
#undef char_t
#undef cstr_t
#undef strfcpy
#undef strfncpy
#undef strxfcpy
#undef strxfncpy

#define T(x) QSE_MT(x)
#define char_t qse_mchar_t
#define cstr_t qse_mcstr_t
#define strfcpy qse_mbsfcpy
#define strfncpy qse_mbsfncpy
#define strxfcpy qse_mbsxfcpy
#define strxfncpy qse_mbsxfncpy
#include "str-fcpy.h"

/* ----------------------------------- */

#undef T
#undef char_t
#undef cstr_t
#undef strfcpy
#undef strfncpy
#undef strxfcpy
#undef strxfncpy

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define cstr_t qse_wcstr_t
#define strfcpy qse_wcsfcpy
#define strfncpy qse_wcsfncpy
#define strxfcpy qse_wcsxfcpy
#define strxfncpy qse_wcsxfncpy
#include "str-fcpy.h"

