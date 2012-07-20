/*
 * $Id: misc.h 135 2009-05-15 13:31:43Z baconevi $
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

#ifndef _QSE_LIB_AWK_ERR_H_
#define _QSE_LIB_AWK_ERR_H_

#ifdef __cplusplus
extern "C" {
#endif

const qse_char_t* qse_awk_dflerrstr (qse_awk_t* awk, qse_awk_errnum_t errnum);

#ifdef __cplusplus
}
#endif

#endif

