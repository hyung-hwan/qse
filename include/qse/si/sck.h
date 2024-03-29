/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _QSE_SI_SCK_H_
#define _QSE_SI_SCK_H_


#include <qse/types.h>
#include <qse/macros.h>
#include <qse/si/nwad.h>

#if defined(_WIN32)
	typedef qse_uintptr_t qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (~(qse_sck_hnd_t)0)
#elif defined(__OS2__)
	typedef int qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (-1)
#elif defined(__DOS__)
	typedef int qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (-1)
#else
	typedef int qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (-1)
#endif

#if (QSE_SIZEOF_SOCKLEN_T == QSE_SIZEOF_INT)
	#if defined(QSE_SOCKLEN_T_IS_SIGNED)
		typedef int qse_sck_len_t;
	#else
		typedef unsigned int qse_sck_len_t;
	#endif
#elif (QSE_SIZEOF_SOCKLEN_T == QSE_SIZEOF_LONG)
	#if defined(QSE_SOCKLEN_T_IS_SIGNED)
		typedef long qse_sck_len_t;
	#else
		typedef unsigned long qse_sck_len_t;
	#endif
#else
	typedef int qse_sck_len_t;
#endif

enum qse_shut_sck_how_t
{
	QSE_SHUT_SCK_R  = 0,
	QSE_SHUT_SCK_W  = 1,
	QSE_SHUT_SCK_RW = 2
};
typedef enum qse_shut_sck_how_t qse_shut_sck_how_t;

enum qse_sck_trait_t
{
	QSE_SCK_TRAIT_NONBLOCK = (1 << 0),
	QSE_SCK_TRAIT_CLOEXEC = (1 << 1)
};
typedef enum qse_sck_trait_t qse_sck_trait_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_is_sck_valid (
	qse_sck_hnd_t handle
);

QSE_EXPORT qse_sck_hnd_t qse_open_sck (
	int family,
	int type,
	int protocol,
	int traits
);

QSE_EXPORT void qse_close_sck (
	qse_sck_hnd_t handle
);


QSE_EXPORT void qse_shut_sck (
	qse_sck_hnd_t      handle,
	qse_shut_sck_how_t how
);

QSE_EXPORT int qse_set_sck_nonblock (
	qse_sck_hnd_t handle,
	int           enabled
);

QSE_EXPORT int qse_set_sck_cloexec (
	qse_sck_hnd_t handle,
	int           enabled
);

QSE_EXPORT int qse_init_sck_conn (
	qse_sck_hnd_t     handle,
	const qse_skad_t* skad
);

QSE_EXPORT int qse_fini_sck_conn (
	qse_sck_hnd_t handle
);

QSE_EXPORT int qse_set_sck_keepalive (
	qse_sck_hnd_t handle,
	int           enabled,
	int           tcp_keepidle,
	int           tcp_keepintvl,
	int           tcp_keepcnt
);

#if defined(__cplusplus)
}
#endif


#endif

