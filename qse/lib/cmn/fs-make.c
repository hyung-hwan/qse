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

#include "fs.h"

static int make_directory_in_fs (qse_fs_t* fs, const qse_fs_char_t* fspath)
{

#if defined(_WIN32)

	if (CreateDirectory (fspath, QSE_NULL) == FALSE)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, GetLastError());
		return -1;
	}

#elif defined(__OS2__)

	APIRET rc;

	rc = DosMkDir (fspath);
	if (rc != NO_ERROR)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, rc);
		return -1;
	}

#elif defined(__DOS__)

	if (mkdir (fspath) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#else

	if (QSE_MKDIR (fspath, 0755) <= -1)
	{
		fs->errnum = qse_fs_syserrtoerrnum (fs, errno);
		return -1;
	}

#endif

	return 0;
}

/* --------------------------------------------------------------------- */

static int make_directory_with_mbs (qse_fs_t* fs, const qse_mchar_t* path)
{
	qse_fs_char_t* fspath;
	int ret;

	if (fs->cbs.mk) 
	{
		
		int x;
		x = fs->cbs.del (fs, path);
		if (x <= -1) return -1;
		if (x == 0) return 0; /* skipped */
	}

	fspath = qse_fs_makefspath(fs, path);
	if (!fspath) return -1;

	ret = delete_file_from_fs (fs, fspath);
	qse_fs_freefspath (fs, path, fspath);

	return ret;
}


static int make_directory_with_wcs (qse_fs_t* fs, const qse_wchar_t* path)
{
	qse_fs_char_t* fspath;
	int ret;

	if (fs->cbs.mk) 
	{
		int x;
		x = fs->cbs.del (fs, path);
		if (x <= -1) return -1;
		if (x == 0) return 0; /* skipped */
	}

	fspath = qse_fs_makefspath(fs, path);
	if (!fspath) return -1;

	ret = delete_file_from_fs (fs, fspath);
	qse_fs_freefspath (fs, path, fspath);

	return ret;
}

/* --------------------------------------------------------------------- */
int qse_fs_mkdirmbs (qse_fs_t* fs, const qse_mchar_t* path)
{

	return 0;
}

int qse_fs_mkdirwcs (qse_fs_t* fs, const qse_wchar_t* path)
{
	return 0;
}
