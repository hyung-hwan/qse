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

#ifndef _QSE_CMN_NWAD_H_
#define _QSE_CMN_NWAD_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/ipad.h>

struct qse_nwad_t
{
	enum
	{
		QSE_NWAD_IP4,
		QSE_NWAD_IP6
	} type;

	union
	{
		struct
		{
			qse_ipad4_t  addr;
			qse_uint16_t port;
		} ip4;

		struct
		{
			qse_ipad6_t  addr;
			qse_uint16_t port;
		} ip6;	
	} u;	
};

#endif
