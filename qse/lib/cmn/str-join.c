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

#include <qse/cmn/str.h>
#include <stdarg.h>

/* ----------------------------------- */

#undef char_t
#undef strjoin
#undef strjoinv
#undef strxjoin
#undef strxjoinv
#undef strcpy
#undef strxcpy

#define char_t qse_mchar_t
#define strjoin qse_mbsjoin
#define strjoinv qse_mbsjoinv
#define strxjoin qse_mbsxjoin
#define strxjoinv qse_mbsxjoinv
#define strcpy qse_mbscpy
#define strxcpy qse_mbsxcpy
#include "str-join.h"

/* ----------------------------------- */

#undef char_t
#undef strjoin
#undef strjoinv
#undef strxjoin
#undef strxjoinv
#undef strcpy
#undef strxcpy

#define char_t qse_wchar_t
#define strjoin qse_wcsjoin
#define strjoinv qse_wcsjoinv
#define strxjoin qse_wcsxjoin
#define strxjoinv qse_wcsxjoinv
#define strcpy qse_wcscpy
#define strxcpy qse_wcsxcpy
#include "str-join.h"

#undef char_t
#undef strjoin
#undef strjoinv
#undef strxjoin
#undef strxjoinv
#undef strcpy
#undef strxcpy

#define char_t qse_char_t
#define strjoin qse_strjoin
#define strjoinv qse_strjoinv
#define strxjoin qse_strxjoin
#define strxjoinv qse_strxjoinv
#define strcpy qse_strcpy
#define strxcpy qse_strxcpy
#include "str-join.h"
