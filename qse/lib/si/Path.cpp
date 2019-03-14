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

#include <qse/si/Path.hpp>
#include <qse/si/fs.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/mem.h>
#include "../cmn/syscall.h"



QSE_BEGIN_NAMESPACE(QSE)

Path::Path (Mmgr* mmgr) QSE_CPP_NOEXCEPT: Mmged(mmgr)
{
	this->set_to_root ();
}

Path::Path (const qse_char_t* n, Mmgr* mmgr) QSE_CPP_NOEXCEPT: Mmged(mmgr)
{
	this->setName (n);
}

Path::Path (const Path& path) QSE_CPP_NOEXCEPT: Mmged(path)
{
	qse_strxcpy (this->full_path, QSE_COUNTOF(this->full_path), path.full_path);
	this->set_base_name ();
}

Path& Path::operator= (const Path& path) QSE_CPP_NOEXCEPT
{
	if (this != &path)
	{
		this->setMmgr (path.getMmgr());
		qse_strxcpy (this->full_path, QSE_COUNTOF(this->full_path), path.full_path);
		this->set_base_name ();
	}
	return *this;
}

void Path::setName (const qse_char_t* n) QSE_CPP_NOEXCEPT
{
	if (n == QSE_NULL || n[0] == QSE_T('\0')) this->set_to_root();
	else 
	{
		qse_strxcpy (this->full_path, QSE_COUNTOF(this->full_path), n);
		this->set_base_name ();
	}
}

#if 0
bool Path::exists (const qse_mchar_t* path) QSE_CPP_NOEXCEPT
{
#if defined(QSE_ACCESS) && defined(F_OK)
	return QSE_ACCESS(path, F_OK) == 0;
#else
#	error UNSUPPORTED PLATFORM
#endif
}

bool Path::exists (const qse_wchar_t* path) QSE_CPP_NOEXCEPT
{
	return true;
}

int Path::getSize (const qse_mchar_t* path, qse_foff_t* sz) QSE_CPP_NOEXCEPT
{
#if defined(QSE_STAT)
	/* use stat() instead of lstat() to get the information about the actual file whe the path is a symbolic link */
	qse_stat_t st;
	if (QSE_STAT(path, &st) == -1) return -1;
	*sz = st.st_size;
	return 0;
#else
#	error UNSUPPORTED PLATFORM
#endif
}

int Path::getSize (const qse_wchar_t* path, qse_foff_t* sz) QSE_CPP_NOEXCEPT
{
	return -1;
}

bool Path::isWritable () const QSE_CPP_NOEXCEPT
{
	return ::qse_access (this->full_path, QSE_ACCESS_WRITE) == 0;
}

bool Path::isReadable () const QSE_CPP_NOEXCEPT
{
	return ::qse_access (this->full_path, QSE_ACCESS_READ) == 0;
}

bool Path::isReadWritable () QSE_CPP_NOEXCEPT
{
	return ::qse_access (this->full_path, QSE_ACCESS_READ | QSE_ACCESS_WRITE) == 0;
}

#if !defined(_WIN32)
bool Path::isExecutable () QSE_CPP_NOEXCEPT
{
	return ::qse_access (this->full_path, QSE_ACCESS_EXECUTE) == 0;
}
#endif

bool Path::isDir (const qse_char_t* path) QSE_CPP_NOEXCEPT
{
	int n;
	qse_stat_t st;

	n = ::qse_stat (path, &st);
	if (n == -1) return 0;
#if defined(_WIN32)
	return (st.st_mode & _S_IFDIR) != 0;
#else
	return S_ISDIR(st.st_mode);
#endif
}

bool Path::isRegular (const qse_char_t* path) QSE_CPP_NOEXCEPT
{
	int n;
	qse_stat_t st;

	n = ::qse_stat(path, &st);
	if (n == -1) return 0;
#if defined(_WIN32)
	return (st.st_mode & _S_IFREG) != 0;
#else
	return S_ISREG(st.st_mode);
#endif
}
#endif


