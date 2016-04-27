/*
 * $Id$
 *
    Copyright (c) 2006-2016 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/io/aio-pro.h>
#include "aio-prv.h"

#include <qse/cmn/str.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

/* ========================================================================= */

struct slave_info_t
{
	qse_aio_dev_pro_make_t* mi;
	qse_aio_syshnd_t pfd;
	int dev_capa;
	qse_aio_dev_pro_sid_t id;
};

typedef struct slave_info_t slave_info_t;

static qse_aio_dev_pro_slave_t* make_slave (qse_aio_t* aio, slave_info_t* si);

/* ========================================================================= */

struct param_t
{
	qse_mchar_t* mcmd;
	qse_mchar_t* fixed_argv[4];
	qse_mchar_t** argv;
};
typedef struct param_t param_t;

static void free_param (qse_aio_t* aio, param_t* param)
{
	if (param->argv && param->argv != param->fixed_argv) 
		QSE_MMGR_FREE (aio->mmgr, param->argv);
	if (param->mcmd) QSE_MMGR_FREE (aio->mmgr, param->mcmd);
	QSE_MEMSET (param, 0, QSE_SIZEOF(*param));
}

static int make_param (qse_aio_t* aio, const qse_mchar_t* cmd, int flags, param_t* param)
{
	int fcnt = 0;
	qse_mchar_t* mcmd = QSE_NULL;

	QSE_MEMSET (param, 0, QSE_SIZEOF(*param));

	if (flags & QSE_AIO_DEV_PRO_SHELL)
	{
		mcmd = (qse_mchar_t*)cmd;

		param->argv = param->fixed_argv;
		param->argv[0] = QSE_MT("/bin/sh");
		param->argv[1] = QSE_MT("-c");
		param->argv[2] = mcmd;
		param->argv[3] = QSE_NULL;
	}
	else
	{
		int i;
		qse_mchar_t** argv;
		qse_mchar_t* mcmdptr;

		mcmd = qse_mbsdup (cmd, aio->mmgr);
		if (!mcmd) 
		{
			aio->errnum = QSE_AIO_ENOMEM;
			goto oops;
		}

		fcnt = qse_mbsspl (mcmd, QSE_MT(""), QSE_MT('\"'), QSE_MT('\"'), QSE_MT('\\')); 
		if (fcnt <= 0) 
		{
			/* no field or an error */
			aio->errnum = QSE_AIO_EINVAL;
			goto oops;
		}

		if (fcnt < QSE_COUNTOF(param->fixed_argv))
		{
			param->argv = param->fixed_argv;
		}
		else
		{
			param->argv = QSE_MMGR_ALLOC (aio->mmgr, (fcnt + 1) * QSE_SIZEOF(argv[0]));
			if (param->argv == QSE_NULL) 
			{
				aio->errnum = QSE_AIO_ENOMEM;
				goto oops;
			}
		}

		mcmdptr = mcmd;
		for (i = 0; i < fcnt; i++)
		{
			param->argv[i] = mcmdptr;
			while (*mcmdptr != QSE_MT('\0')) mcmdptr++;
			mcmdptr++;
		}
		param->argv[i] = QSE_NULL;
	}

	if (mcmd && mcmd != (qse_mchar_t*)cmd) param->mcmd = mcmd;
	return 0;

oops:
	if (mcmd && mcmd != cmd) QSE_MMGR_FREE (aio->mmgr, mcmd);
	return -1;
}

