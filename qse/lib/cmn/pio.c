/*
 * $Id: pio.c,v 1.23 2006/06/30 04:18:47 bacon Exp $
 */

#include <qse/cmn/pio.h>
#include "mem.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#else
#include "syscall.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#endif

qse_pio_t* qse_pio_open (
	qse_mmgr_t* mmgr, qse_size_t ext,
	const qse_char_t* path, int flags, int mode)
{
	qse_pio_t* pio;

	if (mmgr == QSE_NULL)
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	pio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_pio_t) + ext);
	if (pio == QSE_NULL) return QSE_NULL;

	if (qse_pio_init (pio, mmgr, path, flags, mode) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, pio);
		return QSE_NULL;
	}

	return pio;
}

void qse_pio_close (qse_pio_t* pio)
{
	qse_pio_fini (pio);
	QSE_MMGR_FREE (pio->mmgr, pio);
}

qse_pio_t* qse_pio_init (
	qse_pio_t* pio, qse_mmgr_t* mmgr,
	const qse_char_t* path, int flags, int mode)
{
	qse_pio_hnd_t handle;

	QSE_MEMSET (pio, 0, QSE_SIZEOF(*pio));
	pio->mmgr = mmgr;

#ifdef _WIN32
	handle = -1;
#else
	handle = -1;
#endif

	pio->handle = handle;
	return pio;
}

void qse_pio_fini (qse_pio_t* pio)
{
#ifdef _WIN32
	CloseHandle (pio->handle);
#else
	QSE_CLOSE (pio->handle);
#endif
}

qse_pio_hnd_t qse_pio_gethandle (qse_pio_t* pio)
{
	return pio->handle;
}

void qse_pio_sethandle (qse_pio_t* pio, qse_pio_hnd_t handle)
{
	pio->handle = handle;
}

qse_ssize_t qse_pio_read (qse_pio_t* pio, void* buf, qse_size_t size)
{
#ifdef _WIN32
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (ReadFile(pio->handle, buf, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	return QSE_READ (pio->handle, buf, size);
#endif
}

qse_ssize_t qse_pio_write (qse_pio_t* pio, const void* data, qse_size_t size)
{
#ifdef _WIN32
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (WriteFile(pio->handle, data, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	return QSE_WRITE (pio->handle, data, size);
#endif
}

