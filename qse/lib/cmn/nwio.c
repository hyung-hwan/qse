/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either vernwion 3 of 
    the License, or (at your option) any later vernwion.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/nwio.h>
#include "mem.h"

#if defined(_WIN32)
/* TODO: */
#elif defined(__OS2__)
/* TODO: */
#elif defined(__DOS__)
/* TODO: */
#else
#	include "syscall.h"
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif


static qse_ssize_t socket_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);
static qse_ssize_t socket_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size);

static int nwad_to_sockaddr (const qse_nwad_t* nwad, int* family, void* addr)
{
	int addrsize = -1;

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
		{
			struct sockaddr_in* in; 

			in = (struct sockaddr_in*)addr;
			addrsize = QSE_SIZEOF(*in);
			QSE_MEMSET (in, 0, addrsize);

			*family = AF_INET;
			in->sin_family = AF_INET;
			in->sin_addr.s_addr = nwad->u.in4.addr.value;
			in->sin_port = nwad->u.in4.port;
			break;
		}

		case QSE_NWAD_IN6:
		{
#if defined(AF_INET6)
			struct sockaddr_in6* in; 

			in = (struct sockaddr_in6*)addr;
			addrsize = QSE_SIZEOF(*in);
			QSE_MEMSET (in, 0, addrsize);

			*family = AF_INET6;
			in->sin6_family = AF_INET6;
			memcpy (&in->sin6_addr, &nwad->u.in6.addr, QSE_SIZEOF(nwad->u.in6.addr));
			in->sin6_scope_id = nwad->u.in6.scope;
			in->sin6_port = nwad->u.in6.port;
#endif
			break;
		}
	}

	return addrsize;
}

#if defined(_WIN32)
static qse_nwio_errnum_t syserr_to_errnum (DWORD e)
{
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return QSE_NWIO_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_NWIO_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_NWIO_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_NWIO_ENOENT;

		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return QSE_NWIO_EEXIST;

		case ERROR_BROKEN_PIPE:
			return QSE_NWIO_EPIPE;

		default:
			return QSE_NWIO_ESYSERR;
	}
}
#elif defined(__OS2__)
static qse_nwio_errnum_t syserr_to_errnum (APIRET e)
{
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
			return QSE_NWIO_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_NWIO_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_NWIO_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_NWIO_ENOENT;

		case ERROR_ALREADY_EXISTS:
			return QSE_NWIO_EEXIST;

		case ERROR_BROKEN_PIPE:
			return QSE_NWIO_EPIPE;

		default:
			return QSE_NWIO_ESYSERR;
	}
}
#elif defined(__DOS__)
static qse_nwio_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_NWIO_ENOMEM;

		case EINVAL:
			return QSE_NWIO_EINVAL;

		case EACCES:
			return QSE_NWIO_EACCES;

		case ENOENT:
			return QSE_NWIO_ENOENT;

		case EEXIST:
			return QSE_NWIO_EEXIST;
		
	
		default:
			return QSE_NWIO_ESYSERR;
	}
}
#else
static qse_nwio_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_NWIO_ENOMEM;

		case EINVAL:
			return QSE_NWIO_EINVAL;

		case EACCES:
			return QSE_NWIO_EACCES;

		case ENOENT:
			return QSE_NWIO_ENOENT;

		case EEXIST:
			return QSE_NWIO_EEXIST;
	
		case EINTR:
			return QSE_NWIO_EINTR;

		case EPIPE:
			return QSE_NWIO_EPIPE;

#if defined(ECONNREFUSED) || defined(ENETUNREACH)
	#if defined(ECONNREFUSED) 
		case ECONNREFUSED:
	#endif
	#if defined(ENETUNREACH) 
		case ENETUNREACH:
	#endif
			return QSE_NWIO_ECONN;
#endif

		default:
			return QSE_NWIO_ESYSERR;
	}
}
#endif