int Path::chmod (const qse_mchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT
{
#if defined(QSE_CHMOD)
	return QSE_CHMOD(path, mode);
#else
#	error UNSUPPORTED PLATFORM
#endif
}

int Path::chmod (const qse_wchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT
{
#if defined(QSE_CHMOD)
	qse_mchar_t* xpath = qse_wcstombsdup(path, QSE_NULL, QSE_MMGR_GETDFL());
	if (!xpath) return -1;
	return QSE_CHMOD(xpath, mode);
#else
#	error UNSUPPORTED PLATFORM
#endif
}

int Path::unlink (const qse_mchar_t* path) QSE_CPP_NOEXCEPT
{
#if defined(QSE_UNLINK)
	return QSE_UNLINK(path);
#else
#	error UNSUPPORTED PLATFORM
#endif
}

int Path::unlink (const qse_wchar_t* path) QSE_CPP_NOEXCEPT
{
#if defined(QSE_UNLINK)
	qse_mchar_t* xpath = qse_wcstombsdup(path, QSE_NULL, QSE_MMGR_GETDFL());
	if (!xpath) return -1;
	return QSE_UNLINK(xpath);
#else
#	error UNSUPPORTED PLATFORM
#endif
}

int Path::mkdir (const qse_mchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT
{
#if defined(QSE_MKDIR)
	return QSE_MKDIR(path, mode);
#else
#	error UNSUPPORTED PLATFORM
#endif
}

int Path::mkdir (const qse_wchar_t* path, qse_fmode_t mode) QSE_CPP_NOEXCEPT
{
#if defined(QSE_MKDIR)
	qse_mchar_t* xpath = qse_wcstombsdup(path, QSE_NULL, QSE_MMGR_GETDFL());
	if (!xpath) return -1;
	return QSE_MKDIR(xpath, mode);
#else
#	error UNSUPPORTED PLATFORM
#endif
}


void Path::set_base_name () QSE_CPP_NOEXCEPT
{
	qse_size_t len = qse_strlen(this->full_path);
	QSE_ASSERT (len > 0);

	for (qse_size_t i = len; i > 0; i--) 
	{
#if defined(_WIN32)
		if ((this->full_path[i - 1] == QSE_T('/') || this->full_path[i - 1] == QSE_T('\\')) && i != len) 
		{
#else
		
		if (this->full_path[i - 1] == QSE_T('/') && i != len) 
		{
#endif
			const qse_char_t* p = this->full_path;
#if defined(_WIN32)
			if (this->full_path[len - 1] == QSE_T('/') || this->full_path[len - 1] == QSE_T('\\')) 
#else
			if (this->full_path[len - 1] == QSE_T('/')) 
#endif
				qse_strxncpy (this->base_name, QSE_COUNTOF(this->base_name), &p[i], len - i - 1);
			else 
				qse_strxncpy (this->base_name, QSE_COUNTOF(this->base_name), &p[i], len - i);

#if defined(_WIN32)
			if (i == 1 && (this->full_path[i - 1] == QSE_T('/') || this->full_path[i - 1] == QSE_T('\\'))) 
#else
			if (i == 1 && this->full_path[i - 1] == QSE_T('/')) 
#endif
				qse_strxncpy (this->base_dir, QSE_COUNTOF(this->base_dir), p, i);
			else 
				qse_strxncpy (this->base_dir, QSE_COUNTOF(this->base_dir), p, i - 1);

			return;
		}
	}

#if defined(_WIN32)
	if (this->full_path[len - 1] == QSE_T('/') || this->full_path[len - 1] == QSE_T('\\')) 
#else
	if (this->full_path[len - 1] == QSE_T('/')) 
#endif
		qse_strxncpy (this->base_name, QSE_COUNTOF(this->base_name), this->full_path, len - 1);
	else 
		qse_strxcpy (this->base_name, QSE_COUNTOF(this->base_name), this->full_path);

#if defined(_WIN32)
	if (this->full_path[0] == QSE_T('/') || this->full_path[0] == QSE_T('\\'))
#else
	if (this->full_path[0] == QSE_T('/')) 
#endif
		qse_strcpy (this->base_dir, QSE_T("/"));
	else 
		qse_strcpy (this->base_dir, QSE_T(""));
}

void Path::set_to_root () QSE_CPP_NOEXCEPT
{
	qse_strcpy (this->full_path, QSE_T("/"));
	//this->set_base_name ();
	qse_strcpy (this->base_name, QSE_T(""));
	qse_strcpy (this->base_dir,  QSE_T("/"));
}

int Path::setToSelf (const qse_mchar_t* argv0) QSE_CPP_NOEXCEPT
{
	return 0;
}

int Path::setToSelf (const qse_wchar_t* argv0) QSE_CPP_NOEXCEPT
{
	qse_char_t p[MAX_LEN + 1];
	if (qse_get_prog_path_with_mmgr(argv0, p, QSE_COUNTOF(p), this->getMmgr()) == -1) return -1;
	this->setName (p);
	return 0;
}

QSE_END_NAMESPACE(QSE)
