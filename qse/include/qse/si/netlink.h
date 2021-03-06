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

#ifndef _QSE_SI_NETLINK_H_
#define _QSE_SI_NETLINK_H_

#include <qse/types.h>
#include <qse/macros.h>

#if defined(__linux)

struct qse_nlmsg_hdr_t 
{
	qse_uint32_t	nlmsg_len;
	qse_uint16_t	nlmsg_type;
	qse_uint16_t	nlmsg_flags;
	qse_uint32_t	nlmsg_seq;
	qse_uint32_t	nlmsg_pid;
};
typedef struct qse_nlmsg_hdr_t qse_nlmsg_hdr_t;


typedef int (*qse_nlenum_cb_t) (
	qse_nlmsg_hdr_t* hdr,
	void*            ctx
);

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_nlenum_route (
	int             link_af,
	int             addr_af,
	qse_nlenum_cb_t cb,
	void*           ctx
);

#if defined(__cplusplus)
}
#endif

#endif /* __linux */

#endif
