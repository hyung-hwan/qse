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

#ifndef _QSE_SI_APPROOT_H_
#define _QSE_SI_APPROOT_H_

#include <qse/Types.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmged.hpp>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

class AppRoot: Uncopyable, public Types, public Mmged
{
public:
	AppRoot (Mmgr* mmgr): Mmged(mmgr), _root_only(false) {}
	virtual ~AppRoot () {}

	int daemonize (bool chdir_to_root = true, int fork_count = 1) QSE_CPP_NOEXCEPT;

	int chroot (const qse_mchar_t* mpath) QSE_CPP_NOEXCEPT;
	int chroot (const qse_wchar_t* wpath) QSE_CPP_NOEXCEPT;

#if 0
	int switchUser (uid_t uid, gid_t gid, bool permanently) QSE_CPP_NOEXCEPT;
	int restoreUser () QSE_CPP_NOEXCEPT;
#endif

protected:
	bool _root_only;
#if 0
	uid_t saved_uid;
	gid_t saved_gid;
	gid_t saved_groups[NGROUPS_MAX];
	qse_size_t saved_ngroups;

	void on_signal () QSE_CPP_NOEXCEPT;
	void on_signal () QSE_CPP_NOEXCEPT;
#endif
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif