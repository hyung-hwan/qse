/*
 * $Id
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

#ifndef _QSE_FS_PATH_H_
#define _QSE_FS_PATH_H_

#include <qse/types.h>
#include <qse/macros.h>

#ifdef __cplusplus
extern "C" {
#endif

qse_size_t qse_canonpath (
	const qse_char_t* path,
	qse_char_t*       canon
);

qse_size_t qse_realpath (
	const qse_char_t* path,
	qse_char_t* real
);

#ifdef __cplusplus
}
#endif

#endif
