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

#ifndef _QSE_LIB_IO_AIO_PRV_H_
#define _QSE_LIB_IO_AIO_PRV_H_

#include <qse/io/aio.h>
#include "../cmn/mem.h"


typedef struct qse_aio_mux_t qse_aio_mux_t;

struct qse_aio_t
{
	qse_mmgr_t* mmgr;

	qse_aio_errnum_t errnum;
	qse_aio_stopreq_t stopreq;  /* stop request to abort qse_aio_loop() */

	struct
	{
		qse_aio_dev_t* head;
		qse_aio_dev_t* tail;
	} actdev; /* active devices */

	struct
	{
		qse_aio_dev_t* head;
		qse_aio_dev_t* tail;
	} hltdev; /* halted devices */

	struct
	{
		qse_aio_dev_t* head;
		qse_aio_dev_t* tail;
	} zmbdev; /* zombie devices */

	qse_uint8_t bigbuf[65535]; /* TODO: make this dynamic depending on devices added. device may indicate a buffer size required??? */

	unsigned int renew_watch: 1;
	unsigned int in_exec: 1;

	struct
	{
		qse_size_t     capa;
		qse_size_t     size;
		qse_aio_tmrjob_t*  jobs;
	} tmr;

	/* platform specific fields below */
#if defined(_WIN32)
	HANDLE iocp;
#else
	qse_aio_mux_t* mux;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

int qse_aio_makesyshndasync (
	qse_aio_t*       aio,
	qse_aio_syshnd_t hnd
);

qse_aio_errnum_t qse_aio_syserrtoerrnum (
	int no
);

void qse_aio_cleartmrjobs (
	qse_aio_t* aio
);

void qse_aio_firetmrjobs (
	qse_aio_t*         aio,
	const qse_ntime_t* tmbase,
	qse_size_t*    firecnt
);

int qse_aio_gettmrtmout (
	qse_aio_t*         aio,
	const qse_ntime_t* tmbase,
	qse_ntime_t*       tmout
);

#ifdef __cplusplus
}
#endif


#endif
