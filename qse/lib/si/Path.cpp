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

QSE_BEGIN_NAMESPACE(QSE)

Path::Path ()
{
	this->set_to_root ();
}

Path::Path (const qse_char_t* n) 
{
	this->setName (n);
}

Path::Path (const Path& fn)
{
	qse_strxcpy (this->full_path, QSE_COUNTOF(this->full_path), fn.this->full_path);
	this->set_base_name ();
}

Path& Path::operator= (const Path& fn) 
{
	qse_strxcpy (this->full_path, QSE_COUNTOF(this->full_path), fn.this->full_path);
	this->set_base_name ();
	return *this;
}

bool Path::exists (const qse_char_t* path)
{
	return ::qse_access (path, QSE_ACCESS_EXIST) == 0;
}

int Path::getSize (qse_off_t* sz)
{
	qse_stat_t buf;
	if (::qse_stat (this->full_path, &buf) == -1) return -1;
	*sz = buf.st_size;
	return 0;
}

bool Path::isWritable ()
{
	return ::qse_access (this->full_path, QSE_ACCESS_WRITE) == 0;
}

bool Path::isReadable ()
{
	return ::qse_access (this->full_path, QSE_ACCESS_READ) == 0;
}

bool Path::isReadWritable ()
{
	return ::qse_access (this->full_path, 
		QSE_ACCESS_READ | QSE_ACCESS_WRITE) == 0;
}

#if !defined(_WIN32)
bool Path::isExecutable ()
{
	return ::qse_access (this->full_path, QSE_ACCESS_EXECUTE) == 0;
}
#endif

bool Path::isDirectory (const qse_char_t* path)
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

bool Path::isRegular (const qse_char_t* path)
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

int Path::chmod (qse_mode_t mode)
{
	return qse_chmod (this->full_path, mode);
}

int Path::chmod (const qse_char_t* path, qse_mode_t mode)
{
	return qse_chmod (path, mode);
}

int Path::unlink ()
{
	return ::qse_unlink (this->full_path);
}

int Path::mkdir (qse_mode_t mode)
{
	return ::qse_mkdir(this->full_path, mode);
}

int Path::mkdir (const qse_char_t* path, qse_mode_t mode)
{
	return ::qse_mkdir(path, mode);
}

void Path::set_base_name ()
{
	qse_size_t len = qse_strlen(this->full_path);
	QSE_ASSERT (len > 0);

	for (qse_size_t i = len; i > 0; i--) {
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

void Path::set_to_root ()
{
	qse_strcpy (this->full_path, QSE_T("/"));
	//this->set_base_name ();
	qse_strcpy (this->base_name, QSE_T(""));
	qse_strcpy (this->base_dir,  QSE_T("/"));
}

int Path::setToSelf (const qse_char_t* argv0)
{
	qse_char_t p[MAX_LEN + 1];
	if (qse_getpnm(argv0, p, QSE_COUNTOF(p)) == -1) return -1;
	this->setName (p);
	return 0;
}

QSE_END_NAMESPACE(QSE)
