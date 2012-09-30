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

#include <qse/cmn/uri.h>
#include "mem.h"

int qse_mbstouri (const qse_mchar_t* str, qse_uri_t* uri, int flags)
{
	const qse_mchar_t* ptr, * colon;
	qse_uri_t xuri;

	QSE_MEMSET (&xuri, 0, QSE_SIZEOF(xuri));

	/* scheme */
	xuri.scheme.ptr = str;
	while (*str != QSE_MT(':')) 
	{
		if (*str == QSE_MT('\0')) return -1;
		str++;
	}
	xuri.scheme.len = str - (const qse_mchar_t*)xuri.scheme.ptr;

	str++; /* skip : */ 
	if (*str != QSE_MT('/')) return -1;
	str++; /* skip / */
	if (*str != QSE_MT('/')) return -1;
	str++; /* skip / */

	/* username, password, host, port */
	for (colon = QSE_NULL, ptr = str; ; str++)
	{
		if (flags & QSE_MBSTOURI_NOAUTH)
		{
			if (colon == QSE_NULL && *str == QSE_MT(':')) colon = str;
			else if (*str == QSE_MT('/') || *str == QSE_MT('\0')) 
			{
				if (colon)
				{
					xuri.host.ptr = ptr;
					xuri.host.len = colon - ptr;
					xuri.port.ptr = colon + 1;
					xuri.port.len = str - colon - 1;
				}
				else
				{
					xuri.host.ptr = ptr;
					xuri.host.len = str - ptr;
				}
				break;
			}
		}
		else
		{
			if (colon == QSE_NULL && *str == QSE_MT(':')) colon = str;
			else if (xuri.auth.user.ptr == QSE_NULL && *str == QSE_MT('@'))
			{
				if (colon)
				{
					xuri.auth.user.ptr = ptr;
					xuri.auth.user.len = colon - ptr;
					xuri.auth.pass.ptr = colon + 1;
					xuri.auth.pass.len = str - colon - 1;

					colon = QSE_NULL;
				}
				else
				{
					xuri.auth.user.ptr = ptr;
					xuri.auth.user.len = str - ptr;
				}

				ptr = str + 1;
			}
			else if (*str == QSE_MT('/') || *str == QSE_MT('\0')) 
			{
				if (colon)
				{
					xuri.host.ptr = ptr;
					xuri.host.len = colon - ptr;
					xuri.port.ptr = colon + 1;
					xuri.port.len = str - colon - 1;
				}
				else
				{
					xuri.host.ptr = ptr;
					xuri.host.len = str - ptr;
				}

				break;
			}
		}
	}

	if (*str == QSE_MT('/'))
	{
		xuri.path.ptr = str;
		while (*str != QSE_MT('\0'))
		{
			if ((!(flags & QSE_MBSTOURI_NOQUERY) && *str == QSE_MT('?')) ||
			    (!(flags & QSE_MBSTOURI_NOFRAG) && *str == QSE_MT('#'))) break; 
			str++;
		}
		xuri.path.len = str - (const qse_mchar_t*)xuri.path.ptr;

		if (!(flags & QSE_MBSTOURI_NOQUERY) && *str == QSE_MT('?')) 
		{
			xuri.query.ptr = ++str;
			while (*str != QSE_MT('\0'))
			{
				if (!(flags & QSE_MBSTOURI_NOFRAG) && *str == QSE_MT('#')) break; 
				str++;
			}
			xuri.query.len = str - (const qse_mchar_t*)xuri.query.ptr;
		}

		if (!(flags & QSE_MBSTOURI_NOFRAG) && *str == QSE_MT('#'))
		{
			xuri.frag.ptr = ++str;
			while (*str != QSE_MT('\0')) str++;
			xuri.frag.len = str - (const qse_mchar_t*)xuri.frag.ptr;
		}
	}

	QSE_ASSERT (*str == QSE_MT('\0'));
	*uri = xuri;
	return 0;
}