static pid_t standard_fork_and_exec (qse_aio_t* aio, int pfds[], int flags, param_t* param)
{
	pid_t pid;

	pid = fork ();
	if (pid == -1) 
	{
		aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	if (pid == 0)
	{
		/* slave process */

		qse_aio_syshnd_t devnull = QSE_AIO_SYSHND_INVALID;

/* TODO: close all uneeded fds */

		if (flags & QSE_AIO_DEV_PRO_WRITEIN)
		{
			/* slave should read */
			close (pfds[1]);
			pfds[1] = QSE_AIO_SYSHND_INVALID;

			/* let the pipe be standard input */
			if (dup2 (pfds[0], 0) <= -1) goto slave_oops;

			close (pfds[0]);
			pfds[0] = QSE_AIO_SYSHND_INVALID;
		}

		if (flags & QSE_AIO_DEV_PRO_READOUT)
		{
			/* slave should write */
			close (pfds[2]);
			pfds[2] = QSE_AIO_SYSHND_INVALID;

			if (dup2(pfds[3], 1) == -1) goto slave_oops;

			if (flags & QSE_AIO_DEV_PRO_ERRTOOUT)
			{
				if (dup2(pfds[3], 2) == -1) goto slave_oops;
			}

			close (pfds[3]);
			pfds[3] = QSE_AIO_SYSHND_INVALID;
		}

		if (flags & QSE_AIO_DEV_PRO_READERR)
		{
			close (pfds[4]);
			pfds[4] = QSE_AIO_SYSHND_INVALID;

			if (dup2(pfds[5], 2) == -1) goto slave_oops;

			if (flags & QSE_AIO_DEV_PRO_OUTTOERR)
			{
				if (dup2(pfds[5], 1) == -1) goto slave_oops;
			}

			close (pfds[5]);
			pfds[5] = QSE_AIO_SYSHND_INVALID;
		}

		if ((flags & QSE_AIO_DEV_PRO_INTONUL) ||
		    (flags & QSE_AIO_DEV_PRO_OUTTONUL) ||
		    (flags & QSE_AIO_DEV_PRO_ERRTONUL))
		{
		#if defined(O_LARGEFILE)
			devnull = open (QSE_MT("/dev/null"), O_RDWR | O_LARGEFILE, 0);
		#else
			devnull = open (QSE_MT("/dev/null"), O_RDWR, 0);
		#endif
			if (devnull == QSE_AIO_SYSHND_INVALID) goto slave_oops;
		}

		execv (param->argv[0], param->argv);

		/* if exec fails, free 'param' parameter which is an inherited pointer */
		free_param (aio, param); 

	slave_oops:
		if (devnull != QSE_AIO_SYSHND_INVALID) close(devnull);
		_exit (128);
	}

	/* parent process */
	return pid;
}

static int dev_pro_make_master (qse_aio_dev_t* dev, void* ctx)
{
	qse_aio_dev_pro_t* rdev = (qse_aio_dev_pro_t*)dev;
	qse_aio_dev_pro_make_t* info = (qse_aio_dev_pro_make_t*)ctx;
	qse_aio_syshnd_t pfds[6];
	int i, minidx = -1, maxidx = -1;
	param_t param;
	pid_t pid;

	if (info->flags & QSE_AIO_DEV_PRO_WRITEIN)
	{
		if (pipe(&pfds[0]) == -1)
		{
			dev->aio->errnum = qse_aio_syserrtoerrnum(errno);
			goto oops;
		}
		minidx = 0; maxidx = 1;
	}

	if (info->flags & QSE_AIO_DEV_PRO_READOUT)
	{
		if (pipe(&pfds[2]) == -1)
		{
			dev->aio->errnum = qse_aio_syserrtoerrnum(errno);
			goto oops;
		}
		if (minidx == -1) minidx = 2;
		maxidx = 3;
	}

	if (info->flags & QSE_AIO_DEV_PRO_READERR)
	{
		if (pipe(&pfds[4]) == -1)
		{
			dev->aio->errnum = qse_aio_syserrtoerrnum(errno);
			goto oops;
		}
		if (minidx == -1) minidx = 4;
		maxidx = 5;
	}

	if (maxidx == -1)
	{
		dev->aio->errnum = QSE_AIO_EINVAL;
		goto oops;
	}

	if (make_param (rdev->aio, info->cmd, info->flags, &param) <= -1) goto oops;

/* TODO: more advanced fork and exec .. */
	pid = standard_fork_and_exec (rdev->aio, pfds, info->flags, &param);
	if (pid <= -1) 
	{
		free_param (rdev->aio, &param);
		goto oops;
	}

	free_param (rdev->aio, &param);
	rdev->child_pid = pid;

	/* this is the parent process */
	if (info->flags & QSE_AIO_DEV_PRO_WRITEIN)
	{
		/*
		 * 012345
		 * rw----
		 * X
		 * WRITE => 1
		 */
		close (pfds[0]);
		pfds[0] = QSE_AIO_SYSHND_INVALID;

		if (qse_aio_makesyshndasync (dev->aio, pfds[1]) <= -1) goto oops;
	}

	if (info->flags & QSE_AIO_DEV_PRO_READOUT)
	{
		/*
		 * 012345
		 * --rw--
		 *    X
		 * READ => 2
		 */
		close (pfds[3]);
		pfds[3] = QSE_AIO_SYSHND_INVALID;

		if (qse_aio_makesyshndasync (dev->aio, pfds[2]) <= -1) goto oops;
	}

	if (info->flags & QSE_AIO_DEV_PRO_READERR)
	{
		/*
		 * 012345
		 * ----rw
		 *      X
		 * READ => 4
		 */
		close (pfds[5]);
		pfds[5] = QSE_AIO_SYSHND_INVALID;

		if (qse_aio_makesyshndasync (dev->aio, pfds[4]) <= -1) goto oops;
	}

	if (pfds[1] != QSE_AIO_SYSHND_INVALID)
	{
		/* hand over pfds[2] to the first slave device */
		slave_info_t si;

		si.mi = info;
		si.pfd = pfds[1];
		si.dev_capa = QSE_AIO_DEV_CAPA_OUT | QSE_AIO_DEV_CAPA_OUT_QUEUED | QSE_AIO_DEV_CAPA_STREAM;
		si.id = QSE_AIO_DEV_PRO_IN;

		rdev->slave[QSE_AIO_DEV_PRO_IN] = make_slave (dev->aio, &si);
		if (!rdev->slave[QSE_AIO_DEV_PRO_IN]) goto oops;

		pfds[1] = QSE_AIO_SYSHND_INVALID;
		rdev->slave_count++;
	}

	if (pfds[2] != QSE_AIO_SYSHND_INVALID)
	{
		/* hand over pfds[2] to the first slave device */
		slave_info_t si;

		si.mi = info;
		si.pfd = pfds[2];
		si.dev_capa = QSE_AIO_DEV_CAPA_IN | QSE_AIO_DEV_CAPA_STREAM;
		si.id = QSE_AIO_DEV_PRO_OUT;

		rdev->slave[QSE_AIO_DEV_PRO_OUT] = make_slave (dev->aio, &si);
		if (!rdev->slave[QSE_AIO_DEV_PRO_OUT]) goto oops;

		pfds[2] = QSE_AIO_SYSHND_INVALID;
		rdev->slave_count++;
	}

	if (pfds[4] != QSE_AIO_SYSHND_INVALID)
	{
		/* hand over pfds[4] to the second slave device */
		slave_info_t si;

		si.mi = info;
		si.pfd = pfds[4];
		si.dev_capa = QSE_AIO_DEV_CAPA_IN | QSE_AIO_DEV_CAPA_STREAM;
		si.id = QSE_AIO_DEV_PRO_ERR;

		rdev->slave[QSE_AIO_DEV_PRO_ERR] = make_slave (dev->aio, &si);
		if (!rdev->slave[QSE_AIO_DEV_PRO_ERR]) goto oops;

		pfds[4] = QSE_AIO_SYSHND_INVALID;
		rdev->slave_count++;
	}

	for (i = 0; i < QSE_COUNTOF(rdev->slave); i++) 
	{
		if (rdev->slave[i]) rdev->slave[i]->master = rdev;
	}

	rdev->dev_capa = QSE_AIO_DEV_CAPA_VIRTUAL; /* the master device doesn't perform I/O */
	rdev->flags = info->flags;
	rdev->on_read = info->on_read;
	rdev->on_write = info->on_write;
	rdev->on_close = info->on_close;
	return 0;

oops:
	for (i = minidx; i < maxidx; i++)
	{
		if (pfds[i] != QSE_AIO_SYSHND_INVALID) close (pfds[i]);
	}

	if (rdev->mcmd) 
	{
		QSE_MMGR_FREE (rdev->aio->mmgr, rdev->mcmd);
		free_param (rdev->aio, &param);
	}

	for (i = QSE_COUNTOF(rdev->slave); i > 0; )
	{
		i--;
		if (rdev->slave[i])
		{
			qse_aio_killdev (rdev->aio, (qse_aio_dev_t*)rdev->slave[i]);
			rdev->slave[i] = QSE_NULL;
		}
	}
	rdev->slave_count = 0;

	return -1;
}

static int dev_pro_make_slave (qse_aio_dev_t* dev, void* ctx)
{
	qse_aio_dev_pro_slave_t* rdev = (qse_aio_dev_pro_slave_t*)dev;
	slave_info_t* si = (slave_info_t*)ctx;

	rdev->dev_capa = si->dev_capa;
	rdev->id = si->id;
	rdev->pfd = si->pfd;
	/* keep rdev->master to QSE_NULL. it's set to the right master
	 * device in dev_pro_make() */

	return 0;
}

static int dev_pro_kill_master (qse_aio_dev_t* dev, int force)
{
	qse_aio_dev_pro_t* rdev = (qse_aio_dev_pro_t*)dev;
	int i, status;
	pid_t wpid;

	if (rdev->slave_count > 0)
	{
		for (i = 0; i < QSE_COUNTOF(rdev->slave); i++)
		{
			if (rdev->slave[i])
			{
				qse_aio_dev_pro_slave_t* sdev = rdev->slave[i];

				/* nullify the pointer to the slave device
				 * before calling qse_aio_killdev() on the slave device.
				 * the slave device can check this pointer to tell from
				 * self-initiated termination or master-driven termination */
				rdev->slave[i] = QSE_NULL;

				qse_aio_killdev (rdev->aio, (qse_aio_dev_t*)sdev);
			}
		}
	}

	if (rdev->child_pid >= 0)
	{
		if (!(rdev->flags & QSE_AIO_DEV_PRO_FORGET_CHILD))
		{
			int killed = 0;

		await_child:
			wpid = waitpid (rdev->child_pid, &status, WNOHANG);
			if (wpid == 0)
			{
				if (force && !killed)
				{
					if (!(rdev->flags & QSE_AIO_DEV_PRO_FORGET_DIEHARD_CHILD))
					{
						kill (rdev->child_pid, SIGKILL);
						killed = 1;
						goto await_child;
					}
				}
				else
				{
					/* child process is still alive */
					rdev->aio->errnum = QSE_AIO_EAGAIN;
					return -1;  /* call me again */
				}
			}

			/* wpid == rdev->child_pid => full success
			 * wpid == -1 && errno == ECHILD => no such process. it's waitpid()'ed by some other part of the program?
			 * other cases ==> can't really handle properly. forget it by returning success
			 * no need not worry about EINTR because errno can't have the value when WNOHANG is set.
			 */
		}

printf (">>>>>>>>>>>>>>>>>>> REAPED CHILD %d\n", (int)rdev->child_pid);
		rdev->child_pid = -1;
	}

	if (rdev->on_close) rdev->on_close (rdev, QSE_AIO_DEV_PRO_MASTER);
	return 0;
}

static int dev_pro_kill_slave (qse_aio_dev_t* dev, int force)
{
	qse_aio_dev_pro_slave_t* rdev = (qse_aio_dev_pro_slave_t*)dev;

	if (rdev->master)
	{
		qse_aio_dev_pro_t* master;

		master = rdev->master;
		rdev->master = QSE_NULL;

		/* indicate EOF */
		if (master->on_close) master->on_close (master, rdev->id);

		QSE_ASSERT (master->slave_count > 0);
		master->slave_count--;

		if (master->slave[rdev->id])
		{
			/* this call is started by the slave device itself.
			 * if this is the last slave, kill the master also */
			if (master->slave_count <= 0) 
			{
				qse_aio_killdev (rdev->aio, (qse_aio_dev_t*)master);
				/* the master pointer is not valid from this point onwards
				 * as the actual master device object is freed in qse_aio_killdev() */
			}
		}
		else
		{
			/* this call is initiated by this slave device itself.
			 * if it were by the master device, it would be QSE_NULL as
			 * nullified by the dev_pro_kill() */
			master->slave[rdev->id] = QSE_NULL;
		}
	}

	if (rdev->pfd != QSE_AIO_SYSHND_INVALID)
	{
		close (rdev->pfd);
		rdev->pfd = QSE_AIO_SYSHND_INVALID;
	}

	return 0;
}

static int dev_pro_read_slave (qse_aio_dev_t* dev, void* buf, qse_aio_iolen_t* len, qse_aio_devaddr_t* srcaddr)
{
	qse_aio_dev_pro_slave_t* pro = (qse_aio_dev_pro_slave_t*)dev;
	ssize_t x;

	x = read (pro->pfd, buf, *len);
	if (x <= -1)
	{
		if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) return 0;  /* no data available */
		if (errno == EINTR) return 0;
		pro->aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	*len = x;
	return 1;
}

static int dev_pro_write_slave (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t* len, const qse_aio_devaddr_t* dstaddr)
{
	qse_aio_dev_pro_slave_t* pro = (qse_aio_dev_pro_slave_t*)dev;
	ssize_t x;

	x = write (pro->pfd, data, *len);
	if (x <= -1)
	{
		if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) return 0;  /* no data can be written */
		if (errno == EINTR) return 0;
		pro->aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	*len = x;
	return 1;
}

static qse_aio_syshnd_t dev_pro_getsyshnd (qse_aio_dev_t* dev)
{
	return QSE_AIO_SYSHND_INVALID;
}

static qse_aio_syshnd_t dev_pro_getsyshnd_slave (qse_aio_dev_t* dev)
{
	qse_aio_dev_pro_slave_t* pro = (qse_aio_dev_pro_slave_t*)dev;
	return (qse_aio_syshnd_t)pro->pfd;
}

static int dev_pro_ioctl (qse_aio_dev_t* dev, int cmd, void* arg)
{
	qse_aio_dev_pro_t* rdev = (qse_aio_dev_pro_t*)dev;

	switch (cmd)
	{
		case QSE_AIO_DEV_PRO_CLOSE:
		{
			qse_aio_dev_pro_sid_t sid = *(qse_aio_dev_pro_sid_t*)arg;

			if (sid < QSE_AIO_DEV_PRO_IN || sid > QSE_AIO_DEV_PRO_ERR)
			{
				rdev->aio->errnum = QSE_AIO_EINVAL;
				return -1;
			}

			if (rdev->slave[sid])
			{
				/* unlike dev_pro_kill_master(), i don't nullify rdev->slave[sid].
				 * so i treat the closing ioctl as if it's a kill request 
				 * initiated by the slave device itself. */
				qse_aio_killdev (rdev->aio, (qse_aio_dev_t*)rdev->slave[sid]);
			}
			return 0;
		}

		case QSE_AIO_DEV_PRO_KILL_CHILD:
			if (rdev->child_pid >= 0)
			{
				if (kill (rdev->child_pid, SIGKILL) == -1)
				{
					rdev->aio->errnum = qse_aio_syserrtoerrnum(errno);
					return -1;
				}
			}

			return 0;

		default:
			dev->aio->errnum = QSE_AIO_EINVAL;
			return -1;
	}
}

static qse_aio_dev_mth_t dev_pro_methods = 
{
	dev_pro_make_master,
	dev_pro_kill_master,
	dev_pro_getsyshnd,

	QSE_NULL,
	QSE_NULL,
	dev_pro_ioctl
};

static qse_aio_dev_mth_t dev_pro_methods_slave =
{
	dev_pro_make_slave,
	dev_pro_kill_slave,
	dev_pro_getsyshnd_slave,

	dev_pro_read_slave,
	dev_pro_write_slave,
	dev_pro_ioctl
};

/* ========================================================================= */

static int pro_ready (qse_aio_dev_t* dev, int events)
{
	/* virtual device. no I/O */
	dev->aio->errnum = QSE_AIO_EINTERN;
	return -1;
}

static int pro_on_read (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t len, const qse_aio_devaddr_t* srcaddr)
{
	/* virtual device. no I/O */
	dev->aio->errnum = QSE_AIO_EINTERN;
	return -1;
}

static int pro_on_write (qse_aio_dev_t* dev, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_devaddr_t* dstaddr)
{
	/* virtual device. no I/O */
	dev->aio->errnum = QSE_AIO_EINTERN;
	return -1;
}

static qse_aio_dev_evcb_t dev_pro_event_callbacks =
{
	pro_ready,
	pro_on_read,
	pro_on_write
};

/* ========================================================================= */

static int pro_ready_slave (qse_aio_dev_t* dev, int events)
{
	qse_aio_dev_pro_t* pro = (qse_aio_dev_pro_t*)dev;

	if (events & QSE_AIO_DEV_EVENT_ERR)
	{
		pro->aio->errnum = QSE_AIO_EDEVERR;
		return -1;
	}

	if (events & QSE_AIO_DEV_EVENT_HUP)
	{
		if (events & (QSE_AIO_DEV_EVENT_PRI | QSE_AIO_DEV_EVENT_IN | QSE_AIO_DEV_EVENT_OUT)) 
		{
			/* probably half-open? */
			return 1;
		}

		pro->aio->errnum = QSE_AIO_EDEVHUP;
		return -1;
	}

	return 1; /* the device is ok. carry on reading or writing */
}


static int pro_on_read_slave_out (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t len, const qse_aio_devaddr_t* srcaddr)
{
	qse_aio_dev_pro_slave_t* pro = (qse_aio_dev_pro_slave_t*)dev;
	return pro->master->on_read (pro->master, data, len, QSE_AIO_DEV_PRO_OUT);
}

static int pro_on_read_slave_err (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t len, const qse_aio_devaddr_t* srcaddr)
{
	qse_aio_dev_pro_slave_t* pro = (qse_aio_dev_pro_slave_t*)dev;
	return pro->master->on_read (pro->master, data, len, QSE_AIO_DEV_PRO_ERR);
}

static int pro_on_write_slave (qse_aio_dev_t* dev, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_devaddr_t* dstaddr)
{
	qse_aio_dev_pro_slave_t* pro = (qse_aio_dev_pro_slave_t*)dev;
	return pro->master->on_write (pro->master, wrlen, wrctx);
}

static qse_aio_dev_evcb_t dev_pro_event_callbacks_slave_in =
{
	pro_ready_slave,
	QSE_NULL,
	pro_on_write_slave
};

static qse_aio_dev_evcb_t dev_pro_event_callbacks_slave_out =
{
	pro_ready_slave,
	pro_on_read_slave_out,
	QSE_NULL
};

static qse_aio_dev_evcb_t dev_pro_event_callbacks_slave_err =
{
	pro_ready_slave,
	pro_on_read_slave_err,
	QSE_NULL
};

/* ========================================================================= */

static qse_aio_dev_pro_slave_t* make_slave (qse_aio_t* aio, slave_info_t* si)
{
	switch (si->id)
	{
		case QSE_AIO_DEV_PRO_IN:
			return (qse_aio_dev_pro_slave_t*)qse_aio_makedev (
				aio, QSE_SIZEOF(qse_aio_dev_pro_t), 
				&dev_pro_methods_slave, &dev_pro_event_callbacks_slave_in, si);

		case QSE_AIO_DEV_PRO_OUT:
			return (qse_aio_dev_pro_slave_t*)qse_aio_makedev (
				aio, QSE_SIZEOF(qse_aio_dev_pro_t), 
				&dev_pro_methods_slave, &dev_pro_event_callbacks_slave_out, si);

		case QSE_AIO_DEV_PRO_ERR:
			return (qse_aio_dev_pro_slave_t*)qse_aio_makedev (
				aio, QSE_SIZEOF(qse_aio_dev_pro_t), 
				&dev_pro_methods_slave, &dev_pro_event_callbacks_slave_err, si);

		default:
			aio->errnum = QSE_AIO_EINVAL;
			return QSE_NULL;
	}
}

qse_aio_dev_pro_t* qse_aio_dev_pro_make (qse_aio_t* aio, qse_size_t xtnsize, const qse_aio_dev_pro_make_t* info)
{
	return (qse_aio_dev_pro_t*)qse_aio_makedev (
		aio, QSE_SIZEOF(qse_aio_dev_pro_t) + xtnsize, 
		&dev_pro_methods, &dev_pro_event_callbacks, (void*)info);
}

void qse_aio_dev_pro_kill (qse_aio_dev_pro_t* dev)
{
	qse_aio_killdev (dev->aio, (qse_aio_dev_t*)dev);
}

int qse_aio_dev_pro_write (qse_aio_dev_pro_t* dev, const void* data, qse_aio_iolen_t dlen, void* wrctx)
{
	if (dev->slave[0])
	{
		return qse_aio_dev_write ((qse_aio_dev_t*)dev->slave[0], data, dlen, wrctx, QSE_NULL);
	}
	else
	{
		dev->aio->errnum = QSE_AIO_ENOCAPA; /* TODO: is it the right error number? */
		return -1;
	}
}

int qse_aio_dev_pro_timedwrite (qse_aio_dev_pro_t* dev, const void* data, qse_aio_iolen_t dlen, const qse_ntime_t* tmout, void* wrctx)
{
	if (dev->slave[0])
	{
		return qse_aio_dev_timedwrite ((qse_aio_dev_t*)dev->slave[0], data, dlen, tmout, wrctx, QSE_NULL);
	}
	else
	{
		dev->aio->errnum = QSE_AIO_ENOCAPA; /* TODO: is it the right error number? */
		return -1;
	}
}

int qse_aio_dev_pro_close (qse_aio_dev_pro_t* dev, qse_aio_dev_pro_sid_t sid)
{
	return qse_aio_dev_ioctl ((qse_aio_dev_t*)dev, QSE_AIO_DEV_PRO_CLOSE, &sid);
}

int qse_aio_dev_pro_killchild (qse_aio_dev_pro_t* dev)
{
	return qse_aio_dev_ioctl ((qse_aio_dev_t*)dev, QSE_AIO_DEV_PRO_KILL_CHILD, QSE_NULL);
}

#if 0
qse_aio_dev_pro_t* qse_aio_dev_pro_getdev (qse_aio_dev_pro_t* pro, qse_aio_dev_pro_sid_t sid)
{
	switch (type)
	{
		case QSE_AIO_DEV_PRO_IN:
			return XXX;

		case QSE_AIO_DEV_PRO_OUT:
			return XXX;

		case QSE_AIO_DEV_PRO_ERR:
			return XXX;
	}

	pro->dev->aio = QSE_AIO_EINVAL;
	return QSE_NULL;
}
#endif
