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

#ifndef _QSE_CMN_UNI_H_
#define _QSE_CMN_UNI_H_


/** @file
 * This file provides functions, types, macros for unicode handling.
 */

#include <qse/types.h>
#include <qse/macros.h>


#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT int qse_isunitype (qse_wcint_t c, int type);
QSE_EXPORT int qse_isuniupper (qse_wcint_t c);
QSE_EXPORT int qse_isunilower (qse_wcint_t c);
QSE_EXPORT int qse_isunialpha (qse_wcint_t c);
QSE_EXPORT int qse_isunidigit (qse_wcint_t c);
QSE_EXPORT int qse_isunixdigit (qse_wcint_t c);
QSE_EXPORT int qse_isunialnum (qse_wcint_t c);
QSE_EXPORT int qse_isunispace (qse_wcint_t c);
QSE_EXPORT int qse_isuniprint (qse_wcint_t c);
QSE_EXPORT int qse_isunigraph (qse_wcint_t c);
QSE_EXPORT int qse_isunicntrl (qse_wcint_t c);
QSE_EXPORT int qse_isunipunct (qse_wcint_t c);
QSE_EXPORT qse_wcint_t qse_touniupper (qse_wcint_t c);
QSE_EXPORT qse_wcint_t qse_tounilower (qse_wcint_t c);

#ifdef __cplusplus
}
#endif

#endif