static qse_nwio_errnum_t tio_errnum_to_nwio_errnum (qse_tio_t* tio)
{
	switch (tio->errnum)
	{
		case QSE_TIO_ENOMEM:
			return QSE_NWIO_ENOMEM;
		case QSE_TIO_EINVAL:
			return QSE_NWIO_EINVAL;
		case QSE_TIO_EACCES:
			return QSE_NWIO_EACCES;
		case QSE_TIO_ENOENT:
			return QSE_NWIO_ENOENT;
		case QSE_TIO_EILSEQ:
			return QSE_NWIO_EILSEQ;
		case QSE_TIO_EICSEQ:
			return QSE_NWIO_EICSEQ;
		case QSE_TIO_EILCHR:
			return QSE_NWIO_EILCHR;
		default:
			return QSE_NWIO_EOTHER;
	}
}

qse_nwio_t* qse_nwio_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_nwad_t* nwad, int flags)
{
	qse_nwio_t* nwio;

	nwio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_nwio_t) + xtnsize);
	if (nwio == QSE_NULL) return QSE_NULL;

	if (qse_nwio_init (nwio, mmgr, nwad, flags) <= -1)
	{
		QSE_MMGR_FREE (mmgr, nwio);
		return QSE_NULL;
	}

	return nwio;
}

void qse_nwio_close (qse_nwio_t* nwio)
{
	qse_nwio_fini (nwio);
	QSE_MMGR_FREE (nwio->mmgr, nwio);
}

int qse_nwio_init (
	qse_nwio_t* nwio, qse_mmgr_t* mmgr, const qse_nwad_t* nwad, int flags)
{
	union
	{
		struct sockaddr_in in4;
		struct sockaddr_in6 in6;
	} addr;
	int addrlen;
	int family;

	QSE_MEMSET (nwio, 0, QSE_SIZEOF(*nwio));
	nwio->mmgr = mmgr;
	nwio->flags = flags;

	addrlen = nwad_to_sockaddr (nwad, &family, &addr);

#if defined(_WIN32)
/* TODO: */
#elif defined(__OS2__)
/* TODO: */
#elif defined(__DOS__)
/* TODO: */
#else
	nwio->handle = socket (family, SOCK_STREAM, 0);
	if (nwio->handle <= -1)
	{
		nwio->errnum = syserr_to_errnum (errno);
		goto oops;
	}

#if defined(FD_CLOEXEC)
	{ 
		int tmp = fcntl (nwio->handle, F_GETFD);
		if (tmp >= 0) fcntl (nwio->handle, F_SETFD, tmp | FD_CLOEXEC);
	}
#endif

	if (flags & QSE_NWIO_LISTEN)
	{
		qse_nwio_hnd_t handle;

		if (bind (nwio->handle, (struct sockaddr*)&addr, addrlen) <= -1 ||
		    listen (nwio->handle, 10) <= -1)
		{
			nwio->errnum = syserr_to_errnum (errno);
			goto oops;
		}

/* TODO: socklen_t */
		handle = accept (nwio->handle, &addr, &addrlen);
		if (handle <= -1)
		{
			nwio->errnum = syserr_to_errnum (errno);
			goto oops;
		}

		close (nwio->handle);
		nwio->handle = handle;
	}
	else
	{
		if (connect (nwio->handle, (struct sockaddr*)&addr, addrlen) <= -1)
		{
			nwio->errnum = syserr_to_errnum (errno);
			goto oops;
		}
	}
#endif

	if (flags & QSE_NWIO_TEXT)
	{
		int topt = 0;

		if (flags & QSE_NWIO_IGNOREMBWCERR) topt |= QSE_TIO_IGNOREMBWCERR;
		if (flags & QSE_NWIO_NOAUTOFLUSH) topt |= QSE_TIO_NOAUTOFLUSH;

		nwio->tio = qse_tio_open (mmgr, QSE_SIZEOF(qse_nwio_t*), topt);
		if (nwio->tio == QSE_NULL)
		{
			nwio->errnum = QSE_NWIO_ENOMEM;
			goto oops;
		}

		/* store the back-reference to nwio in the extension area.*/
		*(qse_nwio_t**)QSE_XTN(nwio->tio) = nwio;

		if (qse_tio_attachin (nwio->tio, socket_input, QSE_NULL, 4096) <= -1 ||
		    qse_tio_attachout (nwio->tio, socket_output, QSE_NULL, 4096) <= -1)
		{
			if (nwio->errnum == QSE_NWIO_ENOERR) 
				nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);
			goto oops;
		}
	}

	return 0;