/* -------------------------------------------------------- */

int qse_wcstouri (const qse_wchar_t* str, qse_uri_t* uri, int flags)
{
	const qse_wchar_t* ptr, * colon;
	qse_uri_t xuri;

	QSE_MEMSET (&xuri, 0, QSE_SIZEOF(xuri));

	/* scheme */
	xuri.scheme.ptr = str;
	while (*str != QSE_WT(':')) 
	{
		if (*str == QSE_WT('\0')) return -1;
		str++;
	}
	xuri.scheme.len = str - (const qse_wchar_t*)xuri.scheme.ptr;

	str++; /* skip : */ 
	if (*str != QSE_WT('/')) return -1;
	str++; /* skip / */
	if (*str != QSE_WT('/')) return -1;
	str++; /* skip / */

	/* username, password, host, port */
	for (colon = QSE_NULL, ptr = str; ; str++)
	{
		if (flags & QSE_WCSTOURI_NOAUTH)
		{
			if (colon == QSE_NULL && *str == QSE_WT(':')) colon = str;
			else if (*str == QSE_WT('/') || *str == QSE_WT('\0')) 
			{
				if (colon)
				{
					xuri.host.ptr = ptr;
					xuri.host.len = colon - ptr;
					xuri.port.ptr = colon + 1;
					xuri.port.len = str - colon - 1;
				}
				else
				{
					xuri.host.ptr = ptr;
					xuri.host.len = str - ptr;
				}
				break;
			}
		}
		else
		{
			if (colon == QSE_NULL && *str == QSE_WT(':')) colon = str;
			else if (xuri.auth.user.ptr == QSE_NULL && *str == QSE_WT('@'))
			{
				if (colon)
				{
					xuri.auth.user.ptr = ptr;
					xuri.auth.user.len = colon - ptr;
					xuri.auth.pass.ptr = colon + 1;
					xuri.auth.pass.len = str - colon - 1;

					colon = QSE_NULL;
				}
				else
				{
					xuri.auth.user.ptr = ptr;
					xuri.auth.user.len = str - ptr;
				}

				ptr = str + 1;
			}
			else if (*str == QSE_WT('/') || *str == QSE_WT('\0')) 
			{
				if (colon)
				{
					xuri.host.ptr = ptr;
					xuri.host.len = colon - ptr;
					xuri.port.ptr = colon + 1;
					xuri.port.len = str - colon - 1;
				}
				else
				{
					xuri.host.ptr = ptr;
					xuri.host.len = str - ptr;
				}

				break;
			}
		}
	}

	if (*str == QSE_WT('/'))
	{
		xuri.path.ptr = str;
		while (*str != QSE_WT('\0'))
		{
			if ((!(flags & QSE_WCSTOURI_NOQUERY) && *str == QSE_WT('?')) ||
			    (!(flags & QSE_WCSTOURI_NOFRAG) && *str == QSE_WT('#'))) break; 
			str++;
		}
		xuri.path.len = str - (const qse_wchar_t*)xuri.path.ptr;

		if (!(flags & QSE_WCSTOURI_NOQUERY) && *str == QSE_WT('?')) 
		{
			xuri.query.ptr = ++str;
			while (*str != QSE_WT('\0'))
			{
				if (!(flags & QSE_WCSTOURI_NOFRAG) && *str == QSE_WT('#')) break; 
				str++;
			}
			xuri.query.len = str - (const qse_wchar_t*)xuri.query.ptr;
		}

		if (!(flags & QSE_WCSTOURI_NOFRAG) && *str == QSE_WT('#'))
		{
			xuri.frag.ptr = ++str;
			while (*str != QSE_WT('\0')) str++;
			xuri.frag.len = str - (const qse_wchar_t*)xuri.frag.ptr;
		}
	}

	QSE_ASSERT (*str == QSE_WT('\0'));
	*uri = xuri;
	return 0;
}
