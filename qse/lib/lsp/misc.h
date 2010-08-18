/*
 * $Id: misc.h 117 2008-03-03 11:20:05Z baconevi $
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

#ifndef _QSE_LIB_LSP_MISC_H_
#define _QSE_LIB_LSP_MISC_H_

#ifndef _QSE_LSP_LSP_H_
#error Never include this file directly. Include <qse/lsp/lsp.h> instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* qse_lsp_memcpy (void* dst, const void* src, qse_size_t n);
void* qse_lsp_memset (void* dst, int val, qse_size_t n);

#ifdef __cplusplus
}
#endif

#endif

