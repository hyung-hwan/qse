/*
 * $Id: pio.c,v 1.23 2006/06/30 04:18:47 bacon Exp $
 */

#include <qse/cmn/pio.h>
#include "mem.h"

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#else
#include "syscall.h"
#include <errno.h>
#endif

#define HNDIN(pio)  ((pio)->handle[QSE_PIO_HANDLE_IN])
#define HNDOUT(pio) ((pio)->handle[QSE_PIO_HANDLE_OUT])
#define HNDERR(pio) ((pio)->handle[QSE_PIO_HANDLE_ERR])

qse_pio_t* qse_pio_open (
	qse_mmgr_t* mmgr, qse_size_t ext,
	const qse_char_t* path, int flags)
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

	if (qse_pio_init (pio, mmgr, path, flags) == QSE_NULL)
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
	const qse_char_t* path, int flags)
{
	qse_pio_pid_t pid;
	qse_pio_hnd_t handle[6] = { -1, -1, -1, -1, -1, -1 };
	int i, minidx = -1, maxidx = -1;

	QSE_ASSERT (QSE_COUNTOF(pio->hanlde) == QSE_COUNTOF(handle));

	QSE_MEMSET (pio, 0, QSE_SIZEOF(*pio));
	pio->mmgr = mmgr;

#ifdef _WIN32
	/* TODO: XXXXXXXXXXXXXXXXX */
#else
	if (flags & QSE_PIO_WRITEIN)
	{
		if (QSE_PIPE(&handle[0]) == -1) goto oops;
		minidx = 0; maxidx = 1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		if (QSE_PIPE(&handle[2]) == -1) goto oops;
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (flags & QSE_PIO_READERR)
	{
		if (QSE_PIPE(&handle[4]) == -1) goto oops;
		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1) goto oops;

	pid = QSE_FORK();
	if (pid == -1) goto oops;

	if (pid == 0)
	{
		/* child */

		if (flags & QSE_PIO_WRITEIN)
		{
			/* child should read */
			QSE_CLOSE (handle[1]);
			if (QSE_DUP2 (handle[0], 0) == -1) goto child_oops;
			QSE_CLOSE (handle[0]);
		}

		if (flags & QSE_PIO_READOUT)
		{
			/* child should write */
			QSE_CLOSE (handle[2]);
			if (QSE_DUP2 (handle[3], 1) == -1) goto child_oops;

			if (flags & QSE_PIO_ERRTOOUT)
			{
				if (QSE_DUP2 (handle[3], 2) == -1) goto child_oops;
			}

			QSE_CLOSE (handle[3]);
		}

		if (flags & QSE_PIO_READERR)
		{
			/* child should write */
			QSE_CLOSE (handle[4]);
			if (QSE_DUP2 (handle[5], 2) == -1) goto child_oops;

			if (flags & QSE_PIO_OUTTOERR)
			{
				if (QSE_DUP2 (handle[5], 1) == -1) goto child_oops;
			}

			QSE_CLOSE (handle[5]);
		}


		/* TODO: ... */
		//execl ("/bin/sh", "sh", "-c", "cat", QSE_NULL);
		execl ("/bin/sh", "sh", "-c", "cat -aksksks", QSE_NULL);

	child_oops:
		QSE_EXIT(127);
	}

	/* parent */
	pio->child = pid;

	if (flags & QSE_PIO_WRITEIN)
	{
		/* 
		 * 012345
		 * rw----
		 * X  
		 * WRITE => 1
		 */
		QSE_CLOSE (handle[0]); handle[0] = -1;
	}

	if (flags & QSE_PIO_READOUT)
	{
		/* 
		 * 012345
		 * --rw--
		 *    X
		 * READ => 2
		 */
		QSE_CLOSE (handle[3]); handle[3] = -1;
	}

	if (flags & QSE_PIO_READERR)
	{
		/* 
		 * 012345
		 * ----rw
		 *      X   
		 * READ => 4
		 */
		QSE_CLOSE (handle[5]); handle[5] = -1;
	}

#endif

	HNDIN(pio) = handle[1];
	HNDOUT(pio) = handle[2];
	HNDERR(pio) = handle[4];

for (i = minidx; i < maxidx; i++)
{
	qse_printf (QSE_T("%d ==> %d\n"), i, handle[i]);
}
	return pio;

oops:
	for (i = minidx; i < maxidx; i++) QSE_CLOSE (handle[i]);
	return QSE_NULL;
}

void qse_pio_fini (qse_pio_t* pio)
{
#ifdef _WIN32
	CloseHandle (pio->handle);
#else
	int i, status;

	if (HNDOUT(pio) != -1)
	{
		QSE_CLOSE (HNDOUT(pio));
		HNDOUT(pio) = -1;
	}

	if (HNDIN(pio) != -1)
	{
		QSE_CLOSE (HNDIN(pio));
		HNDIN(pio) = -1;
	}

	while (QSE_WAITPID (pio->child, &status, 0) == -1)
	{
		if (errno != EINTR) break;
	}
#endif
}

int qse_pio_wait (qse_pio_t* pio)
{
	int status;

#if 0
	if (opt & QSE_PIO_NOWAIT)
	{
		opt |= WNOHANG;
	}

	while (1)
	{
		n = waitpid (pio->child, &status, opt);
		if (n == 0) break; 
	/* TODO: .... */
	}


	output => return code...
	output => termination cause...
#endif
	return -1;
}

/*
qse_pio_hnd_t qse_pio_gethandle (qse_pio_t* pio, int readorwrite)
{
	return handle[xxx];
}
*/

qse_ssize_t qse_pio_read (
	qse_pio_t* pio, void* buf, qse_size_t size, int flags)
{
#ifdef _WIN32
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (ReadFile(pio->handle, buf, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	qse_pio_hnd_t handle = -1;

	if (flags == 0) flags = QSE_PIO_OUT;
	if (flags & QSE_PIO_ERR) handle = HNDERR(pio);
	if (flags & QSE_PIO_OUT) handle = HNDOUT(pio);

	if (handle == -1) 
	{
		/* the stream is already closed */
		return (qse_ssize_t)-1;
	}

	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	return QSE_READ (handle, buf, size);
#endif
}

qse_ssize_t qse_pio_write (
	qse_pio_t* pio, const void* data, qse_size_t size, int flags)
{
#ifdef _WIN32
	DWORD count;
	if (size > QSE_TYPE_MAX(DWORD)) size = QSE_TYPE_MAX(DWORD);
	if (WriteFile(pio->handle, data, size, &count, QSE_NULL) == FALSE) return -1;
	return (qse_ssize_t)count;
#else
	qse_pio_hnd_t handle;

	if (flags == 0) flags = QSE_PIO_IN;
	if (flags & QSE_PIO_IN) handle = HNDIN(pio);

	if (handle == -1) 
	{
		/* the stream is already closed */
		return (qse_ssize_t)-1;
	}

	if (size > QSE_TYPE_MAX(size_t)) size = QSE_TYPE_MAX(size_t);
	return QSE_WRITE (handle, data, size);
#endif
}

void qse_pio_end (qse_pio_t* pio, int flags)
{
	if ((flags & QSE_PIO_IN) && HNDIN(pio) != -1)
	{
		QSE_CLOSE (HNDIN(pio)); 
		HNDIN(pio) = -1;
	}
	if ((flags & QSE_PIO_ERR) && HNDERR(pio) != -1)
	{
		QSE_CLOSE (HNDERR(pio)); 
		HNDERR(pio) = -1;
	}
	if ((flags & QSE_PIO_OUT) && HNDOUT(pio) != -1)
	{
		QSE_CLOSE (HNDOUT(pio)); 
		HNDOUT(pio) = -1;
	}
}