oops:
	if (nwio->tio) qse_tio_close (nwio->tio);

#if defined(_WIN32)
/* TODO: */
#elif defined(__OS2__)
/* TODO: */
#elif defined(__DOS__)
/* TODO: */
#else
	close (nwio->handle);
#endif
	return -1;
}

void qse_nwio_fini (qse_nwio_t* nwio)
{
	/*if (qse_nwio_flush (nwio) <= -1) return -1;*/
	qse_nwio_flush (nwio);
	if (nwio->tio)
	{
		qse_tio_close (nwio->tio);
		nwio->tio = QSE_NULL;
	}
}

qse_nwio_errnum_t qse_nwio_geterrnum (const qse_nwio_t* nwio)
{
	return nwio->errnum;
}

qse_cmgr_t* qse_nwio_getcmgr (qse_nwio_t* nwio)
{
	return nwio->tio? qse_tio_getcmgr (nwio->tio): QSE_NULL;
}

void qse_nwio_setcmgr (qse_nwio_t* nwio, qse_cmgr_t* cmgr)
{
	if (nwio->tio) qse_tio_setcmgr (nwio->tio, cmgr);
}

qse_nwio_hnd_t qse_nwio_gethandle (const qse_nwio_t* nwio)
{
	return QSE_NWIO_HANDLE(nwio);
}

qse_ubi_t qse_nwio_gethandleasubi (const qse_nwio_t* nwio)
{
	qse_ubi_t ubi;

#if defined(_WIN32)
/* TODO: */
#elif defined(__OS2__)
/* TODO: */
#elif defined(__DOS__)
/* TODO: */
#else
	ubi.i = nwio->handle;
#endif
	return ubi;
}

qse_ssize_t qse_nwio_flush (qse_nwio_t* nwio)
{
	qse_ssize_t n;

	if (nwio->tio)
	{
		nwio->errnum = QSE_NWIO_ENOERR;
		n = qse_tio_flush (nwio->tio);
		if (n <= -1 && nwio->errnum == QSE_NWIO_ENOERR) 
			nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);
	}
	else n = 0;

	return n;
}

void qse_nwio_purge (qse_nwio_t* nwio)
{
	if (nwio->tio) qse_tio_purge (nwio->tio);
}

/* ---------------------------------------------------------- */

static qse_ssize_t nwio_read (qse_nwio_t* nwio, void* buf, qse_size_t size)
{
#if defined(_WIN32)
	DWORD count;
#elif defined(__OS2__)
	ULONG count;
	APIRET rc;
#elif defined(__DOS__)
	int n;
#else
	qse_ssize_t n;
#endif

#if defined(_WIN32)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD);

	if (ReadFile(nwio->handle, buf, (DWORD)size, &count, QSE_NULL) == FALSE) 
	{
		/* ReadFile receives ERROR_BROKEN_PIPE when the write end
		 * is closed in the child process */
		if (GetLastError() == ERROR_BROKEN_PIPE) return 0;
		nwio->errnum = syserr_to_errnum(GetLastError());
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__OS2__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG);

	rc = DosRead (nwio->handle, buf, (ULONG)size, &count);
	if (rc != NO_ERROR)
	{
    		if (rc == ERROR_BROKEN_PIPE) return 0; /* TODO: check this */
		nwio->errnum = syserr_to_errnum(rc);
    		return -1;
    	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)
	/* TODO: verify this */

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);

	n = read (nwio->handle, buf, size);
	if (n <= -1) nwio->errnum = syserr_to_errnum(errno);
	return n;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

