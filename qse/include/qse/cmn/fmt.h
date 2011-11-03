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

#ifndef _QSE_CMN_FMT_H_
#define _QSE_CMN_FMT_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file defines various formatting functions.
 */

enum qse_fmtulongtowcs_flag_t
{
	QSE_FMTULONGTOWCS_UPPERCASE = (0x100 << 0),
#define QSE_FMTULONGTOWCS_UPPERCASE QSE_FMTULONGTOWCS_UPPERCASE
	QSE_FMTULONGTOWCS_FILLRIGHT = (0x100 << 1)
#define QSE_FMTULONGTOWCS_FILLRIGHT QSE_FMTULONGTOWCS_FILLRIGHT
};

enum qse_fmtulongtombs_flag_t
{
	QSE_FMTULONGTOMBS_UPPERCASE = (0x100 << 0),
#define QSE_FMTULONGTOMBS_UPPERCASE QSE_FMTULONGTOMBS_UPPERCASE
	QSE_FMTULONGTOMBS_FILLRIGHT = (0x100 << 1)
#define QSE_FMTULONGTOMBS_FILLRIGHT QSE_FMTULONGTOMBS_FILLRIGHT
};

#ifdef QSE_CHAR_IS_MCHAR
#	define QSE_FMTULONG_UPPERCASE QSE_FMTULONGTOMBS_UPPERCASE
#	define QSE_FMTULONG_FILLRIGHT QSE_FMTULONGTOMBS_FILLRIGHT
#else
#	define QSE_FMTULONG_UPPERCASE QSE_FMTULONGTOWCS_UPPERCASE
#	define QSE_FMTULONG_FILLRIGHT QSE_FMTULONGTOWCS_FILLRIGHT
#endif

#ifdef __cplusplus
extern "C" {
#endif

qse_size_t qse_fmtulongtombs (
     qse_mchar_t* buf, 
	qse_size_t   size,
     qse_long_t   value, 
	int          base_and_flags, 
	qse_mchar_t  fill_char
);

qse_size_t qse_fmtulongtowcs (
     qse_wchar_t* buf, 
	qse_size_t   size,
     qse_long_t   value, 
	int          base_and_flags, 
	qse_wchar_t  fill_char
);

#ifdef QSE_CHAR_IS_MCHAR
#	define qse_fmtulong(b,sz,v,bf,fc) qse_fmtulongtombs(b,sz,v,bf,fc)
#else
#	define qse_fmtulong(b,sz,v,bf,fc) qse_fmtulongtowcs(b,sz,v,bf,fc)
#endif

#ifdef __cplusplus
}
#endif

#endif
