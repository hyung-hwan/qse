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

#include <qse/types.h>
#include <qse/macros.h>

QSE_BEGIN_NAMESPACE(QSE)
class Path
{
public:
	enum
	{
		MAX_LEN = QSE_PATH_MAX
	};

	Path ();
	Path (const qse_char_t* n);
	Path (const Path& fn);
	Path& operator= (const Path& fn);

	const qse_char_t* name () const 
	{
		return full_path;
	}

	void setName (const qse_char_t* n) 
	{
		if (n == QSE_NULL || n[0] == QSE_CHAR('\0')) set_to_root();
		else {
			qse_strxcpy (full_path, qse_countof(full_path), n);
			this->set_base_name ();
		}
	}

	const qse_char_t* baseName () const 
	{
		return base_name;
	}
	const qse_char_t* baseDir () const 
	{
		return base_dir;
	}

	bool exists () 
	{
		return exists (full_path);
	}
	static bool exists (const qse_char_t* path);

	int  getSize        (qse_off_t* sz);
	bool isWritable     ();
	bool isReadable     ();
	bool isReadWritable ();
#ifndef _WIN32
	bool isExecutable   ();
#endif

	bool isDirectory () const { return this->isDirectory (full_path); }
	static bool isDirectory (const qse_char_t* path);

	bool isRegular () const { return this->isRegular (full_path); }
	static bool isRegular (const qse_char_t* path);

	int chmod (qse_mode_t mode);
	static int chmod (const qse_char_t* path, qse_mode_t mode);

	int unlink ();
	static int unlink (const qse_char_t* path);

	int mkdir (qse_mode_t mode);
	static int mkdir (const qse_char_t* path, qse_mode_t mode);

	int setToSelf (const qse_char_t* argv0 = QSE_NULL);

protected:
	qse_char_t full_path[QSE_PATH_MAX + 1];
	qse_char_t base_name[QSE_PATH_MAX + 1];
	qse_char_t base_dir [QSE_PATH_MAX + 1];

	void set_base_name ();
	void set_to_root ();
};

QSE_END_NAMESPACE(QSE)
		
#endif