reread:
	n = recv (nwio->handle, buf, size, 0);
	if (n <= -1) 
	{
		if (errno == EINTR)
		{
			if (nwio->flags & QSE_NWIO_READNORETRY) 
				nwio->errnum = QSE_NWIO_EINTR;
			else goto reread;
		}
		else
		{
			nwio->errnum = syserr_to_errnum (errno);
		}
	}

	return n;
#endif
}

qse_ssize_t qse_nwio_read (qse_nwio_t* nwio, void* buf, qse_size_t size)
{
	if (nwio->tio == QSE_NULL) 
		return nwio_read (nwio, buf, size);
	else
	{
		qse_ssize_t n;

		nwio->errnum = QSE_NWIO_ENOERR;
		n = qse_tio_read (nwio->tio, buf, size);
		if (n <= -1 && nwio->errnum == QSE_NWIO_ENOERR) 
			nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);

		return n;
	}
}

static qse_ssize_t nwio_write (qse_nwio_t* nwio, const void* data, qse_size_t size)
{
#if defined(_WIN32)
	DWORD count;
#elif defined(__OS2__)
	ULONG count;
	APIRET rc;
#elif defined(__DOS__)
	int n;
#else
	qse_ssize_t n;
#endif

#if defined(_WIN32)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(DWORD);

	if (WriteFile (nwio->handle, data, (DWORD)size, &count, QSE_NULL) == FALSE)
	{
		nwio->errnum = syserr_to_errnum(GetLastError());
		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__OS2__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(ULONG);

	rc = DosWrite (nwio->handle, (PVOID)data, (ULONG)size, &count);
	if (rc != NO_ERROR)
	{
		nwio->errnum = syserr_to_errnum(rc);
    		return -1;
	}
	return (qse_ssize_t)count;

#elif defined(__DOS__)

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(unsigned int);

	n = write (nwio->handle, data, size);
	if (n <= -1) nwio->errnum = syserr_to_errnum (errno);
	return n;

#else

	if (size > (QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t)))
		size = QSE_TYPE_MAX(qse_ssize_t) & QSE_TYPE_MAX(size_t);

rewrite:
	n = send (nwio->handle, data, size, 0);
	if (n <= -1) 
	{
		if (errno == EINTR)
		{
			if (nwio->flags & QSE_NWIO_WRITENORETRY)
				nwio->errnum = QSE_NWIO_EINTR;
			else goto rewrite;
		}
		else
		{
			nwio->errnum = syserr_to_errnum (errno);
		}
	}
	return n;

#endif
}

qse_ssize_t qse_nwio_write (qse_nwio_t* nwio, const void* data, qse_size_t size)
{
	if (nwio->tio == QSE_NULL)
		return nwio_write (nwio, data, size);
	else
	{
		qse_ssize_t n;

		nwio->errnum = QSE_NWIO_ENOERR;	
		n = qse_tio_write (nwio->tio, data, size);
		if (n <= -1 && nwio->errnum == QSE_NWIO_ENOERR) 
			nwio->errnum = tio_errnum_to_nwio_errnum (nwio->tio);

		return n;
	}
}

/* ---------------------------------------------------------- */

static qse_ssize_t socket_input (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{
	if (cmd == QSE_TIO_DATA) 
	{
		qse_nwio_t* nwio;

		nwio = *(qse_nwio_t**)QSE_XTN(tio);
		QSE_ASSERT (nwio != QSE_NULL);

		return nwio_read (nwio, buf, size);
	}

	return 0;
}

static qse_ssize_t socket_output (
	qse_tio_t* tio, qse_tio_cmd_t cmd, void* buf, qse_size_t size)
{
	if (cmd == QSE_TIO_DATA) 
	{
		qse_nwio_t* nwio;

		nwio = *(qse_nwio_t**)QSE_XTN(tio);
		QSE_ASSERT (nwio != QSE_NULL);

		return nwio_write (nwio, buf, size);
	}

	return 0;
}

