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

#ifndef _QSE_CMN_GLOB_H_
#define _QSE_CMN_GLOB_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef int (*qse_glob_cbfun_t) (
	const qse_cstr_t* path,
	void*             cbctx
);

enum qse_glob_flags_t
{
	QSE_GLOB_NOESCAPE = (1 << 0),
	QSE_GLOB_PERIOD   = (1 << 1)
};

#ifdef __cplusplus
extern "C" {
#endif

int qse_glob (
	const qse_char_t* pattern,
	qse_glob_cbfun_t  cbfun,
	void*             cbctx,
	int               flags,
	qse_mmgr_t*       mmgr
);

int qse_globwithcmgr (
	const qse_char_t* pattern,
	qse_glob_cbfun_t  cbfun,
	void*             cbctx,
	int               flags,
	qse_mmgr_t*       mmgr,
	qse_cmgr_t*       cmgr
);

#ifdef __cplusplus
}
#endif

#endif
