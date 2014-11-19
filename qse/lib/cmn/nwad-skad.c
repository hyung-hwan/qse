/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/nwad.h>
#include "mem.h"

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h> /* sockaddr_in6 */
#	include <windows.h>
#	undef AF_UNIX
#	if defined(__WATCOMC__) && (__WATCOMC__ < 1200)
		/* the header files shipped with watcom 11 doesn't contain
		 * proper inet6 support. note using the compiler version
		 * in the contidional isn't that good idea since you 
		 * can use newer header files with this old compiler.
		 * never mind it for the time being.
		 */
#		undef AF_INET6
#	endif
#elif defined(__OS2__)
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
	/* though AF_INET6 is defined, there is no support
	 * for it. so undefine it */
#	undef AF_INET6
#	undef AF_UNIX
#	pragma library("tcpip32.lib")
#elif defined(__DOS__)
#	include <tcp.h> /* watt-32 */
#	undef AF_UNIX
#else
#	if defined(HAVE_SYS_TYPES_H)
#		include <sys/types.h>
#	endif
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_SYS_UN_H)
#		include <sys/un.h>
#	endif

#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_IN6) && (QSE_SIZEOF_STRUCT_SOCKADDR_IN6 <= 0)
#		undef AF_INET6
#	endif

#	if defined(QSE_SIZEOF_STRUCT_SOCKADDR_UN) && (QSE_SIZEOF_STRUCT_SOCKADDR_UN <= 0)
#		undef AF_UNIX
#	endif

#endif

union sockaddr_t
{
#if defined(AF_INET) || defined(AF_INET6) || defined(AF_UNIX)
	#if defined(AF_INET)
	struct sockaddr_in in4;
	#endif
	#if defined(AF_INET6)
	struct sockaddr_in6 in6;
	#endif
	#if defined(AF_UNIX)
	struct sockaddr_un un;
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
#elif defined(AF_UNIX)
#	define FAMILY(x) (((struct sockaddr_un*)(x))->sun_family)
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
		#if defined(HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID)
			nwad->u.in6.scope = in->sin6_scope_id;
		#endif
			nwad->u.in6.port = in->sin6_port;
			break;
		}
#endif

#if defined(AF_UNIX)
		case AF_UNIX:
		{
			struct sockaddr_un* un;
			un = (struct sockaddr_un*)skad;
			addrsize = QSE_SIZEOF(*un);

			QSE_MEMSET (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_LOCAL;
		#if defined(QSE_CHAR_IS_MCHAR)
			qse_mbsxcpy (nwad->u.local.path, QSE_COUNTOF(nwad->u.local.path), un->sun_path);
		#else
			{
				qse_size_t wcslen, mbslen;
				mbslen = QSE_COUNTOF(nwad->u.local.path);
				qse_wcstombs (un->sun_path, &wcslen, nwad->u.local.path, &mbslen);
				/* don't care about conversion errors */
			}
		#endif
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
		#if defined(HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID)
			in->sin6_scope_id = nwad->u.in6.scope;
		#endif
			in->sin6_port = nwad->u.in6.port;
#endif
			break;
		}


		case QSE_NWAD_LOCAL:
		{
#if defined(AF_UNIX)
			struct sockaddr_un* un;

			un = (struct sockaddr_un*)skad;
			addrsize = QSE_SIZEOF(*un);
			QSE_MEMSET (un, 0, addrsize);

			un->sun_family = AF_UNIX;
		#if defined(QSE_CHAR_IS_MCHAR)
			qse_mbsxcpy (un->sun_path, QSE_COUNTOF(un->sun_path), nwad->u.local.path);
		#else
			{
				qse_size_t wcslen, mbslen;
				mbslen = QSE_COUNTOF(un->sun_path);
				qse_wcstombs (nwad->u.local.path, &wcslen, un->sun_path, &mbslen);
				/* don't care about conversion errors */
			}

		#endif
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
