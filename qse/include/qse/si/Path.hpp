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

#ifndef _QSE_SI_PATH_CLASS_
#define _QSE_SI_PATH_CLASS_

#include <qse/Types.hpp>
#include <qse/cmn/Mmged.hpp>

QSE_BEGIN_NAMESPACE(QSE)

class Path: public Mmged
{
public:
	enum
	{
		MAX_LEN = QSE_PATH_MAX
	};

	Path (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	Path (const qse_mchar_t* n, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	Path (const qse_wchar_t* n, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	Path (const Path& path) QSE_CPP_NOEXCEPT;
	Path& operator= (const Path& path) QSE_CPP_NOEXCEPT;

	// NOTE: the current implementation doesn't have much to benefit from C++11 Move 
	//       semantics and the rvalue reference. 

	void setName (const qse_wchar_t* n) QSE_CPP_NOEXCEPT;
	void setName (const qse_mchar_t* n) QSE_CPP_NOEXCEPT;

	const qse_char_t* getName () const QSE_CPP_NOEXCEPT { return this->full_path; }
	const qse_char_t* getBaseName () const QSE_CPP_NOEXCEPT { return this->base_name; }
	const qse_char_t* getBaseDir () const QSE_CPP_NOEXCEPT { return this->base_dir; }

#if 0
	bool exists () const QSE_CPP_NOEXCEPT { return this->exists(this->full_path); }
	static bool exists (const qse_mchar_t* path) QSE_CPP_NOEXCEPT;
	static bool exists (const qse_wchar_t* path) QSE_CPP_NOEXCEPT;

	int getSize (qse_foff_t* sz) const QSE_CPP_NOEXCEPT { return this->getSize(this->full_path, sz); }
	static int getSize (const qse_mchar_t* path, qse_foff_t* sz) QSE_CPP_NOEXCEPT;
	static int getSize (const qse_wchar_t* path, qse_foff_t* sz) QSE_CPP_NOEXCEPT;

	bool isWritable () const QSE_CPP_NOEXCEPT;
	bool isReadable () const QSE_CPP_NOEXCEPT;
	bool isReadWritable () const QSE_CPP_NOEXCEPT;
	bool isExecutable () const QSE_CPP_NOEXCEPT;

	bool isDir () const QSE_CPP_NOEXCEPT { return this->isDir(this->full_path); }
	static bool isDir (const qse_char_t* path) QSE_CPP_NOEXCEPT;

	bool isRegular () const QSE_CPP_NOEXCEPT { return this->isRegular(this->full_path); }
	static bool isRegular (const qse_char_t* path) QSE_CPP_NOEXCEPT;
#endif

	int chmod (qse_fmode_t mode) QSE_CPP_NOEXCEPT { return this->chmod(this->full_path, mode); }
	static int chmod (const qse_mchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT;
	static int chmod (const qse_wchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT;

	int unlink () QSE_CPP_NOEXCEPT { return this->unlink(this->full_path); }
	static int unlink (const qse_mchar_t* path) QSE_CPP_NOEXCEPT;
	static int unlink (const qse_wchar_t* path) QSE_CPP_NOEXCEPT;

	int mkdir (qse_fmode_t mode) QSE_CPP_NOEXCEPT { return this->mkdir(this->full_path, mode); }
	static int mkdir (const qse_mchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT;
	static int mkdir (const qse_wchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT;

	int setToSelf (const qse_mchar_t* argv0 = QSE_NULL) QSE_CPP_NOEXCEPT;
	int setToSelf (const qse_wchar_t* argv0 = QSE_NULL) QSE_CPP_NOEXCEPT;

protected:
	qse_char_t full_path[QSE_PATH_MAX + 1];
	qse_char_t base_name[QSE_PATH_MAX + 1];
	qse_char_t base_dir [QSE_PATH_MAX + 1];

	void set_base_name () QSE_CPP_NOEXCEPT;
	void set_to_root () QSE_CPP_NOEXCEPT;
};

QSE_END_NAMESPACE(QSE)

#endif
