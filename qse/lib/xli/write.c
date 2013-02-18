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

#include "xli.h"

int qse_xli_write (qse_xli_t* xli, qse_xli_io_impl_t io)
{
	if (io == QSE_NULL)
	{
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
		return -1;
	}

	/* TODO: write data to io stream */
	qse_xli_seterrnum (xli, QSE_XLI_ENOIMPL, QSE_NULL);
	return -1;
}

