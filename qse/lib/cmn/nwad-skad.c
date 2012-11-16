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

#include <qse/cmn/nwad.h>
#include "mem.h"

#if defined(_WIN32)
#    include <winsock2.h>
#    include <ws2tcpip.h> /* sockaddr_in6 */
#    include <windows.h>
#elif defined(__OS2__)
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	pragma library("tcpip32.lib")
#elif defined(__DOS__)
 	/* TODO:  consider watt-32 */
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

union sockaddr_t
{
#if defined(AF_INET) || defined(AF_INET6)
	#if defined(AF_INET)
	struct sockaddr_in in4;
	#endif
	#if defined(AF_INET6)
	struct sockaddr_in6 in6;
	#endif
#else
	int dummy;
#endif
};

typedef union sockaddr_t sockaddr_t;

#if defined(AF_INET)
#	define FAMILY(x) (((struct sockaddr_in*)(x))->sin_family)
#elif defined(AF_INET6)
#	define FAMILY(x) (((struct sockaddr_in6*)(x))->sin6_family)
#else
#	define FAMILY(x) (-1)
#endif

static QSE_INLINE int skad_to_nwad (const sockaddr_t* skad, qse_nwad_t* nwad)
{
	int addrsize = -1;

	switch (FAMILY(skad))
	{
#if defined(AF_INET)
		case AF_INET:
		{
			struct sockaddr_in* in;
			in = (struct sockaddr_in*)skad;
			addrsize = QSE_SIZEOF(*in);

			QSE_MEMSET (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_IN4;
			nwad->u.in4.addr.value = in->sin_addr.s_addr;
			nwad->u.in4.port = in->sin_port;
			break;
		}
#endif

#if defined(AF_INET6)
		case AF_INET6:
		{
			struct sockaddr_in6* in;
			in = (struct sockaddr_in6*)skad;
			addrsize = QSE_SIZEOF(*in);

			QSE_MEMSET (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_IN6;
			QSE_MEMCPY (&nwad->u.in6.addr, &in->sin6_addr, QSE_SIZEOF(nwad->u.in6.addr));
			nwad->u.in6.scope = in->sin6_scope_id;
			nwad->u.in6.port = in->sin6_port;
			break;
		}
#endif

		default:
			break;
	}

	return addrsize;
}

static QSE_INLINE int nwad_to_skad (const qse_nwad_t* nwad, sockaddr_t* skad)
{
	int addrsize = -1;

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
		{
#if defined(AF_INET)
			struct sockaddr_in* in;

			in = (struct sockaddr_in*)skad;
			addrsize = QSE_SIZEOF(*in);
			QSE_MEMSET (in, 0, addrsize);

			in->sin_family = AF_INET;
			in->sin_addr.s_addr = nwad->u.in4.addr.value;
			in->sin_port = nwad->u.in4.port;
#endif
			break;
		}

		case QSE_NWAD_IN6:
		{
#if defined(AF_INET6)
			struct sockaddr_in6* in;

			in = (struct sockaddr_in6*)skad;
			addrsize = QSE_SIZEOF(*in);
			QSE_MEMSET (in, 0, addrsize);

			in->sin6_family = AF_INET6;
			QSE_MEMCPY (&in->sin6_addr, &nwad->u.in6.addr, QSE_SIZEOF(nwad->u.in6.addr));
			in->sin6_scope_id = nwad->u.in6.scope;
			in->sin6_port = nwad->u.in6.port;
#endif
			break;
		}
	}

	return addrsize;
}

int qse_skadtonwad (const qse_skad_t* skad, qse_nwad_t* nwad)
{
	QSE_ASSERT (QSE_SIZEOF(*skad) >= QSE_SIZEOF(sockaddr_t));
	return skad_to_nwad ((const sockaddr_t*)skad, nwad);
}

int qse_nwadtoskad (const qse_nwad_t* nwad, qse_skad_t* skad)
{
	QSE_ASSERT (QSE_SIZEOF(*skad) >= QSE_SIZEOF(sockaddr_t));
	return nwad_to_skad (nwad, (sockaddr_t*)skad);
}

int qse_skadfamily (const qse_skad_t* skad)
{
	QSE_ASSERT (QSE_SIZEOF(*skad) >= QSE_SIZEOF(sockaddr_t));
	return FAMILY(skad);
}
