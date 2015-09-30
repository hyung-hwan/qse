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

#ifndef _QSE_LIB_CMN_THR_H_
#define _QSE_LIB_CMN_THR_H_

#include <qse/sys/thr.h>


#if (!defined(__unix__) && !defined(__unix)) || defined(HAVE_PTHREAD)

#if defined(_WIN32)
#	include <windows.h>
#	include <process.h>
#	define QSE_THR_HND_INVALID INVALID_HANDLE_VALUE
#elif defined(__OS2__)
	/* not implemented */

#elif defined(__DOS__)

	/* not implemented */

#elif defined(__BEOS__)
#	include <be/kernel/OS.h>
#	define QSE_THR_HND_INVALID (-1)
#else
#	if defined(AIX) && defined(__GNUC__)
		typedef int crid_t;
		typedef unsigned int class_id_t;
#	endif
#	include <pthread.h>
#	include <signal.h>

#	define QSE_THR_HND_INVALID 0
#endif

struct qse_thr_t
{
	qse_mmgr_t*       mmgr;

	qse_thr_routine_t __main_routine;
	qse_thr_routine_t __temp_routine;
	qse_bool_t        __joinable;
	qse_size_t        __stacksize;

	qse_thr_hnd_t     __handle;
	qse_thr_state_t   __state;

	int               __return_code;
};

#endif


#endif
